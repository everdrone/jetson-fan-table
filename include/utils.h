#pragma once

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

/*
 * Read an int from a file
 */
int read_file_int(const char* path) {
  int number;

  std::ifstream in_stream(path);
  if (in_stream) {
    std::string file_contents;

    in_stream >> file_contents;
    try {
      number = std::stoi(file_contents);
    } catch (...) {
      daemon_log(LOG_ERR, "cannot parse int from file `%s'", path);
      sprintf_stderr("%s: cannot parse int from file `%s'", argv0, path);
      exit(EXIT_FAILURE);
    }
  } else {
    daemon_log(LOG_ERR, "cannot open `%s'", path);
    sprintf_stderr("%s: cannot open `%s'", argv0, path);
    exit(EXIT_FAILURE);
  }

  return number;
}

/*
 * Write an int into a file
 */
void write_file_int(const char* path, int value) {
  std::ofstream out_stream(path, std::ios::out);

  if (out_stream) {
    out_stream << std::to_string(value) << std::endl;
  } else {
    daemon_log(LOG_ERR, "cannot open `%s'", path);
    sprintf_stderr("%s: cannot open `%s'", argv0, path);
    exit(EXIT_FAILURE);
  }

  return;
}

/*
 * Write string to file (no endline)
 */
void write_file(const char* path, const char* str) {
  std::ofstream out_stream(path, std::ios::out);

  if (out_stream) {
    out_stream << str;
  } else {
    daemon_log(LOG_ERR, "cannot open `%s'", path);
    sprintf_stderr("%s: cannot open `%s'", argv0, path);
    exit(EXIT_FAILURE);
  }

  return;
}

/*
 * Splits a string into a vector at every delimiter
 */
std::vector<std::string> split_string(const std::string& str, const std::string& delimiter) {
  std::vector<std::string> strings;
  std::string::size_type pos = 0;
  std::string::size_type prev = 0;
  while ((pos = str.find(delimiter, prev)) != std::string::npos) {
    strings.push_back(str.substr(prev, pos - prev));
    prev = pos + 1;
  }

  // To get the last substring (or only, if delimiter is not found)
  strings.push_back(str.substr(prev));

  return strings;
}

/*
 * Read a file and plit it into lines
 */
std::vector<std::string> read_lines(const char* path) {
  std::ifstream in_stream(path);
  std::vector<std::string> result_lines;

  if (in_stream) {
    std::string result((std::istreambuf_iterator<char>(in_stream)),
                       std::istreambuf_iterator<char>());
    result_lines = split_string(result, "\n");
    return result_lines;
  } else {
    daemon_log(LOG_ERR, "cannot open `%s'", path);
    sprintf_stderr("%s: cannot open `%s'", argv0, path);
    exit(EXIT_FAILURE);
  }

  return result_lines;
}

/*
 * Read the whole file into a string
 */
std::string read_file(const char* path) {
  std::ifstream in_stream(path);
  std::string result;

  if (in_stream) {
    result =
        std::string((std::istreambuf_iterator<char>(in_stream)), std::istreambuf_iterator<char>());
    return result;
  } else {
    daemon_log(LOG_ERR, "cannot open `%s'", path);
    sprintf_stderr("%s: cannot open `%s'", argv0, path);
    exit(EXIT_FAILURE);
  }

  return result;
}

// trim from left
inline std::string& ltrim(std::string& s, const char* t = " \t\n\r\f\v") {
  s.erase(0, s.find_first_not_of(t));
  return s;
}

// trim from right
inline std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v") {
  s.erase(s.find_last_not_of(t) + 1);
  return s;
}

// trim from left & right
inline std::string& trim(std::string& s, const char* t = " \t\n\r\f\v") {
  return ltrim(rtrim(s, t), t);
}

static std::string& join(const std::vector<std::string>& elems, std::string& s, std::string delim) {
  for (std::vector<std::string>::const_iterator ii = elems.begin(); ii != elems.end(); ++ii) {
    s += (*ii);
    if (ii + 1 != elems.end()) {
      s += delim;
    }
  }

  return s;
}

static std::string join(const std::vector<std::string>& elems, std::string delim) {
  std::string s;
  return join(elems, s, delim);
}

/*
 * check if string is whitespace or non-ascii characters
 * returns true if empty or whitespace
 * returns false if not empty
 * returns false if non-ascii
 */
bool is_only_ascii_whitespace(const std::string& str) {
  auto it = str.begin();
  do {
    if (it == str.end()) return true;
  } while (*it >= 0 && *it <= 0x7f && std::isspace(*(it++)));
  // one of these conditions will be optimized away by the compiler,
  // which one depends on whether char is signed or not
  return false;
}

bool is_sudo_or_root() {
  if (geteuid() == 0) return true;
  return false;
}
