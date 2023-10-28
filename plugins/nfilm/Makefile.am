SUBDIRS = locale

AM_CPPFLAGS =  \
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

noinst_LTLIBRARIES = nfilm.la

nfilm_la_SOURCES = nfilm.cpp
	 
nfilm_la_LDFLAGS = -rpath $(PLUGINDIR) -module

nfilm_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/nfilm
	install .libs/nfilm.so $(DESTDIR)$(PLUGINDIR)/nfilm
	install -m 0644  nfilm.cfg $(DESTDIR)$(PLUGINDIR)/nfilm
	install -m 0644  nfilm.png $(DESTDIR)$(PLUGINDIR)/nfilm

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/nfilm/nfilm.so
	-rm $(DESTDIR)$(PLUGINDIR)/nfilm/nfilm.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/nfilm/nfilm.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/nfilm
	




