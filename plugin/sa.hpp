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

#ifndef LIBCSP_PLUGIN_SA_HPP
#define LIBCSP_PLUGIN_SA_HPP

#include <algorithm>
#include <ctime>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "fs.hpp"
#include "namer.hpp"

namespace csp {

const std::string call_graph_ext            = ".cg";
const std::string stack_frame_ext           = ".sf";
const std::string config_file_name          = "config.c";
const std::string csp_prefix                = "csp_";
const std::string fn_exit                   = "exit";
const std::string fn_csp_core_proc_exit     = "csp_core_proc_exit";

const size_t default_max_threads            = 1024;
const size_t default_max_procs_hint         = 100000;
const size_t default_default_stack_size     = 1 << 11;

const int flag_stack_by_user                = 0x01;
const int flag_csp_only                     = 0x02;

typedef enum {
  STATIC = 0,
  DYNAMIC,
  DYNAMIC_BOUNDED,
  /* This type means we compute the stack frame manually. */
  MANUALLY,
} stack_usage_type_t;

class stack_usage_t {
public:
  stack_usage_type_t type;

  /* The max stack size of of a function. */
  int64_t max_stack_size;

  /* Stack size set by user. We will use it first if it's set. `-1` means it's
   * not set. */
  int64_t stack_by_user;

  /* Stack frame size of the function. */
  int64_t frame_size;

  /* This field is only for process. It includes `Return Address`, `Memory
   * Arguments` and `Padding` in the process memory layout. See `src/proc.h`. */
  int64_t proc_reserved;

  stack_usage_t(int64_t max_stack_size, int64_t frame_size):
    type(STATIC),
    max_stack_size(max_stack_size),
    frame_size(frame_size),
    stack_by_user(-1),
    proc_reserved(-1) {}

  stack_usage_t(int64_t max_stack_size): stack_usage_t(max_stack_size, -1) {}

  stack_usage_t(): stack_usage_t(-1, -1) {}

  std::string string() {
    std::stringstream ss;
    ss << "<stack_usage_t "
      << "type: " << this->type << " "
      << "max_stack_size: " << this->max_stack_size << " "
      << "stack_by_user: " << this->stack_by_user << " "
      << "frame_size: " << this->frame_size << " "
      << "proc_reserved: " << this->proc_reserved
      << ">";
    return ss.str();
  }

};

class analyzer_options_t {
public:
  bool is_building_libcsp;
  std::string installed_prefix;
  std::string working_dir;
  std::string extra_su_file;
  size_t default_stack_size;
  size_t cpu_cores;
  size_t max_threads;
  size_t max_procs_hint;

  analyzer_options_t():
    is_building_libcsp(false),
    installed_prefix(default_installed_prefix),
    working_dir(default_working_dir),
    extra_su_file(""),
    default_stack_size(default_default_stack_size),
    cpu_cores(0),
    max_threads(default_max_threads),
    max_procs_hint(default_max_procs_hint)
  {}
};

class analyzer_t: public filesystem_t {
public:
  void add_call(std::string caller, std::string callee) {
    this->call_graph[caller].insert(callee);
  }

  void set_callees(std::string caller, std::set<std::string> callees) {
    this->call_graph[caller] = callees;
  }

  void add_stack_usage(std::string fn_name, stack_usage_t su) {
    this->stack_usages[fn_name] = su;
  }

  void save(void) {
    this->save_call_graph();
    this->save_stack_usage();
  }

  void load(void) {
    if (!this->options.is_building_libcsp) {
      this->load_from_dir(this->options.installed_prefix + subpath_share);
    }

    this->load_from_dir(this->get_working_dir());
    if (this->options.extra_su_file != "") {
      this->load_stack_usage_from_file(
        this->options.extra_su_file, flag_stack_by_user
      );
    }
  }

