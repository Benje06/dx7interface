/* ----------------------------------------------------------------------------
* gxinterface -- Gtk+ Extended Interface
* Module and module manager source
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

#ifndef gemod_CC
	#define gemod_CC
	/* app */
	#include "gemod.h"
/*
* Gemod
*/

/* Gemod as module manager
* Call by gxinterface if type=interface
* load the UI and create space for modules
*/
Gemod::Gemod(const Glib::ustring& module_name) : Gx_module(module_name,"Gemod"){
	LOG(LOG_IN());
	try{
		is_manager_ = true;
		max_modules=nb_max_modules;
		modules=new Gx_module[max_modules];
		attach_signals();
		LOG(LOG_OUT());
	}catch(const std::exception& ex){
		msg_err = error( __PRETTY_FUNCTION__, _("Can't create: ") + module_name, ex.what() );
        LOG_ERR( msg_err );
		LOG(LOG_OUT());
		throw;
	};
};
/* Gemod as module module manager with specified numbers of module same as precedent */
Gemod::Gemod(const Glib::ustring& module_name, uint8_t max_mod) : Gx_module(module_name, "Gemod"){
	LOG(LOG_IN());
	try{
		is_manager_ = true;
		max_modules=max_mod;
		modules=new Gx_module[max_modules];
		attach_signals();
		LOG(LOG_OUT());
	}catch(const std::exception& ex){
		msg_err = error( __PRETTY_FUNCTION__, _("Can't create: ") + module_name, ex.what() );
        LOG_ERR( msg_err );
		LOG(LOG_OUT());
		throw;
	};
};
/* Gemod as module call if type=module */
Gemod::Gemod(const Glib::ustring& module_name, uint8_t index, char** argv, int argc) : Gx_module(module_name, index, "Gemod", argv, argc){
	LOG(LOG_IN());
	try{
		if(get_rootbox()){
			set_app_name( (get_rootbox())->get_name() );
			modules=new Gx_module[nb_mod];
			create_window();
		};
		LOG(LOG_OUT());
	}catch(const std::exception& ex){
		msg_err = error( __PRETTY_FUNCTION__, _("Can't create: ") + module_name, ex.what() );
        LOG_ERR( msg_err );
		LOG(LOG_OUT());
		throw;
	};
};

Gemod::~Gemod(){
	LOG(LOG_IN());
	if( is_manager_ ){
		dettach_signals();
	}else{
        destroy_main_window();
    };
	if(modules){
		delete[] modules;
	};
	LOG(LOG_OUT());
};

/*** MODULE ***/
/* count the number of module with same source file */
uint8_t Gemod::get_module_count(const Glib::ustring& module_file){
	uint8_t nb = 0;
	for (uint8_t i = 0 ; i < nb_mod; i++){
		if ( modules[i].get_file() == module_file ){ 
			nb+=1;
		};
	};
	return nb;	
};
/* Get module index by module name(mod.name) */
int8_t Gemod::get_module_index(const Glib::ustring& name){
	for (uint8_t i = 0 ; i < nb_mod; i++){
		if ( modules[i].get_app_name() == name ){ 
			return i;
		};
	};
	return -1;
}
/* get module name by modules[] index */
Glib::ustring Gemod::get_module_name(uint8_t index){ 
	return modules[index].get_name();
};

