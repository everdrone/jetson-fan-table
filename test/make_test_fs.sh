#!/bin/bash

root_prefix="/"
test_prefix="/home/everdrone/Desktop/projects/test_fs/"
cust_prefix="/home/everdrone/Desktop/projects/fantablectl/"

if [[ -d "/home/everdrone/Desktop" ]]; then
	dirs=(
		"proc/device-tree"

		"sys/devices/virtual/thermal/thermal_zone0"
		"sys/devices/virtual/thermal/thermal_zone1"
		"sys/devices/virtual/thermal/thermal_zone2"
		"sys/devices/virtual/thermal/thermal_zone3"
		"sys/devices/virtual/thermal/thermal_zone4"
		"sys/devices/virtual/thermal/thermal_zone5"

		"sys/devices/system/cpu/cpu0/cpufreq"
		"sys/devices/system/cpu/cpu1/cpufreq"
		"sys/devices/system/cpu/cpu2/cpufreq"
		"sys/devices/system/cpu/cpu3/cpufreq"

		"sys/devices/system/cpu/cpu0/cpuidle/state0"
		"sys/devices/system/cpu/cpu1/cpuidle/state0"
		"sys/devices/system/cpu/cpu2/cpuidle/state0"
		"sys/devices/system/cpu/cpu3/cpuidle/state0"

		"sys/devices/system/cpu/cpu0/cpuidle/state1"
		"sys/devices/system/cpu/cpu1/cpuidle/state1"
		"sys/devices/system/cpu/cpu2/cpuidle/state1"
		"sys/devices/system/cpu/cpu3/cpuidle/state1"

		"sys/class/devfreq/57000000.gpu"
		"sys/class/devfreq/57000000.gpu/device/of_node"

		"sys/kernel/debug/tegra_bwmgr"

		"sys/kernel/debug/clk/override.emc"
		"sys/devices/pwm-fan"

		"etc/fantable"
	)

	echo "creating directories..."
	for suffix in ${dirs[*]}; do
		mkdir -p ${test_prefix}${suffix}
	done

	files=(
		"proc/device-tree/compatible"
		"proc/device-tree/model"

		"sys/devices/virtual/thermal/thermal_zone0/type"
		"sys/devices/virtual/thermal/thermal_zone1/type"
		"sys/devices/virtual/thermal/thermal_zone2/type"
		"sys/devices/virtual/thermal/thermal_zone3/type"
		"sys/devices/virtual/thermal/thermal_zone4/type"
		"sys/devices/virtual/thermal/thermal_zone5/type"

		"sys/devices/virtual/thermal/thermal_zone0/temp"
		"sys/devices/virtual/thermal/thermal_zone1/temp"
		"sys/devices/virtual/thermal/thermal_zone2/temp"
		"sys/devices/virtual/thermal/thermal_zone3/temp"
		"sys/devices/virtual/thermal/thermal_zone4/temp"
		"sys/devices/virtual/thermal/thermal_zone5/temp"

		"sys/devices/system/cpu/cpu0/cpuidle/state0/disable"
		"sys/devices/system/cpu/cpu1/cpuidle/state0/disable"
		"sys/devices/system/cpu/cpu2/cpuidle/state0/disable"
		"sys/devices/system/cpu/cpu3/cpuidle/state0/disable"

		"sys/devices/system/cpu/cpu0/cpuidle/state1/disable"
		"sys/devices/system/cpu/cpu1/cpuidle/state1/disable"
		"sys/devices/system/cpu/cpu2/cpuidle/state1/disable"
		"sys/devices/system/cpu/cpu3/cpuidle/state1/disable"

		"sys/devices/system/cpu/cpu0/online"
		"sys/devices/system/cpu/cpu1/online"
		"sys/devices/system/cpu/cpu2/online"
		"sys/devices/system/cpu/cpu3/online"

		"sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
		"sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq"
		"sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
		"sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"
		"sys/devices/system/cpu/cpu1/cpufreq/scaling_governor"
		"sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq"
		"sys/devices/system/cpu/cpu1/cpufreq/scaling_max_freq"
		"sys/devices/system/cpu/cpu1/cpufreq/scaling_cur_freq"
		"sys/devices/system/cpu/cpu2/cpufreq/scaling_governor"
		"sys/devices/system/cpu/cpu2/cpufreq/scaling_min_freq"
		"sys/devices/system/cpu/cpu2/cpufreq/scaling_max_freq"
		"sys/devices/system/cpu/cpu2/cpufreq/scaling_cur_freq"
		"sys/devices/system/cpu/cpu3/cpufreq/scaling_governor"
		"sys/devices/system/cpu/cpu3/cpufreq/scaling_min_freq"
		"sys/devices/system/cpu/cpu3/cpufreq/scaling_max_freq"
		"sys/devices/system/cpu/cpu3/cpufreq/scaling_cur_freq"

		"sys/class/devfreq/57000000.gpu/device/of_node/name"
		"sys/class/devfreq/57000000.gpu/min_freq"
		"sys/class/devfreq/57000000.gpu/max_freq"
		"sys/class/devfreq/57000000.gpu/cur_freq"
		"sys/class/devfreq/57000000.gpu/device/railgate_enable"

		"sys/kernel/debug/tegra_bwmgr/emc_min_rate"
		"sys/kernel/debug/tegra_bwmgr/emc_max_rate"

		"sys/kernel/debug/clk/override.emc/clk_rate"
		"sys/kernel/debug/clk/override.emc/clk_update_rate"
		"sys/kernel/debug/clk/override.emc/clk_state"

		"sys/devices/pwm-fan/pwm_cap"
		"sys/devices/pwm-fan/target_pwm"
		"sys/devices/pwm-fan/temp_control"
	)

	echo "copying files..."
	for suffix in ${files[*]}; do
		sudo cp ${root_prefix}${suffix} ${test_prefix}${suffix}
	done

	echo "copying non-system files..."

	cp ${cust_prefix}data/table ${test_prefix}etc/fantable/table

fi
