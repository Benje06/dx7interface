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
#pragma once
#ifndef Gxmod_H
	#define Gxmod_H
	/* gnome */
	/* app */
	#include "common.h"
	#include <glibmm/module.h>
	#include <pangomm.h>
	// to support font inclusion at load
    #include <pangomm/cairofontmap.h>
    //#include <fontconfig/fontconfig.h>

/*
 ***** Gx_Module *****
*/
class Gx_module {
	public:
		typedef struct st_mod_options {  		/* Module options */
			Glib::ustring name="";
			Glib::ustring color="";
			Glib::ustring cssfile="";     /* CSS file */
			Glib::ustring icon="";
			std::string custom_font="";  /* custom font */
		} St_mod_options;
		/*** CALLED BY MODULE in LoadPlug ***/
        St_mod_options get_module_options();
		/*** CALLED BY GEMOD ***/
		/* send root widget of module */
		Gtk::Box* get_rootbox();
		/* set or update the name in the extpath */
		void set_app_name(unsigned int);
		/* set module */
		bool set_mod(Glib::ustring, uint8_t);
		void unset_mod();
		/* Standalone mode call by gemod */
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

		/*** CONSTRUCTORS/DESTRUCTORS ***/
		/* gx_module as .ui */
		Gx_module(Glib::ustring,Glib::ustring);
		/* gx_module as .la */
		Gx_module(Glib::ustring, uint8_t, Glib::ustring,char**,int);
		/* for array of gx module */
		Gx_module();
		virtual ~Gx_module();
	protected:
		std::string msg_err="", msg_log="";
		/* prototype fonction of module call */
		/*
		*  using Loadplugfunc = std::tuple<std::shared_ptr<void>, Gtk::Box*, Glib::ustring>(*)(uint8_t);
		* loadplugfunc module_func;
		*/
		std::tuple<std::shared_ptr<void>, St_mod_options> (*module_func) (uint8_t);
		/* get a widget inside the refxml should be in the headers to be use as an external C*/
		template <class widgetType>
		widgetType* get_gwidget(Glib::ustring widget_name){
			try{
				auto widget = refXml->get_object(widget_name);
				if (widget){
					return dynamic_cast<widgetType*>(widget.get());
				}else{
					std::regex pattern(R"([a-zA-Z0-9].([a-zA-Z]{3}).(.*).)");
					std::string widget_type = std::regex_replace(typeid(widgetType).name(), pattern, "$1::$2");
					msg_err = error ( __PRETTY_FUNCTION__ , _("Failed to retreive widget: ") + widget_name , _("No widget of this name or not of type ") + widget_type );
					LOG_ERR( msg_err );
					throw std::runtime_error(msg_err);
				};
			}catch (const std::exception & ex){
				msg_err = error ( __PRETTY_FUNCTION__ , _("Failed in retreiving widget: ") + widget_name , ex.what() );
				LOG_ERR( msg_err );
				throw;
			};
		};
		Gtk::Window* get_window();
		/* To give main window to the module */
		void set_main_window(Gtk::Window*);
		/* set or update the name in the extpath */
		void set_module_name(Glib::ustring);
		void set_app_name(Glib::ustring);
		St_mod_options mod_options;
		/* FILES */
		/** Open File Save Dialog **/
        #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
            Gtk::FileDialog* dialog_file_select = nullptr;
            Gtk::FileDialog* dialog_file_save = nullptr;
        #else
            Gtk::FileChooserDialog* dialog_file_select = nullptr;
            Gtk::FileChooserDialog* dialog_file_save = nullptr;
            Gtk::Button* button_accept = nullptr;
        #endif

		/*** LOAD/SAVE ***/
		Gtk::Window* dialog_param = nullptr;
		Gtk::Button* btn_dialog_param = nullptr;
        sigc::connection slot_btn_dialog_param;
		Glib::RefPtr<Gio::File> initial_folder_open=nullptr;
		Glib::RefPtr<Gio::File> initial_folder_save=nullptr;

