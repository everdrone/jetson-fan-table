AM_CPPFLAGS = \
    -I$(top_srcdir)/vendor \
    -I$(top_srcdir)/include

sbin_PROGRAMS = fantable

EXTRA_DIST = data/table data/config readme.md license

fantable_SOURCES = \
    src/main.cpp \
    include/atexit.h \
    include/jetson_clocks.h \
    include/load_config.h \
    include/defines.h \
    include/interpolate.h \
    include/log.h \
    include/parse_table.h \
    include/pid.h \
    include/thermal.h \
    include/utils.h \
    vendor/inih/cpp/INIReader.h \
    vendor/inih/cpp/INIReader.cpp \
    vendor/inih/ini.h \
    vendor/inih/ini.c

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
pkgsysconf_DATA = data/table data/config

# rules to package a .deb file
debfile_name=$(PACKAGE_NAME)-$(PACKAGE_VERSION)

deb: dist
    # extract dist into a new directory
	tar xzf $(top_builddir)/$(debfile_name).tar.gz
	cp -r $(top_srcdir)/debian $(builddir)/$(debfile_name)
    # build into the extracted directory
	cd $(debfile_name) && \
	debuild --no-tgz-check -us -uc -tc
    # clean uo
	rm -r $(debfile_name)
