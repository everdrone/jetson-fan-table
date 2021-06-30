#pragma once

#include "defines.h"
#include "log.h"
#include "thermal.h"

void check_pid() {
  pid_t pid;
  if ((pid = pid_file_is_running()) >= 0) {
    exit(EXIT_SUCCESS);
  } else {
    exit(ESRCH);
  }
}

void print_status(const char* ingore_substr, bool use_highest) {
  pid_t pid;
  int retval = ESRCH;

  if ((pid = pid_file_is_running()) >= 0) {
    printf("process pid: %d\n", pid);

    vector<string> sensor_paths = scan_sensors(ingore_substr);
    unsigned temperature = 0;
    unsigned fan_pwm = 0;
    unsigned cur_rpm = 0;

    temperature = thermal_average(sensor_paths, use_highest);
    fan_pwm = read_file_int(CUR_PWM_PATH);

    printf("temperature: %d C\n", temperature / 1000);
    printf("current pwm: %d\n", fan_pwm);

    if (read_file_int(TACH_ENABLE_PATH) == 1) {
      cur_rpm = read_file_int(MEASURED_RPM_PATH);
      printf("current rpm: %d\n", cur_rpm);
    } else {
      printf("tachometer is disabled\n");
    }

    retval = EXIT_SUCCESS;
  } else {
    sprintf_stderr("%s is not running", argv0);
  }

  exit(retval);
}
