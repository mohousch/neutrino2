AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/system
	
noinst_LIBRARIES = libtuxbox-connection.a

AM_CPPFLAGS += -fno-rtti -fno-exceptions

libtuxbox_connection_a_SOURCES = basicclient.cpp basicsocket.cpp messagetools.cpp
