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

noinst_LTLIBRARIES = tuxcom.la

tuxcom_la_SOURCES = tuxcom.cpp

tuxcom_la_LIBADD = \
	 @FREETYPE_LIBS@ \
	-lz
	 
tuxcom_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version 

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/tuxcom
	install .libs/tuxcom.so $(DESTDIR)$(PLUGINDIR)/tuxcom
	install -m 0644 $(srcdir)/tuxcom.cfg $(DESTDIR)$(PLUGINDIR)/tuxcom
	install -m 0644 $(srcdir)/tuxcom.png $(DESTDIR)$(PLUGINDIR)/tuxcom

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/tuxcom/tuxcom.so
	-rm $(DESTDIR)$(PLUGINDIR)/tuxcom/tuxcom.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/tuxcom/tuxcom.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/tuxcom

