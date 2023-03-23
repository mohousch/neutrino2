AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(srcdir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libxmltree \
	-I$(top_srcdir)/lib/libeventserver \
	@FREETYPE_CFLAGS@

if BOXTYPE_COOLSTREAM
AM_CPPFLAGS += -I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib/libcoolstream
else
AM_CPPFLAGS += -I$(top_srcdir)/lib/libdvbapi
endif

AM_CPPFLAGS += -fno-rtti -fno-exceptions

noinst_LIBRARIES = libneutrino2_gui_widget.a libneutrino2_gui_widget2.a 

libneutrino2_gui_widget_a_SOURCES = \
	widget_helpers.cpp \
	window.cpp \
	textbox.cpp \
	listframe.cpp \
	listbox.cpp \
	framebox.cpp \
	widget.cpp

libneutrino2_gui_widget2_a_SOURCES = \
	drawable.cpp \
	progresswindow.cpp \
	infobox.cpp \
	messagebox.cpp \
	hintbox.cpp \
	helpbox.cpp \
	stringinput.cpp \
	stringinput_ext.cpp \
	colorchooser.cpp \
	keychooser.cpp \
	mountchooser.cpp
	
libneutrino2_gui_widget2_a_SOURCES += \
	menue.cpp


