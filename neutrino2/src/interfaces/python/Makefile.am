
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
	@PYTHON_CPPFLAGS@

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

noinst_LIBRARIES = libneutrino2_interfaces_python.a

libneutrino2_interfaces_python_a_SOURCES = \
	neutrino2_python.cpp

nodist_libneutrino2_interfaces_python_a_SOURCES = \
	neutrino2_python_wrap.cxx

neutrino2_python_wrap.cxx :
	$(AM_V_GEN)$(SWIG) $(AX_SWIG_PYTHON_OPT) -I$(top_srcdir) -cpperraswarn neutrino2_python.i

installdir = $(DATADIR)/python

nobase_install_PYTHON = \
	neutrino2.py

CLEANFILES = neutrino2.py neutrino2_python_wrap.cxx



