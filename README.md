
<h1 align="center">
  Welcome to neutrino2 software
</h1>

-------
![](https://github.com/mohousch/neutrino2/blob/master/neutrino2/doc/resources/mainmenu.png)

![](https://github.com/mohousch/neutrino2/blob/master/neutrino2/doc/resources/channellist.png)

![](https://github.com/mohousch/neutrino2/blob/master/neutrino2/doc/resources/infoviewer.bmp)

![](https://github.com/mohousch/neutrino2/blob/master/neutrino2/doc/resources/epgview.bmp)

![](https://github.com/mohousch/neutrino2/blob/master/neutrino2/doc/resources/eventlist.bmp)

![](https://github.com/mohousch/neutrino2/blob/master/neutrino2/doc/resources/epgplus.bmp)

![](https://github.com/mohousch/neutrino2/blob/master/neutrino2/doc/resources/pluginsbrowser.bmp)

![](https://github.com/mohousch/neutrino2/blob/master/neutrino2/doc/resources/moviebrowser.bmp)

![](https://github.com/mohousch/neutrino2/blob/master/neutrino2/doc/resources/movietrailer.bmp)

![](https://github.com/mohousch/neutrino2/blob/master/neutrino2/doc/resources/movieinfowidget.bmp)

![](https://github.com/mohousch/neutrino2/blob/master/neutrino2/doc/resources/skinselect.bmp)

![](https://github.com/mohousch/neutrino2/blob/master/neutrino2/doc/resources/metrixhd.png)

![](https://github.com/mohousch/neutrino2/blob/master/neutrino2/doc/resources/KravenHD.png)

![](https://github.com/mohousch/neutrino2/blob/master/neutrino2/doc/resources/mainmenu_2.bmp)

## How to build neutrino2 for PC (x86) ##

```bash
$:~ git clone https://github.com/mohousch/neutrino2.git
```
```bash
$:~ cd neutrino2
```

* check preqs (debian):
```bash
$:~ sudo bash prepare-for-neutrino.sh

```bash
$:~ make
```

* to run neutrino
```bash
$:~ make run
```

* to update neutrino2 source code:
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

## how to build neutrino2 for sh4/arm/mips boxes ##
see:
* https://github.com/mohousch/buildsystem






