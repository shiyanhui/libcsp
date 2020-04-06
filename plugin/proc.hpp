/*
 * Copyright (c) 2020, Yanhui Shi <lime.syh at gmail dot com>
 * All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef LIBCSP_PLUGIN_PROC_HPP
#define LIBCSP_PLUGIN_PROC_HPP

#include "c-tree.h"
#include "tree.h"
#include "tree-iterator.h"
#include "tree-pretty-print.h"
#include "stringpool.h"
#include "namer.hpp"
#include "sa.hpp"
#include <cstdio>
#include <iostream>
#include <unordered_map>

#define build_arg(parms, types, decl) do {                                     \
  /* build argument */                                                         \
  tree decl_clone = copy_node(decl);                                           \
  DECL_CHAIN(decl_clone) = (parms);                                            \
  (parms) = decl_clone;                                                        \
                                                                               \
  /* build argument type */                                                    \
  tree type = TREE_TYPE(decl);                                                 \
  DECL_ARG_TYPE(decl_clone) = type;                                            \
  (types) = tree_cons(0, type, (types));                                       \
} while (0)                                                                    \

namespace csp {

const std::string main                  = "main";
const std::string csp_main              = "csp_main";
const std::string csp_proc_nchild_set   = "csp_proc_nchild_set";
const std::string csp_proc_new          = "csp_proc_new";
const std::string csp_sched_proc_anchor = "csp_sched_proc_anchor";
const std::string csp_sched_put_proc    = "csp_sched_put_proc";
const std::string csp_sched_put_timer   = "csp_sched_put_timer";
const std::string csp_sched_yield       = "csp_sched_yield";
const std::string csp_timer_anchor      = "csp_timer_anchor";

typedef enum {
  /* Build processes in `csp_async`. */
  TYPE_ASYNC_PROC,

  /* Build processes in `csp_sync`. */
  TYPE_SYNC_PROC,

  /* Build a specific process to wrap the `main` function. The process behaves
   * the same as `csp_async` processes except that it calls function `exit`
   * after the `main` function returns while `csp_async` processes call
   * `csp_core_proc_exit`. */
  TYPE_MAIN_PROC,

  /* Build processes in `csp_timer_at` or `csp_timer_after`. */
  TYPE_TIMER_PROC,

  /* Build the real `main` function.
   *
   * To do this, first we rename the original `main` function to `csp_main`,
   * then we build a wrapper process function(the name of it may be something
   * like `csp__async_0_csp_main`) for `csp_main`, and finally we build the
   * real `main` function manually in assembly language. */
  TYPE_MAIN_FUNC
} build_type_t;

std::string get_callee_name(tree call) {
  pretty_printer pp;
  dump_generic_node(&pp, CALL_EXPR_FN(call), 0, TDF_VOPS|TDF_MEMSYMS, false);
  return std::string(xstrdup(pp_formatted_text(&pp)));
}

class proc_builder_t {
public:
  struct c_declspecs* build_specs(location_t loc) {
    struct c_declspecs *specs = build_null_declspecs();

    /* For compatibility, we set the return type of wrapper function to `int`.
     * The return value will never be used except of that of the first process(
     * i.e the MAIN_PROC). */
    struct c_typespec type = {
      .kind=ctsk_resword,
      .expr_const_operands=true,
      .spec=get_identifier("int"),
      .expr=NULL_TREE
    };

    declspecs_add_type(loc, specs, type);

    /* Mark the function naked. */
    declspecs_add_attrs(
      loc, specs, tree_cons(get_identifier("naked"), NULL, NULL)
    );

    finish_declspecs(specs);
    return specs;
  }

  /* Clone the parameters and extra_arg and generate a new parameter list. */
  struct c_arg_info *build_args(tree wrapped_fn, tree extra_arg) {
    struct c_arg_info *args = build_arg_info();

    tree parms = NULL_TREE, types = void_list_node;
    for (tree decl = DECL_ARGUMENTS(wrapped_fn); decl != NULL;) {
      build_arg(parms, types, decl);
      decl = DECL_CHAIN(decl);
    }

    if (extra_arg != NULL_TREE) {
      build_arg(parms, types, extra_arg);
    }

    args->parms = nreverse(parms);
    args->types = types;

    return args;
  }

