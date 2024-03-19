AUTOMAKE_OPTIONS = subdir-objects

noinst_LIBRARIES = libeplayer3.a

CXXFLAGS = -Wall

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(srcdir)/include \
	-I$(top_srcdir)/lib/libdvbapi

libeplayer3_a_SOURCES =  \
	container/container.c container/container_ffmpeg.c \
	manager/manager.c manager/audio.c manager/video.c manager/subtitle.c \
	output/linuxdvb.c output/subtitle.c output/output.c \
	output/writer/writer.c output/writer/misc.c \
	output/writer/aac.c output/writer/wmv.c output/writer/ac3.c \
	output/writer/dts.c output/writer/wma.c output/writer/mp3.c \
	output/writer/pcm.c output/writer/amr.c output/writer/lpcm.c \
	output/writer/pes.c output/writer/mpeg2.c output/writer/divx3.c output/writer/mjpeg.c output/writer/vp.c \
	output/writer/divx.c  output/writer/h264.c output/writer/h263.c output/writer/vc1.c output/writer/h265.c \
	output/writer/framebuffer.c \
	playback/playback.c

AM_CPPFLAGS += -Wall -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D__STDC_CONSTANT_MACROS

# get rid of many warnings. This is not my project, so fixing them is not
# top priority
AM_CPPFLAGS += \
	-Wno-unused -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable \
	-Wno-sign-compare -Wno-missing-field-initializers

# silence ffmpeg deprecation warnings
AM_CPPFLAGS += -Dattribute_deprecated=''

