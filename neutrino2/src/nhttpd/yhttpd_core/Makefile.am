AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(srcdir) \
	-I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/nhttpd \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libconfigfile \
	@FREETYPE_CFLAGS@

AM_CPPFLAGS += -fno-rtti -fno-exceptions -D_FILE_OFFSET_BITS=64

noinst_LIBRARIES = libyhttpd.a

libyhttpd_a_SOURCES = \
	helper.cpp \
	ylanguage.cpp \
	ywebserver.cpp \
	yconnection.cpp \
	yrequest.cpp \
	yresponse.cpp \
	yhook.cpp \
	ysocket.cpp

