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

## how to build neutrino2 for sh4/arm/mips boxes ##
see:
* https://github.com/mohousch/buildsystem






