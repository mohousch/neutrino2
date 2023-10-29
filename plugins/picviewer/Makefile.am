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

noinst_LTLIBRARIES = picviewer.la

picviewer_la_SOURCES = picviewer.cpp
	 
picviewer_la_LDFLAGS = -rpath $(PLUGINDIR) -module

picviewer_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/picviewer
	install .libs/picviewer.so $(DESTDIR)$(PLUGINDIR)/picviewer
	install -m 0644 picviewer.cfg $(DESTDIR)$(PLUGINDIR)/picviewer
	install -m 0644 picviewer.png $(DESTDIR)$(PLUGINDIR)/picviewer

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/picviewer/picviewer.so
	-rm $(DESTDIR)$(PLUGINDIR)/picviewer/picviewer.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/picviewer/picviewer.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/picviewer
	




