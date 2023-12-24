#AUTOMAKE_OPTIONS = subdir-objects

noinst_LIBRARIES = libeplayer3.a

AM_CXXFLAGS = -Wall

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libdvbapi \
	-I$(top_srcdir)/lib/libeplayer3
	
libeplayer3_a_SOURCES = playback.cpp

AM_CPPFLAGS += -Wall -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE

# get rid of many warnings. This is not my project, so fixing them is not
# top priority
AM_CPPFLAGS += \
	-Wno-unused -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable \
	-Wno-sign-compare -Wno-missing-field-initializers

# silence ffmpeg deprecation warnings
AM_CPPFLAGS += -Dattribute_deprecated=''

