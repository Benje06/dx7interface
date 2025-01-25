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
	#include "debug.h"
	#include "common.h"
	#include "gemod.h"
/*
* Gemod
*/

/* 
* Gemod as module manager
* Call by gxinterface if type=interface
* load the UI and create space for modules
*/
Gemod::Gemod(Glib::ustring module_name) : Gx_module(module_name,"Gemod"){
	LOG_IN();
	nb_mod=0;
	max_modules=5;
	try{
		modules=new Gx_module[max_modules];
		attach_signals();
		LOG_OUT();
	}catch(const std::exception& ex){
		LOG_OUT();
		throw;
	};
};

/*
* Gemod as module call if type=module
*/
Gemod::Gemod(Glib::ustring module_name, uint8_t index) : Gx_module(module_name,index,"Gemod"){
	LOG_IN();
	try{
		nb_mod=0;
		if( get_ext() != "la" ){ // check if the main is a .la
			max_modules=5;
			modules=new Gx_module[max_modules];
			attach_signals();
		}else{
			if(get_rootbox()){
				set_app_name( (get_rootbox())->get_name() );
				modules=new Gx_module[0];
				create_window();
			};
		};
	}catch(const std::exception& ex){
		LOG_OUT();
		throw;
	};
};

/* 
* Gemod as module with specified numbers of module
*/
Gemod::Gemod(Glib::ustring module_name, uint8_t index, uint8_t max_mod) : Gx_module(module_name, index,"Gemod"){ 
	LOG_IN();
	nb_mod=0;
	max_modules=max_mod;
	modules=new Gx_module[max_modules];
	attach_signals();
	LOG_OUT();
};

Gemod::~Gemod(){
	LOG_IN();
	// TODO: clean all what is constructed by new
	if(modules){
		delete[] modules;
	};
	LOG_OUT();
};

/*** MODULE ***/
/* count the number of module with same source file */
uint8_t Gemod::get_module_count(Glib::ustring module_file){
	uint8_t nb = 0;
	for (uint8_t i = 0 ; i < nb_mod; i++){
		if ( modules[i].get_file() == module_file ){ 
			nb+=1;
		};
	};
	return nb;	
};

/* Get module index by module name(mod.name) */
int8_t Gemod::get_module_index(Glib::ustring name){
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
		return modules[index].get_rootbox(); // before callin was get_boxmain
	}catch(const std::exception& ex){
		std::cerr << ex.what() << std::endl;
		return nullptr; /* return NULL */
	}
};

/* !!! CAUTION !!! get root widget by module name based on module index can be empty */
Gtk::Box* Gemod::get_module_root(Glib::ustring module_name){
	/* TODO not return fake gtk::box */
	LOG_IN();
	auto index = get_module_index(module_name);
	if ( index != -1 ){
		return modules[get_module_index(module_name)].get_rootbox(); /* before was calling get_boxmain */
	}else{
		return nullptr;
	}
	LOG_OUT();
};

/*** Manage Modules ***/
bool Gemod::add_module(Glib::ustring module_name){
	LOG_IN();
	try{
		if ( nb_mod < max_modules ){
			/* */
			if(modules[nb_mod].set_mod(module_name,nb_mod)){
				uint8_t nb_mod_ident = get_module_count(module_name);
				std::cout << "Module: " << modules[nb_mod].get_name() << std::endl;
				if  ( nb_mod_ident != 0 ){
					modules[nb_mod].set_app_name( modules[nb_mod].get_app_name() + " (" + tostr<uint>(nb_mod_ident) +")" );
				};
				std::cout << "\tNombre de module avec ce nom(nb_mod_ident) : " << tostr<uint>(nb_mod_ident) << std::endl;
				std::cout << "\tIndex module(nb_mod): " << tostr<uint>(nb_mod)  << std::endl;
				std::cout << "\tApp name(mod.name): " << modules[nb_mod].get_app_name() << std::endl;
				std::cout << "\tModule name(extpath.name): " << modules[nb_mod].get_name() << std::endl;
				std::cout << "\tModule ext(extpath.ext): " << modules[nb_mod].get_ext() << std::endl;
				Gtk::Box* box = modules[nb_mod].get_rootbox();
				if (box != nullptr){
					if ( (get_gwidget<Gtk::CheckButton>("checkbutton_standalone"))->get_active() == FALSE  ){
						(get_gwidget<Gtk::Notebook>("notebook_main"))->append_page(
							*box,
							tostr<std::string>(modules[nb_mod].get_app_name()),
							false
						);
						nb_mod++;
						std::cout<< "set menu"<<std::endl;
						// attacher les signaux
						//add_menu("menu_modules", modules[nb_mod].get_name());
						LOG_OUT();
						return true;
					}else{
						// create_window and attach widget widget box
						modules[nb_mod].create_window();
						LOG_OUT();
						return true;
					};
				}else{
					std::cerr << "rootbox from get_rootbox -> null pointer" << std::endl;
					LOG_OUT();
					return false;
				};
			}else{
				std::cerr << "cannot set_mod: " << module_name << " n°: " << nb_mod << std::endl;
				LOG_OUT();
				return false;
			};
		};
		LOG_OUT(); 
		return false;
	}catch(const std::exception& ex){
		std::cout << ex.what() << std::endl;
		LOG_OUT(); 
		return false;
	}
};


