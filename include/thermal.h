#pragma once

#include <glob.h>

#include "defines.h"
#include "log.h"
#include "utils.h"

#define SYSLOG_MERCY

unsigned thermal_average(bool use_max = false) {
  glob_t globResult;
  glob(THERMAL_ZONE_GLOB, GLOB_TILDE, NULL, &globResult);

  unsigned temp_sum = 0;
  unsigned num_sensors = 0;
  unsigned temp_max = 0;

  for (unsigned i = 0; i < globResult.gl_pathc; i++) {
    std::string thermal_zone_path(globResult.gl_pathv[i]);
    std::string sensor_name_path = thermal_zone_path + "/type";
    std::string sensor_temp_path = thermal_zone_path + "/temp";

    std::string name = read_file(sensor_name_path.c_str());
#ifndef SYSLOG_MERCY
    debug_log("reading sensor `%s'", thermal_zone_path.c_str());
#endif
    unsigned temp = read_file_int(sensor_temp_path.c_str());

    // name contains "PMIC"
    // this sensor is not accurate, skip
    if (name.find("PMIC") != std::string::npos) {
#ifndef SYSLOG_MERCY
      debug_log("ignoring sensor `%s'", trim(name).c_str());
#endif
      continue;
    } else {
      num_sensors++;
      temp_sum += temp;
      temp_max = std::max(temp_max, temp);
    }
  }

  if (use_max) {
    return temp_max;
  } else {
    if (num_sensors > 0) {
      return temp_sum / num_sensors;
    }
  }

  if (num_sensors < 1) {
    daemon_log(LOG_ERR, "no temperature sensors found");
    sprintf_stderr("%s: no temperature sensors found", argv0);
    exit(EXIT_FAILURE);
  }

  return 0;
}
