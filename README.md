<h1 align="center">
  Welcome to Neutrino2 software
</h1>

- The GUI is completely reworked.
- I18n localizaton.
- Skin support.
- Plugins interfaces: legacy, lua and python (exp) NeutrinoNG shares all his functionality with plugins.
  and more...
- Still in development...
  
  happy zapping

## How to build Neutrino2 for PC (x86) ##

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

* to update Neutrino2 source code:
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

## How to build neutrino2 for sh4/arm/mips boxes ##
see:
* https://github.com/mohousch/buildsystem






