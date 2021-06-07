#pragma once

#include <glob.h>

#include "defines.h"
#include "log.h"
#include "utils.h"

#define SYSLOG_MERCY

unsigned thermal_average(bool use_max, const char* ignore_substring) {
  glob_t globResult;
  glob(THERMAL_ZONE_GLOB, GLOB_TILDE, NULL, &globResult);

  unsigned temp_sum = 0;
  unsigned num_sensors = 0;
  unsigned temp_max = 0;

#ifndef SYSLOG_MERCY
  std::vector<std::string> used_sensors;
  std::vector<std::string> ignored_sensors;
#endif

  for (unsigned i = 0; i < globResult.gl_pathc; i++) {
    std::string thermal_zone_path(globResult.gl_pathv[i]);
    std::string sensor_name_path = thermal_zone_path + "/type";
    std::string sensor_temp_path = thermal_zone_path + "/temp";

    std::string name = read_file(sensor_name_path.c_str());
    name = trim(name);
    unsigned temp = read_file_int(sensor_temp_path.c_str());

    // name contains ignore_substring
    // this sensor is not accurate, skip
    if (name.find(ignore_substring) != std::string::npos) {
#ifndef SYSLOG_MERCY
      ignored_sensors.push_back(name);
#endif
      continue;
    } else {
#ifndef SYSLOG_MERCY
      used_sensors.push_back(name);
#endif
      num_sensors++;
      temp_sum += temp;
      temp_max = std::max(temp_max, temp);
    }
  }

#ifndef SYSLOG_MERCY
  debug_log("using sensors: %s", join(used_sensors, ", ").c_str());
  debug_log("ignored sensors: %s", join(ignored_sensors, ", ").c_str());
#endif

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
