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

#include "gcc-plugin.h"
#include "plugin-version.h"
#include "fs.hpp"
#include "namer.hpp"
#include "proc.hpp"
#include "sa.hpp"
#include <iostream>
#include <unordered_map>

int plugin_is_GPL_compatible = 1;

const char *plugin_name = "libcsp";

/* Since we rename the `main` function to `csp_main` and add a new `main`
 * function manually, we should mark whether we have already handled it to
 * prevent handling the newly added `main` function again. */
bool is_main_handled = false;

std::unordered_map<std::string, std::pair<csp::stack_usage_t,
    std::set<std::string>>> naked_func_infos = {
  {"csp_proc_restore",         {csp::stack_usage_t(-1, 0), {}}},
  {"csp_core_anchor_save",     {csp::stack_usage_t(-1, 0), {}}},
  {"csp_core_anchor_restore",  {csp::stack_usage_t(-1, 0), {}}},
  /* `csp_core_proc_exit_inner` calls `csp_proc_destroy` on the thread stack,
   * so we ignore it. */
  {"csp_core_proc_exit_inner", {csp::stack_usage_t(-1, 0), {}}},
  {
    "csp_core_block_epilogue",
    {csp::stack_usage_t(-1, 8), {"csp_core_block_epilogue_inner"}}
  },
  {
    "csp_core_yield",
    {csp::stack_usage_t(-1, 8), {"csp_core_anchor_restore"}}
  },
};

tree collect_call(tree *node, int *walk_subtrees, void *caller) {
  if (TREE_CODE(*node) == CALL_EXPR) {
    csp::analyzer.add_call(*(std::string *)caller, csp::get_callee_name(*node));
  }
  return NULL;
}

void on_start_unit(void *gcc_data, void *user_data) {
  /* Tell GCC to record the stack frame usage. */
  flag_stack_usage_info = true;
}

void on_start_parse_function(void *gcc_data, void *user_data) {
  tree fndecl = (tree)gcc_data;
  if (std::string(fndecl_name(fndecl)) == csp::main && !is_main_handled) {
    /* Change the name of `main()` to `csp_main`. */
    DECL_NAME(fndecl) = get_identifier(csp::csp_main.c_str());
    is_main_handled = true;
  }
}

void on_finish_parse_function(void *gcc_data, void *user_data) {
  tree fndecl = (tree)gcc_data;
  csp::proc_builder.build(fndecl);
  std::string caller(fndecl_name(fndecl));

  /* Collect all functions called by current function. */
  walk_tree_without_duplicates(&DECL_SAVED_TREE(fndecl), collect_call, &caller);
}

void on_all_passes_start(void *gcc_data, void *user_data) {
  /* Silence all "stack usage computation not supported for this target"
   * warnnings. */
  current_function_static_stack_size = 0;
}

void on_all_passes_end(void *gcc_data, void *user_data) {
  std::string current_name = current_function_name();

  /* Stack frame size of wrapper function is already set in build_fn_body, so we
   * just return here. */
  if (csp::namer.is_generated(current_name)) {
    return;
  }

  auto it = naked_func_infos.find(current_name);
  if (it != naked_func_infos.end()) {
    csp::analyzer.add_stack_usage(current_name, it->second.first);
    csp::analyzer.set_callees(current_name, it->second.second);
    return;
  }

  /* Below is mainly modified from function `output_stack_usage1` in GCC. */
  csp::stack_usage_type_t stack_usage_type = csp::STATIC;
  int_fast64_t stack_usage = current_function_static_stack_size;

  /* Add the maximum amount of space pushed onto the stack. */
  if (maybe_ne(current_function_pushed_stack_size, 0)) {
    int_fast64_t extra;
    if (current_function_pushed_stack_size.is_constant(&extra)) {
      stack_usage += extra;
      stack_usage_type = csp::DYNAMIC_BOUNDED;
    } else {
      extra = constant_lower_bound(current_function_pushed_stack_size);
      stack_usage += extra;
      stack_usage_type = csp::DYNAMIC;
    }
  }

  /* Now on to the tricky part: dynamic stack allocation. */
  if (current_function_allocates_dynamic_stack_space) {
    if (stack_usage_type != csp::DYNAMIC) {
      if (current_function_has_unbounded_dynamic_stack_size) {
        stack_usage_type = csp::DYNAMIC;
      } else {
        stack_usage_type = csp::DYNAMIC_BOUNDED;
      }
    }

    /* Add the size even in the unbounded case, this can't hurt. */
    stack_usage += current_function_dynamic_stack_size;
  }

  csp::stack_usage_t su;
  su.type = stack_usage_type;
  su.frame_size = stack_usage;
  csp::analyzer.add_stack_usage(current_name, su);
}

void on_finish(void *gcc_data, void *user_data) {
  csp::namer.save();
  csp::analyzer.save();
}

int plugin_init(struct plugin_name_args *plugin,
    struct plugin_gcc_version *version) {
  if (!plugin_default_version_check(version, &gcc_version)) {
    return 1;
  }

  plugin->base_name = (char *)plugin_name;
  plugin->help = "Plugin for building concurrency programs.";

  int events[] = {
    PLUGIN_START_UNIT,
    PLUGIN_START_PARSE_FUNCTION,
    PLUGIN_FINISH_PARSE_FUNCTION,
    PLUGIN_ALL_PASSES_START,
    PLUGIN_ALL_PASSES_END,
    PLUGIN_FINISH
  };

  plugin_callback_func callbacks[] = {
    on_start_unit,
    on_start_parse_function,
    on_finish_parse_function,
    on_all_passes_start,
    on_all_passes_end,
    on_finish
  };

  for (size_t i = 0; i < (sizeof(events) / sizeof(int)); i++) {
    register_callback(plugin_name, events[i], callbacks[i], NULL);
  }

  auto is_building_libcsp = false;
  auto working_dir = csp::default_working_dir;
  auto installed_prefix =csp::default_installed_prefix;

  for (int i = 0; i < plugin->argc; i++) {
    auto key = std::string(plugin->argv[i].key);
    auto val = std::string(plugin->argv[i].value);

    if (val == "") {
      continue;
    }

    if (key == "building-libcsp") {
      is_building_libcsp = val == "true";
    } else if (!csp::filesystem_t::exist(val)) {
      std::cerr << csp::err_prefix << val << " doesn't exist." << std::endl;
      return EXIT_FAILURE;
    }

    if (key == "installed-prefix") {
      installed_prefix = val;
    } else if (key == "working-dir") {
      working_dir = val;
    }
  }

  csp::namer.initialize(is_building_libcsp, installed_prefix, working_dir);
  csp::analyzer.set_working_dir(working_dir);

  return EXIT_SUCCESS;
}
