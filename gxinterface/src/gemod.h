/* ----------------------------------------------------------------------------
* gxinterface -- Gtk+ Extended Interface
* Module and module manager Header
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
*/
#pragma once
#ifndef Gemod_H
	#define Gemod_H
	/* gnome */
	/* app */
	#include "common.h"
	#include "gxmodule.h"

/*
***** Gemod *****
*/
class Gemod : public Gx_module {
	private:
		Gx_module * modules;        // array of modules
		uint8_t nb_mod, max_modules;  // nombre de module chargé et max module
		/* callback function */
		virtual void attach_signals();
		virtual void dettach_signals();
		/* callback functions */
		virtual void on_menu_add_module_event();
		virtual void on_menu_del_module_event();
	protected:
		/* objet specifique a l interface de base */		

		uint8_t get_module_count(Glib::ustring);    // count the number of module provided by same file 
		int8_t get_module_index(Glib::ustring);     // get the index of a module from his name (mod.name)
		Glib::ustring get_module_name(uint8_t);     // get module name from his index
		Gtk::Box* get_module_root(uint8_t);         // get box_main from refxml by modules[] index
		Gtk::Box* get_module_root(Glib::ustring);   // !!! CAUTION !!! get rootbox by module name finding the module index can be empty Gtk::Box	

		bool load(Glib::ustring);
		/* fonctions interface  */
		virtual void on_module_select_event();
		virtual bool add_module(Glib::ustring);
		virtual bool del_module(Glib::ustring);
		/* Menu construction */
		//virtual gboolean add_menu(Glib::ustring, Glib::ustring);
		//virtual gboolean del_menu(Glib::ustring);
		/* submenu constructor 
		* return : created Menu
		* ustring* : array of name(s) of the items in menu
		*/
		//virtual Gtk::Menu * create_submenu(Glib::ustring*);
		/* return : MenuItem dynamically created 
		* ustring : name of the menu in .ui
		*/
		//virtual Gtk::MenuItem * get_menu_dyn(Glib::ustring);
		/* load module 
		* accept .la or .ui
		* return : 1 if good else 0
		* ustring : name of the .la or .ui to load
		*/
	public:
		/* return the main Gtk::Window for gx as interface xml root is type window */
		Gtk::Window* get_main();
		Gtk::Window* get_window();
		/* for gemod as base module manager not as a module 
		* ustring name of the .ui
		*/
		Gemod(Glib::ustring);
		/* for gemod as module (inheritance) 
		* ustring : the module_name, name of the .la
		* guint : index of the module in the module manager
		*/
		Gemod(Glib::ustring,uint8_t);
		/* for gemod as module with specified number of modules 
		* ustring : the module_name, name of the .la
		* guint : index of the module in the module manager
		* guint : number of module in the module
		*/
		Gemod(Glib::ustring,uint8_t,uint8_t);
		/*Gemod();	*/
		virtual ~Gemod();
};
#endif  /* gemod_H */
