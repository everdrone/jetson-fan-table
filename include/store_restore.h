#pragma once

/*
 * If there's a file called 'initial.conf' then
 * this is not the first time the program is running
 *
 * If that file does not exist, then save it.
 *
 * If 'initial.conf' exists, save the configuration to 'latest.conf'
 * and then make all the changes you want
 *
 * When the program exits restore the 'latest.conf' settings
 */

#include <glob.h>
#include <unistd.h>

#include "defines.h"
#include "log.h"
#include "utils.h"

using std::string;
using std::vector;

// #define CONDITIONAL_RESTORE

/*
 * Checks if file exists, then checks the permissions
 * returns true if exists AND the permissions are granted
 * returns false if no exist OR the access is negated
 */
bool check_permissions(const char* path, int permissions) {
  string message_template = "%s: no such file `%s'";
  if (access(path, F_OK) == -1) {
    // doesnt exist
    daemon_log(LOG_ERR, "no such file `%s'", path);
    sprintf_stderr(message_template.c_str(), argv0, path);
    return false;
  } else {
    if (permissions == R_OK) {
      message_template = "%s: no read access to file `%s'";
      daemon_log(LOG_ERR, "no read access to file `%s'", path);
    } else if (permissions == W_OK) {
      daemon_log(LOG_ERR, "no write access to file `%s'", path);
      message_template = "%s: no write access to file `%s'";
    } else if (permissions == X_OK) {
      daemon_log(LOG_ERR, "no execute access to file `%s'", path);
      message_template = "%s: no execute access to file `%s'";
    } else if (permissions == R_OK | W_OK) {
      daemon_log(LOG_ERR, "no read/write access to file `%s'", path);
      message_template = "%s: no read/write access to file `%s'";
    } else if (permissions == F_OK) {
      // already done, just exit
      return true;
    }

    int result = access(path, permissions);
    if (result == -1) {
      // no access
      sprintf_stderr(message_template.c_str(), argv0, path);
      return false;
    } else {
      // std::cout << "ok " << path << std::endl;
      return true;
    }
  }

  return true;
}

string get_soc_family() {
  string soc_family = read_file(SOC_FAMILY_PATH);

  // save clean sock family
  if (soc_family.find(TEGRA_186) != string::npos) {
    soc_family = TEGRA_186;
  } else if (soc_family.find(TEGRA_210) != string::npos) {
    soc_family = TEGRA_210;
  } else if (soc_family.find(TEGRA_194) != string::npos) {
    soc_family = TEGRA_194;
  } else {
    // not supported
    daemon_log(LOG_ERR, "unsupported device");
    sprintf_stderr(
        "%s: this device is not supported\n"
        "%s",
        argv0, soc_family.c_str());
    exit(EXIT_FAILURE);
  }

  return soc_family;
}

