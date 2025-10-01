####################################################################################################
# Makefile for building neutrino2 for x86		
# build options:
#  --with-configdir=PATH   where to find the config files [PREFIX/var/tuxbox/config]
#  --with-datadir=PATH     where to find data [PREFIX/share/tuxbox]
#  --with-plugindir=PATH   where to find the plugins [PREFIX/var/tuxbox/plugins]
#  --with-dvbincludes=PATH path for dvb includes [NONE]
#  --with-driver=PATH      path for driver sources [NONE]
#  --with-boxtype 
#  --with-boxmodel	       
#  --enable-opengl         include opengl framebuffer support for x86
#  --enable-gstreamer      include gstreamer as player engine support
#  --with-gstversion       use gstreamer version (major.minor)
#  --enable-lcd            include lcd support
#  --enable_tftlcd	   include tft lcd support
#  --enable-4digits        include 4 segment lcd support
#  --enable-scart          enable scart output
#  --enable-ci             enable ci cam
#  --enable-functionkeys   include RC functions keys support
#  --enable-lua
#  --enable-python
#  --enable-testing        include testing plugins support
####################################################################################################
LC_ALL:=C
LANG:=C
export LC_ALL LANG

#
#
#

BOXTYPE = generic
DEST = $(PWD)/$(BOXTYPE)
N2_SRC  = $(PWD)/neutrino2
PLUGINS_SRC = $(PWD)/plugins
N2_OPTS = --with-boxtype=$(BOXTYPE)

CFLAGS = -Wall 
CFLAGS += -O2 
CFLAGS += -pipe 
CFLAGS += -fno-strict-aliasing 
CFLAGS += -O0 
CFLAGS += -std=c++11 
CFLAGS += -g
CFLAGS += -ggdb3
CFLAGS += -D__KERNEL_STRICT_NAMES
CFLAGS += -D__STDC_FORMAT_MACROS
CFLAGS += -D__STDC_CONSTANT_MACROS
#CFLAGS += -fsanitize=address
#CFLAGS += -Wno-analyzer-double-free
#CFLAGS += -Wno-analyzer-malloc-leak

CXXFLAGS = $(CFLAGS) 

export CFLAGS CXXFLAGS

# first target is default...
all: neutrino2 plugins

run:
	$(DEST)/bin/neutrino2		
	
run-gdb:
	gdb -ex run $(DEST)/bin/neutrino2
	
run-valgrind:
	valgrind --leak-check=full --track-origins=yes --error-limit=no --log-file="logfile.out" -v $(DEST)/bin/neutrino2
	
# init	
init:
# opengl
	@echo -e "\nopengl:"
	@echo -e "   \033[01;32m1) yes\033[00m"
	@echo "   2) no"
	@read -p "opengl (1-2)?" OPENGL; \
	OPENGL=$${OPENGL}; \
	case "$$OPENGL" in \
		1) echo "OPENGL=opengl" > config.local;; \
		2) echo "OPENGL=" > config.local;; \
		*) echo "OPENGL=opengl" > config.local;; \
	esac; \
	echo ""
# lirc
	@echo -e "\nlirc:"
	@echo "   1) yes"
	@echo -e "   \033[01;32m2) no\033[00m"
	@read -p "lirc (1-2)?" LIRC; \
	LIRC=$${LIRC}; \
	case "$$LIRC" in \
		1) echo "LIRC=lirc" >> config.local;; \
		2|*) echo "LIRC=" >> config.local;; \
	esac; \
	echo ""
# Media framework
	@echo -e "\nMedia Framework:"
	@echo -e "   \033[01;32m1) buildinplayer\033[00m"
	@echo "   2) gstreamer"
	@read -p "Select media framework (1-2)?" MEDIAFW; \
	MEDIAFW=$${MEDIAFW}; \
	case "$$MEDIAFW" in \
		1) echo "MEDIAFW=buildinplayer" >> config.local;; \
		2) echo "MEDIAFW=gstreamer" >> config.local;; \
		*) echo "MEDIAFW=buildinplayer" >> config.local;; \
	esac; \
	echo ""
