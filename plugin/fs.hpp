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

#ifndef LIBCSP_PLUGIN_FS_HPP
#define LIBCSP_PLUGIN_FS_HPP

#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

namespace csp {

const std::string default_working_dir       = "/tmp/libcsp/";
const std::string session_name              = ".session";
const std::string err_prefix                = "libcsp error: ";
const char slash                            = '/';

class filesystem_t {
public:
  filesystem_t(): working_dir(default_working_dir) {}

  void set_working_dir(std::string working_dir) {
    if (working_dir == "") {
      working_dir = default_working_dir;
    }
    if (working_dir[working_dir.size() - 1] != slash) {
      working_dir.push_back(slash);
    }
    this->working_dir = working_dir;
  }

  std::string get_working_dir(void) {
    return this->working_dir;
  }

  static bool exist(std::string path) {
    return access(path.c_str(), F_OK) == EXIT_SUCCESS;
  }

  std::fstream open(std::string path, std::ios_base::openmode mode) {
    std::fstream file;
    file.open(path, mode);
    return file;
  }

  std::fstream assert_open(std::string path, std::ios_base::openmode mode) {
    auto file = this->open(path, mode);
    if (!file.is_open()) {
      std::cerr << err_prefix << "failed to open " << path << std::endl;
      exit(EXIT_FAILURE);
    }
    return file;
  }

  std::string gen_file_name(std::string ext) {
    std::stringstream ss;
    ss << this->working_dir;
    ss << time(NULL);
    ss << ext;
    return ss.str();
  }

  std::string full_path(std::string subpath) {
    std::string path(this->working_dir);
    if (subpath.size() > 0 && subpath[0] == slash) {
      path += subpath.substr(1);
    } else {
      path += subpath;
    }
    return path;
  }

  int read_session(std::string path) {
    std::string content;
    auto file = this->open(path, std::fstream::in);
    if (file.is_open()) {
      file >> content;
      file.close();
    }

    int next_id = 0;
    if (content.length() > 0) {
      std::stringstream ss(content);
      ss >> next_id;
    }
    return next_id;
  }

private:
  std::string working_dir;
};

}

#endif