  struct c_declarator *build_declarator(std::string name,
      struct c_arg_info *args, location_t loc) {
    struct c_declarator *declarator = build_id_declarator(
      get_identifier(name.c_str())
    );
    declarator->id_loc = loc;
    return build_function_declarator(args, declarator);
  }

  /*
   * Build the wrapper function body in pure assembly language. The steps are,
   *
   *  - Create a process entity for it.
   *  - Calculate and store initial context to the process.
   *  - Call the wrapped function when the scheduler restores context of this
   *    process.
   *  - After the wrapped function returns, destroy the process and then
   *    re-schedule again.
   *
   * This wrapper function will never return.
   */
  tree build_fn_body(build_type_t build_type, tree wrapped_fn, int args_len,
      tree extra_arg) {
    tree fn_body = c_begin_compound_stmt(true);

    /* The generated asm code buffer. */
    std::string buff;

    /* 256 bytes is enough to store an instruction. */
    char instr[256];

    /* When build the real `main` function, we just call the wrapper function of
     * `csp_main` and then begin to schedule. */
    if (build_type == TYPE_MAIN_FUNC) {
      buff.append("push %rbp\n");
      buff.insert(buff.length(), instr, sprintf(instr,
        "call %s\n", fndecl_name(wrapped_fn)
      ));
      buff.append("call csp_core_start_main@plt\n");

      /* Put the asm statement to the statement list. */
      build_asm_stmt(true, build_asm_expr(
        DECL_SOURCE_LOCATION(wrapped_fn),
        build_string(buff.length(), buff.c_str()),
        NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE, true, false
      ));

      return c_end_compound_stmt(
        DECL_SOURCE_LOCATION(wrapped_fn), fn_body, true
      );
    }

    const char* pushes[6] = {
      "push %rdi\n",
      "push %rsi\n",
      "push %rdx\n",
      "push %rcx\n",
      "push %r8\n",
      "push %r9\n"
    };

    const char *pops[6] = {
      "popq 0x30(%rdi)\n",
      "popq 0x38(%rdi)\n",
      "popq 0x40(%rdi)\n",
      "popq 0x48(%rdi)\n",
      "popq 0x50(%rdi)\n",
      "popq 0x58(%rdi)\n"
    };

    /* Stack frame size of the wrapper function in bytes. */
    int stack_frame = 0;

    /* The stack should be 16-bytes aligned before the call instruction
     * according to System V x86_64 ABI, so we should push one extra garbage
     * register here if the length of arugments passed by registers is even. */
    bool need_padding = args_len > 6 || (args_len & 0x01) == 0;
    if (need_padding) {
      buff.append("push %rbp\n");
      stack_frame += 8;
    }

    /* Store the arguments passed by registers to the stack cause calling
     * `csp_proc_new` may overrides them. */
    for (int i = (args_len < 6 ? args_len : 6) - 1; i >= 0; i--) {
      buff.append(pushes[i]);
      stack_frame += 8;
    }

    /* Pass `id` of current wrapper function to function `csp_proc_new`. */
    buff.insert(buff.length(), instr, sprintf(instr,
      "mov $0x%x, %%rdi\n", csp::namer.current_id()
    ));

    /* Pass `waited_by_parent` to function `csp_proc_new`. When it's true
     * current process will block until all child processes exit. */
    buff.insert(buff.length(), instr, sprintf(instr,
      "mov $%d, %%rsi\n", build_type == TYPE_SYNC_PROC
    ));

    buff.append(
      /* Create a new process. */
      "call csp_proc_new@plt\n"
      "mov  %rax, %rdi\n"

      /* Store mxcsr register and the x87 control word. */
      "stmxcsr 0x18(%rdi)\n"
      "fstcw   0x1c(%rdi)\n"
    );

    /* Restore the stack and then store the arguments passed by registers. */
    for (int i = 0, total = args_len < 6 ? args_len : 6; i < total; i++) {
      buff.append(pops[i]);
    }

    /* Store the timestamp. */
    if (build_type == TYPE_TIMER_PROC) {
      if (args_len <= 6) {
        buff.insert(buff.length(), instr, sprintf(instr,
          "mov 0x%x(%%rdi), %%rax\n", 0x30 + ((args_len - 1) << 3)
        ));
      } else {
        buff.insert(buff.length(), instr, sprintf(instr,
          "mov 0x%x(%%rsp), %%rax\n", (args_len - 5) << 3
        ));
      }
      buff.append("mov %rax, 0x60(%rdi)\n");
    }

    /* The reserved space in the process stack in 8-bytes. The initial one is
     * for %rip. */
    int rsv_num = 1;

    /* Reserve space for arguments passed by stack. */
    if (args_len > 6) {
      rsv_num += args_len - 6;
    }

    /* Reserve space for 16-bytes alignment. Cause `rvs_num` includes `%rip`,
    * so we should add one plus 8-bytes padding when `rsv_num & 0x01 == 0`. */
    rsv_num += !(rsv_num & 0x01);

    /* Load %rbp. */
    buff.append("mov 0x28(%rdi), %rax\n");

    /* Store %rsp. */
    buff.insert(buff.length(), instr, sprintf(instr,
      "sub $0x%x, %%rax\n", rsv_num << 3
    ));
    buff.append("mov %rax, 0x20(%rdi)\n");

    /* Copy arguments passed by stack from right to left if any. */
    for (int i = args_len - 6; i >= 1; i--) {
      buff.insert(buff.length(), instr, sprintf(instr,
        "mov 0x%x(%%rsp), %%rsi\n", (i + 1) << 3
      ));

      buff.insert(buff.length(), instr, sprintf(instr,
        "mov %%rsi, 0x%x(%%rax)\n", i << 3
      ));
    }

    /* Store %rip. */
    buff.append(
      "lea 0f(%rip), %rsi\n"
      "mov %rsi, (%rax)\n"
    );

    /* If we haven't added padding to the stack for 16-bytes alignment before
     * calling `csp_proc_new`, now we need to do it. */
    if (!need_padding) {
      buff.append("push %rbp\n");
      stack_frame += 8;
    }

    /* Put the process to the run queue or timer queue for scheduling. */
    buff.append(build_type == TYPE_TIMER_PROC ?
      "call csp_sched_put_timer@plt\n" :
      "call csp_sched_put_proc@plt\n"
    );

    /* Remove the garbage padding and return. */
    buff.append(
      "pop %rbp\n"
      "retq\n"
    );

    /* The scheduler will schedule this process and jump here to run. */
    buff.insert(buff.length(), instr, sprintf(instr,
      "0: call %s@plt\n", fndecl_name(wrapped_fn)
    ));

    if (build_type == TYPE_MAIN_PROC) {
      /* When current process is MAIN_PROC, set the exit status and exit. */
      buff.append(
        "mov %rax, %rdi\n"
        "call exit@plt\n"
      );
    } else {
      /* Otherwise destroy current destroy and schedule to find the next
       * process to run. */
      buff.append("call csp_core_proc_exit@plt\n");
    }

    /* Put the asm statement to the statement list. */
    build_asm_stmt(true, build_asm_expr(
      DECL_SOURCE_LOCATION(wrapped_fn),
      build_string(buff.length(), buff.c_str()),
      NULL_TREE, NULL_TREE, NULL_TREE, NULL_TREE, true, false
    ));

    fn_body = c_end_compound_stmt(
      DECL_SOURCE_LOCATION(wrapped_fn), fn_body, true
    );

    /* Collect call graphs of this wrapper function. */
    csp::analyzer.add_call(csp::namer.current_name(), csp_proc_new);
    csp::analyzer.add_call(csp::namer.current_name(),
      build_type == TYPE_TIMER_PROC ?  csp_sched_put_timer : csp_sched_put_proc
    );

    /* Collect stack frame of this process. */
    csp::stack_usage_t su;
    su.type = csp::MANUALLY;
    su.frame_size = stack_frame;
    su.proc_reserved = rsv_num << 3;
    csp::analyzer.add_stack_usage(csp::namer.current_name(), su);

    return fn_body;
  }

