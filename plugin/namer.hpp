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

#ifndef LIBCSP_PLUGIN_NAMER_HPP
#define LIBCSP_PLUGIN_NAMER_HPP

#include <iostream>
#include <string>
#include <vector>
#include "fs.hpp"

namespace csp {

const std::string csp_proc_prefix           = "csp___";
const std::string default_installed_prefix  = "/usr/local/";
const std::string subpath_share             = "share/libcsp/";

const std::vector<std::string> namer_type_labels = {
  "async", "sync", "timer", "other"
};

typedef enum {
  NAMER_TYPE_ASYNC,
  NAMER_TYPE_SYNC,
  NAMER_TYPE_TIMER,
  NAMER_TYPE_OTHER
} namer_type_t;

class namer_entity_t {
public:
  size_t id;
  std::string name;
  namer_type_t type;

  namer_entity_t() = default;

  namer_entity_t(size_t id, std::string name, namer_type_t type):
    id(id), name(name), type(type)
  {};
};

class namer_t: public filesystem_t {
public:
  namer_t(): next_id(0), prefix(csp_proc_prefix) {};

  void initialize(bool is_building_libcsp, std::string installed_prefix,
      std::string working_dir) {
    this->set_working_dir(working_dir);
    this->next_id = this->read_session(this->full_path(session_name));

    /* If we are building our apps, set the next_id to that stored in session
     * file which is in the `share` path. */
    if (this->next_id == 0 && !is_building_libcsp) {
      if (installed_prefix[installed_prefix.size() - 1] != slash) {
        installed_prefix.push_back(slash);
      }
      this->next_id = this->read_session(
        installed_prefix + subpath_share + session_name
      );
    }
  }

  int current_id(void) {
    return this->next_id - 1;
  }

  std::string current_name(void) {
    return this->latest_name;
  }

  std::string next_name(std::string fn_name, namer_type_t type) {
    this->latest_name = this->format(
      namer_entity_t(this->next_id, fn_name, type)
    );
    this->next_id++;
    return this->latest_name;
  }

  bool is_generated(std::string name) {
    for (auto label: namer_type_labels) {
      std::string prefix(this->prefix + label);
      if (name.substr(0, prefix.size()) == prefix) {
        return true;
      }
    }
    return false;
  }

  bool parse(std::string name, namer_entity_t &entity) {
    if (!this->is_generated(name)) {
      return false;
    }

    name = name.substr(this->prefix.size());

    for (int i = 0; i < 2; i++) {
      auto idx = name.find('_');
      if (idx == std::string::npos) {
        return false;
      }

      std::string part = name.substr(0, idx);
      if (i == 0) {
        bool found = false;
        for (int j = 0; j < namer_type_labels.size(); j++) {
          if (part == namer_type_labels[j]) {
            found = true;
            entity.type = (namer_type_t)j;
            break;
          }
        }
        if (!found) {
          return false;
        }
      } else {
        std::stringstream ss(part);
        if (!(ss >> entity.id)) {
          return false;
        }
      }
      name = name.substr(idx + 1);
    }

    entity.name = name;
    return true;
  }

  void save(void) {
    auto file = this->open(this->full_path(session_name), std::fstream::out);
    if (!file.is_open()) {
      std::cerr << err_prefix << "failed to save session info." << std::endl;
      exit(EXIT_FAILURE);
    }

    file << this->next_id;
    file.close();
  }

private:
  std::string format(namer_entity_t entity) {
    std::stringstream ss;
    ss << this->prefix
      << namer_type_labels[entity.type]
      << "_"
      << entity.id
      << "_"
      << entity.name;
    return ss.str();
  }

  int next_id;
  std::string prefix;
  std::string latest_name;
} namer;

}

#endif
