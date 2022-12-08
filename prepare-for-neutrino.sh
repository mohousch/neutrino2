#!/bin/bash

# originally created by schischu and konfetti
# fedora parts prepared by lareq
# fedora/suse/ubuntu scripts merged by kire pudsje (kpc)

if [[ $EUID -ne 0 ]]; then
	echo "This script must be run as root (sudo $0)" 1>&2
	exit 1
fi

# make sure defines have not already been defined
UBUNTU=
FEDORA=
SUSE=
GENTOO=

# Try to detect the distribution
if `which lsb_release > /dev/null 2>&1`; then 
	case `lsb_release -s -i` in
		Debian*) UBUNTU=1; INSTALL="apt-get -y install";;
		Fedora*) FEDORA=1; INSTALL="yum install -y";;
		CentOS*) FEDORA=1; INSTALL="yum install -y";;
		SUSE*)   SUSE=1;   INSTALL="zypper install -y";;
		Ubuntu*) UBUNTU=1; INSTALL="apt-get -y install";;
		Linuxm*) UBUNTU=2; INSTALL="apt-get install";;
		Gentoo)  GENTOO=1; INSTALL="emerge -uN";;
	esac
fi

# Not detected by lsb_release, try release files
if [ -z "$FEDORA$GENTOO$SUSE$UBUNTU" ]; then
	if   [ -f /etc/redhat-release ]; then FEDORA=1; INSTALL="yum install -y"; 
	elif [ -f /etc/fedora-release ]; then FEDORA=1; INSTALL="yum install -y"; 
	elif [ -f /etc/centos-release ]; then FEDORA=1; INSTALL="yum install -y"; 
	elif [ -f /etc/SuSE-release ];   then SUSE=1;   INSTALL="zypper install -n";
	elif [ -f /etc/debian_version ]; then UBUNTU=1; INSTALL="apt-get --force-yes install";
	elif [ -f /etc/gentoo-release ]; then GENTOO=1; INSTALL="emerge -uN"
	fi
fi

# still not detected, display error and let the user manually install
if [ -z "$FEDORA$GENTOO$SUSE$UBUNTU" ]; then
	echo
	echo "Cannot determine which OS distribution you use," 
	echo "or your distribution is not (yet) supported." 
	echo "Please report this fact in the proper forum(s)"
	echo
	echo "Try installing the following packages: "
	# determine probable distribution, based on package system, 
	# Suse should be last because the others may also have rpm installed.
	{ `which apt-get > /dev/null 2>&1` && UBUNTU=1; } || \
	{ `which yum     > /dev/null 2>&1` && FEDORA=1; } || \
	{ `which dnf     > /dev/null 2>&1` && FEDORA=1; } || \
	{ `which emerge  > /dev/null 2>&1` && GENTOO=1; } || \
	SUSE=2
	INSTALL="echo "
fi

if [ "$FEDORA" == 1 ]; then
	FEDORA_VERSION=`cat /etc/fedora-release | cut -d ' ' -f3`
	if [[ "$FEDORA_VERSION" -gt "21" ]]; then
		INSTALL="dnf install -y --best --allowerasing"
	fi
fi

if [ "$SUSE" == 1 ]; then
	SUSE_VERSION=`cat /etc/SuSE-release | line | awk '{ print $2 }'`
	zypper ar "http://download.opensuse.org/repositories/openSUSE:/$SUSE_VERSION:/Live/standard/" fakeroot
	zypper ref
fi

