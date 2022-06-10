################################################################################################################################################################################################################################################################
# Makefile for building neutrinoHD2 for x86		
# build options:
#  --with-configdir=PATH   where to find the config files [PREFIX/var/tuxbox/config]
#  --with-datadir=PATH     where to find data [PREFIX/share/tuxbox]
#  --with-plugindir=PATH   where to find the plugins [PREFIX/var/tuxbox/plugins]
#  --with-dvbincludes=PATH path for dvb includes [NONE]
#  --with-driver=PATH      path for driver sources [NONE]
#  --with-boxtype          
#  --enable-keyboard-no-rc enable keyboard control, disable rc control
#  --enable-opengl         include opengl framebuffer support for x86
#  --enable-playback       include enable playback for opengl and satip
#  --enable-gstreamer      include gstreamer as player engine support
#  --enable-lcd            include lcd support
#  --enable-scart          enable scart output
#  --enable-ci             enable ci cam
#  --enable-4digits        include 4 segment lcd support
#  --enable-functionkeys   include RC functions keys support
#  --enable-lua
#  --enable-python
#
#
# build preqs
# sudo apt-get install autoconf libtool libtool-bin g++ gdb swig flex bison make texinfo subversion intltool dialog wget cmake gperf libglew-dev freeglut3-dev libcurl4-gnutls-dev libfreetype6-dev libid3tag0-dev libmad0-dev libavformat-dev libfribidi-dev libogg-dev libpng-dev libgif-dev libjpeg-dev libflac-dev libvorbis-dev libopenthreads-dev libblkid-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev lua5.2 lua5.2-dev lua-json lua-expat lua-posix lua-socket lua-soap lua-curl python2-dev libao-dev libass-dev
################################################################################################################################################################################################################################################################
SHELL = /bin/bash
UID := $(shell id -u)
ifeq ($(UID), 0)
warn:
	@echo "You are running as root. Do not do this, it is dangerous."
	@echo "Aborting the build. Log in as a regular user and retry."
else
LC_ALL:=C
LANG:=C
export TOPDIR LC_ALL LANG

#
#
#

BOXTYPE = generic
DEST = $(PWD)/$(BOXTYPE)

N_SRC  = $(PWD)/nhd2-exp

CFLAGS = -Wall -O2 -fno-strict-aliasing -O0

CXXFLAGS = $(CFLAGS)

export CFLAGS CXXFLAGS

# first target is default...
default: neutrino plugins

run:
	$(DEST)/bin/neutrino
	
run-info:
	$(DEST)/bin/neutrino -v 1
	
run-debug:
	$(DEST)/bin/neutrino -v 2		
	
run-gdb:
	gdb -ex run $(DEST)/bin/neutrino
	
# init	
init:
# opengl
#	@echo -e "\nOpengl:"
#	@echo "   1) yes"
#	@echo "   2) no"
#	@read -p "Select opengl (1-2)?" OPENGL; \
#	OPENGL=$${OPENGL}; \
#	case "$$OPENGL" in \
#		1) echo "OPENGL=opengl" > config.local;; \
#		2) echo "OPENGL=" > config.local;; \
#		*) echo "OPENGL=opengl" > config.local;; \
#	esac; \
#	echo ""
# Media framework
	@echo -e "\nMedia Framework:"
	@echo "   1) buildinplayer (revisited libeplayer3 recommended for sh4 boxes)"
	@echo -e "   \033[01;32m2) gstreamer (recommended for x86 / mips and arm boxes)\033[00m"
	@read -p "Select media framework (1-2)?" MEDIAFW; \
	MEDIAFW=$${MEDIAFW}; \
	case "$$MEDIAFW" in \
		1) echo "MEDIAFW=buildinplayer" > config.local;; \
		2|*) echo "MEDIAFW=gstreamer" > config.local;; \
	esac; \
	echo ""