/* get root widget(box_main) from refxml by module index */
Gtk::Box* Gemod::get_module_root(uint8_t index)	{ 
	try {
		if (index >= max_modules) {
			msg_err = error( __PRETTY_FUNCTION__,
							 _("Invalid module index: ") + tostr<unsigned int>(index),
							 _("Index out of bounds (max_modules = ") + tostr<unsigned int>(max_modules) + ")" );
			LOG_ERR( msg_err );
			return nullptr;
		};
		return modules[index].get_rootbox();
	}catch(const std::exception& ex){
		msg_err = error( __PRETTY_FUNCTION__, _("Fail to get rootbox"), ex.what() );
        LOG_ERR( msg_err );
		return nullptr;
	}
};
/* !!! CAUTION !!! get root widget by module name based on module index can be empty */
Gtk::Box* Gemod::get_module_root(const Glib::ustring& module_name){
	LOG(LOG_IN());
	try {
		auto index = get_module_index(module_name);
		if ( index != -1 ){
			return modules[index].get_rootbox();
		}else{
			return nullptr;
		}
		LOG(LOG_OUT());
	}catch( std::exception& ex){
		msg_err = error( __PRETTY_FUNCTION__, _("Fail to get rootbox"), ex.what() );
        LOG_ERR( msg_err );
		return nullptr;
	}
};

/*** Manage Modules ***/
/*  get root window */
Gtk::Window* Gemod::get_window() { return Gx_module::get_window(); };				/* return the Gtk::Window created */

/* Events */
/* attache les signaux sur tous les elements */
void Gemod::attach_signals(){
	try{
		/*(get_gwidget<Gtk::MenuItem>("menu_add_module"))->signal_activate().connect(
			sigc::mem_fun(*this, &Gemod::on_menu_add_module_event));
		(get_gwidget<Gtk::MenuItem>("menu_del_module"))->signal_activate().connect(
			sigc::mem_fun(*this, &Gemod::on_menu_del_module_event));
		*/
		slot_module_select = (get_gwidget<Gtk::Button>("module_select"))->signal_clicked().connect(
			sigc::mem_fun(*this, &Gemod::on_module_select_event));
	}catch ( std::exception& ex ){
		msg_err = error( __PRETTY_FUNCTION__, _("Cannot attach signals"), ex.what() );
        LOG_ERR( msg_err );
		throw std::runtime_error(msg_err);
	}
};
void Gemod::dettach_signals(){
	LOG(LOG_IN()); 
	slot_module_select.disconnect();
	LOG(LOG_OUT());
};

void Gemod::on_module_select_event(){
	LOG(LOG_IN());
	// Gtk::FileDialog set_initial_folder
	//						  select_folder
    try {
        #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
            auto dialog = get_gwidget<Gtk::FileDialog>("FileDialog_module_select");
            dialog->set_title(_("Select Module .la, .so or .ui"));
            dialog->set_modal(true);
            dialog->open( *(get_window()), [this,dialog](const Glib::RefPtr<Gio::AsyncResult>& result ) {
                    try {
                        auto file = dialog->open_finish(result);
                        if (file) {
                            std::string filename = file->get_path();
                            this->add_module(filename);
                        }
                    } catch (const std::exception & ex) {
                        msg_err = error(__PRETTY_FUNCTION__,_("Can't select module: "), ex.what());
                        LOG_ERR( msg_err );
                    }
                }
            );
        #else
			GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
			auto dialog = new Gtk::FileChooserDialog(_("Please choose a file"), Gtk::FileChooser::Action::OPEN);
			dialog->set_transient_for(*(get_window()));
			dialog->set_modal(true);
			dialog->add_button("_Cancel", Gtk::ResponseType::CANCEL);
            dialog->add_button("_Open", Gtk::ResponseType::ACCEPT);
            dialog->signal_response().connect(
                [this, dialog](int response) {
                    try {
                        if (response == Gtk::ResponseType::ACCEPT) {
                            add_module((dialog->get_file())->get_parse_name());
                        }
                        dialog->hide();
                    } catch (const std::exception & ex) {
						msg_err = error(__PRETTY_FUNCTION__,_("Can't select module: "), ex.what());
                        LOG_ERR( msg_err );
                    };
                });
            dialog->show();
	#endif
    } catch (const std::exception & ex) {
		msg_err = error(__PRETTY_FUNCTION__,_("Other exception."), ex.what());
        LOG_ERR( msg_err );
    };
	LOG(LOG_OUT());
};

