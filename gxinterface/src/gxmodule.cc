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

#ifndef Gxmod_CC
    #define Gxmod_CC
    /* app */
    #include "gxmodule.h"

/*
 **** Gx_module ****
 */
 
/* gx as ui or la, 
 * as ui for Gemod as a Gx module manager in gxinterface
 * or la in standalone mode (-m)
 * function are in gemod class
 */
Gx_module::Gx_module(Glib::ustring filename,Glib::ustring caller){
    std::cerr << caller; 
    LOG_IN();
    mod.desc=caller;
    mod.name=filename;
    try{
        load(filename,0);
        std::cerr << caller << " ";
        LOG_OUT();
    }catch(const std::exception& ex){
        std::cerr << caller << " ";
        LOG_OUT();
        std::string err_msg = "!!! " +std::string(__PRETTY_FUNCTION__) + _(" Cannot be created !!!\n => Cannot load : ") + filename + "\n" + _("Reason ") + ex.what();
        throw std::runtime_error(err_msg);
    };
};

void Gx_module::analyse_param(char** argv, int argc){
    for ( int i = 1; i <= argc; i++) {
        if ( (argv[i] != NULL) && ( Glib::ustring(argv[i]) == "-c" || (Glib::ustring(argv[i]) == "-c") )
            && (argv[i+1] != NULL) && ( Glib::ustring(argv[i+1]) != "" )
        ){  // -i and interface filename as argument
            mod.color = Glib::ustring(argv[i+1]);
        };
    };
};

/* gx module as .la */
Gx_module::Gx_module(Glib::ustring filename, uint8_t index, Glib::ustring caller,char** argv,int argc) {
    std::cerr << caller; 
	LOG_IN();
    mod.desc=caller;
    mod.name=filename;
    try{
        analyse_param(argv,argc);
        load(filename,index);
        std::cerr << caller << " ";
        LOG_OUT();
    }catch(const std::exception& ex){
        std::cerr << caller << " ";
        std::string err_msg = "!!! " +std::string(__PRETTY_FUNCTION__) + _(" Cannot be created !!!\n => Cannot load : ") + filename + "\n" + _("Reason ") + ex.what();
        LOG_OUT();
        throw std::runtime_error(err_msg);
    };
};

/* array of gx_module */
Gx_module::Gx_module() {
	LOG_IN();
    mod.desc="part of array";
    mod.name="Gemod";
	LOG_OUT();
};

Gx_module::~Gx_module(){
    std::cerr << mod.name << "->" << mod.desc;
	LOG_IN();
    if (gmodule){
        module_pointer.reset();
        delete gmodule;
    };
    std::cerr << mod.name << "->" << mod.desc;
	LOG_OUT();
};

/* get main box from refxml */
Gtk::Box* Gx_module::get_boxmain(){ // (get_module)
	return get_gwidget<Gtk::Box>("box_main");
};
/* get root box from internal propeties */
Gtk::Box* Gx_module::get_rootbox() 	{
    return rootbox;
};

/* get Glib pointer from refxml */
/* (unused) call from : nowhere */
Glib::RefPtr<Glib::Object> Gx_module::get_gobject(Glib::ustring object_name) {
	return refXml->get_object(object_name);
};

/*** CSS STYLE ***/
/* set the CSS style file provided*/
void Gx_module::set_style_file(Glib::ustring file_css){
    LOG_IN();
    if( std::filesystem::exists(file_css.c_str()) ){
        cssfile=file_css;
    }else{
        std::cerr<< _("Warning the css style file ")<< file_css << _(" doesn't exist or is not readable")<< std::endl;
        cssfile="";
    };
    LOG_OUT();
};

/* WINDOW */
void Gx_module::create_window(){
    LOG_IN();
    try{
        main_scrolledwindow = new Gtk::ScrolledWindow();
        main_window = new Gtk::Window();
        main_viewport = new Gtk::Viewport(main_scrolledwindow->get_hadjustment(),
                                            main_scrolledwindow->get_vadjustment());
        main_viewport->set_child(*(get_rootbox()));
        main_scrolledwindow->set_child(*main_viewport);
        main_window->set_child(*main_scrolledwindow);
        main_window->set_title(get_app_name());
        main_window->set_default_size(1024, 768);
        //clear_style_of_window(main_window);
        //apply_style_to<Gtk::Window>(main_window);
        apply_style_to_screen();
        main_window->set_visible();
        std::static_pointer_cast<Gx_module>(module_pointer)->set_main_window(main_window);
        //std::cout << " get_APP_name: "<< module_manager->get_app_name() << std::endl;
        //std::cout << " get_name: " << module_manager->get_name() << std::endl;
    }catch (const std::exception& ex){
        std::string err_msg = "!!! " +std::string(__PRETTY_FUNCTION__) + _(" Creating window failed !!!\n") + _("Reason ") + ex.what();
        LOG_OUT();
        throw std::runtime_error(err_msg);
    };
    LOG_OUT();
}
/* CSS style for WINDOWS */