bool check_all_access() {
  // FIXME: move this into main()
  if (!is_sudo_or_root()) {
    daemon_log(LOG_ERR, "this program must be run as root");
    sprintf_stderr("%s: this program must be run as root", argv0);
    exit(EXIT_FAILURE);
  }

  bool state = true;
  glob_t glob_result;

  state &= check_permissions(SOC_FAMILY_PATH, R_OK);
  state &= check_permissions(MACHINE_NAME_PATH, R_OK);

  string soc_family = get_soc_family();
  string machine_name = read_file(MACHINE_NAME_PATH);

  // thermal zones
  glob(THERMAL_ZONE_GLOB, GLOB_TILDE, NULL, &glob_result);
  for (unsigned i = 0; i < glob_result.gl_pathc; i++) {
    string thermal_zone_path(glob_result.gl_pathv[i]);
    string sensor_name_path = thermal_zone_path + "/type";
    string sensor_temp_path = thermal_zone_path + "/temp";

    state &= check_permissions(sensor_name_path.c_str(), R_OK);
    state &= check_permissions(sensor_temp_path.c_str(), R_OK);
  }

  // cpus
  glob(CPU_GLOB, GLOB_TILDE, NULL, &glob_result);
  for (unsigned i = 0; i < glob_result.gl_pathc; i++) {
    string cpu_path(glob_result.gl_pathv[i]);
    string cpu_online_path = cpu_path + "/online";
    string cpu_scaling_governor_path = cpu_path + "/cpufreq/scaling_governor";
    string cpu_min_freq_path = cpu_path + "/cpufreq/scaling_min_freq";
    string cpu_max_freq_path = cpu_path + "/cpufreq/scaling_max_freq";
    string cpu_cur_freq_path = cpu_path + "/cpufreq/scaling_cur_freq";

    state &= check_permissions(cpu_online_path.c_str(), R_OK | W_OK);
    state |= check_permissions(cpu_scaling_governor_path.c_str(), R_OK);
    state &= check_permissions(cpu_min_freq_path.c_str(), R_OK | W_OK);
    state &= check_permissions(cpu_max_freq_path.c_str(), R_OK);
    state |= check_permissions(cpu_cur_freq_path.c_str(), R_OK);
  }

  glob(CPU_IDLE_STATE_GLOB, GLOB_TILDE, NULL, &glob_result);
  for (unsigned i = 0; i < glob_result.gl_pathc; i++) {
    string cpu_idle_state_path(glob_result.gl_pathv[i]);

    state |= check_permissions(cpu_idle_state_path.c_str(), R_OK | W_OK);
  }

  glob(GPU_GLOB, GLOB_TILDE, NULL, &glob_result);
  for (unsigned i = 0; i < glob_result.gl_pathc; i++) {
    string gpu_path(glob_result.gl_pathv[i]);
    state &= check_permissions(gpu_path.c_str(), F_OK);

    string gpu_name_path = gpu_path + "/device/of_node/name";  // name ends with \0
    state &= check_permissions(gpu_name_path.c_str(), R_OK);

    // NOTE: only for gp10b, gv11b, gpu
    string gpu_name = read_file(gpu_name_path.c_str());
    // equivalent of tr -d '\0'
    gpu_name.erase(std::find(gpu_name.begin(), gpu_name.end(), '\0'), gpu_name.end());

    if (gpu_name == "gp10b" || gpu_name == "gv11b" || gpu_name == "gpu") {
      string gpu_min_freq_path = gpu_path + "/min_freq";
      string gpu_max_freq_path = gpu_path + "/max_freq";
      string gpu_cur_freq_path = gpu_path + "/cur_freq";
      string gpu_rail_gate_path = gpu_path + "/device/railgate_enable";

      state &= check_permissions(gpu_min_freq_path.c_str(), R_OK | W_OK);
      state |= check_permissions(gpu_max_freq_path.c_str(), R_OK);
      state |= check_permissions(gpu_cur_freq_path.c_str(), R_OK);
      state &= check_permissions(gpu_rail_gate_path.c_str(), R_OK | W_OK);
    }
  }

  // emc
  // TODO: add contitional SOC_FAMILY for tegra186 | tegra194
  if (soc_family == TEGRA_210) {
    state |= check_permissions(TEGRA_210_EMC_MIN_FREQ_PATH, R_OK);
    state &= check_permissions(TEGRA_210_EMC_MAX_FREQ_PATH, R_OK);
    state |= check_permissions(TEGRA_210_EMC_CUR_FREQ_PATH, R_OK);
    state &= check_permissions(TEGRA_210_EMC_UPDATE_FREQ_PATH, R_OK | W_OK);
    state &= check_permissions(TEGRA_210_EMC_FREQ_OVERRIDE_PATH, R_OK | W_OK);
  }

  // TODO: add contitional SOC_FAMILY
  // fan
  state &= check_permissions(PWM_CAP_PATH, R_OK | W_OK);
  state &= check_permissions(TARGET_PWM_PATH, R_OK | W_OK);
  state &= check_permissions(TEMP_CONTROL_PATH, R_OK | W_OK);
  // fan config
  state &= check_permissions(TABLE_PATH, R_OK | W_OK);

  // cleanup glob
  globfree(&glob_result);

  return state;
}

string store_line(const char* path, const char* value) {
  string result = path;
  result += ":";
  result += value;

  return result;
}

string store_line(const char* path) {
  string value = read_file(path);

  return store_line(path, value.c_str());
}