  csp::namer_type_t get_namer_type(build_type_t build_type) {
    csp::namer_type_t namer_type;
    switch (build_type) {
    case TYPE_ASYNC_PROC:
      namer_type = csp::NAMER_TYPE_ASYNC;
      break;
    case TYPE_SYNC_PROC:
      namer_type = csp::NAMER_TYPE_SYNC;
      break;
    case TYPE_TIMER_PROC:
      namer_type = csp::NAMER_TYPE_TIMER;
      break;
    default:
      namer_type = csp::NAMER_TYPE_OTHER;
    }
    return namer_type;
  }

  /* Wrap the called function.
   *
   * If the `build_type` is TYPE_TIMER_PROC, the `extra_arg` will be set to
   * the timestamp, otherwise it will be set to NULL_TREE. */
  tree build_fn(build_type_t build_type, tree wrapped_fn, tree extra_arg) {
    auto is_main_func = build_type == TYPE_MAIN_FUNC;

    auto namer_type = this->get_namer_type(build_type);
    auto fn_name = is_main_func ? main : std::string(fndecl_name(wrapped_fn));
    auto cache_key = fn_name + ":" + csp::namer_type_labels[namer_type];

    if (this->fndecl_cache.find(cache_key) != this->fndecl_cache.end()) {
      return this->fndecl_cache[cache_key];
    }

    /* If we are not building the main function, we should generate the wrapper
     * function name by related rules. */
    if (!is_main_func) {
      fn_name = csp::namer.next_name(fn_name, namer_type);
    }

    /* We should reset current_function_decl to NULL_TREE first, otherwise the
     * function we generated will be a nested function. */
    current_function_decl = NULL_TREE;

    location_t loc = DECL_SOURCE_LOCATION(wrapped_fn);
    struct c_declspecs* specs = this->build_specs(loc);
    struct c_arg_info *args = this->build_args(wrapped_fn, extra_arg);

    tree attrs = specs->attrs;
    specs->attrs = NULL_TREE;
    start_function(specs, this->build_declarator(fn_name, args, loc), attrs);

    tree fn_decl = current_function_decl;
    store_parm_decls();
    add_stmt(this->build_fn_body(
      build_type, wrapped_fn, list_length(args->parms), extra_arg
    ));
    finish_function();

    this->fndecl_cache[cache_key] = fn_decl;
    return fn_decl;
  }

