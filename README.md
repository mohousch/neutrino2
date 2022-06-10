
<h1 align="center">
  Welcome to neutrino-HD2 software
</h1>

-------
![](https://github.com/mohousch/neutrinohd2/blob/master/nhd2-exp/doc/resources/mainmenu.png)

![](https://github.com/mohousch/neutrinohd2/blob/master/nhd2-exp/doc/resources/channellist.png)

![](https://github.com/mohousch/neutrinohd2/blob/master/nhd2-exp/doc/resources/infoviewer.bmp)

![](https://github.com/mohousch/neutrinohd2/blob/master/nhd2-exp/doc/resources/epgview.bmp)

![](https://github.com/mohousch/neutrinohd2/blob/master/nhd2-exp/doc/resources/eventlist.bmp)

![](https://github.com/mohousch/neutrinohd2/blob/master/nhd2-exp/doc/resources/epgplus.bmp)

![](https://github.com/mohousch/neutrinohd2/blob/master/nhd2-exp/doc/resources/pluginsbrowser.bmp)

![](https://github.com/mohousch/neutrinohd2/blob/master/nhd2-exp/doc/resources/moviebrowser.bmp)

![](https://github.com/mohousch/neutrinohd2/blob/master/nhd2-exp/doc/resources/movietrailer.bmp)

![](https://github.com/mohousch/neutrinohd2/blob/master/nhd2-exp/doc/resources/movieinfowidget.bmp)

![](https://github.com/mohousch/neutrinohd2/blob/master/nhd2-exp/doc/resources/skinselect.bmp)

![](https://github.com/mohousch/neutrinohd2/blob/master/nhd2-exp/doc/resources/metrixhd.png)

![](https://github.com/mohousch/neutrinohd2/blob/master/nhd2-exp/doc/resources/KravenHD.png)

![](https://github.com/mohousch/neutrinohd2/blob/master/nhd2-exp/doc/resources/mainmenu_2.bmp)

## How to build neutrino-HD2 for PC (x86) ##

* check preqs (debian):

```bash
$:~ sudo apt-get install autoconf libtool libtool-bin g++ gdb swig flex bison make texinfo subversion intltool dialog wget cmake gperf libglew-dev freeglut3-dev libcurl4-gnutls-dev libfreetype6-dev libid3tag0-dev libmad0-dev libavformat-dev libfribidi-dev libogg-dev libpng-dev libgif-dev libjpeg-dev libflac-dev libvorbis-dev libopenthreads-dev libblkid-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev lua5.2 lua5.2-dev lua-json lua-expat lua-posix lua-socket lua-soap lua-curl python2-dev libao-dev libass-dev
```

```bash
$:~ git clone https://github.com/mohousch/neutrinohd2.git
```
```bash
$:~ cd neutrinohd2
```

```bash
$:~ make
```

* to run neutrino
```bash
$:~ make run
```

* to update neutrinohd2 source code:
```bash
$:~ make update
```

* to clean build:
```bash
$:~ make clean
```

* distclean build:
```bash
$:~ make distclean
```

* tested with:
 debian 8 Jessie, 9 Stretch and 11 Bullseye
 linuxmint 20.1 Ulyssa, 20.2 Uma, 20.3 Una and LMDE 5 Elsie
 Ubuntu 20.04 Focal Fossa

## how to build neutrino-HD2 for sh4/arm/mips boxes ##
see:
* https://github.com/mohousch/buildsystem






