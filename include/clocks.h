#pragma once

#include "defines.h"
#include "log.h"
#include "store_restore.h"
#include "utils.h"

using std::string;

// TODO: try using jetson_clocks and rm state.conf
void clocks_max_freq() {
  glob_t glob_result;

  string soc_family = get_soc_family();
  string machine_name = read_file(MACHINE_NAME_PATH);

  // don't write target_pwm, the daemon will handle it
#if WRITE_SYSTEM_FILES_DANGEROUS
  // set temp_control to 0
  write_file(TEMP_CONTROL_PATH, "0");  // add newline
#endif

  // set scaling_min_freq = scaling_max_freq
  glob(CPU_GLOB, GLOB_TILDE, NULL, &glob_result);
  for (unsigned i = 0; i < glob_result.gl_pathc; i++) {
    string cpu_path(glob_result.gl_pathv[i]);
    string cpu_max_freq_path = cpu_path + "/cpufreq/scaling_max_freq";
    string cpu_min_freq_path = cpu_path + "/cpufreq/scaling_min_freq";

    string cpu_max_freq = read_file(cpu_max_freq_path.c_str());

#if WRITE_SYSTEM_FILES_DANGEROUS
    write_file_no_eol(cpu_min_freq_path.c_str(), cpu_max_freq.c_str());
#endif
  }

  // set cpuidle/state[0-9]/disable = 1
  glob(CPU_IDLE_STATE_GLOB, GLOB_TILDE, NULL, &glob_result);
  for (unsigned i = 0; i < glob_result.gl_pathc; i++) {
    string cpu_idle_state_path(glob_result.gl_pathv[i]);

#if WRITE_SYSTEM_FILES_DANGEROUS
    write_file(cpu_idle_state_path.c_str(), "1");
#endif
  }

  glob(GPU_GLOB, GLOB_TILDE, NULL, &glob_result);
  for (unsigned i = 0; i < glob_result.gl_pathc; i++) {
    string gpu_path(glob_result.gl_pathv[i]);
    string gpu_name_path = gpu_path + "/device/of_node/name";  // name ends with \0

    // NOTE: only for gp10b, gv11b, gpu
    string gpu_name = read_file(gpu_name_path.c_str());
    // equivalent of tr -d '\0'
    gpu_name.erase(std::find(gpu_name.begin(), gpu_name.end(), '\0'), gpu_name.end());

    if (gpu_name == "gp10b" || gpu_name == "gv11b" || gpu_name == "gpu") {
      string gpu_min_freq_path = gpu_path + "/min_freq";
      string gpu_max_freq_path = gpu_path + "/max_freq";
      string gpu_rail_gate_path = gpu_path + "/device/railgate_enable";

      string gpu_max_freq = read_file(gpu_max_freq_path.c_str());

#if WRITE_SYSTEM_FILES_DANGEROUS
      write_file(gpu_rail_gate_path.c_str(), "0");
      write_file_no_eol(gpu_min_freq_path.c_str(), gpu_max_freq.c_str());
#endif
    }
  }
  // cleanup glob
  globfree(&glob_result);

  if (soc_family == TEGRA_210) {
    string emc_max_freq = read_file(TEGRA_210_EMC_MAX_FREQ_PATH);

#if WRITE_SYSTEM_FILES_DANGEROUS
    write_file(TEGRA_210_EMC_FREQ_OVERRIDE_PATH, "1");
    write_file_no_eol(TEGRA_210_EMC_UPDATE_FREQ_PATH, emc_max_freq.c_str());
#endif
  }

  return;
}
