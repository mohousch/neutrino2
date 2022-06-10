AM_CPPFLAGS =  \
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

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/test/test.so
	-rm $(DESTDIR)$(PLUGINDIR)/test/test.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/test/test.png
	-rm $(DESTDIR)$(PLUGINDIR)/test/skin.xml
	-rmdir $(DESTDIR)$(PLUGINDIR)/test
	