void Gemod::on_menu_add_module_event(){ LOG(LOG_IN());
	/*add_module( MODULE_UI_DIR"libgxsynth-0.0.1.la");*/
LOG(LOG_OUT()); };
void Gemod::on_menu_del_module_event(){ LOG(LOG_IN());
	/*del_module("data/ui/dx7.glade");*/
LOG(LOG_OUT()); };
bool Gemod::add_module(const Glib::ustring& module_name){
	LOG(LOG_IN());
	try{
		if ( nb_mod < max_modules ){
			/* */
			if(modules[nb_mod].set_mod(module_name,nb_mod)){
				uint8_t nb_mod_ident = get_module_count(module_name);
				msg_log = _("Module: ") + modules[nb_mod].get_name();
				LOG( msg_log );
				if  ( nb_mod_ident != 0 ){
					modules[nb_mod].set_app_name(nb_mod_ident);
				};
				msg_log = '\t' + _("number of module with this name (nb_mod_ident): ") + tostr<unsigned int>(nb_mod_ident) + EOL;
				msg_log += '\t' + _("Index module (nb_mod): ") + tostr<unsigned int>(nb_mod) + EOL;
				msg_log += '\t' + _("App name (mod.name): ") + modules[nb_mod].get_app_name() + EOL;
				msg_log += '\t' + _("Module name (extpath.name): ") + modules[nb_mod].get_name() + EOL;
				msg_log += '\t' + _("Module ext (extpath.ext): ") + modules[nb_mod].get_ext() + EOL;
				LOG( msg_log );
				Gtk::Box* box = modules[nb_mod].get_rootbox();
				if (box != nullptr){
					if ( (get_gwidget<Gtk::CheckButton>("checkbutton_standalone"))->get_active() == FALSE  ){
						(get_gwidget<Gtk::Notebook>("notebook_main"))->append_page(
							*box,
							tostr<std::string>(modules[nb_mod].get_app_name()),
							false
						);
						nb_mod++;
						msg_log = _("set menu");
						LOG( msg_log );
						
						// attacher les signaux
						//add_menu("menu_modules", modules[nb_mod].get_name());
						LOG(LOG_OUT());
						return true;
					}else{
						// create_window and attach widget widget box
						modules[nb_mod].create_window();
						nb_mod++;
						LOG(LOG_OUT());
						return true;
					};
				}else{
					msg = error(__PRETTY_FUNCTION__,_("Can't set rootbox from get_rootbox."), _("rootbox is null pointer"));
                    LOG_ERR( msg );
					LOG(LOG_OUT());
					return false;
				};
			}else{
				std::string msg = _("Can't set_mod: ");
				msg += module_name;
				msg +=" n°: ";
				msg +=nb_mod;
				msg_err = error(__PRETTY_FUNCTION__, msg, _("unknow"));
                LOG_ERR( msg_err );
				LOG(LOG_OUT());
				return false;
			};
		}else{
			msg_err = error( __PRETTY_FUNCTION__,
							 _("Cannot add module: ") + module_name,
							 _("Maximum number of modules reached: ") + tostr<unsigned int>(max_modules) );
			LOG_ERR( msg_err );
		};
		LOG(LOG_OUT()); 
		return false;
	}catch(const std::exception& ex){
		msg_err = error(__PRETTY_FUNCTION__, _("Can't add module") + module_name, ex.what());
    	LOG_ERR( msg_err );
		LOG(LOG_OUT()); 
		return false;
	}
};
bool Gemod::del_module(const Glib::ustring& module_name){ 
	LOG(LOG_IN());
	// must base on app_name
	/** TODO : remove module
	* call finish on extern or unload
	**/
		if ( nb_mod > 0 && get_module_count(module_name) != 0 ){
			//(get_gwidget<Gtk::Notebook>("notebook_main"))->pages().remove( *(get_module_root(get_module_index(module_name)))  );
			//modules[get_module_index(module_name)].unset_mod();
			//del_menu(module_name);
			nb_mod--;
			LOG(LOG_OUT());
			return true;
		}else{
			msg_err = error(__PRETTY_FUNCTION__, _("Can't delete module: ") + module_name, _("The module doesn't exist"));
            LOG_ERR( msg_err );
			LOG(LOG_OUT());
			return false;
		};
	LOG(LOG_OUT());
};