# playback
	@echo -e "\nPlayback:"
	@echo -e "   \033[01;32m1) yes\033[00m"
	@echo "   2) no"
	@read -p "Select playback (1-2)?" PLAYBACK; \
	PLAYBACK=$${PLAYBACK}; \
	case "$$PLAYBACK" in \
		1) echo "PLAYBACK=playback" >> config.local;; \
		2) echo "PLAYBACK=" >> config.local;; \
		*) echo "PLAYBACK=playback" >> config.local;; \
	esac; \
	echo ""
# lua
	@echo -e "\nlua support ?:"
	@echo -e "   \033[01;32m1) yes\033[00m"
	@echo "   2)  no"
	@read -p "Select lua support (1-2)?" LUA; \
	LUA=$${LUA}; \
	case "$$LUA" in \
		1) echo "LUA=lua" >> config.local;; \
		2) echo "LUA=" >> config.local;; \
		*) echo "LUA=lua" >> config.local;; \
	esac; \
	echo ""
# python
	@echo -e "\npython support ?:"
	@echo "   1)  yes"
	@echo -e "   \033[01;32m2) no\033[00m"
	@read -p "Select python support (1-2)?" PYTHON; \
	PYTHON=$${PYTHON}; \
	case "$$PYTHON" in \
		1) echo "PYTHON=python" >> config.local;; \
		2|*) echo "PYTHON=" >> config.local;; \
	esac; \
	echo ""
# cicam
	@echo -e "\nCICAM support ?:"
	@echo "   1)  yes"
	@echo -e "   \033[01;32m2) no\033[00m"
	@read -p "Select CI-CAM support (1-2)?" CICAM; \
	CICAM=$${CICAM}; \
	case "$$CICAM" in \
		1) echo "CICAM=cicam" >> config.local;; \
		2|*) echo "CICAM=" >> config.local;; \
	esac; \
	echo ""
# vfd / 4digits / lcd
	@echo -e "\nLCD support ?:"
	@echo "   1)  VFD"
	@echo "   2)  4 Digits"
	@echo "   3)  LCD"
	@echo -e "   \033[01;32m4) none\033[00m"
	@read -p "Select LCD support (1-4)?" LCD; \
	LCD=$${LCD}; \
	case "$$LCD" in \
		1|4|*) echo "LCD=" >> config.local;; \
		2) echo "LCD=4-digits" >> config.local;; \
		3) echo "LCD=lcd" >> config.local;; \
	esac; \
	echo ""	
# scart
	@echo -e "\nScart support ?:"
	@echo "   1)  yes"
	@echo -e "   \033[01;32m2) no\033[00m"
	@read -p "Select SCART support (1-2)?" SCART; \
	SCART=$${SCART}; \
	case "$$SCART" in \
		1) echo "SCART=scart" >> config.local;; \
		2|*) echo "SCART=" >> config.local;; \
	esac; \
	echo ""
# FKEYS
	@echo -e "\nFKEYS support ?:"
	@echo "   1)  yes"
	@echo -e "   \033[01;32m2) no\033[00m"
	@read -p "Select FKEYS support (1-2)?" FKEYS; \
	FKEYS=$${FKEYS}; \
	case "$$FKEYS" in \
		1) echo "FKEYS=fkeys" >> config.local;; \
		2|*) echo "FKEYS=" >> config.local;; \
	esac; \
	echo ""		
# fake tuner for testing
	@echo -e "\nFake Tuner support ?:"
	@echo "   1)  yes"
	@echo -e "   \033[01;32m2) no\033[00m"
	@read -p "Select FAKETUNER support (1-2)?" FAKETUNER; \
	FAKETUNER=$${FAKETUNER}; \
	case "$$FAKETUNER" in \
		1) echo "FAKETUNER=faketuner" >> config.local;; \
		2|*) echo "FAKETUNER=" >> config.local;; \
	esac; \
	echo ""
# testing
	@echo -e "\nTesting support ?:"
	@echo "   1)  yes"
	@echo -e "   \033[01;32m2) no\033[00m"
	@read -p "Select TESTING support (1-2)?" TESTING; \
	TESTING=$${TESTING}; \
	case "$$TESTING" in \
		1) echo "TESTING=testing" >> config.local;; \
		2|*) echo "TESTING=" >> config.local;; \
	esac; \
	echo ""				