void store_config(const char* path) {
  vector<string> lines;
  glob_t glob_result;

  string soc_family = get_soc_family();

  // store cpu online and scaling_min_freq
  glob(CPU_GLOB, GLOB_TILDE, NULL, &glob_result);
  for (unsigned i = 0; i < glob_result.gl_pathc; i++) {
    string cpu_path(glob_result.gl_pathv[i]);
    string cpu_online_path = cpu_path + "/online";
    string cpu_min_freq_path = cpu_path + "/cpufreq/scaling_min_freq";

    lines.push_back(store_line(cpu_online_path.c_str()));
    lines.push_back(store_line(cpu_min_freq_path.c_str()));
  }

  // store cpu idle state disable
  glob(CPU_IDLE_STATE_GLOB, GLOB_TILDE, NULL, &glob_result);
  for (unsigned i = 0; i < glob_result.gl_pathc; i++) {
    string cpu_idle_state_path(glob_result.gl_pathv[i]);

    lines.push_back(store_line(cpu_idle_state_path.c_str()));
  }

  // store gpu min_freq and railgate_enable
  glob(GPU_GLOB, GLOB_TILDE, NULL, &glob_result);
  for (unsigned i = 0; i < glob_result.gl_pathc; i++) {
    string gpu_path(glob_result.gl_pathv[i]);
    string gpu_name_path = gpu_path + "/device/of_node/name";  // name ends with \0
    string gpu_name = read_file(gpu_name_path.c_str());
    // equivalent of tr -d '\0'
    gpu_name.erase(std::find(gpu_name.begin(), gpu_name.end(), '\0'), gpu_name.end());

    if (gpu_name == "gp10b" || gpu_name == "gv11b" || gpu_name == "gpu") {
      string gpu_min_freq_path = gpu_path + "/min_freq";
      string gpu_rail_gate_path = gpu_path + "/device/railgate_enable";

      lines.push_back(store_line(gpu_min_freq_path.c_str()));
      lines.push_back(store_line(gpu_rail_gate_path.c_str()));
    }
  }

  // store emc freq override
  // TODO: add contitional SOC_FAMILY for tegra186 | tegra194
  if (soc_family == TEGRA_210) {
    lines.push_back(store_line(TEGRA_210_EMC_FREQ_OVERRIDE_PATH));
  }

  // store target_pwm and temp_control
  lines.push_back(store_line(TARGET_PWM_PATH));
  lines.push_back(store_line(TEMP_CONTROL_PATH));
  // TODO: add cases for soc_family == "tegra194" && machine = "Clara-AGX"

  string file_contents = join(lines, "");
#if WRITE_SYSTEM_FILES_DANGEROUS
  write_file_no_eol(path, file_contents.c_str());
#endif  // WRITE_SYSTEM_FILES_DANGEROUS

  // cleanup glob
  globfree(&glob_result);

  return;
}

// TODO: we might need to set the railgate first
void restore_config(const char* path) {
  vector<string> lines = read_lines(path);

#ifdef CONDITIONAL_RESTORE
  vector<string> conditional_restore_files;
  vector<string> might_fail_files;

  const char* conditional_restore_online_glob = "/sys/devices/system/cpu/cpu*/online";
  const char* conditional_restore_override_glob = "/sys/kernel/debug/clk/override*/state";
  const char* might_fail_freq_glob = "/sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq";

  // special cases
  glob_t glob_result;
  glob(conditional_restore_online_glob, GLOB_TILDE, NULL, &glob_result);
  for (unsigned i = 0; i < glob_result.gl_pathc; i++) {
    conditional_restore_files.push_back(glob_result.gl_pathv[i]);
  }

  glob(conditional_restore_override_glob, GLOB_TILDE, NULL, &glob_result);
  for (unsigned i = 0; i < glob_result.gl_pathc; i++) {
    conditional_restore_files.push_back(glob_result.gl_pathv[i]);
  }

  glob(might_fail_freq_glob, GLOB_TILDE, NULL, &glob_result);
  for (unsigned i = 0; i < glob_result.gl_pathc; i++) {
    might_fail_files.push_back(glob_result.gl_pathv[i]);
  }

  globfree(&glob_result);
#endif  // CONDITIONAL_RESTORE

  for (size_t i = 0; i < lines.size() - 1; i++) {
    vector<string> components = split_string(lines[i], ":");

    if (components.size() == 2) {
      string file = components[0];
      string value = components[1];

      // std::cout << file << ":" << value << std::endl;

#ifdef CONDITIONAL_RESTORE
      // handle special cases (if file in vector)
      if (std::find(conditional_restore_files.begin(), conditional_restore_files.end(), file) !=
          conditional_restore_files.end()) {
        // Element in vector.
        if () } else {
#endif  // CONDITIONAL_RESTORE
#if WRITE_SYSTEM_FILES_DANGEROUS
        write_file(file.c_str(), value.c_str());
#endif  // WRITE_SYSTEM_FILES_DANGEROUS

#ifdef CONDITIONAL_RESTORE
      }
#endif  // CONDITIONAL_RESTORE

    } else {
      /*
       * NOTE: can also be an empty line
       * but if the store_config() works there should be none
       */
      daemon_log(LOG_ERR, "cannot parse state file `%s' at line %d: `%s'", path, i,
                 lines[i].c_str());
      sprintf_stderr(
          "%s: cannot parse state file `%s'\n"
          "at line %d: `%s'",
          argv0, path, i, lines[i].c_str());
      exit(EXIT_FAILURE);
    }
  }

  return;
}
