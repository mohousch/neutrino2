AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(srcdir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/zapit/include/zapit \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libxmltree

AM_CPPFLAGS += -fno-rtti -fno-exceptions

noinst_LIBRARIES = libtimerd.a

libtimerd_a_SOURCES = timerd.cpp timermanager.cpp

