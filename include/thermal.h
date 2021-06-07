#pragma once

#include <glob.h>

#include "defines.h"
#include "log.h"
#include "utils.h"

std::vector<std::string> scan_sensors(const char* ignore_substring) {
  glob_t glob_result;

  std::vector<std::string> using_sensors;
  std::vector<std::string> ignored_sensors;

  glob(THERMAL_ZONE_GLOB, GLOB_TILDE, NULL, &glob_result);
  for (unsigned i = 0; i < glob_result.gl_pathc; i++) {
    std::string thermal_zone_path(glob_result.gl_pathv[i]);
    std::string sensor_name_path = thermal_zone_path + "/type";
    std::string sensor_temp_path = thermal_zone_path + "/temp";

    std::string name = read_file(sensor_name_path.c_str());
    name = trim(name);
    unsigned temp = read_file_int(sensor_temp_path.c_str());

    // name contains ignore_substring
    // this sensor is not accurate, skip
    if (name.find(ignore_substring) != std::string::npos) {
      ignored_sensors.push_back(sensor_temp_path);
      continue;
    } else {
      using_sensors.push_back(sensor_temp_path);
    }
  }

  // cleanup
  globfree(&glob_result);

  debug_log("using sensors: %s", join(using_sensors, ", ").c_str());
  debug_log("ignored sensors: %s", join(ignored_sensors, ", ").c_str());

  if (using_sensors.size() < 1) {
    daemon_log(LOG_ERR, "no temperature sensors found");
    sprintf_stderr("%s: no temperature sensors found", argv0);
    exit(EXIT_FAILURE);
  }

  return using_sensors;
}

unsigned thermal_average(const std::vector<std::string>& sensors, bool use_max) {
  unsigned temp_sum = 0;
  unsigned temp_max = 0;

  for (const auto& temp_path : sensors) {
    unsigned temp = read_file_int(temp_path.c_str());

    temp_sum += temp;
    temp_max = std::max(temp_max, temp);
  }

  if (use_max) {
    return temp_max;
  } else {
    if (sensors.size() > 0) {
      return temp_sum / sensors.size();
    }
  }

  return 0;
}