init-clean:
	rm -f config.local
	
-include config.local

# opengl
OPENGL ?= opengl

ifeq ($(OPENGL), opengl)
NHD2_OPTS += --enable-opengl
endif	

# MEDIAFW
MEDIAFW ?= gstreamer

ifeq ($(MEDIAFW), gstreamer)
NHD2_OPTS += --enable-gstreamer --with-gstversion=1.0
endif

# opengl
PLAYBACK ?= playback

ifeq ($(PLAYBACK), playback)
NHD2_OPTS += --enable-playback
endif

# python
PYTHON ?=

ifeq ($(PYTHON), python)
NHD2_OPTS += --enable-python
endif

# lua
LUA ?= lua

ifeq ($(LUA), lua)
NHD2_OPTS += --enable-lua
endif

# CICAM
CICAM ?=

ifeq ($(CICAM), cicam)
NHD2_OPTS += --enable-ci
endif

# SCART
SCART ?=

ifeq ($(SCART), scart)
NHD2_OPTS += --enable-scart
endif

# LCD 
LCD ?=

ifeq ($(LCD), lcd)
NHD2_OPTS += --enable-lcd
endif

ifeq ($(LCD), 4-digits)
NHD2_OPTS += --enable-4digits
endif

# FKEYS
FKEY ?=

ifeq ($(FKEYS), fkeys)
NHD2_OPTS += --enable-functionkeys
endif

# FAKETUNER
FAKETUNER ?=

ifeq ($(FAKETUNER), faketuner)
NHD2_OPTS += --enable-fake_tuner
endif

# test plugins
TESTING ?=

ifeq ($(TESTING), testing)
NHD2_OPTS += --enable-testing
endif			

neutrino: $(N_SRC)/config.status
	-rm -f $(N_SRC)/src/gui/svn_version.h
	$(MAKE) -C $(N_SRC) install

$(N_SRC)/config.status: | $(N_SRC) $(DEST)
	$(N_SRC)/autogen.sh
	set -e; cd $(N_SRC); \
		$(N_SRC)/configure \
			--prefix=$(DEST) \
			--build=i686-pc-linux-gnu \
			--enable-silent-rules \
			--enable-maintainer-mode \
			--with-boxtype=$(BOXTYPE) \
			$(NHD2_OPTS)
$(DEST):
	mkdir $@

$(N_SRC):
	git pull

neutrino-checkout: $(N_SRC)

neutrino-clean:
	-$(MAKE) -C $(N_SRC) clean

neutrino-distclean:
	-$(MAKE) -C $(N_SRC) distclean
	rm -f $(N_SRC)/config.status

# plugins
PLUGINS_SRC = $(PWD)/plugins
$(PLUGINS_SRC):
	git pull

plugins-checkout: $(PLUGINS_SRC)

plugins: $(PLUGINS_SRC)/config.status $(N_SRC)/config.status
	$(MAKE) -C $(PLUGINS_SRC) install

$(PLUGINS_SRC)/config.status: $(PLUGINS_SRC) $(DEST)
	$(PLUGINS_SRC)/autogen.sh
	set -e; cd $(PLUGINS_SRC); \
		$(PLUGINS_SRC)/configure \
			--prefix=$(DEST)  \
			--build=i686-pc-linux-gnu \
			--enable-silent-rules \
			--enable-maintainer-mode \
			--without-debug \
			--with-boxtype=$(BOXTYPE) \
			--enable-python \
			--enable-lua \
			--enable-testing

plugins-clean:
	-$(MAKE) -C $(PLUGINS_SRC) clean

plugins-distclean:
	-$(MAKE) -C $(PLUGINS_SRC) distclean
	rm -f $(PLUGINS)/config.status

update:
	git pull

clean: neutrino-clean plugins-clean
distclean: neutrino-distclean plugins-distclean

PHONY = neutrino-checkout plugins-checkout
.PHONY: $(PHONY)

endif
