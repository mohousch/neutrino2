AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/gui/widget \
	-I$(top_srcdir)/src/system \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libxmltree \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libdvbapi \
	@FREETYPE_CFLAGS@

AM_CPPFLAGS += -fno-rtti -fno-exceptions -Wno-narrowing

noinst_LIBRARIES = libtuxtxt.a

libtuxtxt_a_SOURCES = libtuxtxt.cpp tuxtxt.cpp

install-exec-local:
	install -d $(DESTDIR)$(CONFIGDIR)/tuxtxt
	install -m 0644 $(srcdir)/tuxtxt2.conf $(DESTDIR)$(CONFIGDIR)/tuxtxt