# gstreamer opengl overlay
	@echo -e "\ngstreamer overlay:"
	@echo "   1) yes"
	@echo -e "   \033[01;32m2) no\033[00m"
	@read -p "Select overlay (1-2)?" OVERLAY; \
	OVERLAY=$${OVERLAY}; \
	case "$$OVERLAY" in \
		1) echo "OVERLAY=overlay" >> config.local;; \
		2|*) echo "OVERLAY=" >> config.local;; \
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
	@echo "   1)  yes (experimental)"
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
	@echo -e "   \033[01;32m1)  yes\033[00m"
	@echo "   2) no"
	@read -p "Select CI-CAM support (1-2)?" CICAM; \
	CICAM=$${CICAM}; \
	case "$$CICAM" in \
		1) echo "CICAM=cicam" >> config.local;; \
		2) echo "CICAM=" >> config.local;; \
		*) echo "CICAM=cicam" >> config.local;; \
	esac; \
	echo ""
# 4digits / vfd / lcd / tftlcd
	@echo -e "\nLCD support ?:"
	@echo -e "   \033[01;32m1)  None\033[00m"
	@echo "   2)  4 Digits"
	@echo "   3)  VFD"
	@echo "   4)  LCD"
	@echo "   5)  TFT LCD"
	@read -p "Select LCD support (1-5)?" LCD; \
	LCD=$${LCD}; \
	case "$$LCD" in \
		1) echo "LCD=" >> config.local;; \
		2) echo "LCD=4-digits" >> config.local;; \
		3) echo "LCD=vfd" >> config.local;; \
		4) echo "LCD=lcd" >> config.local;; \
		5) echo "LCD=tftlcd" >> config.local;; \
		*) echo "LCD=" >> config.local;; \
	esac; \
	echo ""
# graphlcd
	@echo -e "\GraphLCD spport ?:"
	@echo "   1)  yes"
	@echo -e "   \033[01;32m2)  No\033[00m"
	@read -p "Select NGLCD support (1-2)?" NGLCD; \
	NGLCD=$${NGLCD}; \
	case "$$NGLCD" in \
		1) echo "NGLCD=nglcd" >> config.local;; \
		2|*) echo "NGLCD=" >> config.local;; \
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
#	
	@echo ""
	@make printenv		

init-clean:
	rm -f config.local
	
-include config.local

# opengl
ifeq ($(OPENGL), opengl)
N2_OPTS += --enable-opengl
endif

# lirc
ifeq ($(LIRC), lirc)
N2_OPTS += --enable-lirc
endif

# MEDIAFW
MEDIAFW ?= buildinplayer

ifeq ($(MEDIAFW), gstreamer)
N2_OPTS += --enable-gstreamer --with-gstversion=1.0
endif

# OVERLAY
ifeq ($(OVERLAY), overlay)
N2_OPTS += --enable-overlay
endif

# python
ifeq ($(PYTHON), python)
N2_OPTS += --enable-python
endif

# lua
LUA ?= lua

ifeq ($(LUA), lua)
N2_OPTS += --enable-lua
endif

# CICAM
CICAM ?= cicam

ifeq ($(CICAM), cicam)
N2_OPTS += --enable-ci
endif

# SCART
ifeq ($(SCART), scart)
N2_OPTS += --enable-scart
endif

# LCD
ifeq ($(LCD), vfd)
N2_OPTS += --enable-vfd
endif
 
ifeq ($(LCD), lcd)
N2_OPTS += --enable-lcd
endif

ifeq ($(LCD), 4-digits)
N2_OPTS += --enable-4digits
endif

ifeq ($(LCD), tftlcd)
N2_OPTS += --enable-tftlcd
endif

# NGLCD
ifeq ($(NGLCD), nglcd)
N2_OPTS += --enable-graphlcd
endif

# FKEYS
ifeq ($(FKEYS), fkeys)
N2_OPTS += --enable-functionkeys
endif

# test plugins
ifeq ($(TESTING), testing)
N2_OPTS += --enable-testing
endif

