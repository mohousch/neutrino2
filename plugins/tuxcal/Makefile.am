#SUBDIRS = daemon

AM_CPPFLAGS =  \
	-I$(srcdir) \
	-I$(top_srcdir)/../nhd2-exp \
	-I$(top_srcdir)/../nhd2-exp/src \
	-I$(top_srcdir)/../nhd2-exp/src/zapit/include/zapit \
	-I$(top_srcdir)/../nhd2-exp/lib \
	-I$(top_srcdir)/../nhd2-exp/lib/connection \
	-I$(top_srcdir)/../nhd2-exp/lib/libeventserver \
	-I$(top_srcdir)/../nhd2-exp/lib/libconfigfile \
	-I$(top_srcdir)/../nhd2-exp/lib/libnet \
	-I$(top_srcdir)/../nhd2-exp/lib/libxmltree \
	-I$(top_srcdir)/../nhd2-exp/lib/libmd5sum \
	-I$(top_srcdir)/../nhd2-exp/lib/libdvbapi \
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