/* Menu construction*/
/*Gtk::MenuItem* Gemod::create_menuitem(Glib::ustring name) { LOG(LOG_IN());
	Gtk::Menu_Helpers::MenuList::iterator iter = (get_menu("menu_modules"))->items().end();
	(get_menu("menu_modules"))->items().insert(iter,Gtk::Menu_Helpers::MenuElem(name) );
	(get_menuitem("Add"))->set_submenu( *(create_submenu()) );
LOG(LOG_OUT()); };*/

/*Gtk::Menu* Gemod::create_submenu(Glib::ustring* menu) { 
	LOG(LOG_IN()); 
	Gtk::Menu * submenu;
	submenu = manage (new Gtk::Menu);
	uint length = sizeof(menu)/sizeof(Glib::ustring);
	for ( uint i=0 ; i <= length ; i++ ){
		submenu->items().push_back (Gtk::Menu_Helpers::MenuElem(menu[i]));
	};
	LOG(LOG_OUT());
	return (Gtk::Menu *) submenu;
};

Gtk::MenuItem * Gemod::get_menu_dyn(Glib::ustring menu){
	for ( Gtk::Menu_Helpers::MenuList::iterator iter = (get_gwidget<Gtk::Menu>("menu_modules"))->items().begin() ; iter != (get_gwidget<Gtk::Menu>("menu_modules"))->items().end(); iter++) {
		if ((*iter).get_label() == menu ){
			return (Gtk::MenuItem*) &(*iter) ;
		};
	};
}

gboolean Gemod::add_menu(Glib::ustring menu_name, Glib::ustring module_name){ 
	LOG(LOG_IN()); 
	if (nb_mod >= 0){
		Gtk::Menu_Helpers::MenuList::iterator iter = (get_gwidget<Gtk::Menu>(menu_name))->items().end();
		(get_gwidget<Gtk::Menu>(menu_name))->items().insert(iter,Gtk::Menu_Helpers::MenuElem(module_name) );
		Glib::ustring menus[2] = {_("activé"),_("désactivé")};
		(get_menu_dyn(module_name))->set_submenu( *(create_submenu(menus)) );
	}else{
		std::cout << "echec menu" << std::endl;
		LOG(LOG_OUT()); return 0;
	};
LOG(LOG_OUT()); };

gboolean Gemod::del_menu(Glib::ustring module_name){
	LOG(LOG_IN());
	try {
		if ( nb_mod > 0 ){
			Gtk::Menu_Helpers::MenuList::iterator iter_end = (get_gwidget<Gtk::Menu>("menu_modules"))->items().end();
			for ( Gtk::Menu_Helpers::MenuList::iterator iter = (get_gwidget<Gtk::Menu>("menu_modules"))->items().begin() ; iter != iter_end ; iter++) {
				if ((*iter).get_label() == module_name ){
					std::cout << (*iter).get_label() << std::endl;
					(get_gwidget<Gtk::Menu>("menu_modules"))->items().remove(*(iter));
				};
			};
			//(get_menu("menu_modules"))->items().insert((get_menu("menu_modules"))->items().end(),Gtk::Menu_Helpers::SeparatorElem());
			LOG_OUT (); return 1;
		}else{
			LOG_OUT (); return 0;
		};
	}catch (const std::exception & ex){
			std::cerr << ex.what() << std::endl; 
	};
	LOG(LOG_OUT()); 
};		
*/

#endif  /* gemod_CC */
