
<h1 align="center">
  Welcome to neutrino2 software
</h1>

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
```

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
 debian 8 Jessie, 9 Stretch, 11 Bullseye and 12 Bookworm
 linuxmint 20.1 Ulyssa, 20.2 Uma, 20.3 Una, 21.1 Vera and LMDE 5 Elsie
 Ubuntu 20.04 Focal Fossa, 22.04 Jammy Jellyfish and 23.04 lunar lobster

## how to build neutrino2 for sh4/arm/mips boxes ##
see:
* https://github.com/mohousch/buildsystem






