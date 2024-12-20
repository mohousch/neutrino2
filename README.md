![License](https://img.shields.io/badge/License-GPL%20v2-blue.svg)

<h1 align="center">
  Welcome to NeutrinoNG software
</h1>

- the GUI is completely reworked.
- i18n localizaton.
- skin support.
- plugins interfaces: legacy, lua and python (exp) NeutrinoNG shares all his functionality with plugins.
  and more...
  
  happy zapping

## How to build NeutrinoNG for PC (x86) ##

```bash
$:~ git clone https://github.com/mohousch/neutrino2.git
```
```bash
$:~ cd neutrino2
```

* check preqs (debian):
```bash
$:~ sudo bash prepare-for-neutrino.sh
```

```bash
$:~ make init
```

```bash
$:~ make
```

* to run neutrino
```bash
$:~ make run
```

* to update NeutrinoNG source code:
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
- debian 8 Jessie, 9 Stretch and 11 Bullseye
- linuxmint 20.1 Ulyssa, 20.2 Uma, 20.3 Una, 21.1 Vera, 21,2 Victoria, LMDE 5 Elsie and LMDE 6 Faye
- Ubuntu 20.04 Focal Fossa, 22.04 Jammy Jellyfish and 23.10 Mantic Minotaur

## how to build neutrino2 for sh4/arm/mips boxes ##
see:
* https://github.com/mohousch/buildsystem






