- [Introduction](#introduction)
- [Description](#about)
- [HOWTO](#howto)
	- [Linux](#linux)
	- [Windows](#windows)
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
## Windows
Version Debug console:
- Direct Download link:
 	[Installeur Dx7interface](https://github.com/Benje06/dx7interface/blob/gtk4/Dx7interface_debug_console.exe?raw=true)
  
	![dx7interface avec console de debug](https://github.com/user-attachments/assets/78956483-d185-4516-9482-534118454dd0)

# Licences
GPL-v3
# Contact