#
#
#
printenv:
	@echo
	@echo '================================================================================'
	@echo "OPENGL			: $(OPENGL)"
	@echo "LIRC			: $(LIRC)"
	@echo "MEDIAFW			: $(MEDIAFW)"
	@echo "PYTHON			: $(PYTHON)"
	@echo "LUA			: $(LUA)"
	@echo "CICAM			: $(CICAM)"
	@echo "SCART			: $(SCART)"
	@echo "LCD			: $(LCD)"
	@echo "NGLCD			: $(NGLCD)"
	@echo "FKEYS			: $(FKEYS)"
	@echo "TESTING			: $(TESTING)"
	@echo '================================================================================'

#
#
#
help:
	@echo "main target:"
	@echo " make init			- setup build options"
	@echo " make init-clean                 - reset build options"
	@echo " make printenv			- show build options"
	@echo " make 				- build neutrino2 / plugins"
	@echo " make run			- start neutrino2 / neutrino2 plugins"
	@echo " make run-gdb			- start neutrino2 GDB"
	@echo " make run-valgrind		- start neutrino2 with valgrind"
	@echo " make package			- build debian package"
	@echo		

#
# neutrino2
#
neutrino2: $(N2_SRC)/config.status
	$(MAKE) -C $(N2_SRC) install

$(N2_SRC)/config.status: | $(N2_SRC) $(DEST)
	$(N2_SRC)/autogen.sh
	set -e; cd $(N2_SRC); \
		$(N2_SRC)/configure \
			--prefix=$(DEST) \
			--build=i686-pc-linux-gnu \
			--enable-silent-rules \
			--enable-maintainer-mode \
			$(N2_OPTS)
			
$(DEST):
	mkdir $@

neutrino2-clean:
	$(MAKE) -C $(N2_SRC) clean

neutrino2-distclean:
	$(MAKE) -C $(N2_SRC) distclean
	rm -f $(N2_SRC)/config.status

#
# plugins
#
plugins: $(PLUGINS_SRC)/config.status $(N2_SRC)/config.status
	$(MAKE) -C $(PLUGINS_SRC) install

$(PLUGINS_SRC)/config.status: $(PLUGINS_SRC) $(DEST)
	$(PLUGINS_SRC)/autogen.sh
	set -e; cd $(PLUGINS_SRC); \
		$(PLUGINS_SRC)/configure \
			--prefix=$(DEST)  \
			--build=i686-pc-linux-gnu \
			--enable-silent-rules \
			--enable-maintainer-mode \
			$(N2_OPTS)

plugins-clean:
	$(MAKE) -C $(PLUGINS_SRC) clean

plugins-distclean:
	$(MAKE) -C $(PLUGINS_SRC) distclean
	rm -f $(PLUGINS)/config.status

update:
	git stash && git stash show -p > ./pull-stash-neutrino2.patch || true && git pull || true;

clean: neutrino2-clean plugins-clean
distclean: neutrino2-distclean plugins-distclean

#
# debian package
#
neutrino2-deb: | $(N2_SRC) $(PWD)/debian
	$(N2_SRC)/autogen.sh
	set -e; cd $(N2_SRC); \
		$(N2_SRC)/configure \
			--prefix=/usr \
			--build=i686-pc-linux-gnu \
			--enable-silent-rules \
			--enable-maintainer-mode \
			$(N2_OPTS)
	$(MAKE) -C $(N2_SRC) install DESTDIR=$(PWD)/debian

plugins-deb: $(PLUGINS_SRC) $(PWD)/debian
	$(PLUGINS_SRC)/autogen.sh
	set -e; cd $(PLUGINS_SRC); \
		$(PLUGINS_SRC)/configure \
			--prefix=/usr  \
			--build=i686-pc-linux-gnu \
			--enable-silent-rules \
			--enable-maintainer-mode \
			$(N2_OPTS)
	$(MAKE) -C $(PLUGINS_SRC) install DESTDIR=$(PWD)/debian

package: neutrino2-deb plugins-deb
	dpkg --build debian neutrinong_`sed -n 's/\#define PACKAGE_VERSION "//p' neutrino2/config.h | sed 's/"//'`_`git log | grep "^commit" | wc -l`_all.deb
	
package-clean:
	rm -rf neutrinong_*

PHONY = clean distclean
.PHONY: $(PHONY)

