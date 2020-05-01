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

#include <cstdio>
#include <getopt.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <vector>
#include "fs.hpp"
#include "sa.hpp"

namespace csp {

const std::string cli_usage = (
  "Usage:                                                                    \n"
  "  cspcli <command> [options]                                              \n"
  "                                                                          \n"
  "Commands:                                                                 \n"
  "  init:                                                                   \n"
  "    Initialize the environment for the building. Libcsp will create the   \n"
  "    working directory if it doesn't exist or otherwise clean the generated\n"
  "    files left by the previous building.                                  \n"
  "                                                                          \n"
  "    Options:                                                              \n"
  "      --working-dir:                                                      \n"
  "        The working directory. Default is /tmp/libcsp.                    \n"
  "                                                                          \n"
  "  analyze:                                                                \n"
  "    Analyze the memory usages of processes and generate the configuration \n"
  "    file `config.c`. Libcsp plugin will generate the function stack frame \n"
  "    size to files with extension .sf and the function call graph to files \n"
  "    with extension .cg. This command will analyze the memory usage of all \n"
  "    processes according to these files. You can set some configurations   \n"
  "    with the following options:                                           \n"
  "                                                                          \n"
  "    Options:                                                              \n"
  "      --working-dir:                                                      \n"
  "        The working directory. Default is `/tmp/libcsp/`.                 \n"
  "      --installed-prefix:                                                 \n"
  "        The value of option `--prefix` in `./configure` when you build and\n"
  "        install libcsp from source. Default is `/usr/local/`.             \n"
  "      --extra-su-file:                                                    \n"
  "        The extra stack usage file. The format of every line in it is `fn \n"
  "        size`(e.g. `main 64`). It's used first if it's set.               \n"
  "      --default-stack-size:                                               \n"
  "        The default stack size for an unknown function. Default is 2KB.   \n"
  "      --cpu-cores:                                                        \n"
  "        The number of CPU cores on which libcsp will run. Default is max  \n"
  "        CPU cores.                                                        \n"
  "      --max-threads:                                                      \n"
  "        The max threads libcsp can create. Default is 1024.               \n"
  "      --max-procs-hint:                                                   \n"
  "        The hint of the max processes. Libcsp will initialize related     \n"
  "        resource according to it. Default is 100000.                      \n"
  "                                                                          \n"
  "  clean:                                                                  \n"
  "    Clear related generated files in the working directory.               \n"
  "                                                                          \n"
  "    Options:                                                              \n"
  "      --working-dir:                                                      \n"
  "        The working directory. Default is /tmp/libcsp/.                   \n"
  "                                                                          \n"
  "  version:                                                                \n"
  "    Display the cspcli version.                                           \n"
);

const std::string cli_version     = "0.0.1";

const std::string cli_cmd_init    = "init";
const std::string cli_cmd_analyze = "analyze";
const std::string cli_cmd_clean   = "clean";
const std::string cli_cmd_version = "version";

typedef enum {
  cli_cmd_type_init = 1,
  cli_cmd_type_analyze,
  cli_cmd_type_clean,
  cli_cmd_type_version,
} cli_cmd_type_t;

std::unordered_map<std::string, cli_cmd_type_t> cli_cmd_types = {
  {cli_cmd_init,          cli_cmd_type_init},
  {cli_cmd_analyze,       cli_cmd_type_analyze},
  {cli_cmd_clean,         cli_cmd_type_clean},
  {cli_cmd_version,       cli_cmd_type_version},
};

struct option cli_cmd_init_and_clean_options[] = {
  {"working-dir",         optional_argument, NULL, 0},
};

struct option cli_cmd_analyze_options[] = {
  /* Used to compile libcsp itself. */
  {"building-libcsp",     optional_argument, NULL, 0},