/* clear all class of a window */
void Gx_module::clear_style_of_window(Gtk::Window* window){

};


/*** WIDGET ***/
/* clear all class style of a widget */
template <class widgetType>
void Gx_module::clear_style_of(widgetType* widget){
    /* todo remove a specific class */
    auto style_ctx = widget->get_style_context();
    auto class_list = style_ctx->list_classes();
    for (const auto& class_name : class_list) {
        style_ctx->remove_class(class_name);
    };
};

/* apply style to a widget */
/*template <class widgetType>
void Gx_module::apply_style_to(widgetType* widget){*/
    /* TODO: clear style before apply a new one */
    /*LOG_IN();
    try{
        auto css = Gtk::CssProvider::create();
        css->load_from_path(cssfile);
        auto style_ctx = widget->get_style_context_for_node(widget->get_node());
        style_ctx->add_provider(css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }catch(const std::exception& ex){
        std::cout << "Failed to load style:" << ex.what() << std::endl;
        std::cout << "\tCSS file: " << cssfile << std::endl;
    };
    LOG_OUT();
};
*/

/*** SCREEN ***/
/* Clear all custom style */
/*void Gx_module::clear_style_for_screen(Glib::RefPtr<Gtk::StyleProvider> css_provider){

};*/

/*** apply style to all the application(screen) ***/
void Gx_module::apply_style_to_screen(){
    LOG_IN();
    try{
        if(cssfile != ""){
            auto settings = Gtk::Settings::get_default();
            auto css = Gtk::CssProvider::create();
            auto custom_provider = Gtk::CssProvider::create();
            css->load_from_path(cssfile);

            #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
                if(mod.color != ""){
                    custom_provider->load_from_string("title { background-color: " + mod.color + "; }");
                };
                auto display = Gdk::Display::get_default();
                if (display) {
                    Gtk::StyleProvider::add_provider_for_display(display, css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
                    if(mod.color != ""){
                        Gtk::StyleProvider::add_provider_for_display(display, custom_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
                    };
                };

            #else
                if(mod.color != ""){
                    custom_provider->load_from_data("title { background-color: " + mod.color + "; }");
                };
                auto display = Gdk::Display::get_default();
                Gtk::StyleContext::add_provider_for_display(display, css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            #endif
        };
    } catch (const std::exception& ex) {
        std::string err_msg = "!!! " +std::string(__PRETTY_FUNCTION__) + _(" Failed to load style: !!!\n") + cssfile + "\n" + _("Reason => ") + ex.what();
        LOG_OUT();
        throw std::runtime_error(err_msg);
    };
    LOG_OUT();
};


/*** SET MODULE ***/
bool Gx_module::load_so_la(Glib::ustring filename,uint8_t index){
    LOG_IN();
    gmodule = new Glib::Module(filename);
	if (gmodule) {
        try{
            /* see if last_error is empty after the first read with something in */
	        if( gmodule->get_last_error() == "" &&
	            gmodule->get_symbol( "LoadPlug", (void*&) module_func ) ){
	            std::cout << "Name of the GLIB::Module: "<< std::endl;
                std::cout << "\t" << gmodule->get_name() << std::endl;
		        auto [mod_pointer, rootbox_ptr, cssfile_String] = module_func(index);
                cssfile=cssfile_String;
                module_pointer = mod_pointer;
                rootbox = rootbox_ptr;
                std::cout << cssfile << std::endl;
                set_style_file(cssfile);
                set_app_name( (rootbox)->get_name() );
                std::cout << "Name of app: " << get_app_name() << std::endl;
                mod.index=index;
		        LOG_OUT();
		        return true;
		    }else{
           		std::cerr << "Error : no function or error. " << std::endl;
           		std::cerr << "\tLast module error : " << gmodule->get_last_error() << std::endl;
                std::cerr << "\tfor module file: " << filename << std::endl;
       			LOG_OUT();
		        return false;
	        };
        }catch (const std::exception& ex){
		    LOG_OUT();
		    return false;
	    };   		
	}else{
	    std::cerr << "Error : no module." << std::endl;
        std::cerr << "\tlast module load error: " << gmodule->get_last_error() << std::endl;
        std::cerr << "\tfor the module file: " << filename << std::endl;
		LOG_OUT();
		return false;
    };
};

void Gx_module::set_main_window(Gtk::Window* window){
    LOG_IN();
    main_window=window;
    LOG_OUT();
};

/* set refxml from ui file */
bool Gx_module::set_refxml(Glib::ustring filename)	{
    LOG_IN();
	try { 
	    refXml = Gtk::Builder::create_from_file(filename);
	    LOG_OUT();
		return true;
	}catch (const std::exception& ex){
        std::string err_msg = "from: " + std::string(__PRETTY_FUNCTION__)\
        + "\nSet refXml failed with error : " + ex.what();
        throw std::runtime_error(err_msg);
		return false;
	};
};

/* load ui */
bool Gx_module::load_ui(Glib::ustring filename, uint8_t index){
    LOG_IN();
    try{
        /* TODO: maybe try catch and throw lower part */
        if(set_refxml(filename)){
            rootbox=refXml->get_widget<Gtk::Box>("box_main");
            if(rootbox){
                if( (rootbox)->get_parent() != nullptr ){     // if the box_main widget has a parent
                    if( "interface.ui" != filename.substr(filename.find_last_of("/")+1, filename.length()) ){
                        // box_main cannot be attached reset and go
                        std::cerr<< "box_main get a parent cannot be include" << std::endl;
                        rootbox = nullptr;
                    };
                    LOG_OUT();
                    return false;
                };
                set_app_name( rootbox->get_name() );
                mod.index=index;
                LOG_OUT();
                return true;
            }else{
                std::string err_msg = "from: " + std::string(__PRETTY_FUNCTION__)\
                                    + "\nGtk::Box 'box_main' not found in UI file";
                throw std::runtime_error(err_msg);
                LOG_OUT();
                return false;
            };
        }else{
            LOG_OUT();
            return false;
        };
    }catch (const std::exception& ex){
            LOG_OUT();
            throw;
            return false;
    }
};

/* set extpath struc from filename information */
void Gx_module::extractPath(Glib::ustring filename){
	LOG_IN();
	/* extract to extpath ( extention, filename, path ,name ,file ) */
	int start, end;
	mod.extpath.file=filename;
	start = filename.find_last_of(DS);
	mod.extpath.filename = filename.substr(start+1, filename.length() - (start+1) );
	mod.extpath.path = filename.substr(0, start+1 );
	end = filename.find_last_of(".");
	mod.extpath.ext = filename.substr( end+1, filename.length() );
	mod.extpath.name = filename.substr(start+1, mod.extpath.filename.length() - mod.extpath.ext.length() -1 );
	/** TODO remove std **/
	std::cout << " mod.extpath.name " << mod.extpath.name << std::endl;
	std::cout << " mod.extpath.ext " << mod.extpath.ext << std::endl;
	std::cout << " mod.extpath.path " << mod.extpath.path << std::endl;
	std::cout << " mod.extpath.filename " << mod.extpath.filename << std::endl;
	std::cout << " mod.extpath.file " << mod.extpath.file << std::endl;
	LOG_OUT();
};

/* load .ui or .la in a module construction */
bool Gx_module::load(Glib::ustring filename, uint8_t index){
    /* TODO: maybe try catch to throw lower throw */
    LOG_IN();
	extractPath(filename);
	if ( (mod.extpath.ext == "ui") ||  (mod.extpath.ext == "xml") ){
		return load_ui(filename,index);
	}else if ( (mod.extpath.ext == "la") || (mod.extpath.ext == "so") || (mod.extpath.ext == "lo") || (mod.extpath.ext == "dll") ) {
		return load_so_la(filename,index);
	}else{
		std::cerr << "Error : Mauvaise extention de fichier." << std::endl;
        std::cerr << "\tLes types acceptés sont: " << std::endl;
        std::cerr << "\t\t.ui ou .xml pour une UI" << std::endl;
        std::cerr << "\t\t.la or .so or .lo pour un module" << std::endl;
		LOG_OUT();
		return false;
	};
};

/* set a module */
bool Gx_module::set_mod( Glib::ustring filename, uint8_t index ) 	{
    LOG_IN();
	if( load(filename,index) ){
	    LOG_OUT();
	    return true;
	}else{
	    LOG_OUT();
	    return false;
	};
};

/* unset a module */
void Gx_module::unset_mod()	{ LOG_IN(); 
	/*dettach_signals();
	if ( mod.extpath.ext == "la" ){
		delete module;
	};*/
    LOG_OUT(); 
};

/*** SIGNAL ***/
void Gx_module::attach_signals(){};
void Gx_module::dettach_signals(){};

/*** MOD ***/
Gtk::Window* Gx_module::get_window(){
    return main_window;
};
/* mod */
void Gx_module::set_app_name(Glib::ustring name){
    if(main_window){
      main_window->set_title(name);
    };
    mod.name=name;
};
Glib::ustring Gx_module::get_app_name(){ return mod.name ; };
Glib::ustring  Gx_module::get_type(){	return mod.type ; };
Glib::ustring  Gx_module::get_cat(){	return mod.cat ; };
Glib::ustring  Gx_module::get_desc(){	return mod.desc ; };
/* extpath */
void Gx_module::set_module_name(Glib::ustring name) { mod.extpath.name = name; };	
Glib::ustring  Gx_module::get_name(){	return mod.extpath.name ; };
Glib::ustring  Gx_module::get_file(){	return mod.extpath.file ; };
Glib::ustring  Gx_module::get_ext(){	return mod.extpath.ext ; };
Glib::ustring  Gx_module::get_filename(){	return mod.extpath.filename ; };
Glib::ustring  Gx_module::get_path(){	return mod.extpath.path ; };

#endif  /* gxmod_CC */
