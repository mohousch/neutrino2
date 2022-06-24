#SUBDIRS = daemon

AM_CPPFLAGS =  \
	-I$(srcdir) \
	-I$(top_srcdir)/../neutrino2 \
	-I$(top_srcdir)/../neutrino2/src \
	-I$(top_srcdir)/../neutrino2/src/zapit/include/zapit \
	-I$(top_srcdir)/../neutrino2/lib \
	-I$(top_srcdir)/../neutrino2/lib/connection \
	-I$(top_srcdir)/../neutrino2/lib/libeventserver \
	-I$(top_srcdir)/../neutrino2/lib/libconfigfile \
	-I$(top_srcdir)/../neutrino2/lib/libnet \
	-I$(top_srcdir)/../neutrino2/lib/libxmltree \
	-I$(top_srcdir)/../neutrino2/lib/libmd5sum \
	-I$(top_srcdir)/../neutrino2/lib/libdvbapi \
	@FREETYPE_CFLAGS@

noinst_LTLIBRARIES = tuxcal.la

tuxcal_la_SOURCES = tuxcal.cpp

tuxcal_la_LIBADD = \
	-lz \
	@FREETYPE_LIBS@

tuxcal_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/tuxcal
	install .libs/tuxcal.so $(DESTDIR)$(PLUGINDIR)/tuxcal
	install -m 0644 $(srcdir)/tuxcal.cfg $(DESTDIR)$(PLUGINDIR)/tuxcal
	install -m 0644 $(srcdir)/tuxcal.conf $(DESTDIR)$(PLUGINDIR)/tuxcal
	install -m 0644 $(srcdir)/tuxcal.png $(DESTDIR)$(PLUGINDIR)/tuxcal

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/tuxcal/tuxcal.so
	-rm $(DESTDIR)$(PLUGINDIR)/tuxcal/tuxcal.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/tuxcal/tuxcal.conf
	-rm $(DESTDIR)$(PLUGINDIR)/tuxcal/tuxcal.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/tuxcal
