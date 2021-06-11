#pragma once

#include <stdio.h>

#include <string>

#include "log.h"

using std::string;

void remove_file(const char* path) {
  if (access(path, F_OK)) {
    if (access(path, W_OK)) {
      debug_log("removing file: `%s'", path);
      remove(path);
    } else {
      daemon_log(LOG_ERR, "cannot remove file `%s'", path);
      exit(EXIT_FAILURE);
    }
  } else {
    debug_log("file: `%s' does not exist (yet)", path);
  }
}

void store_config(const char* path) {
  // save config
  string full_command = "jetson_clocks --store ";
  full_command += path;
  if (system(full_command.c_str()) == 0) {
    // success
  } else {
    daemon_log(LOG_ERR, "cannot save config file `%s'", path);
    exit(EXIT_FAILURE);
  }
  return;
}

void restore_config(const char* path) {
  // reload from file
  string full_command = "jetson_clocks --restore ";
  full_command += path;
  if (system(full_command.c_str()) == 0) {
    return;
  } else {
    daemon_log(LOG_ERR, "cannot restore config file `%s'", path);
    exit(EXIT_FAILURE);
  }
  return;
}

void clocks_max_freq() {
  // set all to max freq
  if (system("jetson_clocks") == 0) {
    // success
  } else {
    daemon_log(LOG_ERR, "cannot set max frequency");
    exit(EXIT_FAILURE);
  }
  return;
}
