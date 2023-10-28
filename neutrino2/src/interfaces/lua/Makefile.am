
AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/gui/widget \
	-I$(top_srcdir)/src/interfaces \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/libxmltree \
	-I$(top_srcdir)/lib/libmd5sum \
	-I$(top_srcdir)/lib/libdvbapi \
	-I$(top_srcdir)/lib/libdvbci \
	@FREETYPE_CFLAGS@ \
	@FRIBIDI_CFLAGS@ \
	@LUA_INCLUDE@

if !ENABLE_GSTREAMER
AM_CPPFLAGS += -I$(top_srcdir)/lib/libeplayer3/include
endif

AM_CPPFLAGS += -fno-rtti -fno-exceptions -D__KERNEL_STRICT_NAMES

noinst_LIBRARIES = libneutrino2_interfaces_lua.a

libneutrino2_interfaces_lua_a_SOURCES = \
	neutrino2_lua.cpp

nodist_libneutrino2_interfaces_lua_a_SOURCES = \
	neutrino2_lua_wrap.cxx

neutrino2_lua_wrap.cxx :
	$(AM_V_GEN)$(SWIG) -lua -I$(top_srcdir) -cpperraswarn neutrino2_lua.i

CLEANFILES = neutrino2_lua_wrap.cxx





