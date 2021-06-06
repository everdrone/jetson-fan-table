#pragma once

#include <string>

#define THERMAL_ZONE_GLOB "/sys/devices/virtual/thermal/thermal_zone*"
#define JETSON_CLOCKS_PATH "/usr/bin/jetson_clocks"
#define PWM_CAP_PATH "/sys/devices/pwm-fan/pwm_cap"
#define TARGET_PWM_PATH "/sys/devices/pwm-fan/target_pwm"
#define TABLE_PATH "/etc/fantable/table"

static bool enable_debug = false;
static bool enable_max_freq = true;
static char* argv0 = "fantable";
