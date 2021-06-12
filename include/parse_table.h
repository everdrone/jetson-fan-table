#pragma once

/*
 * Uncomment this to compile with regex
 * size changes from 740 KB to 2.5 MB!!
 */
// #define USE_REGEX

#ifdef USE_REGEX
#include <regex>
#endif
#include <iostream>
#include <string>

#include "interpolate.h"
#include "log.h"
#include "utils.h"

using std::string;
using std::vector;

vector<coord_t> parse_table(const char* path, bool check = false) {
  vector<coord_t> result;
  vector<string> lines;

  try {
    lines = read_lines(path);
  } catch (...) {
    daemon_log(LOG_ERR, "cannot parse `%s'", path);
    sprintf_stderr("%s: cannot parse `%s'", argv0, path);
    exit(EXIT_FAILURE);
  }

#ifdef USE_REGEX
  std::regex re("([0-9]+)(?:,?(?:\\s+)?)([0-9]+)");

  for (size_t i = 0; i < lines.size(); i++) {
    std::smatch matches;
    std::regex_match(lines[i], matches, re);

    if (matches.size() == 3) {
      coord_t row;
      try {
        row.x = std::stoi(matches[1]);
        row.y = std::stoi(matches[2]);

        result.push_back(row);
      } catch (...) {
        daemon_log(LOG_ERR, "cannot parse `%s' at line %d", path, i);
        sprintf_stderr("%s: cannot parse `%s' at line %d", argv0, path, i);
      }
    }
  }
#else   // USE_REGEX
  for (size_t i = 0; i < lines.size(); i++) {
    if (is_only_ascii_whitespace(lines[i])) {
      continue;
    } else {
      vector<string> parsed = split_string(lines[i], " ");
      coord_t row;

      try {
        row.x = std::stoi(parsed[0]);
        row.y = std::stoi(parsed[1]);

        result.push_back(row);
      } catch (...) {
        daemon_log(LOG_ERR, "cannot parse `%s' at line %d", path, i);
        sprintf_stderr("%s: cannot parse `%s' at line %d", argv0, path, i);
        exit(EXIT_FAILURE);
      }
    }
  }
#endif  // USE_REGEX

  // NOTE: this part is only to check that the file is written correctly
  if (check) {
    for (size_t i = 0; i < result.size(); i++) {
      if (result[i].y < 0 || result[i].y > 100) {
        daemon_log(LOG_ERR,
                   "%s: parse error in `%s' at row %d:"
                   "    fan speed should be >= 0 and <= 100. got %d",
                   path, i, result[i].y);
        sprintf_stderr(
            "%s: parse error in `%s' at row %d:"
            "    fan speed should be >= 0 and <= 100. got %d",
            argv0, path, i, result[i].y);
        exit(EXIT_FAILURE);
      }
    }
  }

  return result;
}

void check_table(const char* path, bool exit_after = true) {
  vector<string> lines;
  try {
    lines = read_lines(path);
  } catch (...) {
    daemon_log(LOG_ERR, "cannot parse `%s'", path);
    sprintf_stderr("%s: cannot parse `%s'", argv0, path);
    exit(EXIT_FAILURE);
  }
#ifdef USE_REGEX
#else
  for (size_t i = 0; i < lines.size(); i++) {
    if (is_only_ascii_whitespace(lines[i])) {
      continue;
    } else {
      try {
        vector<string> parsed = split_string(lines[i], " ");
        if (!parsed[0].empty() && !parsed[1].empty()) {
          std::stoi(parsed[0]);
          std::stoi(parsed[1]);
        } else {
          daemon_log(LOG_ERR, "cannot parse line %d: `%s'", i, path);
          sprintf_stderr("%s: cannot parse line %d: `%s'", argv0, i, lines[i].c_str());
          exit(EXIT_FAILURE);
        }
      } catch (...) {
        daemon_log(LOG_ERR, "cannot parse line %d: `%s'", i, path);
        sprintf_stderr("%s: cannot parse line %d: `%s'", argv0, i, lines[i].c_str());
        exit(EXIT_FAILURE);
      }
    }
  }
#endif
  std::cout << "ok" << std::endl;
  if (exit_after) {
    exit(EXIT_SUCCESS);
  }
}
