- [Introduction](#introduction)
- [Description](#about)
- [HOWTO](#howto)
	- [Linux](#linux)
	- [Windows](#windows)
	- [Sources](#sources)
- [Licences](#license)
- [Contact](#contact)

# Introduction
**Dx7interface** is a Complete **GUI to drive** the Yamaha **Dx7** synthetizer \
and it's derivated such as **Tx816 Tx216** as well as their **emulators** \
as long as they accept **SysEx messaging**\
and manage **sounds banks**.

# Description
Dx7interface is a **graphical interfaces** to edit sounds banks and drive the **Dx7** / **Tx216** / **Tx816** physical synth as well as their **emulators**.\
It use **SysEx messages** and it's based on **alsa** sequencer (for **Linux**) and **rtmidi** + loopMIDI (for **Windows**).\
The interface is entirely driveable by **Control Change** (CC) and **Program Change** (PC).\
It has a **MIDI Learn** function for this, with the **load** and **save** of the configuration from/to file.\
ADSR and Pitch **curves** are editable by **mouse controls**.\
You can **start** it with a **specified color** to quickly **identify** it.

It support:
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
It is a **derivated** from **GxModule** which come from Gxinterface.\
**GxInterface/Gxmodule** provides it's **base functions** such as: 
- **Create/Load** Modules
- **Present** Dialogs/Windows for Load/Save/Messages
- DataStream for **Read/Write**
- **Gettings** Ui widget
- **Setting Style** (Font/Theme)
- **Command line** options
- **Log** Manager
- **Threads**

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

# Licences
GPL-v3
# Contact
