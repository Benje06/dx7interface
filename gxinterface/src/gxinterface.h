/* ----------------------------------------------------------------------------
* gxinterface -- Gtk+ Extended Interface
* GxInterface Header
*
* ----------------------------------------------------------------------------
* copyright © 2006, 2007, 2008, 2009, 2010 Jérôme BENHAÏM <benhaimjerome@gmail.com>
*
* ----------------------------------------------------------------------------
*
*   This file is part of GxInterface.
*
*	 GxInterface is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* ----------------------------------------------------------------------------
*
* Before attempting to install gtkmm 2.4, you might first need to install these other packages.
* libsigc++ 2.0
* GTK+ 2.4
* cairomm
* These dependencies have their own dependencies, including the following applications and libraries:
* pkg-config
* glib
* ATK
* Pango
* cairo
*/
#pragma once
#ifndef Gx_interface_H
	#define Gx_interface_H
	/* app */
	#include "common.h"
	#include "gemod.h"
	/* PROGRAMNAME_UI_DIR */
	#if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
		#define UI_FILE PROGRAMNAME_UI_DIR"interface.ui"
	#else
		#define UI_FILE PROGRAMNAME_UI_DIR"interface_4.8.ui"
	#endif
	#define APPLICATION_NAME "gxinterface"

class Gx_interface: public Gtk::Application {
	private:
		/* Application variables */
		Glib::ustring itype;						/* interface type */
		Glib::ustring iname;						/* interface name */
		std::string err_msg, msg;
		Gemod* module_manager = nullptr;			/* gestionnaire de module */
		int argc;
        char** argv;
		Gx_interface();
		~Gx_interface();
		/* Override of Gtk::Application */
		void on_activate() override;
        int on_command_line( const Glib::RefPtr<Gio::ApplicationCommandLine>& ) override;
	public:
		static Glib::RefPtr<Gx_interface> create();
};	

#endif /* Gx_interface_H */
