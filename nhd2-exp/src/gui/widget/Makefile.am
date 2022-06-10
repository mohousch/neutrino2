AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(srcdir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/zapit/include/zapit \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libxmltree \
	@FREETYPE_CFLAGS@

if BOXTYPE_COOLSTREAM
AM_CPPFLAGS += -I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib/libcoolstream
else
AM_CPPFLAGS += -I$(top_srcdir)/lib/libdvbapi
endif

AM_CPPFLAGS += -fno-rtti -fno-exceptions

noinst_LIBRARIES = libneutrino_gui_widget.a libneutrino_gui_widget2.a 

libneutrino_gui_widget_a_SOURCES = \
	drawable.cpp \
	widget_helpers.cpp \
	window.cpp \
	stringinput.cpp \
	stringinput_ext.cpp \
	infobox.cpp \
	messagebox.cpp \
	hintbox.cpp \
	helpbox.cpp \
	textbox.cpp \
	listframe.cpp \
	listbox.cpp \
	progresswindow.cpp \
	framebox.cpp \
	widget.cpp \
	menue.cpp

libneutrino_gui_widget2_a_SOURCES = \
	colorchooser.cpp \
	keychooser.cpp \
	mountchooser.cpp




