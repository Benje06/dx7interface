/*
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

#ifndef Gxmod_H
	#define Gxmod_H
	/* gnome */
	/* app */
	#include "debug.h"
	#include "common.h"
	#include <glibmm/module.h>
/*
 ***** Gx_Module *****
*/

class Gx_module {
	private:
		struct st_extPath{             /* Provided file informations */
			Glib::ustring ext;
			Glib::ustring name;
			Glib::ustring filename;
			Glib::ustring path;
			Glib::ustring file;
		} ;
		struct st_mod {                 /* Module informations */
			struct st_extPath extpath;
			Glib::ustring name;
			Glib::ustring type;
			Glib::ustring cat;
			Glib::ustring desc;
			uint8_t index;
		} mod;

		Glib::Module *gmodule = nullptr;					/* the module (.la .so ...) itself */
		std::shared_ptr<void> module_pointer;				/* returned from module function use to call the destructor of module */
		/* UI */
		Glib::RefPtr<Gtk::Builder> refXml;					/* refxml to store ui file */
		Glib::ustring cssfile;								/* CSS file */
		Gtk::Box* rootbox = nullptr;									/* Root Widget from the refxml box_main or window_main */
		Gtk::Window* main_window = nullptr;					// needed for standalone
		Gtk::ScrolledWindow* main_scrolledwindow = nullptr;	// needed for standalone
		Gtk::Viewport* main_viewport = nullptr;				// needed for standalone
		/* Signals */
		virtual void attach_signals();
		virtual void dettach_signals();
		/* Set module */
		void extractPath(Glib::ustring);
		bool set_refxml(Glib::ustring);
		bool load(Glib::ustring,uint8_t);
		bool load_ui(Glib::ustring,uint8_t);
		bool load_so_la(Glib::ustring,uint8_t);
		/*** CSS ***/
		void set_style_file(Glib::ustring); // CSS to be call by the module
        /* all the app */
		void apply_style_to_screen(); // apply_style to the window
		void clear_style_for_screen(Glib::RefPtr<Gtk::StyleProvider>);
        /* window */
        void clear_style_of_window(Gtk::Window*); // apply_style to the window
		/* clear all class of a widget */
        template <class widgetType>
        void apply_style_to(widgetType*);
		template <class widgetType>
		void clear_style_of(widgetType*);


	protected: 
		/* prototype fonction of module call */
		/*
		*  using Loadplugfunc = std::tuple<std::shared_ptr<void>, Gtk::Box*, Glib::ustring>(*)(uint8_t);
		* loadplugfunc module_func;
		*/
		std::tuple<std::shared_ptr<void>, Gtk::Box*, Glib::ustring> (*module_func) (uint8_t);

		/* get a widget inside the refxml should be in the headers to be use as an external C*/
		template <class widgetType>
		widgetType* get_gwidget(Glib::ustring widget_name)
		{
			try{
				auto widget = refXml->get_object(widget_name);
				if (widget){
					return dynamic_cast<widgetType*>(widget.get());
				}else{
					std::regex pattern(R"([a-zA-Z0-9].([a-zA-Z]{3}).(.*).)");
					std::string result = std::regex_replace(typeid(widgetType).name(), pattern, "$1::$2");
					std::string err_msg = "Widget: "+ widget_name +" not found or not of type " + result;
					throw std::runtime_error(err_msg);
				}
			}catch (const std::exception & ex){
				throw;
			};
		};
		Glib::RefPtr<Glib::Object> get_gobject(Glib::ustring object_name);
		Gtk::Window* get_window();
	public:
		/** send root widget of module **/
		Gtk::Box* get_boxmain();     // return box from refxml get_module
		Gtk::Box* get_rootbox();    // return box from rootbox
		/* To give main window to the module */
		void set_main_window(Gtk::Window*);
		/* gx_module as .ui */
		Gx_module(Glib::ustring,Glib::ustring);
		/* gx_module as .la */
		Gx_module(Glib::ustring, uint8_t, Glib::ustring);
		/* for array of gx module */
		Gx_module();
		virtual ~Gx_module();
		/* set or update the name in the extpath */
		void set_module_name(Glib::ustring);
		void set_app_name(Glib::ustring);
        void set_title(Glib::ustring);
		/* set module */
		bool set_mod(Glib::ustring, uint8_t);
		void unset_mod();
		/* Standalone mode */
		void create_window();
		/* extpath properties */
		Glib::ustring get_name();
		Glib::ustring get_file();
		Glib::ustring get_ext();
		Glib::ustring get_filename();
		Glib::ustring get_path();
		/* mod properties */
		Glib::ustring get_app_name();
		Glib::ustring get_type();
		Glib::ustring get_cat();
		Glib::ustring get_desc();
};
#endif  /* gxmod_H */
