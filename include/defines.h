#pragma once

#include <string>

#include "config.h"

#define TEGRA_186 "tegra186"
#define TEGRA_210 "tegra210"
#define TEGRA_194 "tegra194"

#define JETSON_CLOCKS_PATH "/usr/bin/jetson_clocks"
// general
#define SOC_FAMILY_PATH "/proc/device-tree/compatible"
#define MACHINE_NAME_PATH "/proc/device-tree/model"
// thermal
#define THERMAL_ZONE_GLOB "/sys/devices/virtual/thermal/thermal_zone*"
// fan
#define PWM_CAP_PATH "/sys/devices/pwm-fan/pwm_cap"
#define TARGET_PWM_PATH "/sys/devices/pwm-fan/target_pwm"
#define CUR_PWM_PATH "/sys/devices/pwm-fan/cur_pwm"
#define TEMP_CONTROL_PATH "/sys/devices/pwm-fan/temp_control"
#define TACH_ENABLE_PATH "/sys/devices/pwm-fan/tach_enable"
#define MEASURED_RPM_PATH "/sys/devices/pwm-fan/rpm_measured"
// cpu
#define CPU_GLOB "/sys/devices/system/cpu/cpu[0-9]*"
#define CPU_IDLE_STATE_GLOB "/sys/devices/system/cpu/cpu[0-9]/cpuidle/state[0-9]/disable"
// gpu
#define GPU_GLOB "/sys/class/devfreq/*"
// emc
#define TEGRA_210_EMC_MIN_FREQ_PATH "/sys/kernel/debug/tegra_bwmgr/emc_min_rate"
#define TEGRA_210_EMC_MAX_FREQ_PATH "/sys/kernel/debug/tegra_bwmgr/emc_max_rate"
#define TEGRA_210_EMC_CUR_FREQ_PATH "/sys/kernel/debug/clk/override.emc/clk_rate"
#define TEGRA_210_EMC_UPDATE_FREQ_PATH "/sys/kernel/debug/clk/override.emc/clk_update_rate"
#define TEGRA_210_EMC_FREQ_OVERRIDE_PATH "/sys/kernel/debug/clk/override.emc/clk_state"

// configurations
#define TABLE_PATH "/etc/fantable/table"
#define STORE_FILE "/etc/fantable/state.conf"
#define INITIAL_STORE_FILE "/etc/fantable/initial_state.conf"
#define CONFIG_FILE_PATH "/etc/fantable/config"

#define MAX_FREQ_WAIT 30

static const char* argv0 = PACKAGE_NAME;

static bool enable_debug = false;
static bool enable_max_freq = true;
static bool enable_tach = false;

static bool clocks_did_set = false;
static bool is_first_run = false;
static int tach_state = 0;