/*  get root window */
Gtk::Window* Gemod::get_main() { return get_gwidget<Gtk::Window>("window_main"); };	/* return the Gtk::Window from the xml file */
Gtk::Window* Gemod::get_window() { return Gx_module::get_window(); };				/* rutne the Gtk::Window created */

/* attache les signaux sur tous les elements */
void Gemod::attach_signals(){
	try{
	/*(get_gwidget<Gtk::MenuItem>("menu_add_module"))->signal_activate().connect(
			sigc::mem_fun(*this, &Gemod::on_menu_add_module_event));
		(get_gwidget<Gtk::MenuItem>("menu_del_module"))->signal_activate().connect(
			sigc::mem_fun(*this, &Gemod::on_menu_del_module_event));
		*/
		(get_gwidget<Gtk::Button>("module_select"))->signal_clicked().connect(
			sigc::mem_fun(*this, &Gemod::on_module_select_event));
	}catch ( std::exception& ex ){
		std::string err_msg = "from " +std::string(__PRETTY_FUNCTION__) + "\nCannot attach signals !!!\n" + ex.what();
		throw std::runtime_error(err_msg);
	}
};

void Gemod::dettach_signals(){LOG_IN(); LOG_OUT();};

/* Events */
void Gemod::on_module_select_event(){
	LOG_IN();
	// Gtk::FileDialog set_initial_folder
	//						  select_folder
	auto dialog = get_gwidget<Gtk::FileDialog>("FileDialog_module_select");
	dialog->set_title("Select Module .la, .so or .ui");
	dialog->set_modal(true);
	dialog->open( *(get_main()), [this,dialog](const Glib::RefPtr<Gio::AsyncResult>& result ) {
			try {
				auto file = dialog->open_finish(result);
				if (file) {
					std::string filename = file->get_path();
					this->add_module(filename );
				}
			} catch (const std::exception & error) {
				std::cerr << "Error: " << error.what() << std::endl;
			}
		}
	);
	LOG_OUT();
};

void Gemod::on_menu_add_module_event(){ LOG_IN();
	/*add_module( MODULE_UI_DIR"libgxsynth-0.0.1.la");*/
LOG_OUT(); };

void Gemod::on_menu_del_module_event(){ LOG_IN();
	/*del_module("data/ui/dx7.glade");*/
LOG_OUT(); };

bool Gemod::del_module(Glib::ustring module_name){ 
	LOG_IN();
	// must base on app_name
	/** TODO : remove module
	* call finish on extern or unload
	**/
		if ( nb_mod > 0 && get_module_count(module_name) != 0 ){
			//(get_gwidget<Gtk::Notebook>("notebook_main"))->pages().remove( *(get_module_root(get_module_index(module_name)))  );
			//modules[get_module_index(module_name)].unset_mod();
			//del_menu(module_name);					
			nb_mod--;
			LOG_OUT(); return true;
		}else{
			std::cout << "Le module n'existe pas" << std::endl;
			LOG_OUT(); return false;
		};
LOG_OUT();
};

/* Menu construction*/
/*Gtk::MenuItem* Gemod::create_menuitem(Glib::ustring name) { LOG_IN();
	Gtk::Menu_Helpers::MenuList::iterator iter = (get_menu("menu_modules"))->items().end();
	(get_menu("menu_modules"))->items().insert(iter,Gtk::Menu_Helpers::MenuElem(name) );
	(get_menuitem("Add"))->set_submenu( *(create_submenu()) );
LOG_OUT(); };*/

/*
Gtk::Menu* Gemod::create_submenu(Glib::ustring* menu) { 
	LOG_IN(); 
	Gtk::Menu * submenu;
	submenu = manage (new Gtk::Menu);
	uint length = sizeof(menu)/sizeof(Glib::ustring);
	for ( uint i=0 ; i <= length ; i++ ){
		submenu->items().push_back (Gtk::Menu_Helpers::MenuElem(menu[i]));
	};
	LOG_OUT();
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
	LOG_IN(); 
	if (nb_mod >= 0){
		Gtk::Menu_Helpers::MenuList::iterator iter = (get_gwidget<Gtk::Menu>(menu_name))->items().end();
		(get_gwidget<Gtk::Menu>(menu_name))->items().insert(iter,Gtk::Menu_Helpers::MenuElem(module_name) );
		Glib::ustring menus[2] = {_("activé"),_("désactivé")};
		(get_menu_dyn(module_name))->set_submenu( *(create_submenu(menus)) );
	}else{
		std::cout << "echec menu" << std::endl;
		LOG_OUT(); return 0;
	};
LOG_OUT(); };

gboolean Gemod::del_menu(Glib::ustring module_name){
	LOG_IN();
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
	LOG_OUT(); 
};		
*/

#endif  /* gemod_CC */
