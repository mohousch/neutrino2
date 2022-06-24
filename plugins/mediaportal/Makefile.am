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

noinst_LTLIBRARIES = mediaportal.la

mediaportal_la_SOURCES = mediaportal.cpp
	 
mediaportal_la_LDFLAGS = -rpath $(PLUGINDIR) -module

mediaportal_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/mediaportal
	install .libs/mediaportal.so $(DESTDIR)$(PLUGINDIR)/mediaportal
	install -m 0644 mediaportal.cfg $(DESTDIR)$(PLUGINDIR)/mediaportal
	install -m 0644 mp.png $(DESTDIR)$(PLUGINDIR)/mediaportal

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/mediaportal/mediaportal.so
	-rm $(DESTDIR)$(PLUGINDIR)/mediaportal/mediaportal.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/mediaportal/mp.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/mediaportal



