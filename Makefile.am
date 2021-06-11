AM_CPPFLAGS = \
    -I$(top_srcdir)/vendor \
    -I$(top_srcdir)/include

sbin_PROGRAMS = fantable

EXTRA_DIST = data/table readme.md license

fantable_SOURCES = \
    src/main.cpp \
    include/atexit.h \
    include/clocks.h \
    include/jetson_clocks.h \
    include/defines.h \
    include/interpolate.h \
    include/log.h \
    include/parse_table.h \
    include/thermal.h \
    include/store_restore.h \
    include/utils.h

# jft_daemon_CPPFLAGS = $(LIBDAEMON_CFLAGS)
# jft_daemon_LDFLAGS = $(LIBDAEMON_LIBS)

sytemdsysdir = $(sysconfdir)/systemd/system
sytemdsys_DATA = data/fantable.service

.service: .service.in
	$(AM_V_GEN)sed -e 's,@prefix\@,$(prefix),g' $< > $@
	$(AM_V_GEN)sed -e 's,@exec_prefix\@,$(exec_prefix),g' $< > $@
	$(AM_V_GEN)sed -e 's,@sbindir\@,$(sbindir),g' $< > $@
	$(AM_V_GEN)sed -e 's,@sysconfdir\@,$(sysconfdir),g' $< > $@

pkgsysconfdir=$(sysconfdir)/fantable
pkgsysconf_DATA = data/table