        std::vector<Glib::RefPtr<Gio::DataInputStream>> data_stream = std::vector<Glib::RefPtr<Gio::DataInputStream>>(3, nullptr);
        std::vector<Glib::RefPtr<Gio::DataOutputStream>>data_stream_out = std::vector<Glib::RefPtr<Gio::DataOutputStream>>(3, nullptr);
		virtual void set_param();          				// function to set the parameters of action
		/* LOAD */
		void OpenDialogFileSelect(unsigned int,
                                  std::function<void(unsigned int, Glib::RefPtr<Gio::File>)>);
        void read_file_as_datastream(unsigned int data_stream_index,
                                    Glib::RefPtr<Gio::File> file,
                                    std::function<void(unsigned int, Glib::RefPtr<Gio::File>)> funct);
        std::tuple<Glib::ustring, Glib::ustring, unsigned int> get_file_attribut(Glib::RefPtr<Gio::File> file);
		bool isStreamClosed(Glib::RefPtr<Gio::DataInputStream>&);
		/* SAVE */
        void OpenDialogFileSave(unsigned int,
                                std::function<void(unsigned int, Glib::RefPtr<Gio::File>)>);	// function to show the select file dialog for save
		void write_file_as_datastream(unsigned int,
                                     Glib::RefPtr<Gio::File>,
                                     unsigned char*,
                                     unsigned int);
		/* Dialog Paramaters for action */
		void OpenDialogParam(Glib::ustring);  			// function to show the dialog parameters
		virtual void set_dialog(Glib::ustring);    		// function to set the dialog parameters displayed

	private:
		typedef struct st_extPath{             	/* Provided file informations */
			Glib::ustring ext;
			Glib::ustring name;
			Glib::ustring filename;
			Glib::ustring path;
			Glib::ustring file;
		} St_extPath;
        St_extPath extpath;
		typedef struct st_mod {                 /* Module informations */
			St_extPath extpath;
			Glib::ustring name;
			Glib::ustring type;
			Glib::ustring cat;
			Glib::ustring desc;
            Glib::ustring color = "";
            Glib::ustring cssfile = "";			/* CSS file */
            std::string custom_font = "";  		/* custom font */
			uint8_t index;
		} St_mod;
        St_mod mod;
        Glib::Module *gmodule = nullptr;					// the module itself  (.la .so ...)
		std::shared_ptr<void> module_pointer;				// returned from module function use to call the destructor of module
		/* UI */
		Glib::RefPtr<Gtk::Builder> refXml;					// refxml to store ui file
		Gtk::Box* rootbox = nullptr;						// Root Widget from the refxml box_main or window_main
		Gtk::Window* main_window = nullptr;					// needed for standalone
		Gtk::ScrolledWindow* main_scrolledwindow = nullptr;	// needed for standalone
		Gtk::Viewport* main_viewport = nullptr;				// needed for standalone
		/* Signals */
		virtual void attach_signals();
		virtual void dettach_signals();
		/* Set module */
		void analyse_param(char**, int);
		void extractPath(Glib::ustring);
		bool set_refxml(Glib::ustring);
		bool load(Glib::ustring,uint8_t);
		bool load_ui(Glib::ustring,uint8_t);
		bool load_so_la(Glib::ustring,uint8_t);
		void set_module_options(St_mod_options);
        /* all the app */
        void set_icon_file(Glib::ustring);
		void set_app_icon(Gtk::Window*);

		/*** CSS STYLE ***/
		void set_style_file(Glib::ustring); // CSS to be call by the module
		/** Apply style **/
		void apply_style_to_screen(); // apply_style to the window
		/*template <class widgetType>
        void apply_style_to(widgetType*); 			// apply style to specific widget
		*/
		/** Clear Style **/
		//void clear_style_for_screen(Glib::RefPtr<Gtk::StyleProvider>);
        void clear_style_of_window(Gtk::Window*);   // clear style of the window
		template <class widgetType>
		void clear_style_of(widgetType*);			//  clear all class of a widget

		/*** FONTS ***/
		void set_custom_font_file(Glib::ustring);
		void load_custom_font();
		/* Pango/Cairo */
		void load_font_into_pango();
		void list_pango_fonts();
		bool is_font_present(Glib::ustring font_name);
		/* Windows */
		void add_custom_font(const std::string&);
		/* FontConfig */
		FcConfig* load_font_into_fontconfig(const std::string&);
};
#endif  /* gxmod_H */
