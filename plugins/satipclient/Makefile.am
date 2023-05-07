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

noinst_LTLIBRARIES = satipclient.la

satipclient_la_SOURCES = satipclient.cpp
	 
satipclient_la_LDFLAGS = -rpath $(PLUGINDIR) -module

satipclient_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/satipclient
	install .libs/satipclient.so $(DESTDIR)$(PLUGINDIR)/satipclient
	install -m 0644 satipclient.cfg $(DESTDIR)$(PLUGINDIR)/satipclient
	install -m 0644 satipclient.png $(DESTDIR)$(PLUGINDIR)/satipclient
	install -m 0644 satip-client.conf $(DESTDIR)/etc

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/satipclient/satipclient.so
	-rm $(DESTDIR)$(PLUGINDIR)/satipclient/satipclient.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/satipclient/satipclient.png
	-rm $(DESTDIR)/etc/satip-client.conf
	-rmdir $(DESTDIR)$(PLUGINDIR)/satipclient
	
bin_PROGRAMS = satip_client

satip_client_SOURCES = log.c polltimer.c satip_config.c satip_main.c satip_rtp.c satip_rtsp.c satip_vtuner.c

satip_client_LDADD = -lrt -lpthread

installdir = $(bindir)

install_DATA = \
	satip-client.sh

install-data-hook:
	chmod 0755 $(DESTDIR)$(bindir)/satip-client.sh



	