  void build_proc_entry(tree_stmt_iterator *head, bool is_sync) {
    tree node = tsi_stmt(*head);
    if (TREE_CODE(node) != CALL_EXPR ||
        get_callee_name(node) != csp_proc_nchild_set) {
      std::cerr << err_prefix
        << "It must be the CALL_EXPR " << csp_proc_nchild_set
        << " immediately after " << csp_sched_proc_anchor
        << std::endl;
      exit(EXIT_FAILURE);
    }

    /* Count the number of task calls. */
    int n = -1;
    for (tree_stmt_iterator it = *head; !tsi_end_p(it); n++) {
      node = tsi_stmt(it);
      if (TREE_CODE(node) != CALL_EXPR) {
        std::cerr << err_prefix
          << "Tasks in `csp_async` or `csp_sync` should be CALL_EXPR."
          << std::endl;
        exit(EXIT_FAILURE);
      }
      if (get_callee_name(node) == csp_sched_yield) {
        break;
      }
      tsi_next(&it);
    }

    /* Remove the whole block if no CALL_EXPR found. */
    if (n == 0) {
      while (get_callee_name(tsi_stmt(*head)) != csp_sched_yield) {
        tsi_delink(head);
      }
      /* Delete `csp_sched_yield()`. */
      tsi_delink(head);
      return;
    }

    if (is_sync) {
      /* Set parameter `nchild` of `csp_proc_nchild_set()`. We must clone it
       * first cause different `csp_sched_run` share the same AST node. */
      node = tsi_stmt(*head);
      CALL_EXPR_ARG(node, 0) = copy_node(CALL_EXPR_ARG(node, 0));
      TREE_INT_CST_ELT(CALL_EXPR_ARG(node, 0), 0) = n;

      /* Preserve `csp_proc_nchild_set()` and skip to the next. */
      tsi_next(head);
    } else {
      /* Delete `csp_proc_nchild_set()`. */
      tsi_delink(head);
    }

    /* Rebuild all CALL_EXPR. */
    while (get_callee_name(tsi_stmt(*head)) != csp_sched_yield) {
      node = tsi_stmt(*head);
      tsi_link_before(head, build_call_expr_loc_array(
        EXPR_LOCATION(node), build_fn(
          is_sync ? TYPE_SYNC_PROC : TYPE_ASYNC_PROC,
          TREE_OPERAND(CALL_EXPR_FN(node), 0), NULL_TREE
        ), call_expr_nargs(node), CALL_EXPR_ARGP(node)
      ), TSI_SAME_STMT);
      tsi_delink(head);
    }

    if (is_sync) {
      /* Preserve `csp_sched_yield()` and skip to the next. */
      tsi_next(head);
    } else {
      /* Delete `csp_sched_yield()`. */
      tsi_delink(head);
    }
  }

