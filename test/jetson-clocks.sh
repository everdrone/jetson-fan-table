#!/bin/bash
# Copyright (c) 2015-2020, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

JETSON_CLOCKS=$(basename ${0})
FAN_SPEED_OVERRIDE=0
CONF_FILE=${HOME}/l4t_dfs.conf
RED='\e[0;31m'
GREEN='\e[0;32m'
BLUE='\e[0;34m'
BRED='\e[1;31m'
BGREEN='\e[1;32m'
BBLUE='\e[1;34m'
NC='\e[0m' # No Color

usage() {
	if [ "$1" != "" ]; then
		echo -e ${RED}"$1"${NC}
	fi

	cat >&2 <<EOF
Maximize jetson performance by setting static max frequency to CPU, GPU and EMC clocks.
Usage:
${JETSON_CLOCKS} [options]
  options,
  --help, -h         show this help message
  --show             display current settings
  --fan              set PWM fan speed to maximal
  --store [file]     store current settings to a file (default: \${HOME}/l4t_dfs.conf)
  --restore [file]   restore saved settings from a file (default: \${HOME}/l4t_dfs.conf)
  run ${JETSON_CLOCKS} without any option to set static max frequency to CPU, GPU and EMC clocks.
EOF

	exit 0
}

detect_gpu_type() {
	iGPU_DEV_NODES=(/dev/nvhost-gpu)
	dGPU_DEV_NODES=(/dev/nvidiactl /dev/nvgpu-pci/*)

	for dev_node in ${iGPU_DEV_NODES[@]}; do
		if [ -e $dev_node ]; then
			gpu_type="iGPU"
			return
		fi
	done

	for dev_node in ${dGPU_DEV_NODES[@]}; do
		if [ -e $dev_node ]; then
			gpu_type="dGPU"
			return
		fi
	done
}

check_nvidia_smi() {
	nvidia-smi &>/dev/null
	ret=$?
	if [ ${ret} -ne 0 ]; then
		case ${ret} in
		127)
			echo "Error: nvidia-smi not found."
			exit 1
			;;
		*)
			echo "Error: fail to do nvidia-smi operation." \
				"The exit code of nvidia-smi is ${ret}"
			exit 1
			;;
		esac
	fi
}

restore() {
	for conf in $(cat "${CONF_FILE}"); do
		file=$(echo $conf | cut -f1 -d :)
		data=$(echo $conf | cut -f2 -d :)
		case "${file}" in
		/sys/devices/system/cpu/cpu*/online | \
			/sys/kernel/debug/clk/override*/state)
			if [ $(cat $file) -ne $data ]; then
				echo "${data}" >"${file}"
			fi
			;;
		/sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq)
			echo "${data}" >"${file}" 2>/dev/null
			;;
		*)
			echo "${data}" >"${file}"
			ret=$?
			if [ ${ret} -ne 0 ]; then
				echo "Error: Failed to restore $file"
			fi
			;;
		esac
	done

	if [[ "${gpu_type}" == "dGPU" ]]; then
		nvidia-smi -rac -i 0 >/dev/null
		ret=$?
		if [ ${ret} -ne 0 ]; then
			echo "Error: Failed to restore dGPU application clocks frequency!"
		fi
	fi
}

store() {
	for file in $@; do
		if [ -e "${file}" ]; then
			echo "${file}:$(cat ${file})" >>"${CONF_FILE}"
		fi
	done
}

do_nvpmodel() {
	case "${ACTION}" in
	show)
		NVPMODEL_BIN="/usr/sbin/nvpmodel"
		NVPMODEL_CONF="/etc/nvpmodel.conf"
		if [ -e "${NVPMODEL_BIN}" ]; then
			if [ -e "${NVPMODEL_CONF}" ]; then
				POWER_MODE="$(nvpmodel -q | grep "NV Power Mode")"
				echo "${POWER_MODE}"
			fi
		fi
		;;
	esac
}