PACKAGES="\
	make \
	subversion \
	ccache \
	flex \
	bison \
	texinfo \
	intltool \
	libtool \
	swig \
	dialog \
	wget \
	cmake \
	gperf \
	\
	${UBUNTU:+rpm}                                               ${FEDORA:+rpm-build}      ${GENTOO:+rpm}         \
	${UBUNTU:+lsb-release}          ${SUSE:+lsb-release}         ${FEDORA:+redhat-lsb}     ${GENTOO:+lsb-release} \
	${UBUNTU:+git}             ${SUSE:+git-core}            ${FEDORA:+git}            ${GENTOO:+git}         \
	${UBUNTU:+libncurses5-dev}      ${SUSE:+ncurses-devel}       ${FEDORA:+ncurses-devel}  ${GENTOO:+ncurses}     \
	${UBUNTU:+libncursesw5-dev}                                                                                   \
	${UBUNTU:+gettext}              ${SUSE:+gettext-devel}       ${FEDORA:+gettext-devel}  ${GENTOO:+gettext}     \
	${UBUNTU:+zlib1g-dev}           ${SUSE:+zlib-devel}          ${FEDORA:+zlib-devel}     ${GENTOO:+zlib}        \
	${UBUNTU:+g++}                  ${SUSE:+gcc gcc-c++}         ${FEDORA:+gcc-c++}                               \
	${UBUNTU:+libffi-dev}           ${SUSE:+libffi-devel}        ${FEDORA:+libffi-devel}   ${GENTOO:+libffi}      \
	${UBUNTU:+automake}             ${SUSE:+automake make}                                 ${GENTOO:+automake}    \
	${UBUNTU:+xfslibs-dev}          ${SUSE:+xfsprogs-devel}                                ${GENTOO:+xfsprogs}    \
	${UBUNTU:+pkg-config}           ${SUSE:+pkg-config}                                    ${GENTOO:+pkg-config}  \
	${UBUNTU:+patch}                ${SUSE:+patch}                                         ${GENTOO:+patch}       \
	${UBUNTU:+autopoint}                                                                                          \
	${UBUNTU:+fakeroot}                                                                    ${GENTOO:+fakeroot}    \
	${UBUNTU:+gawk}                                                                        ${GENTOO:+gawk}        \
	${UBUNTU:+libglib2.0-bin}       ${SUSE:+glib2-devel}         ${FEDORA:+glib2-devel}    ${GENTOO:+glib:2}      \
	${UBUNTU:+libglib2.0-dev}       ${SUSE:+glibc-devel-static}  ${FEDORA:+glibc-static}                          \
	${UBUNTU:+doc-base}             ${SUSE:+libuuid-devel}       ${FEDORA:+libuuid-devel}                         \
	${UBUNTU:+texi2html}                                                                                          \
	${UBUNTU:+help2man}                                                                                           \
	${UBUNTU:+libcurl4-openssl-dev} ${SUSE:+libcurl-devel}       ${FEDORA:+libcurl-devel}                         \
	${UBUNTU:+liblzo2-dev}          ${SUSE:+lzo-devel}           ${FEDORA:+lzo-devel}      ${GENTOO:+lzo:2}       \
	${UBUNTU:+ruby}                                                                        ${GENTOO:+ruby}        \
	${UBUNTU:+libltdl-dev}                                       ${FEDORA:+libtool-ltdl-devel}                    \
	                                                             ${FEDORA:+byacc}                                 \
	${UBUNTU:+libssl-dev}           ${SUSE:+libopenssl-devel}    ${FEDORA:+openssl-devel}                         \
	${UBUNTU:+libmount-dev}                                                                                       \
	${UBUNTU:+mtools}                                                                                             \
	${UBUNTU:+u-boot-tools}												\
	${UBUNTU:+android-sdk-libsparse-utils}										\
	${UBUNTU:+lua5.2}                                                                                       	\
	${UBUNTU:+lua5.2-dev}                                                                                       	\
    	${UBUNTU:+python2-dev}                                                                                       	\
    	${UBUNTU:+libfreetype6-dev}											\
    	${UBUNTU:+libid3tag0-dev}											\
    	${UBUNTU:+libmad0-dev}												\
    	${UBUNTU:+libavformat-dev}											\
    	${UBUNTU:+libfribidi-dev}											\
    	${UBUNTU:+libogg-dev}												\
    	${UBUNTU:+libpng-dev}												\
    	${UBUNTU:+libgif-dev}												\
    	${UBUNTU:+libjpeg-dev}												\
    	${UBUNTU:+libflac-dev}												\
    	${UBUNTU:+libvorbis-dev}											\
    	${UBUNTU:+libopenthreads-dev}											\
    	${UBUNTU:+libblkid-dev}												\
    	${UBUNTU:+libgstreamer1.0-dev}											\
    	${UBUNTU:+libgstreamer-plugins-base1.0-dev}									\
    	${UBUNTU:+libgstreamer-plugins-bad1.0-dev}									\
    	${UBUNTU:+lua-json}												\
    	${UBUNTU:+lua-expat}												\
    	${UBUNTU:+lua-posix}												\
    	${UBUNTU:+lua-socket}												\
    	${UBUNTU:+lua-soap}												\
    	${UBUNTU:+lua-curl}												\
    	${UBUNTU:+libass-dev}												\
    	${UBUNTU:+libglew-dev}												\
    	${UBUNTU:+freeglut3-dev}											\
    	";

if [ "$UBUNTU" == 1 ]; then
	UBUNTU_VERSION=`lsb_release -r | grep "Release" | cut -f2 | cut -d . -f1`
elif [ "$UBUNTU" == 2 ]; then
	MINT_VERSION=`lsb_release -r | grep "Release" | cut -f2 | cut -d . -f1`
fi
if ([ "$UBUNTU" == 1  ] &&  [ "$UBUNTU_VERSION" -ge "11" ]) || ([ "$UBUNTU" == 2 ] && [ "$MINT_VERSION" -ge "5" ]); then
	PACKAGES="$PACKAGES \
	${UBUNTU:+libtool-bin} \
	";
fi
if ([ "$UBUNTU" == 1  ] &&  [ "$UBUNTU_VERSION" -ge "22" ]) || ([ "$UBUNTU" == 2 ] && [ "$MINT_VERSION" -ge "21" ]); then
	PACKAGES="$PACKAGES \
	${UBUNTU:+gtk-doc-tools} \
	";
fi

if [ `which arch > /dev/null 2>&1 && arch || uname -m` == x86_64 ]; then
	# ST changed to the -m32 option for their gcc compiler build
	# we might need to install more 32bit versions of some packages
	PACKAGES="$PACKAGES \
	${UBUNTU:+gcc-multilib}         ${SUSE:+gcc-32bit}              ${FEDORA:+libstdc++-devel.i686}               \
	${UBUNTU:+binutils-multiarch}   ${SUSE:+glibc-devel-32bit}      ${FEDORA:+glibc-devel.i686}                   \
	${UBUNTU:+lib32z1-dev}          ${SUSE:+zlib-devel-32bit}       ${FEDORA:+libgcc.i686}                        \
	                                                                ${FEDORA:+ncurses-devel.i686}                 \
	                                                                ${FEDORA:+redhat-lsb.i686}                    \
	                                ${SUSE:+libstdc++-devel-32bit}  ${FEDORA:+glibc-static.i686}                  \
	";
fi
echo $INSTALL $PACKAGES
$INSTALL $PACKAGES

# Link sh to bash instead of dash on Ubuntu (and possibly others)
/bin/sh --version 2>/dev/null | grep bash -s -q
if [ ! "$?" -eq "0" ]; then
	echo
	read -p "/bin/sh should link to /bin/bash, adjust it (Y/n)? "
	if [ -z "$REPLY" -o "$REPLY" == "Y" -o "$REPLY" == "y" ]; then
		rm -f /bin/sh
		ln -s /bin/bash /bin/sh
	fi
fi

# for user mknod
#chmod +s /bin/mknod

