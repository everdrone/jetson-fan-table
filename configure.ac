AC_INIT([fantable], [1.1.0], [])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])

AX_CXX_COMPILE_STDCXX_17

# this should be handled by dh_systemd_enable
prefix=/usr
sysconfdir=/etc
localstatedir=/var
# sbindir=/usr/sbin

AC_PROG_CXX
AC_PROG_CC

# PKG_CHECK_MODULES(LIBDAEMON, [ libdaemon >= 0.14 ])
# AC_SUBST(LIBDAEMON_CFLAGS)
# AC_SUBST(LIBDAEMON_LIBS)


AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
  Makefile
  data/fantable.service
])
AC_OUTPUT
