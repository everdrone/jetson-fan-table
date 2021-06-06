#pragma once

#include <stdarg.h>
#include <syslog.h>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "config.h"
#include "defines.h"

void _daemon_logv(int priority, const char* message, va_list arglist) {
  int saved_errno = errno;

  openlog(PACKAGE_NAME ? PACKAGE_NAME : "UNKNOWN", LOG_PID, LOG_DAEMON);
  vsyslog(priority | LOG_DAEMON, message, arglist);

  errno = saved_errno;
}

void daemon_log(int priority, const char* message, ...) {
  va_list arglist;

  va_start(arglist, message);
  _daemon_logv(priority, message, arglist);
  va_end(arglist);
}

// TOFO: move to stdout?
void debug_log(const char* message, ...) {
  if (enable_debug) {
    va_list arglist;

    va_start(arglist, message);
    _daemon_logv(LOG_DEBUG, message, arglist);
    va_end(arglist);
  }
}

/*
 * Format a string like sprintf
 * from: https://stackoverflow.com/a/26221725
 */
template <typename... Args>
std::string string_format(const std::string& format, Args... args) {
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;  // Extra space for '\0'
  if (size_s <= 0) {
    throw std::runtime_error("Error during formatting.");
  }
  auto size = static_cast<size_t>(size_s);
  auto buf = std::make_unique<char[]>(size);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(),
                     buf.get() + size - 1);  // We don't want the '\0' inside
}

template <typename... Args>
void sprintf_stderr(const char* format, Args... args) {
  std::string message = string_format(format, args...);
  std::cerr << message << std::endl;
}