  void build_timer_entry(tree_stmt_iterator *curr, tree csp_timer_anchor_call) {
    tree node = tsi_stmt(*curr);
    if (TREE_CODE(node) != CALL_EXPR) {
      std::cerr << err_prefix
        << "It must be a CALL_EXPR immediately after csp_timer_anchor."
        << std::endl;
      exit(EXIT_FAILURE);
    }

    int args_n = call_expr_nargs(node);
    tree *args = XALLOCAVEC(tree, args_n + 1);

    for (int i = 0; i < args_n; i++) {
      args[i] = CALL_EXPR_ARG(node, i);
    }
    /* Set the last argument to timestamp. */
    args[args_n] = CALL_EXPR_ARG(csp_timer_anchor_call, 0);

    tsi_link_before(curr, build_call_expr_loc_array(
      EXPR_LOCATION(node), this->build_fn(
        TYPE_TIMER_PROC,
        TREE_OPERAND(CALL_EXPR_FN(node), 0),
        DECL_ARGUMENTS(TREE_OPERAND(CALL_EXPR_FN(csp_timer_anchor_call), 0))
      ), args_n + 1, args
    ), TSI_SAME_STMT);

    tsi_delink(curr);
  }

  void build(tree node) {
    if (node == NULL_TREE) {
      return;
    }

    switch (TREE_CODE(node)) {
      case FUNCTION_DECL: {
        auto name = std::string(fndecl_name(node));
        if (!csp::namer.is_generated(name)) {
          this->build(DECL_SAVED_TREE(node));
          if (name == csp_main) {
            this->build_fn(TYPE_MAIN_FUNC, this->build_fn(
              TYPE_MAIN_PROC, node, NULL_TREE
            ), NULL_TREE);
          }
        }
        break;
      }
      case BIND_EXPR: {
        this->build(BIND_EXPR_BODY(node));
        break;
      }
      case DECL_EXPR: {
        node = DECL_EXPR_DECL(node);
        if (VAR_P(node) && DECL_INITIAL(node)) {
          this->build(DECL_INITIAL(node));
        }
        break;
      }
      case TARGET_EXPR: {
        this->build(TARGET_EXPR_INITIAL(node));
        break;
      }
      case STATEMENT_LIST: {
        tree_stmt_iterator it = tsi_start(node);
        while (!tsi_end_p(it)) {
          node = tsi_stmt(it);
          if (TREE_CODE(node) != CALL_EXPR) {
            this->build(node);
            tsi_next(&it);
            continue;
          }

          std::string callee_name = get_callee_name(node);
          if (callee_name == csp_sched_proc_anchor) {
            tsi_delink(&it);
            bool need_sync = TREE_INT_CST_LOW(CALL_EXPR_ARG(node, 0));
            this->build_proc_entry(&it, need_sync);
            continue;
          } else if (callee_name == csp_timer_anchor) {
            tsi_delink(&it);
            this->build_timer_entry(&it, node);
            continue;
          }
          tsi_next(&it);
        }
        break;
      }
      case COND_EXPR: {
        this->build(COND_EXPR_THEN(node));
        this->build(COND_EXPR_ELSE(node));
        break;
      }
      case SWITCH_EXPR: {
        this->build(SWITCH_BODY(node));
        break;
      }
      case CONVERT_EXPR:
      case RETURN_EXPR: {
        this->build(TREE_OPERAND(node, 0));
        break;
      }
      case MODIFY_EXPR: {
        this->build(TREE_OPERAND(node, 1));
        break;
      }
      default: {
        return;
      }
    }
  }

private:
  std::unordered_map<std::string, tree> fndecl_cache;
} proc_builder;

}

#endif