  /*
   * Our target is to compute the total memory usage of every process.
   *
   * To compute the memory size of a process, we need to compute the size of
   * every part of it which is described in `src/proc.h`. Actually we already
   * know the size of all the parts except that of `Stack`. The `Stack` size is
   * `max(wrapper_funcs.max_stack_size, csp_core_proc_exit.max_stack_size) + 8`.
   * We already know `csp_core_proc_exit.max_stack_size`, so the only thing we
   * need to do is analyzing the `wrapper_funcs.max_stack_size`.
   *
   * For a given common function `f`, it's max stack size `f.max_stack_size` is
   * `f.frame_size + 8 + max([callee.max_stack_size for callee in f.callees])`.
   * So we need to get all its callees' max stack size, and for getting a
   * callee's max stack size we need to get all callees' max stack size of this
   * callee and so on till the callee is a leaf function.
   *
   * A simple straightforward algorithm is DFS, but when the call graph is too
   * deep, the stack may overflows. So we use Toplogical sorting(BFS) here.
   */
  void analyze(analyzer_options_t options) {
    this->options = options;
    this->set_working_dir(this->options.working_dir);
    this->load();

    auto wrapper_funcs = this->collect_wrapper_funcs();
    if (wrapper_funcs.size() == 0) {
      this->gen_config(wrapper_funcs);
      return;
    }

    auto order = this->get_analyzing_order(wrapper_funcs);
    for (auto caller: order) {
      auto &su = this->stack_usages[caller];

      /* Already analyzed. */
      if (su.max_stack_size >= 0) {
        continue;
      }

      if (su.stack_by_user >= 0) {
        su.max_stack_size = su.stack_by_user;
        continue;
      }

      size_t max_stack_size = 0;
      for (auto callee: this->call_graph[caller]) {
        size_t size = this->must_get_max_stack_size(callee);
        if (size > max_stack_size) {
          max_stack_size = size;
        }
      }
      su.max_stack_size = max_stack_size + 8;
    }

    /* Compute the final memory usage of every process. */
    for (auto pair: wrapper_funcs) {
      auto size = this->must_get_max_stack_size(
        pair.second == "csp_main" ? fn_exit : fn_csp_core_proc_exit
      );

      auto &su = this->stack_usages[pair.second];
      if (su.max_stack_size < size) {
        su.max_stack_size = size;
      }

      /* The size of `csp_proc_t`. Cause we make %rbp to be 16-bytes alignment,
       * so we add extra 8-bytes if `sizeof(csp_procs_t) % 16 != 0`. */
      size_t csp_proc_t_size = 22 << 3;

      /* All parts of the process plus 8-bytes call instruction space. */
      su.max_stack_size += su.proc_reserved + csp_proc_t_size + 8;
    }

    this->gen_config(wrapper_funcs);
  }

private:
  void save_call_graph(void) {
    /* We need to set the openmode to fstream::app cause the path may exists. */
    auto file = this->assert_open(
      this->gen_file_name(call_graph_ext), std::fstream::app
    );

    auto cg = this->call_graph;
    for (auto it = cg.begin(); it != cg.end(); it++) {
      if (it->second.size() == 0) {
        continue;
      }

      file << it->first;
      for (auto itt = it->second.begin(); itt != it->second.end(); itt++) {
        file << " " << *itt;
      }
      file << std::endl;
    }

    file.close();
  }

  /*
   * There are two or three columns in ervery line of the generated .su file.
   *
   * - The first column is the function name.
   * - The second column is its frame size.
   * - The third column is stack_usage_t.proc_reserved. It is saved to the file
   *   only when it's larger than zero.
   */
  void save_stack_usage(void) {
    /* We need to set the openmode to fstream::app cause the path may exists. */
    auto file = this->assert_open(
      this->gen_file_name(stack_frame_ext), std::fstream::app
    );

    auto su = this->stack_usages;
    for (auto it = su.begin(); it != su.end(); it++) {
      /* According to the GCC document:
       *
       *   > The qualifier `dynamic` means that the function frame size is not
       *   > static. It happens mainly when some local variables have a dynamic
       *   > size. When this qualifier appears alone, the second field is not a
       *   > reliable measure of the function stack analysis.
       *
       *   See `https://gcc.gnu.org/onlinedocs/gnat_ugn/Static-Stack-Usage-Analysis.html`.
       *
       * We know that `dynamic` is not reliable, so we will discard this kind of
       * stack usage. */
      if (it->second.type == DYNAMIC && it->second.frame_size < 0) {
        continue;
      }

      file << it->first << " " << it->second.frame_size;
      if (it->second.proc_reserved > 0) {
        file << " " << it->second.proc_reserved;
      }
      file << std::endl;
    }

    file.close();
  }

