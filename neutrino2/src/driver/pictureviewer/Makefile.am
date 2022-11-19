AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/zapit/include/zapit \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libxmltree \
	@FREETYPE_CFLAGS@

if BOXTYPE_COOLSTREAM
AM_CPPFLAGS += -I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib/libcoolstream
else
AM_CPPFLAGS += -I$(top_srcdir)/lib/libdvbapi
endif

AM_CXXFLAGS = -fno-rtti -fno-exceptions
 
noinst_LIBRARIES = libneutrino2_pictureviewer.a

libneutrino2_pictureviewer_a_SOURCES = pictureviewer.cpp