do_fan() {
	TARGET_PWM="/sys/devices/pwm-fan/target_pwm"
	TEMP_CONTROL="/sys/devices/pwm-fan/temp_control"
	FAN_SPEED=255

	if [ ! -w "${TARGET_PWM}" ]; then
		echo "Can't access Fan!"
		return
	fi

	case "${ACTION}" in
	show)
		echo "Fan: PWM=$(cat ${TARGET_PWM})"
		;;
	store)
		store "${TARGET_PWM}"
		store "${TEMP_CONTROL}"
		;;
	*)
		if [ "${FAN_SPEED_OVERRIDE}" -eq "0" ]; then
			return
		fi
		if [ -w "${TEMP_CONTROL}" ]; then
			echo "0" >"${TEMP_CONTROL}"
		fi
		echo "${FAN_SPEED}" >"${TARGET_PWM}"
		;;
	esac

	if [ "${SOCFAMILY}" = "tegra194" -a "${machine}" = "Clara-AGX" ]; then
		FPGA_TARGET_PWM="/sys/devices/pwm-fan-fpga/target_pwm"
		FPGA_TEMP_CONTROL="/sys/devices/pwm-fan-fpga/temp_control"

		if [ ! -w "${FPGA_TARGET_PWM}" ]; then
			echo "Can't access Fan (FPGA)!"
			return
		fi

		case "${ACTION}" in
		show)
			echo "Fan (FPGA): PWM=$(cat ${FPGA_TARGET_PWM})"
			;;
		store)
			store "${FPGA_TARGET_PWM}"
			store "${FPGA_TEMP_CONTROL}"
			;;
		*)
			if [ "${FAN_SPEED_OVERRIDE}" -eq "0" ]; then
				return
			fi
			if [ -w "${FPGA_TEMP_CONTROL}" ]; then
				echo "0" >"${FPGA_TEMP_CONTROL}"
			fi
			echo "${FAN_SPEED}" >"${FPGA_TARGET_PWM}"
			;;
		esac
	fi
}

do_hotplug() {
	case "${ACTION}" in
	show)
		echo "Online CPUs: $(cat /sys/devices/system/cpu/online)"
		;;
	store)
		for file in /sys/devices/system/cpu/cpu[0-9]/online; do
			store "${file}"
		done
		;;
	*) ;;

	esac
}

do_cpu() {
	FREQ_GOVERNOR="cpufreq/scaling_governor"
	CPU_MIN_FREQ="cpufreq/scaling_min_freq"
	CPU_MAX_FREQ="cpufreq/scaling_max_freq"
	CPU_CUR_FREQ="cpufreq/scaling_cur_freq"

	case "${ACTION}" in
	show)
		for folder in /sys/devices/system/cpu/cpu[0-9]; do
			CPU=$(basename ${folder})
			idle_states=""
			for idle in ${folder}/cpuidle/state[0-9]; do
				idle_states+="$(cat ${idle}/name)"
				idle_disable="$(cat ${idle}/disable)"
				idle_states+="=$((idle_disable == 0)) "
			done
			if [ -e "${folder}/${FREQ_GOVERNOR}" ]; then
				echo "$CPU: Online=$(cat ${folder}/online)" \
					"Governor=$(cat ${folder}/${FREQ_GOVERNOR})" \
					"MinFreq=$(cat ${folder}/${CPU_MIN_FREQ})" \
					"MaxFreq=$(cat ${folder}/${CPU_MAX_FREQ})" \
					"CurrentFreq=$(cat ${folder}/${CPU_CUR_FREQ})" \
					"IdleStates: $idle_states"
			fi
		done
		;;
	store)
		for file in \
			/sys/devices/system/cpu/cpu[0-9]/cpufreq/scaling_min_freq; do
			store "${file}"
		done

		for file in \
			/sys/devices/system/cpu/cpu[0-9]/cpuidle/state[0-9]/disable; do
			store "${file}"
		done
		;;
	*)
		for folder in /sys/devices/system/cpu/cpu[0-9]; do
			cat "${folder}/${CPU_MAX_FREQ}" >"${folder}/${CPU_MIN_FREQ}" 2>/dev/null
		done

		for file in \
			/sys/devices/system/cpu/cpu[0-9]/cpuidle/state[0-9]/disable; do
			echo 1 >"${file}"
		done
		;;
	esac
}

