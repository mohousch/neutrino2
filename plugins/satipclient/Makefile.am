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

noinst_LTLIBRARIES = satipclient.la

satipclient_la_SOURCES = satipclient.cpp
	 
satipclient_la_LDFLAGS = -rpath $(PLUGINDIR) -module

satipclient_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/satipclient
	install .libs/satipclient.so $(DESTDIR)$(PLUGINDIR)/satipclient
	install -m 0644 satipclient.cfg $(DESTDIR)$(PLUGINDIR)/satipclient
	install -m 0644 satipclient.png $(DESTDIR)$(PLUGINDIR)/satipclient
	install -m 0644 satip-client.conf $(DESTDIR)$(PLUGINDIR)/satipclient

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/satipclient/satipclient.so
	-rm $(DESTDIR)$(PLUGINDIR)/satipclient/satipclient.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/satipclient/satipclient.png
	-rm $(DESTDIR)$(PLUGINDIR)/satipclient/satip-client.conf
	-rmdir $(DESTDIR)$(PLUGINDIR)/satipclient

	




