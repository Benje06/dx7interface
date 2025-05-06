- [Introduction](#introduction)
- [Description](#about)
- [HOWTO](#howto)
	- [Windows](#windows)
	- [Linux](#linux)
	- [Sources](#build-it-from-sources)
- [History](#history)
- [Thanks](#thanks)
- [References](#references)
- [Licenses](#licenses)
- [Authors and Contributors](#authors-and-contributors)

# Introduction
**Dx7interface** is a Complete **GUI to drive** the Yamaha **Dx7** synthetizer \
and it's derivated such as **Tx816 Tx216** as well as their **emulators** as long as they accept **SysEx messaging**\
and manage **sounds banks**.

# Description
Dx7interface is a **graphical interfaces** to edit sounds banks and drive the **Dx7** / **Tx216** / **Tx816** physical synth as well as their **emulators**.\
It use **SysEx messages** and it's based on **alsa** sequencer (for **Linux**) and **rtmidi** + loopMIDI (for **Windows**).\
The interface is entirely driveable by **Control Change** (CC) and **Program Change** (PC).\
It has a **MIDI Learn** function for this, with the **load** and **save** of the configuration from/to file.\
ADSR and Pitch **curves** are editable by **mouse controls**.\
You can **start** it with a **specified color** to quickly **identify** it.

It supports:
- **controller parameters**:
	- by sound in **TF1 mode** (Tx816/216)
	- by bank in **native mode** of the Dx7.
- **1**, **32** and up to **128** sounds by bank.
- to **edit sounds banks** by menu functions (Insert/Replace/Delete).
- to **load** or **save** sounds or bank from/to **Raw** file (no sysex headers) or **SysEx** file,\
in **Bulk 1** and **Bulk 32** formats.
- to **Compare**, **Restore** sound/bank and **Send** Bank.\
the send sound is done when you select a sound.
- **independant channels** for the MIDI **Input** and **Output**.
- an **indicator** on the list of sound for **modified sounds**.
- **Midi panic** function.
- to **log** in **file** and **console**.

It is based on **C++** using **GLIB** (glibmm-2.68) and **GTK 4.0** (gtkmm-4.0)\
with **Cairo** for the drawable (cairomm 1.16) and **Pango** for the font support (pangomm 2.68).\
It can be **customised** as long as you **preserving** the **object type** and there **name** as it's build on:
- an **XML** for the **UI**
- a **CSS** file for the **theme**
- and **Custom font** load support

As **Dx7interface** is a **plugin/module** (Glib::Gmodule) it rely on **GxInterface** as it's **module loader**.\
It is a **derivated** from:
- **GxModule**, which come from Gxinterface
- **Synth** which hold all the Midi functions

**GxInterface/Gxmodule** provides it's **base functions** such as:
- **Create/Load** Modules
- **Present** Dialogs/Windows for Load/Save/Messages
- **Read/Write** by Bytes using DataStream
- **Gettings** Ui widget
- **Setting Style** (Font/Theme)
- **Command line** options
- **Log** Manager
- **Threads**

**Dx7interface** provides:
- the definition of the sysex format for the Dx7 and Tx816/Tx216
- the definition of the messages
- the definition of read parsing and write structures
- the parsing itself
- the link between the ui and functions using callback and events
- the sounds bank management
- the draw function (have to be moove somewhere else to be reusable)
- the midi learn function (have to be mooved somewhere else to be reusable)


# HOWTO
## Windows
The windows version doesn't implement Midi driver, for this it rely on [loopMidi](https://www.tobias-erichsen.de/software/loopmidi.html) by Tobias Erichsen.\
After loopMidi is installed, start it and create **2 ports** named **Dx7interface_in** and **Dx7interface_out**.\
At start Dx7interface will automatically connect to these ports.\
You'll have to set in advanced page, the maximum sysex size to 4096 kiloBytes at least, as 32 sounds bank can be send.\
Once started any software that support Direct Music will see it.

Windows 10 version download:
- Installer [Dx7interface without console](https://github.com/Benje06/dx7interface/blob/gtk4/Dx7interface.exe?raw=true)

- Installer [Dx7interface with console](https://github.com/Benje06/dx7interface/blob/gtk4/Dx7interface_debug_console.exe?raw=true)

![dx7interface avec console de debug](https://github.com/user-attachments/assets/78956483-d185-4516-9482-534118454dd0)

## Linux
**Supported Distribution**:
- **Archlinux** / **Manjaro**: PKGBUILD for
[Gxinterface](https://github.com/Benje06/dx7interface/blob/gtk4/gxinterface/PKGBUILD?raw=true) and
[Dx7interface](https://github.com/Benje06/dx7interface/blob/gtk4dx7interface/PKGBUILD?raw=true)
- **Debian 12** / **Ubuntu 24.04**:
using [Checkinstall](#debian--ubuntu)

**Run**:\
Once installed you can run it by menu or start it from terminal as:\
<code>$ gxinterface -m /usr/share/dx7interface/1.0.0/dx7interface-1.0.0.so</code>

<u>Gxinterface / Dx7interface start options are</u>:\
**Colorise section title**\
<code>-c "html_color"</code> it colorise title section with the specified color to identify it visually.\
Uses when you start multiple instance of the program.
- <code>gxinterface -c "red" -m /usr/share/dx7interface/1.0.0/dx7interface-1.0.0.so</code>
- <code>gxinterface -c "#e15a46" -m /usr/share/dx7interface/1.0.0/dx7interface-1.0.0.so</code>

**Log level**\
<code>-l [0|1|2]</code> to specify le log level.\
<code>0=no log; 1=log to file; 2=log to file and console</code>
- <code>gxinterface -l 0 -m /usr/share/dx7interface/1.0.0/dx7interface-1.0.0.so</code>
- <code>gxinterface -l 2 -m /usr/share/dx7interface/1.0.0/dx7interface-1.0.0.so</code>

**Load an element**\
<code>-m filename.[ui|so|la|dll]</code> \
<code>-m *.so</code> it's the common way to load a module, alternatively you can load the .la too\
<code>-m *.dll</code> is use for windows version\
<code>-m *.ui</code> is to load an xml file as it's main interface but no code is attached to this.\
It's use to see modification on the an ui file. You will never use it until you made your own interface or a module.


## Build it from Sources

You need: 
- **Common** DEV:
	- **base-devel** (Arch: 1-2 , UCRT64: >=2024.11-1) or build-essential >= 12.9 ( gcc make ... )
	- GNU autotool: **autogen** >= 5.18 / **autoconf** >= 2.71 / **automake** >= 1.16
	- **intltool** >= 0.51 and **gettext** >= 0.21 
	- **glib-gettextize** => 2.74:
		- Archlinux: glib2 >= 2.82.4
		- Debian: libglib2.0-dev
		- Windows: ucrt64/mingw-w64-ucrt-x86_64-glib2
	- **libtool** >= 2.4.7
	- **aclocal** >= 1.16
		- Archlinux: included in automake
		- Debian: included in automake
		- Windows: msys/automake-wrapper 20240607-1
	- **m4** >= 1.4.19
	- **gtkmm-4.0** >= 4.8.0 / **glibmm-2.68** >= 2.68     (lib...-dev for Debian like)
	- **cairomm-1.16** >= 1.16 / **pangomm-2.48** >= 2.48  (lib...-dev for Debian like)
	- **libsigc++-3.0** >= 3.4.0  					   (lib...-dev for Debian like)
	- **pthread**:
		- Windows ucrt64: ucrt64/mingw-w64-ucrt-x86_64-winpthreads-git >=  12.0.0.r679
		- Debian: included in glibc (optionnal: libpthread-stubs0-dev >= 0.4.1)
		- Archlinux: glibc >= 2.41
	- **Packager**:
		- Archlinux: **makepkg** git (optional: devtools)
		- Manjaro:   manjaro-tools-base manjaro-tools-pkg
		- Debian/Ubuntu:  **checkinstall** >= 1.6.2
		- Windows: **NSIS** mingw-w64-ucrt-x86_64-nsis >= 3.11.1
- **Dx7interface** specifc:
	- **gxinterface** >= 1.0.0
	- **MIDI**:
		- Archlinux: **alsa-lib** >= 1.2.14
		- Debian/Ubuntu: **libasound2-dev** >= 1.2.8
		- Windows: **RtMidi** mingw-w64-ucrt-x86_64-rtmidi >= 6.0.0-3

### Devellopement environment</u>:
- <u>Kdevelop for Linux</u>:\
Files provides env variables (kdev4 global) and launcher (kdev4 specific)\
 for common actions those strating with D or DEBUG are flagged for debug with gdb inside kdevelop
 for both project gxinterface and dx7interface.\
[kdev4 global]()\
<u>You have to change path in all .kdev4 files</u>:
	- "file:///home/jerome/dev/git/gtk4" to your git clone directory
	- "/home/jerome/dev/build/" to your archlinux makepkg build directory
	- gxinterface: 
[kdev4 specific](https://github.com/Benje06/dx7interface/blob/gtk4/gxinterface/.kdev4/gxinterface.kdev4)
	- dx7interface:
[kdev4 specific](https://github.com/Benje06/dx7interface/blob/gtk4/dx7interface/.kdev4/dx7interface.kdev4)

- <u>VS Code for windows build with ucrt64</u>:\
To build it under windows you need [MSYS2/UCRT64](https://www.msys2.org/])\
You can get [the installer](https://www.msys2.org/docs/installer/) and [documentation](https://www.msys2.org/docs/what-is-msys2/]) for MSYS2/UCRT64\
I do a list of the package installed on UCRT64 to build the windows package.\
This list [UCRT64 package list](https://github.com/Benje06/dx7interface/blob/gtk4/ucrt64_pkg_list.txt) include more than necessary to build the package take it as an information.\
After cloning the repo, in files of [.vscode directory](https://github.com/Benje06/dx7interface/blob/gtk4/.vscode), change path (D:\\crosscompile\\msys2) to your msys2 install directory and load the root git clone directory in vscode.

### <u>BY HAND:</u>
**It is recommanded to go through makepkg or checkinstall**\
There is a [**PKGBUILD-gx**](https://github.com/Benje06/dx7interface/blob/gtk4/gxinterface/PKGBUILD-gx) and [**PKGBUILD-dx**](https://github.com/Benje06/dx7interface/blob/gtk4/dx7interface/PKGBUILD-dx) that use your local git directory to build


#### BUILDING IT
<code>$ cd to_git_directory/[gxinterface|dx7interface]/</code>
##### RESET SOURCES
<code>$ make clean || true</code>\
<code>$ make discleanclean || true</code>
##### AUTOGEN
- for gxinterface:\
<code>$ ./autogen.sh --prefix=/usr --enable-log=2 --enable-console=0 --disable-maintainer-mode</code>
- for dx7interface:\
<code>$ ./autogen.sh --prefix=/usr --enable-log=2 --enable-console=0 --disable-maintainer-mode --enable-alsa</code>

"--enable-console=1" enable for windows the start of the application in a coonsole.\
"--enable-log=2" set the log precision : 0=no print, 1= print function name 2=function name + line + file.\
##### MAKE
<code>$ make -j3</code>

#### INSTALL
##### <u>By Make install</u>:
you can run both gxinterface and dx7interface without need of installation, but dx7interface need libs and headers from gxinterface to build.\
	<code>$ make install </code>
##### <u>By packager</u>:
###### - **Archlinux** / **Manjaro**:
- <u>for gxinterface</u>:\
	<code>$ cd to_build_directory</code>\
	<code>$ cp from_git_directory/gxinterface/PKGBUILD PKGBUILD-GX</code>\
	<code>makepkg -sfip PKGBUILD-GX</code>
- <u>for dx7interface:</u>\
	<code>$ cd to_build_directory</code>\
	<code>$ cp from_git_directory/dx7interface/PKGBUILD PKGBUILD-DX</code>\
	<code>$ makepkg -sfip PKGBUILD-DX</code>
###### - <u>**Debian** / **Ubuntu**:</u>
- <u>for gxinterface</u>:\
	<code>$ cd to_git_clone_directory/gxinterface</code>\
	<code>$ sudo checkinstall -D --fstrans=no --install=yes --pkgversion="1.0.0" -y</code>
- <u>for Dx7interface</u>:\
	<code>$ cd cd to_git_clone_directory/dx7interface</code>\
	<code>$ sudo checkinstall -D --fstrans=no --install=yes --pkgversion="1.0.0" -y</code>


#### RUN from source
- **Gxinterface** alone:\
	<code>$ cd to_git_directory/gxinterface/src</code>\
	<code>$ ./gxinterface</code>
- **Dx7interface**:\
	<code>cd to_git_directory/dx7interface/src</code>

	<code>$ ../../gxinterface/src/gxinterface -l 2 -m src/.libs/dx7interface-0.0.1.so</code>\
	or if gxinterface is installed\
	<code>$ gxinterface -l 2 -m src/.libs/dx7interface-0.0.1.so </code>

# History
This project was start in 2006, to made an interface for the configuration of Linux system, starting by the network it was called [GNetAdm](https://sourceforge.net/projects/gnetadm/) was made in C using glade and gtk2. The first goal was to load modules with customisable interface for each kind of "services" to configure them. Going through C with multithreading, modules and interface, all this dynamically was painfull so i moove it to C++.\
This core give birth of Gxinterface, the module loader. In same time a musician friend Jean-Michel, user of a Dx7 Synth, pain to find how to simply edit the sound bank his synth. So we go through the idea to made an interface for it using gnetadm. It give birth of Dx7interface. We do it under c++ with gtk2 and glade.\
It end in 2011 with a working version and a non working version for standalone mode (start dx7interface detached from gxinterface), it was hosted on [Savannah](https://savannah.nongnu.org/bzr/?group=gxinterface) using bzr and was doing basic stuff such as drive the synth with the ui.\
[Hexter](https://github.com/theabolton/hexter) is, from this time, the reference emulator for this synth but the editor inside was the retro mode version, so we decide as it accept SysEx to drive it and as it can write files it was enough for us to get something functionnal.\
At the start of 2025, i reopen the code and try to run it, but gtk2 and all was so pretty deprecated, so i decided to upgrade it to gtk3, at first to see how paintfull it could be, with the goal of gtk4.\
Supprissingly it was not so, which made me thing the core and concept was good enough. So after mooving it to gtk3 ([last bzr commit](https://bzr.savannah.gnu.org/lh/gxinterface/changes/105?start_revid=105)), we moove the repository to the git version and add a new [branch gtk4](https://git.savannah.nongnu.org/cgit/gxinterface.git).\
We adding stuff step by step, like drawable, mouse editable, saving file, midi learn, compare, edit menu. In parallele we correct bugs, and update core, we moove generic function to the core, put the midi part in a separate class, trying to more specialize class, and detach UI stuff from functionnality as much as we can.\
And finnaly moove all the code here. ^^ \
We hope you will enjoy this soft and his functionnality, to drive this mythics Synth from the 80's.\

# Thanks
A big thanks to:
- [Thea Bolton](https://github.com/theabolton) for have done the amazing [Hexter](https://github.com/theabolton/hexter).
- [Dave Benson](https://homepages.abdn.ac.uk/d.j.benson/pages/index.html) for his [reference pages of the physical synth](https://homepages.abdn.ac.uk/d.j.benson/pages/html/dx7.html)
- [Tim Conrardy](http://tim-conrardy.last-memories.com/) for his Dx7 atari page [tim's atari world](https://web.archive.org/web/20160308015913/http://tamw.atari-users.net/dx7.htm)

# References
- presentation\
[Yamaha design Dx7](https://www.yamaha.com/en/tech-design/design/synapses/id_009)

- manuals\
[Yamaha Dx7 manuels (fr)](https://fr.yamaha.com/fr/support/manuals/index.html?l=fr&c=&k=dx7)\
[Yamaha Dx7 manuals (en)](https://uk.yamaha.com/en/support/manuals/index.html?l=en&c=&k=dx7)\
[Yamaha Tx816 Tx216 manuels (fr)](https://fr.yamaha.com/fr/products/contents/music_production/downloads/manuals/index.html?l=en&c=music_production&k=Tx816)\
[Yamaha Tx816 Tx216 manuels (en)](https://usa.yamaha.com/products/contents/music_production/downloads/manuals/index.html?l=en&c=music_production&k=tx816)
- books\
The complete Dx7 by Howard Massey.
- historic\
[Yamaha Chapitre 2 la synthese fm (fr)](https://fr.yamaha.com/fr/products/contents/music_production/synth_40th/history/chapter02/index.html)

# Licenses
## Gxinterface and Dx7interface
Copyright (C) 2010-2025 under [GPL-v3](https://www.gnu.org/licenses/gpl-3.0.html)\
except for font and images see below.
## Images and Font
- All Images (.png, .ico .svg) are under [Creative Common CC-By-SA](http://creativecommons.org/licenses/by-sa/3.0/)
	- by Jean-Michel Thiémonge and Jérome Benhaïm:
		- "icon_dx7interface.svg","dx7interface.png", "dx7interface.ico"
		- "32_algo_dx732_algo_dx7_150*105px.svg", "algo[1-32].png"
		- "lfo_wave_142*48px.svg", "SAW+.png", "SAW-.png", "SIN.png", "SQU.png", "S_HOLD.png", "TRI.png"
		- "PianoKeyboard.svg", "keyboard.png", "keyboard_background.png", touche_b.png", "touche_w.png"
- Font:
	- "Araster-fonts-6x8.ttf" is under [Creative Common CC-By-SA](http://creativecommons.org/licenses/by-sa/3.0/)
		- by "DOS" see [licence](https://github.com/Benje06/dx7interface/blob/gtk4/dx7interface/data/fonts/license.txt)
# Authors and Contributors
## Authors:
[THIÉMONGE Jean-Michel](https://github.com/jmechmech)\
[BENHAÏM Jérome](https://github.com/Benje06)

## Contributors:
ENNAIME Mirsal thanks for the help at start and the debug.h stuff \
Termitor, thanks for the math computation ^^
