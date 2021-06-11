#pragma once

#include <string>

using std::string;

void store_config(const char* path) {
  // save config
  string full_command = "jetson_clocks --store ";
  full_command += path;
  system(full_command.c_str());
  return;
}

void restore_config(const char* path) {
  // reload from file
  string full_command = "jetson_clocks --restore ";
  full_command += path;
  system(full_command.c_str());
  return;
}

void clocks_max_freq() {
  // set all to max freq
  system("jetson_clocks");
  return;
}
