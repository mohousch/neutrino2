
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

noinst_LTLIBRARIES = internetradio.la

internetradio_la_SOURCES = internetradio.cpp
	 
internetradio_la_LDFLAGS = -rpath $(PLUGINDIR) -module

internetradio_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/internetradio
	install .libs/internetradio.so $(DESTDIR)$(PLUGINDIR)/internetradio
	install -m 0644 internetradio.cfg $(DESTDIR)$(PLUGINDIR)/internetradio
	install -m 0644 internetradio.png $(DESTDIR)$(PLUGINDIR)/internetradio
	install -m 0644 radio-stations.xml $(DESTDIR)$(PLUGINDIR)/internetradio


uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/internetradio/internetradio.so
	-rm $(DESTDIR)$(PLUGINDIR)/internetradio/internetradio.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/internetradio/internetradio.png
	-rm $(DESTDIR)$(PLUGINDIR)/internetradio/radio-stations.xml
	-rmdir $(DESTDIR)$(PLUGINDIR)/internetradio

	