  void load_call_graph_from_file(std::string path) {
    auto file = this->assert_open(path, std::fstream::in);

    std::string line;
    while (std::getline(file, line)) {
      std::stringstream ss(line);

      std::string caller;
      if (!(ss >> caller)) {
        continue;
      }

      std::string callee;
      while (ss >> callee) {
        this->add_call(caller, callee);
      }
    }

    file.close();
  }

  void load_stack_usage_from_file(std::string path, int flags) {
    auto file = this->assert_open(path, std::fstream::in);

    std::string line;
    while (std::getline(file, line)) {
      std::stringstream ss(line);

      std::string fn;
      int64_t frame, reserved;

      if (!(ss >> fn) || !(ss >> frame) || frame < 0 || (
          (flags & flag_csp_only) &&
          fn.substr(0, csp_prefix.size()) != csp_prefix)) {
        continue;
      }

      stack_usage_t su;
      if (flags & flag_stack_by_user) {
        su.stack_by_user = frame;
      } else {
        su.frame_size = frame;
        if (ss >> reserved && namer.is_generated(fn) && reserved > 0) {
          su.proc_reserved = reserved;
        }
      }

      this->add_stack_usage(fn, su);
    }

    file.close();
  }

  void load_from_dir(std::string dir_path) {
    std::vector<std::string> sf_names, cg_names;
    std::unordered_map<std::string, std::vector<std::string>> exts({
      {stack_frame_ext, sf_names}, {call_graph_ext, cg_names}
    });

    auto dir = opendir(dir_path.c_str());
    if (dir == NULL) {
      std::cerr << err_prefix << "open " << dir_path << " failed." << std::endl;
      exit(EXIT_FAILURE);
    }

    while (true) {
      auto item = readdir(dir);
      if (item == NULL) {
        break;
      }

      std::string file_name(item->d_name);

      auto i = file_name.rfind('.');
      if (i == std::string::npos) {
        continue;
      }

      std::string ext = file_name.substr(i);
      if (exts.find(ext) != exts.end()) {
        exts[ext].push_back(
          file_name.substr(0, file_name.length() - ext.length())
        );
      }
    }
    closedir(dir);

    for (auto item: exts) {
      /* There may be garbage files built before, so we load files by time from
       * least recently to most recently. */
      std::sort(item.second.begin(), item.second.end(),
          [](std::string a, std::string b) -> bool {
        size_t time_a, time_b;

        std::stringstream ss;
        ss << a << " " << b;
        ss >> time_a;
        ss >> time_b;

        return time_a < time_b;
      });

      for (auto name: item.second) {
        if (item.first == call_graph_ext) {
          this->load_call_graph_from_file(dir_path + name + call_graph_ext);
        } else {
          int flags = 0;
          if (dir_path == this->options.installed_prefix + subpath_share) {
            flags |= flag_csp_only;
          }
          this->load_stack_usage_from_file(
            dir_path + name + stack_frame_ext, flags
          );
        }
      }
    }
  }

  /* Find all wrapper functions we need to analyze. */
  std::unordered_map<size_t, std::string> collect_wrapper_funcs(void) {
    /* The key is id of the wrapper function. */
    std::unordered_map<size_t, std::string> wrapper_funcs;

    for (auto item: this->stack_usages) {
      namer_entity_t entity;
      if (!namer.parse(item.first, entity)) {
        continue;
      }
      if (wrapper_funcs.find(entity.id) != wrapper_funcs.end()) {
        std::cerr << err_prefix << "duplicated process id." << std::endl;
        exit(EXIT_FAILURE);
      }
      wrapper_funcs[entity.id] = entity.name;
    }

    return wrapper_funcs;
  }

