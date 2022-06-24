
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

	