do_igpu() {
	name=""
	for devfreq in /sys/class/devfreq/*; do
		name=$(tr -d '\0' <${devfreq}/device/of_node/name)
		if [[ "${name}" == "gv11b" || "${name}" == "gp10b" || "${name}" == "gpu" ]]; then
			GPU_MIN_FREQ="${devfreq}/min_freq"
			GPU_MAX_FREQ="${devfreq}/max_freq"
			GPU_CUR_FREQ="${devfreq}/cur_freq"
			GPU_RAIL_GATE="${devfreq}/device/railgate_enable"
			break
		fi
	done

	if [[ "${name}" == "" ]]; then
		echo "Error! Unknown GPU!"
		exit 1
	fi

	case "${ACTION}" in
	show)
		echo "GPU MinFreq=$(cat ${GPU_MIN_FREQ})" \
			"MaxFreq=$(cat ${GPU_MAX_FREQ})" \
			"CurrentFreq=$(cat ${GPU_CUR_FREQ})"
		;;
	store)
		store "${GPU_MIN_FREQ}"
		store "${GPU_RAIL_GATE}"
		;;
	*)
		echo 0 >"${GPU_RAIL_GATE}"
		cat "${GPU_MAX_FREQ}" >"${GPU_MIN_FREQ}"
		ret=$?
		if [ ${ret} -ne 0 ]; then
			echo "Error: Failed to max GPU frequency!"
		fi
		;;
	esac
}

do_dgpu() {
	NV_SMI_QUERY_GPU="nvidia-smi --format=csv,noheader,nounits --query-gpu"
	dGPU_DEF_MEM=$(${NV_SMI_QUERY_GPU}=clocks.default_applications.memory)
	dGPU_MAX_MEM=$(${NV_SMI_QUERY_GPU}=clocks.max.memory)
	dGPU_CUR_MEM=$(${NV_SMI_QUERY_GPU}=clocks.applications.memory)
	dGPU_DEF_GRA=$(${NV_SMI_QUERY_GPU}=clocks.default_applications.graphics)
	dGPU_MAX_GRA=$(${NV_SMI_QUERY_GPU}=clocks.max.graphics)
	dGPU_CUR_GRA=$(${NV_SMI_QUERY_GPU}=clocks.applications.graphics)

	case "${ACTION}" in
	show)
		echo "dGPU DefaultMemFreq=${dGPU_DEF_MEM}MHz" \
			"MaxMemFreq=${dGPU_MAX_MEM}MHz" \
			"CurrentMemFreq=${dGPU_CUR_MEM}MHz"
		echo "dGPU DefaultGraFreq=${dGPU_DEF_GRA}MHz" \
			"MaxGraFreq=${dGPU_MAX_GRA}MHz" \
			"CurrentGraFreq=${dGPU_CUR_GRA}MHz"
		;;
	*)
		nvidia-smi -pm ENABLED -i 0 >/dev/null
		nvidia-smi -ac ${dGPU_MAX_MEM},${dGPU_MAX_GRA} -i 0 >/dev/null
		ret=$?
		if [ ${ret} -ne 0 ]; then
			echo "Error: Failed to max dGPU application clocks frequency!"
		fi
		;;
	esac
}

do_emc() {
	case "${SOCFAMILY}" in
	tegra186 | tegra194)
		EMC_ISO_CAP="/sys/kernel/nvpmodel_emc_cap/emc_iso_cap"
		EMC_MIN_FREQ="/sys/kernel/debug/bpmp/debug/clk/emc/min_rate"
		EMC_MAX_FREQ="/sys/kernel/debug/bpmp/debug/clk/emc/max_rate"
		EMC_CUR_FREQ="/sys/kernel/debug/bpmp/debug/clk/emc/rate"
		EMC_UPDATE_FREQ="/sys/kernel/debug/bpmp/debug/clk/emc/rate"
		EMC_FREQ_OVERRIDE="/sys/kernel/debug/bpmp/debug/clk/emc/mrq_rate_locked"
		;;
	tegra210)
		EMC_MIN_FREQ="/sys/kernel/debug/tegra_bwmgr/emc_min_rate"
		EMC_MAX_FREQ="/sys/kernel/debug/tegra_bwmgr/emc_max_rate"
		EMC_CUR_FREQ="/sys/kernel/debug/clk/override.emc/clk_rate"
		EMC_UPDATE_FREQ="/sys/kernel/debug/clk/override.emc/clk_update_rate"
		EMC_FREQ_OVERRIDE="/sys/kernel/debug/clk/override.emc/clk_state"
		;;
	*)
		echo "Error! unsupported SOC ${SOCFAMILY}"
		exit 1
		;;

	esac

	if [ "${SOCFAMILY}" = "tegra186" -o "${SOCFAMILY}" = "tegra194" ]; then
		emc_cap=$(cat "${EMC_ISO_CAP}")
		emc_fmax=$(cat "${EMC_MAX_FREQ}")
		if [ "$emc_cap" -gt 0 ] && [ "$emc_cap" -lt "$emc_fmax" ]; then
			EMC_MAX_FREQ="${EMC_ISO_CAP}"
		fi
	fi

	case "${ACTION}" in
	show)
		echo "EMC MinFreq=$(cat ${EMC_MIN_FREQ})" \
			"MaxFreq=$(cat ${EMC_MAX_FREQ})" \
			"CurrentFreq=$(cat ${EMC_CUR_FREQ})" \
			"FreqOverride=$(cat ${EMC_FREQ_OVERRIDE})"
		;;
	store)
		store "${EMC_FREQ_OVERRIDE}"
		;;
	*)
		echo 1 >"${EMC_FREQ_OVERRIDE}"
		cat "${EMC_MAX_FREQ}" >"${EMC_UPDATE_FREQ}"
		;;
	esac
}

main() {
	while [ -n "$1" ]; do
		case "$1" in
		--show)
			echo "SOC family:${SOCFAMILY}  Machine:${machine}"
			ACTION=show
			;;
		--store)
			[ -n "$2" ] && CONF_FILE=$2
			ACTION=store
			shift 1
			;;
		--restore)
			[ -n "$2" ] && CONF_FILE=$2
			ACTION=restore
			shift 1
			;;
		--fan)
			FAN_SPEED_OVERRIDE=1
			;;
		-h | --help)
			usage
			exit 0
			;;
		*)
			usage "Unknown option: $1"
			exit 1
			;;
		esac
		shift 1
	done

	[ $(whoami) != root ] &&
		echo Error: Run this script\($0\) as a root user && exit 1

	case $ACTION in
	store)
		if [ -e "${CONF_FILE}" ]; then
			echo "File $CONF_FILE already exists. Can I overwrite it? Y/N:"
			read answer
			case $answer in
			y | Y)
				rm -f $CONF_FILE
				;;
			*)
				echo "Error: file $CONF_FILE already exists!"
				exit 1
				;;
			esac
		fi
		;;
	restore)
		if [ ! -e "${CONF_FILE}" ]; then
			echo "Error: $CONF_FILE file not found !"
			exit 1
		fi
		restore
		exit 0
		;;
	esac

	do_hotplug
	do_cpu
	gpu_type=""
	detect_gpu_type
	if [[ "${gpu_type}" == "iGPU" ]]; then
		do_igpu
	elif [[ "${gpu_type}" == "dGPU" ]]; then
		check_nvidia_smi
		do_dgpu
	else
		echo "Error: Unknown GPU Type!"
		exit 1
	fi
	do_emc
	do_fan
	do_nvpmodel
}

if [ -e "/proc/device-tree/compatible" ]; then
	if [ -e "/proc/device-tree/model" ]; then
		machine="$(tr -d '\0' </proc/device-tree/model)"
	fi
	CHIP="$(tr -d '\0' </proc/device-tree/compatible)"
	if [[ "${CHIP}" =~ "tegra186" ]]; then
		SOCFAMILY="tegra186"
	elif [[ "${CHIP}" =~ "tegra210" ]]; then
		SOCFAMILY="tegra210"
	elif [[ "${CHIP}" =~ "tegra194" ]]; then
		SOCFAMILY="tegra194"
	fi
fi

main $@
exit 0