  {"installed-prefix",    optional_argument, NULL, 0},
  {"working-dir",         optional_argument, NULL, 0},
  {"extra-su-file",       optional_argument, NULL, 0},
  {"default-stack-size",  optional_argument, NULL, 0},
  {"cpu-cores",           optional_argument, NULL, 0},
  {"max-threads",         optional_argument, NULL, 0},
  {"max-procs-hint",      optional_argument, NULL, 0},
  {NULL,                  no_argument,       NULL, 0}
};

class cli_t: public filesystem_t {
public:
  bool parse_options(int argc, char *argv[], cli_cmd_type_t cmd_type,
      analyzer_options_t &options) {
    struct option *longopts;

    switch (cmd_type) {
    case csp::cli_cmd_type_init:
    case csp::cli_cmd_type_clean:
      longopts = csp::cli_cmd_init_and_clean_options;
      break;
    case csp::cli_cmd_type_analyze:
      longopts = csp::cli_cmd_analyze_options;
      break;
    case csp::cli_cmd_type_version:
      return true;
    default:
      return false;
    }

    int val, idx;
    while ((val = getopt_long(argc, argv, "", longopts, &idx)) != -1) {
      if (val == '?') {
        return false;
      }

      if (optarg == NULL) {
        continue;
      }

      std::string arg_val(optarg);
      if (arg_val == "") {
        continue;
      }

      std::string arg_name((longopts + idx)->name);
      if ((arg_name == "installed-prefix" || arg_name == "extra-su-file") &&
          !filesystem_t::exist(arg_val)) {
        std::cerr << err_prefix << arg_val << " doesn't exist." << std::endl;
        return false;
      }

      switch (cmd_type) {
      case csp::cli_cmd_type_init:
      case csp::cli_cmd_type_clean:
        options.working_dir = arg_val;
        break;
      case csp::cli_cmd_type_analyze:
        int64_t num;
        if (idx >= 4) {
          std::stringstream ss(arg_val);
          if (!(ss >> num) || num <= 0) {
            break;
          }
        }

        switch (idx) {
        case 0:
          options.is_building_libcsp = arg_val == "true";
          break;
        case 1:
          options.installed_prefix = arg_val;
          break;
        case 2:
          options.working_dir = arg_val;
          break;
        case 3:
          options.extra_su_file = arg_val;
          break;
        case 4:
          options.default_stack_size = num;
          break;
        case 5:
          options.cpu_cores = num;
          break;
        case 6:
          options.max_threads = num;
          break;
        case 7:
          options.max_procs_hint = num;
          break;
        }
      }
    }
    return true;
  }

  void init(void) {
    std::string working_dir = this->get_working_dir();
    if (this->exist(working_dir)) {
      this->clean();
    } else if (mkdir(working_dir.c_str(), S_IRWXU) == EXIT_FAILURE) {
      std::cerr
        << err_prefix << "create working directory failed" << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  void analyze(analyzer_options_t options) {
    analyzer.analyze(options);
  }

  void clean(void) {
    if (!this->exist(this->get_working_dir())) {
      return;
    }

    auto dir = opendir(this->get_working_dir().c_str());
    while (true) {
      auto item = readdir(dir);
      if (item == NULL) {
        break;
      }

      std::string file_name(item->d_name), ext;

      auto i = file_name.rfind('.');
      if (i != std::string::npos) {
        ext = file_name.substr(i);
      }

      if ((file_name == config_file_name || file_name == session_name ||
          ext == call_graph_ext || ext == stack_frame_ext) &&
          remove(this->full_path(file_name).c_str()) == -1) {
        std::cerr
          << err_prefix
          << "clean failed, you may need to clean the file with `rm -f "
          << this->full_path(file_name) << " manually."
          << std::endl;
      }
    }
    closedir(dir);
  }

  void version(void) {
    std::cout << csp::cli_version << std::endl;
  }
};

}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << csp::cli_usage << std::endl;
    return EXIT_FAILURE;
  }

  std::string cmd = std::string(argv[1]);

  if (cmd == "-h" || cmd == "--help") {
    std::cout << csp::cli_usage << std::endl;
    return EXIT_SUCCESS;
  }

  if (csp::cli_cmd_types.find(cmd) == csp::cli_cmd_types.end()) {
    std::cerr << csp::err_prefix
      << "invalid command " << cmd << "!" << std::endl << std::endl
      << csp::cli_usage << std::endl;
    return EXIT_FAILURE;
  }

  csp::cli_t cli;
  csp::analyzer_options_t options;
  csp::cli_cmd_type_t cmd_type = csp::cli_cmd_types[cmd];

  if (!cli.parse_options(argc, argv, cmd_type, options)) {
    return EXIT_FAILURE;
  }

  cli.set_working_dir(options.working_dir);

  switch (cmd_type) {
  case csp::cli_cmd_type_init:
    cli.init();
    break;
  case csp::cli_cmd_type_analyze:
    cli.analyze(options);
    break;
  case csp::cli_cmd_type_clean:
    cli.clean();
    break;
  case csp::cli_cmd_type_version:
    cli.version();
    break;
  }
  return EXIT_SUCCESS;
}
