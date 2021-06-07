#pragma once

#include <string>

#define TEGRA_186 "tegra186"
#define TEGRA_210 "tegra210"
#define TEGRA_194 "tegra194"

#define TEST_FS "/home/everdrone/Desktop/projects/test_fs"

#define JETSON_CLOCKS_PATH "/usr/bin/jetson_clocks"

// general
#define SOC_FAMILY_PATH TEST_FS "/proc/device-tree/compatible"
#define MACHINE_NAME_PATH TEST_FS "/proc/device-tree/model"

// thermal
#define THERMAL_ZONE_GLOB TEST_FS "/sys/devices/virtual/thermal/thermal_zone*"

// fan
#define PWM_CAP_PATH TEST_FS "/sys/devices/pwm-fan/pwm_cap"
#define TARGET_PWM_PATH TEST_FS "/sys/devices/pwm-fan/target_pwm"
#define TEMP_CONTROL_PATH TEST_FS "/sys/devices/pwm-fan/temp_control"

// cpu
#define CPU_GLOB TEST_FS "/sys/devices/system/cpu/cpu[0-9]*"
#define CPU_IDLE_STATE_GLOB TEST_FS "/sys/devices/system/cpu/cpu[0-9]/cpuidle/state[0-9]/disable"

// gpu
#define GPU_GLOB TEST_FS "/sys/class/devfreq/*"

// emc
#define TEGRA_210_EMC_MIN_FREQ_PATH TEST_FS "/sys/kernel/debug/tegra_bwmgr/emc_min_rate"
#define TEGRA_210_EMC_MAX_FREQ_PATH TEST_FS "/sys/kernel/debug/tegra_bwmgr/emc_max_rate"
#define TEGRA_210_EMC_CUR_FREQ_PATH TEST_FS "/sys/kernel/debug/clk/override.emc/clk_rate"
#define TEGRA_210_EMC_UPDATE_FREQ_PATH TEST_FS "/sys/kernel/debug/clk/override.emc/clk_update_rate"
#define TEGRA_210_EMC_FREQ_OVERRIDE_PATH TEST_FS "/sys/kernel/debug/clk/override.emc/clk_state"

// configurations
#define TABLE_PATH TEST_FS "/etc/fantable/table"
#define STORE_FILE TEST_FS "/etc/fantable/state.conf"
#define INITIAL_STORE_FILE TEST_FS "/etc/fantable/initial_state.conf"

// set to true to write into filesystem sensitive files
#define WRITE_SYSTEM_FILES_DANGEROUS true

static bool enable_debug = false;
static bool enable_max_freq = true;
static char* argv0 = "fantable";
static bool is_first_run = false;