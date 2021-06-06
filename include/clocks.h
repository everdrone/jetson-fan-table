#pragma once

#include "defines.h"
#include "log.h"
#include "subprocess.hpp"  // FIXME: daemon-safe?
#include "utils.h"

int jetson_clocks_store(const char* path) {
  // saves configuration
  // subprocess::popen cmd(JETSON_CLOCKS_PATH, {"--store", path});
  subprocess::popen cmd(JETSON_CLOCKS_PATH, {path});

  std::string err;
  std::getline(cmd.stderr(), err);

  if (err.size() > 0) {
    daemon_log(LOG_ERR, "failed to run jetson-clocks --store");
    sprintf_stderr("%s: failed to run jetson-clocks --store %s", argv0, path);
    std::cerr << err << std::endl;
    exit(EXIT_FAILURE);
  }

  return cmd.wait();
}

int jetson_clocks_restore(const char* path) {
  // restore previous configuration
  subprocess::popen cmd(JETSON_CLOCKS_PATH, {"--restore", path});

  std::string err;
  std::getline(cmd.stderr(), err);

  if (err.size() > 0) {
    daemon_log(LOG_ERR, "failed to run jetson-clocks --restore");
    sprintf_stderr("%s: failed to run jetson-clocks --restore %s", argv0, path);
    std::cerr << err << std::endl;
    // do not exit here
  }

  return cmd.wait();
}

int jetson_clocks_enable() {
  // set clocks to max frequency
  subprocess::popen cmd(JETSON_CLOCKS_PATH, {});

  std::string err;
  std::getline(cmd.stderr(), err);

  if (err.size() > 0) {
    daemon_log(LOG_ERR, "failed to run jetson-clockss");
    sprintf_stderr("%s: failed to run jetson-clocks", argv0);
    std::cerr << err << std::endl;
    exit(EXIT_FAILURE);
  }

  return cmd.wait();
}
