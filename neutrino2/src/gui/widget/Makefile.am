AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(srcdir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libxmltree \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libdvbapi \
	@FREETYPE_CFLAGS@

AM_CPPFLAGS += -fno-rtti -fno-exceptions

noinst_LIBRARIES = libneutrino2_gui_widget.a libneutrino2_gui_widget2.a 

libneutrino2_gui_widget_a_SOURCES = \
	component.cpp \
	textbox.cpp \
	listframe.cpp \
	listbox.cpp \
	framebox.cpp \
	widget.cpp

libneutrino2_gui_widget2_a_SOURCES = \
	progresswindow.cpp \
	infobox.cpp \
	messagebox.cpp \
	hintbox.cpp \
	helpbox.cpp \
	stringinput.cpp \
	stringinput_ext.cpp \
	keyboard_input.cpp \
	colorchooser.cpp \
	keychooser.cpp
	
