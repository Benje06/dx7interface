- [Introduction](#introduction)
- [Description](#about)
- [HOWTO](#howto)
	- [Linux](#linux)
	- [Windows](#windows)
	- [Sources](#sources)
- [Licences](#license)
- [Contact](#contact)

# Introduction
Dx7 interface is a Complete GUI to drive the Yamaha Dx7 synthetizer and it's derivated such as Tx816 Tx216 as well as emulators and manage sounds banks.

# Description
Dx7 interface is a **graphical interfaces** to edit sounds banks and drive the **Dx7** / **Tx216** / **Tx816** physical synth as well as their **emulators**.\
It use **SysEx messages** and it's based on alsa sequencer(for **Linux**) and rtmidi + midiloop(for **Windows**).\
The interface is entirely driveable by **Control Change** (CC) and **Program Change** (PC).\
It has a **MIDI Learn** function for this with the load and save of the configuration from/to file.\
ADSR and Pitch curves are modifibale by **mouse controls**.

It support:
- controller parameters by:
	- by sound in TF1 mode(Tx816/216)
	- by bank in native Dx7 mode.
- **1, 32 and up to 128 sounds by bank**.
- to **edit sounds banks** by menu functions (Insert/Replace/Delete).
- to **load or save** sounds or bank from/to **Raw** file(no sysex headers) or **SysEx** file, in **Bulk 1** or **Bulk 32** formats.
- to **Compare**, **Restore** sound/bank and **Send** Bank (the send sound is done when you select a sound).
- **independant channels** for the MIDI Input and Output.
- an indicator on the list of sound for modified sounds.
- Midi panic function.
- to log in file and console.

It is based on C++ of GLIB (glibmm 2.68) and GTK 4.0 (gtkmm 4) with Cairo for the drawable (cairomm 1.16) and Pango for the font support (pangomm 2.68).\
It can be customised as long as you preserving the object type and there name as it's build on:
- an XML for the UI
- a CSS file for the theme
- and custom font load support

As Dx7interface is a plugin/module (Glib::Gmodule) it rely on GxInterface as it's module loader.\
It is a derivated from GxModule which come from Gxinterface.\
GxInterface/Gxmodule provides it's base functions such as: 
- Create/Load Modules
- Present Dialogs/Windows for Load/Save/Messages
- DataStream for Read/Write
- Gettings Ui widget
- Setting Style (Font/Theme)
- Command line options
- Log Manager
- Threads

# HOWTO
## Linux
Supported Distribution:
- Archlinux / Manjaro:
[PKGBUILD Gxinterface](https://github.com/Benje06/dx7interface/blob/gtk4/gxinterface/PKGBUILD?raw=true)
[PKGBUILD Dx7interface](https://github.com/Benje06/dx7interface/blob/gtk4dx7interface/PKGBUILD?raw=true)
- Debian 12 / Ubuntu 24.04:
	using checkinstall

## Windows
Windows 10 version:
- Direct Download link:
[Installeur Dx7interface](https://github.com/Benje06/dx7interface/blob/gtk4/Dx7interface.exe?raw=true)

- Version Debug console:
	- Direct Download link:
[Installeur Dx7interface](https://github.com/Benje06/dx7interface/blob/gtk4/Dx7interface_debug_console.exe?raw=true)

![dx7interface avec console de debug](https://github.com/user-attachments/assets/78956483-d185-4516-9482-534118454dd0)

## Sources

You need: 
- common DEV:
	- base-devel (Arch:  , UCRT64: >=2024.11-1) or build-essential >= 12.9 ( gcc make ... )
	- GNU autotool: autogen >= 5.18 / autoconf >= 2.71 / automake >= 1.16
	- intltool >= 0.51 and gettext >= 0.21 
	- glib-gettextize => 2.74:
		- archlinux: glib2-dev
		- debian: libglib2.0-dev
		- ucrt64: ucrt64/mingw-w64-ucrt-x86_64-glib2
	- libtool >= 2.4.7
	- aclocal >= 1.16
		- Archlinux: 
		- Debian: included in automake
		- Windows: msys/automake-wrapper 20240607-1
	- m4 >= 1.4.19
	- gtkmm-4.0 >= 4.8.0 / glibmm-2.68 >= 2.68 (lib...-dev for Debian like)
	- cairomm-1.16 >= 1.16 / pangomm-2.48 >= 2.48  (lib...-dev for Debian like)
	- libsigc++-3.0 >= 3.4.0  (-dev for Debian like)
	- pthread:
		- Windows ucrt64: ucrt64/mingw-w64-ucrt-x86_64-winpthreads-git >=  12.0.0.r679
		- Debian: included in libc ( optionnel libpthread-stubs0-dev >= 0.4.1 )
		- Archlinux:
	- packager:
		- Archlinux: makepkg devtools git
		- Manjaro:   manjaro-tools-base manjaro-tools-pkg
		- Debian/Ubuntu:  checkinstall >= 1.6.2
		- Windows: NSIS mingw-w64-ucrt-x86_64-nsis >= 3.11.1
- dx7interface:
	- Archlinux: alsa-lib 
	- Debian/Ubuntu: libasound2-dev >= 1.2.8
	- Windows: mingw-w64-ucrt-x86_64-rtmidi >= 6.0.0-3

## Devellopement environment:
- Kdevelop for Linux:

- VS code for windows build with ucrt64:
Change path (D:\\crosscompile\\msys2) in files of .vscode directory with your msys2 directory.

[VSCODE config](https://github.com/Benje06/dx7interface/blob/gtk4/.vscode)

## By hand


# Licences
GPL-v3
# Contact
