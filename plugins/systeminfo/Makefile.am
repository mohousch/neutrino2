SUBDIRS = locale

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

noinst_LTLIBRARIES = systeminfo.la

systeminfo_la_SOURCES = systeminfo.cpp
	 
systeminfo_la_LDFLAGS = -rpath $(PLUGINDIR) -module

systeminfo_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/systeminfo
	install .libs/systeminfo.so $(DESTDIR)$(PLUGINDIR)/systeminfo
	install -m 0644  systeminfo.cfg $(DESTDIR)$(PLUGINDIR)/systeminfo
	install -m 0644  systeminfo.png $(DESTDIR)$(PLUGINDIR)/systeminfo

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/systeminfo/systeminfo.so
	-rm $(DESTDIR)$(PLUGINDIR)/systeminfo/systeminfo.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/systeminfo/systeminfo.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/systeminfo
	