  /* Get the toplogical sorting order of the functions to be analyzed. */
  std::vector<std::string> get_analyzing_order(
      std::unordered_map<size_t, std::string> wrapper_funcs) {
    std::vector<std::string> order;

    /* We will use the stack size of csp_core_proc_exit later, so we put it to
     * the list to compute it. */
    std::queue<std::string> queue({fn_csp_core_proc_exit, fn_exit});
    std::set<std::string> visited({fn_csp_core_proc_exit, fn_exit});

    for (auto pair: wrapper_funcs) {
      if (visited.find(pair.second) == visited.end()) {
        queue.push(pair.second);
        visited.insert(pair.second);
      }
    }

    /* Reversed call graph. */
    std::unordered_map<std::string, std::set<std::string>> rcg;
    while (!queue.empty()) {
      std::string caller = queue.front();
      queue.pop();

      auto it = this->call_graph.find(caller);
      if (it != this->call_graph.end()) {
        for (auto callee: it->second) {
          rcg[callee].insert(caller);
          if (visited.find(callee) == visited.end()) {
            queue.push(callee);
            visited.insert(callee);
          }
        }
      }
    }

    /* Initialize the in degrees of all nodes in the reversed call graph. */
    std::unordered_map<std::string, size_t> degrees;
    for (auto pair: rcg) {
      auto from = pair.first;
      if (degrees.find(from) == degrees.end()) {
        degrees[from] = 0;
      }

      for (auto to: pair.second) {
        degrees[to]++;
      }
    }

    /* Collect all zero-degree nodes. */
    std::queue<std::string> zero_degrees;
    for (auto pair: degrees) {
      if (pair.second == 0) {
        zero_degrees.push(pair.first);
        if (this->stack_usages.find(pair.first) != this->stack_usages.end()) {
          auto &su = this->stack_usages[pair.first];
          if (su.stack_by_user >= 0) {
            su.max_stack_size = su.stack_by_user;
          } else {
            su.max_stack_size = su.frame_size;
          }
        } else {
          this->stack_usages[pair.first] = stack_usage_t(
            this->options.default_stack_size
          );
        }
      }
    }

    while (!zero_degrees.empty()) {
      auto from = zero_degrees.front();
      zero_degrees.pop();
      order.push_back(from);

      if (this->stack_usages.find(from) == this->stack_usages.end()) {
        this->stack_usages[from] = stack_usage_t();
      }

      for (auto to: rcg[from]) {
        degrees[to]--;
        if (degrees[to] == 0) {
          zero_degrees.push(to);
        }
      }
    }

    /* If there are circles in the call graph, the order may not contains all
     * wrapper functions. We put them to the end of order to force to compute
     * the max stack size of every wrapper function. */
    for (auto pair: wrapper_funcs) {
      order.push_back(pair.second);
      if (this->stack_usages.find(pair.second) == this->stack_usages.end()) {
        this->stack_usages[pair.second] = stack_usage_t();
      }
    }

    return order;
  }

  /* Return the max_stack_size of the function. If it doesn't exist or is not
   * computed yet, return the default_stack_size. */
  size_t must_get_max_stack_size(std::string name) {
    if (this->stack_usages.find(name) == this->stack_usages.end()) {
      return this->options.default_stack_size;
    }
    auto su = this->stack_usages[name];
    return su.max_stack_size >= 0 ? su.max_stack_size :
      this->options.default_stack_size;
  }

  /* Generate the configure file. */
  void gen_config(std::unordered_map<size_t, std::string> wrapper_funcs) {
    auto total = wrapper_funcs.size();

    auto file = this->assert_open(
      this->full_path(config_file_name), std::fstream::out
    );

    auto cpu_cores = this->options.cpu_cores;
    auto max_threads = this->options.max_threads;
    auto max_procs_hint = this->options.max_procs_hint;

    file
      << "// Configure file generated by libcsp cli." << std::endl
      << "//" << std::endl
      << "// DO NOT modify it!" << std::endl << std::endl
      << "#include <stdlib.h>" << std::endl
      << "size_t csp_cpu_cores = " << cpu_cores << ";" << std::endl
      << "size_t csp_max_threads = " << max_threads << ";" << std::endl
      << "size_t csp_max_procs_hint = " << max_procs_hint << ";" << std::endl
      << "size_t csp_procs_num = " << total << ";" << std::endl;

    file << "size_t csp_procs_size[] = {";
    for (decltype(total) id = 0; id < total; id++) {
      auto it = wrapper_funcs.find(id);
      if (it == wrapper_funcs.end()) {
        std::cerr
          << err_prefix << "process id " << id << " not found" << std::endl;
        exit(EXIT_FAILURE);
      }

      size_t page_size = 1 << 12;
      size_t size = this->stack_usages[it->second].max_stack_size;
      file << ((size / page_size) + !!(size % page_size)) * page_size << ", ";
    }
    file << "};" << std::endl;

    file.close();
  }

private:
  std::unordered_map<std::string, stack_usage_t> stack_usages;
  std::unordered_map<std::string, std::set<std::string>> call_graph;
  analyzer_options_t options;
} analyzer;

}

#endif
