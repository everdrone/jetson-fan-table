#pragma once

#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "defines.h"
#include "log.h"
#include "utils.h"

#define VARRUN "/var/run"

#ifndef PATH_MAX
#define PATH_MAX 512
#endif

const char* pid_file_name() {
  static char fn[PATH_MAX];
  snprintf(fn, sizeof(fn), "%s/%s.pid", VARRUN, argv0 ? argv0 : "unknown");

  return fn;
}

static int lock_file(int fd, bool enable) {
  struct flock f;

  memset(&f, 0, sizeof(f));
  f.l_type = enable ? F_WRLCK : F_UNLCK;
  f.l_whence = SEEK_SET;
  f.l_start = 0;
  f.l_len = 0;

  if (fcntl(fd, F_SETLKW, &f) < 0) {
    if (enable && errno == EBADF) {
      f.l_type = F_RDLCK;

      if (fcntl(fd, F_SETLKW, &f) >= 0) return 0;
    }

    daemon_log(LOG_ERR, "fcntl(F_SETLKW) failed: %s", strerror(errno));
    return -1;
  }

  return 0;
}

pid_t pid_file_is_running() {
  const char* fn;
  static char txt[256];
  int fd = -1;
  int locked = -1;

  pid_t ret = (pid_t)-1;
  pid_t pid;

  ssize_t l;
  long lpid;
  char* e = NULL;

  if (!(fn = pid_file_name())) {
    errno = EINVAL;
    goto finish;
  }

  if ((fd = open(fn, O_RDWR, 0644)) < 0) {
    if ((fd = open(fn, O_RDONLY, 0644)) < 0) {
      // this sets errno = ENOENT
      if (errno != ENOENT) daemon_log(LOG_WARNING, "failed to open PID file: %s", strerror(errno));

      goto finish;
    }
  }

  if ((locked = lock_file(fd, 1)) < 0) goto finish;

  if ((l = read(fd, txt, sizeof(txt) - 1)) < 0) {
    int saved_errno = errno;
    daemon_log(LOG_WARNING, "read(): %s", strerror(errno));
    unlink(fn);
    errno = saved_errno;
    goto finish;
  }

  txt[l] = 0;
  txt[strcspn(txt, "\r\n")] = 0;

  errno = 0;
  lpid = strtol(txt, &e, 10);
  pid = (pid_t)lpid;

  if (errno != 0 || !e || *e || (long)pid != lpid) {
    daemon_log(LOG_WARNING, "pid file corrupt, removing. (%s)", fn);
    unlink(fn);
    errno = EINVAL;
    goto finish;
  }

  if (kill(pid, 0) != 0 && errno != EPERM) {
    int saved_errno = errno;
    daemon_log(LOG_WARNING, "process %lu died: %s; trying to remove PID file. (%s)",
               (unsigned long)pid, strerror(errno), fn);
    unlink(fn);
    errno = saved_errno;
    goto finish;
  }

  ret = pid;

finish:

  if (fd >= 0) {
    int saved_errno = errno;
    if (locked >= 0) lock_file(fd, 0);
    close(fd);
    errno = saved_errno;
  }

  return ret;
}

int pid_file_create() {
  const char* fn;
  int fd = -1;
  int ret = -1;
  int locked = -1;
  char t[64];
  ssize_t l;
  mode_t u;

  u = umask(022);

  if (!(fn = pid_file_name())) {
    errno = EINVAL;
    goto finish;
  }

  if ((fd = open(fn, O_CREAT | O_RDWR | O_EXCL, 0644)) < 0) {
    daemon_log(LOG_ERR, "open(%s): %s", fn, strerror(errno));
    goto finish;
  }

  if ((locked = lock_file(fd, 1)) < 0) {
    int saved_errno = errno;
    unlink(fn);
    errno = saved_errno;
    goto finish;
  }

  snprintf(t, sizeof(t), "%lu\n", (unsigned long)getpid());

  l = strlen(t);
  if (write(fd, t, l) != l) {
    int saved_errno = errno;
    daemon_log(LOG_WARNING, "write(): %s", strerror(errno));
    unlink(fn);
    errno = saved_errno;
    goto finish;
  }

  ret = 0;

finish:

  if (fd >= 0) {
    int saved_errno = errno;

    if (locked >= 0) lock_file(fd, 0);

    close(fd);
    errno = saved_errno;
  }

  umask(u);

  return ret;
}

int pid_file_remove() {
  const char* fn;

  if (!(fn = pid_file_name())) {
    errno = EINVAL;
    return -1;
  }

  return unlink(fn);
}
