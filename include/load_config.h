#pragma once

#include <vendor/inih/cpp/INIReader.h>

#include <string>

#include "defines.h"
#include "log.h"

using std::string;

enum options_enum {
  OPTION_DEBUG = 256,
};

typedef struct options_struct {
  bool help = false;
  bool version = false;
  bool check = false;
  bool status = false;
  bool use_highest = false;
  string substring = "PMIC";
  unsigned interval = 2;
} options_t;

void load_config(options_t* oobj) {
  INIReader reader(CONFIG_FILE_PATH);

  if (reader.ParseError() < 0) {
    debug_log("cannot load config %s", CONFIG_FILE_PATH);
    sprintf_stderr("%s: cannot load config %s", argv0, CONFIG_FILE_PATH);
  }

  oobj->substring = reader.Get("", "ignore_sensors", "PMIC");
  // average is the opposite of use_highest, so invert
  oobj->use_highest = !reader.GetBoolean("", "average", false);
  oobj->interval = reader.GetInteger("", "interval", 2);
  enable_tach = reader.GetBoolean("", "enable_tach", false);
  enable_max_freq = reader.GetBoolean("", "max_freq", true);
}
