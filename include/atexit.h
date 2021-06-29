#pragma once

#include <signal.h>

#include "defines.h"
#include "jetson_clocks.h"
#include "log.h"
#include "pid.h"
#include "utils.h"

/**
 * Exit handler. turn off the fan before leaving
 */
void exit_handler(int status = EXIT_SUCCESS) {
  if (enable_tach) {
// restore tach
#if WRITE_SYSTEM_FILES_DANGEROUS
    debug_log("restoring previous tachometer state");
    write_file_int(TACH_ENABLE_PATH, tach_state);
#endif
  }

  if (enable_max_freq) {
    if (clocks_did_set) {
      const char* restore_from_path = is_first_run ? INITIAL_STORE_FILE : STORE_FILE;

      debug_log("restoring config %s", restore_from_path);
      restore_config(restore_from_path);
    }
  }

  // set target pwm to 0
#if WRITE_SYSTEM_FILES_DANGEROUS
  debug_log("resetting target_pwm to 0");
  write_file_int(TARGET_PWM_PATH, 0);
#endif

  daemon_log(LOG_INFO, "removing pid file");
  if (pid_file_remove() < 0) {
    daemon_log(LOG_ERR, "cannot remove pid file");
    sprintf_stderr("%s: cannot remove pid file", argv0);
  }

  daemon_log(LOG_INFO, "Exiting with status code %d", errno);
  std::cout << std::endl;
  exit(errno);
}

/**
 * Initialize the exit handler
 */
void register_exit_handler() {
  debug_log("registering exit handler for SIGINT, SIGTERM");

  struct sigaction sigint_handler;

  sigint_handler.sa_handler = exit_handler;
  sigemptyset(&sigint_handler.sa_mask);
  sigint_handler.sa_flags = 0;

  sigaction(SIGINT, &sigint_handler, NULL);
  sigaction(SIGTERM, &sigint_handler, NULL);
}
