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

noinst_LTLIBRARIES = test.la

test_la_SOURCES = test.cpp
	 
test_la_LDFLAGS = -rpath $(PLUGINDIR) -module

test_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/test
	install .libs/test.so $(DESTDIR)$(PLUGINDIR)/test
	install -m 0644  test.cfg $(DESTDIR)$(PLUGINDIR)/test
	install -m 0644 test.png $(DESTDIR)$(PLUGINDIR)/test
	install -m 0644 skin.xml $(DESTDIR)$(PLUGINDIR)/test
	install -m 0644 test.xml $(DESTDIR)$(PLUGINDIR)/test

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/test/test.so
	-rm $(DESTDIR)$(PLUGINDIR)/test/test.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/test/test.png
	-rm $(DESTDIR)$(PLUGINDIR)/test/skin.xml
	-rm $(DESTDIR)$(PLUGINDIR)/test/test.xml
	-rmdir $(DESTDIR)$(PLUGINDIR)/test
	




