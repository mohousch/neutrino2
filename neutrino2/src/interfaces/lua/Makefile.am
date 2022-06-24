
AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/zapit/include/zapit \
	-I$(top_srcdir)/src/gui/widget \
	-I$(top_srcdir)/src/interfaces \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/libxmltree \
	-I$(top_srcdir)/lib/libmd5sum \
	-I$(top_srcdir)/lib/libupnpclient \
	@FREETYPE_CFLAGS@ \
	@FRIBIDI_CFLAGS@ \
	@LUA_INCLUDE@

if BOXTYPE_COOLSTREAM
AM_CPPFLAGS += -I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib/libcoolstream
else
AM_CPPFLAGS += -I$(top_srcdir)/lib/libdvbapi
if ENABLE_CI
AM_CPPFLAGS += -I$(top_srcdir)/lib/libdvbci
endif
endif

if !ENABLE_GSTREAMER
AM_CPPFLAGS += -I$(top_srcdir)/lib/libeplayer3/include
endif

AM_CPPFLAGS += -fno-rtti -fno-exceptions -D__KERNEL_STRICT_NAMES

noinst_LIBRARIES = libneutrino_interfaces_lua.a

libneutrino_interfaces_lua_a_SOURCES = \
	neutrino_lua.cpp

nodist_libneutrino_interfaces_lua_a_SOURCES = \
	neutrino_lua_wrap.cxx

neutrino_lua_wrap.cxx :
	$(AM_V_GEN)$(SWIG) -lua -I$(top_srcdir) -cpperraswarn neutrino_lua.i

CLEANFILES = neutrino_lua_wrap.cxx





