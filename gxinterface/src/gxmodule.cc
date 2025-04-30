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
    PRE_LOG(caller.c_str(),"");
    LOG_IN();
    mod.desc=caller;
    mod.name=filename;
    try{
        load(filename,0);
        LOG_OUT();
        POST_LOG(caller.c_str()," ");
    }catch(const std::exception& ex){
        LOG_OUT();
        POST_LOG(caller.c_str()," ");
        std::string err_msg = "!!! " +std::string(__PRETTY_FUNCTION__) + _(" Cannot be created !!!\n => Cannot load : ") + filename + "\n" + _("Reason ") + ex.what();
        throw std::runtime_error(err_msg);
    };
};
/* gx module as .la */
Gx_module::Gx_module(Glib::ustring filename, uint8_t index, Glib::ustring caller,char** argv,int argc) {
    PRE_LOG(caller.c_str(),""); 
	LOG_IN();
    mod.desc=caller;
    mod.name=filename;
    try{
        analyse_param(argv,argc);
        load(filename,index);
        LOG_OUT();
        POST_LOG(caller.c_str()," "); 
    }catch(const std::exception& ex){
        std::string err_msg = "!!! " +std::string(__PRETTY_FUNCTION__) + _(" Cannot be created !!!\n => Cannot load : ") + filename + "\n" + _("Reason ") + ex.what();
        LOG_OUT();
        POST_LOG(caller.c_str()," ");
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
    PRE_LOG(mod.name.c_str(),mod.desc.c_str());
	LOG_IN();
    if (gmodule){
        module_pointer.reset();
        delete gmodule;
    };
	LOG_OUT();
    POST_LOG(mod.name.c_str(),mod.desc.c_str());
};

/*** DIALOGS ***/
void Gx_module::set_param(){};
/* Select/Load File */
void Gx_module::OpenDialogFileSelect(std::function<void(Glib::RefPtr<Gio::File>)> funct){
    LOG_IN();
    try{
        if( dialog_file_select ){
            set_param();
            dialog_file_select->set_title("Select File");
            #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
                dialog_file_select->set_initial_folder(initial_folder_open);
                dialog_file_select->open( *(get_window()), [this,funct](const Glib::RefPtr<Gio::AsyncResult>& result ) {
                    try {
                        Glib::RefPtr<Gio::File> file = dialog_file_select->open_finish(result);
                        if (file) {
                            funct(file);
                            initial_folder_open = Gio::File::create_for_path(file->get_parent()->get_path());
                        };
                    } catch (const std::exception & ex) {
                        std::string err_msg = "From: " + std::string(__PRETTY_FUNCTION__)\
                        + "Reason: " + ex.what();
                        std::cerr << err_msg << std::endl;
                    };
                }); /* end dialog open function */
            #else
                dialog_file_select->set_transient_for(*(get_window()));
                slot_file_dialog_select = dialog_file_select->signal_response().connect([this,funct](int response) {
                    try {
                        if (response == Gtk::ResponseType::ACCEPT) {
                            auto file = dialog_file_select->get_file();
                            if (file) {
                                funct(file);
                                initial_folder_open= Gio::File::create_for_path(file->get_path());;
                            };
                        };
                        dialog_file_select->hide();
                        slot_dialog_file_select.disconnect();
                    } catch (const std::exception & ex) {
                        std::string err_msg = "From: " + std::string(__PRETTY_FUNCTION__)\
                        + "Reason: " + ex.what();
                        std::cerr << err_msg << std::endl;
                    };
                });
                dialog_file_select->show();
            #endif
        };
    }catch (const std::exception & ex) {
        std::string err_msg = "from: " + std::string(__PRETTY_FUNCTION__)\
        + "Reason: " + ex.what();
        std::cerr << err_msg << std::endl;
        //throw std::runtime_error(err_msg);
    };
    LOG_OUT();
};
void Gx_module::load_file_as_datastream(Glib::RefPtr<Gio::File> file, std::function<void(Glib::RefPtr<Gio::File>, Glib::ustring, Glib::ustring, unsigned int)> funct){
    LOG_IN();
    try {
        unsigned int file_size = (file->query_info(G_FILE_ATTRIBUTE_STANDARD_SIZE))->get_size();
        Glib::ustring filename = file->get_path();
        Glib::ustring file_base = filename.substr(0,filename.find_last_of("."));
        Glib::ustring file_name = file_base.substr( file_base.find_last_of("/")+1, file_base.length() );

        data_stream = Gio::DataInputStream::create(file->read());
        funct(file,file_name,file_base,file_size);
        if(!isStreamClosed(data_stream)){
            data_stream->close();
        };

    }catch(const std::exception& ex){
        Glib::ustring filename = (file->query_info(G_FILE_ATTRIBUTE_STANDARD_NAME))->get_name();
        std::string err_msg = "from: " + std::string(__PRETTY_FUNCTION__)\
        + "Cannot load: " + filename + "\n"
        + "Reason: " + ex.what();
        throw std::runtime_error(err_msg);
    };
    LOG_OUT();
};
bool Gx_module::isStreamClosed(Glib::RefPtr<Gio::DataInputStream>& data_stream) {
    try{
        if( data_stream ){  return false;
        }else{              return true;
        };
    }catch( const Gio::Error& e ){
        //if( e.code() == Gio::Error::CLOSED ){ return true; };
        return true;
    };
};
/* Select/Save File */
void Gx_module::OpenDialogFileSave(std::function<void(Glib::RefPtr<Gio::File>)> funct){
    try{
        if(dialog_file_save){
            set_param();
            #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
                try{
                    dialog_file_save->set_initial_folder(initial_folder_save);
                    dialog_file_save->save(*(get_window()), [this,funct](const Glib::RefPtr<Gio::AsyncResult>& result) {
                        try{
                            Glib::RefPtr<Gio::File> file = dialog_file_save->save_finish(result);
                            if( file ){
                                std::cout << "Writing file: " << file->get_path() << std::endl;
                                initial_folder_save = Gio::File::create_for_path(file->get_parent()->get_path());
                                funct(file);
                            };
                        }catch( const std::exception& ex ){
                            std::string err_msg = "From: " + std::string(__PRETTY_FUNCTION__) +
                            " Reason: " + ex.what();
                            std::cerr << err_msg << std::endl;
                        };
                    });
                }catch( const std::exception& ex ){
                    std::string err_msg = "From: " + std::string(__PRETTY_FUNCTION__) +
                    " Reason: " + ex.what();
                    std::cerr << err_msg << std::endl;
                };
            #else
                dialog_file_save->set_transient_for(*(get_window()));
                dialog_file_save->set_current_folder(initial_folder_save);
                dialog_file_save->signal_response().connect([this,funct](int response) {
                    try {
                        if (response == Gtk::ResponseType::ACCEPT) {
                            auto file = dialog_file_save->get_file();
                            if (file) {
                                funct(file);
                            };
                        }
                        dialog_file_save->hide();
                    } catch (const std::exception & ex) {
                        std::string err_msg = "From: " + std::string(__PRETTY_FUNCTION__)\
                        + "Reason: " + ex.what();
                        std::cerr << err_msg << std::endl;
                    };
                });
                dialog_file_save->show();
            #endif
        };
    }catch( std::exception& ex){
        std::string err_msg = "From: " + std::string(__PRETTY_FUNCTION__) +
        " Reason: " + ex.what();
        std::cerr << err_msg << std::endl;
    };
};
void Gx_module::write_file_as_datastream(Glib::RefPtr<Gio::File> file, unsigned char* msg, unsigned int msg_size){
    auto output_stream = file->replace();
    auto data_stream = Gio::DataOutputStream::create(output_stream);
    for( unsigned int i=0; i<msg_size; i++ ){
        data_stream->put_byte(msg[i]);
    };
    data_stream->flush();
    data_stream->close();
    output_stream->close();
};
/* Parameters */
void Gx_module::OpenDialogParam(Glib::ustring title){
    LOG_IN();
    try{
        if(dialog_param){
            dialog_param->set_transient_for(*(get_window()));
            dialog_param->set_title(title);
            set_dialog(title);
            dialog_param->present();
        };
    }catch (const std::exception & ex) {
        std::string err_msg = "From: " + std::string(__PRETTY_FUNCTION__) +
        " Reason: " + ex.what();
        std::cerr << err_msg << std::endl;
        //throw std::runtime_error(err_msg);
    };
    LOG_OUT();
};
void Gx_module::set_dialog(Glib::ustring title){};


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

/*** MODULE ***/
void Gx_module::analyse_param(char** argv, int argc){
    for ( int i = 1; i <= argc; i++) {
        if ( (argv[i] != NULL) && ( Glib::ustring(argv[i]) == "-c" || (Glib::ustring(argv[i]) == "--color") )
            && (argv[i+1] != NULL) && ( Glib::ustring(argv[i+1]) != "" )
        ){  // -c and html color code as argument
            mod.color = Glib::ustring(argv[i+1]);
        };
    };
};
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
bool Gx_module::set_refxml(Glib::ustring filename)	{
    LOG_IN();
	try { 
	    refXml = Gtk::Builder::create_from_file(filename);
	    LOG_OUT();
		return true;
	}catch (const std::exception& ex){
        std::string err_msg = "from: " + std::string(__PRETTY_FUNCTION__)\
        + "\nSet refXml failed with error : " + ex.what();
        LOG_OUT();
        throw std::runtime_error(err_msg);
		return false;
	};
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
bool Gx_module::load_ui(Glib::ustring filename, uint8_t index){
    LOG_IN();
    try{
        /* TODO: maybe throw lower part */
        if(set_refxml(filename)){
            rootbox=refXml->get_widget<Gtk::Box>("box_main");
            if(rootbox){
                if( (rootbox)->get_parent() != nullptr ){     // if the box_main widget has a parent
                    if( "interface.ui" != filename.substr(filename.find_last_of("/")+1, filename.length()) ){
                        // box_main cannot be attached reset and go
                        // TODO: unparent and attach
                        std::cerr<< "box_main get a parent cannot be include" << std::endl;
                        rootbox = nullptr;
                    }else{
                        main_window = get_gwidget<Gtk::Window>("window_main");
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
		        auto [mod_pointer, mod_options] = module_func(index);
                set_module_options(mod_options);
                module_pointer = mod_pointer;
                rootbox = std::static_pointer_cast<Gx_module>(module_pointer)->get_rootbox();
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
void Gx_module::set_module_options(St_mod_options mod_options){
    set_style_file(mod_options.cssfile);
    set_custom_font_file(mod_options.custom_font);
    set_icon_file(mod_options.icon);
}
Gx_module::St_mod_options Gx_module::get_module_options(){
    return mod_options;
};
/* Widgets */
void Gx_module::set_main_window(Gtk::Window* window){ // set main window on a module
    LOG_IN();
    main_window=window;
    LOG_OUT();
};
Gtk::Window* Gx_module::get_window(){
    return main_window;
};
Gtk::Box* Gx_module::get_rootbox() 	{
    return rootbox;
};

/*** CSS STYLE ***/
/* set the CSS style file provided*/
void Gx_module::set_style_file(Glib::ustring file_css){
    LOG_IN();
    if( std::filesystem::exists(file_css.c_str()) ){
        mod.cssfile=file_css;
        std::cout << _("Css style file: ")<< mod.cssfile << std::endl;
    }else{
        std::cerr<< _("Warning: the css style file ")<< file_css << _(" doesn't exist or is not readable")<< std::endl;
        mod.cssfile="";
    };
    LOG_OUT();
};
/** Apply style **/
void Gx_module::apply_style_to_screen(){
    LOG_IN();
    //clear_style_of_window(main_window);
    //apply_style_to<Gtk::Window>(main_window);
    try{
        load_custom_font();
        /* Prepare for clear or dark theme
        auto settings = Gtk::Settings::get_default();
        auto giosettings = Gio::Settings::create("org.gnome.desktop.interface");

        giosettings->signal_changed().connect([mod.cssfile](const Glib::ustring& key) {
            if (key == "gtk-theme" || key == "color-scheme") {
                auto theme = settings->get_string("gtk-theme");
                auto color_scheme = settings->get_string("color-scheme");
                std::cout << "Theme changed! gtk-theme: " << theme
                << " color-scheme: " << color_scheme << std::endl;
                // Here, update your CSS or UI as needed
            }
        });*/
        auto custom_provider = Gtk::CssProvider::create();
        auto css = Gtk::CssProvider::create();
        if(mod.cssfile != ""){
            css->load_from_path(mod.cssfile);
        };
        auto display = Gdk::Display::get_default();
        if (display) {
            #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
                Gtk::StyleProvider::add_provider_for_display(display, css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            #else
                Gtk::StyleContext::add_provider_for_display(display, css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            #endif
            if(mod.color != ""){
                #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
                        custom_provider->load_from_string("title { background-color: " + mod.color + "; }");
                        Gtk::StyleProvider::add_provider_for_display(display, custom_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
                #else
                        custom_provider->load_from_data("title { background-color: " + mod.color + "; }");
                        Gtk::StyleContext::add_provider_for_display(display, custom_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
                #endif
            };
        };
    } catch (const std::exception& ex) {
        std::string err_msg = "!!! " +std::string(__PRETTY_FUNCTION__) + _(" Failed to load style: !!!\n") + mod.cssfile + "\n" + _("Reason => ") + ex.what();
        LOG_OUT();
        throw std::runtime_error(err_msg);
    };
    LOG_OUT();
};
/*template <class widgetType>
void Gx_module::apply_style_to(widgetType* widget){
    // TODO: clear style before apply a new one 
    LOG_IN();
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
};*/
/** Clear Style **/
/*void Gx_module::clear_style_for_screen(Glib::RefPtr<Gtk::StyleProvider> css_provider){
};*/
void Gx_module::clear_style_of_window(Gtk::Window* window){
    // clear all class of a window
};
template <class widgetType>
void Gx_module::clear_style_of(widgetType* widget){
    // clear all class style of a widget
    // TODO: remove a specific class
    auto style_ctx = widget->get_style_context();
    auto class_list = style_ctx->list_classes();
    for (const auto& class_name : class_list) {
        style_ctx->remove_class(class_name);
    };
};

/*** FONTS ***/
void Gx_module::set_custom_font_file(Glib::ustring custom_font_file){
    LOG_IN();
    if( std::filesystem::exists(custom_font_file.c_str()) ){
        mod.custom_font=custom_font_file;
        std::cout << _("Custom font file: ")<< mod.custom_font << std::endl;
    }else{
        std::cerr<< _("Warning: the custom font file ")<< custom_font_file << _(" doesn't exist or is not readable")<< std::endl;
        mod.custom_font="";
    };
    LOG_OUT();
};
void Gx_module::load_custom_font() {
    /*
     * Function to load a custom font into Fontconfig
     * load_font_into_fontconfig: add font to fontconfig (working)
     * load_font_into_pango: add font to pango (not working)
     */
    if(mod.custom_font != ""){
        try{
            //FcConfig* config = load_font_into_fontconfig(font_path);
            add_custom_font(mod.custom_font);
            load_font_into_pango();
            //if(config){
            //    FcConfigDestroy(config);
            //};
        }catch(const std::exception& ex){
            std::cerr << _("Error: ") + std::string(__PRETTY_FUNCTION__) +" -> " << ex.what() << std::endl;
            LOG_OUT();
        };
    };
};
/** Pango **/
void Gx_module::load_font_into_pango() {  // Function to make the custom font available in Pango
    std::filesystem::path full_path;
    try{
        auto font_map = Pango::CairoFontMap::get_default();
        full_path = std::filesystem::absolute(mod.custom_font);
        if( !font_map->add_font_file(full_path.string()) ){
            std::cerr << "Failed add font: " << full_path.string() << std::endl;
            //list_pango_fonts();
        };
    }catch(const std::exception & ex){
        std::string err_msg = "!!! " +std::string(__PRETTY_FUNCTION__) + _(" Failed load font: !!!\n") + full_path.string() + "\n" + _("Reason => ") + ex.what();
        LOG_OUT();
        throw std::runtime_error(err_msg);
    };
};
void Gx_module::list_pango_fonts(){       // List all available font families
    try{
        auto families = ((Pango::CairoFontMap::get_default())->create_context())->list_families();
        for (const auto& family : families) {
            std::cerr << "  - " << family->get_name() << std::endl;
        };
    }catch(const std::exception & ex){
        std::cerr << _("Fail to list pango font with error: ") << ex.what() << std::endl;
    };
};
bool Gx_module::is_font_present(Glib::ustring font_name){
    bool found = false;
    auto families = ((Pango::CairoFontMap::get_default())->create_context())->list_families();
    for (const auto& family : families) {
        if (family->get_name() == font_name) {
            found=true;
            break;
        };
    };
    return found;
};
/** Windows **/
void Gx_module::add_custom_font(const std::string& font_path) {
    /*
     * Function to load a custom font into Fontconfig
     * load_font_into_fontconfig: add font to fontconfig (working)
     * load_font_into_pango: add font to pango (not working)
     */
    //FcConfig* config = load_font_into_fontconfig(font_path);
    //load_font_into_pango(config, "Raster Fonts 6x8");
    //if(config){
    //    FcConfigDestroy(config);
    //};
    #if(defined(__WIN32) || defined(__MINGW32__))
        try {
            std::filesystem::path full_path = std::filesystem::absolute(font_path); // 1. Convert relative path to absolute using current working directory
            std::wstring wfont_path = full_path.wstring();  // 2. Convert to Windows-native wide string (UTF-16)
            std::wcout << L"Adding font to application: " << wfont_path << std::endl;
            if (AddFontResourceExW(wfont_path.c_str(), FR_PRIVATE, 0) == 0) {
                DWORD error_code = GetLastError();
                std::error_code ec(error_code, std::system_category());             // Convert error code to human-readable message
                std::string error_msg = ec.message();                               // Or use FormatMessage (below)
                fprintf(stderr, "Font load failed (Error %lu): %s\nPath: %s", error_code, error_msg.c_str(), wfont_path.c_str());
                LPSTR message_buffer = nullptr;
                FormatMessageA(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    nullptr, error_code, 0,
                    reinterpret_cast<LPSTR>(&message_buffer), 0, nullptr
                );
                if (message_buffer) {
                    MessageBoxA(NULL, message_buffer, "Font Load Error", MB_ICONERROR);
                    LocalFree(message_buffer);
                } else {
                    MessageBoxW(NULL, L"Unknown font loading error", L"Error", MB_ICONERROR);
                };
            }else{
                SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);  // System-wide notification
                std::cout << "Font Loaded: " << full_path << std::endl;
            };
            // DEBUG GDI FONT CONFIG
            /* LOGFONT lf = {};
            lf.lfCharSet = DEFAULT_CHARSET;
            lstrcpy(lf.lfFaceName, "Raster Fonts 6x8");

            HDC hdc = GetDC(NULL);
            EnumFontFamiliesEx(hdc, &lf, [](const LOGFONT* lf, const TEXTMETRIC*, DWORD, LPARAM) -> int {
                std::cout << "Found font: " << lf->lfFaceName << "\n";
                return 1;
            },   0, 0);*/

        } catch (const std::exception & ex) {
            MessageBoxA(NULL, ex.what(), "Filesystem Error", MB_ICONERROR);
        };
    #endif
};
/** Fontconfig **/
FcConfig* Gx_module::load_font_into_fontconfig(const std::string& font_path) {
    // Check if the font file exists
    if( !std::filesystem::exists(font_path) ){
        std::cerr << "Font file does not exist: " << font_path << std::endl;
        return nullptr;
    };

    // Create a custom Fontconfig configuration
    FcConfig* config = FcConfigCreate();
    if( !config ){
        std::cerr << "Failed to create Fontconfig configuration." << std::endl;
        return nullptr;
    };

    // Add the custom font file to Fontconfig
    if( !FcConfigAppFontAddFile(config, reinterpret_cast<const FcChar8*>(font_path.c_str())) ){
        std::cerr << "Failed to add custom font: " << font_path << std::endl;
        FcConfigDestroy(config);
        return nullptr;
    }else{
        std::cout << "Successfully added custom font: " << font_path << std::endl;
    };

    // Verify that the custom font is registered in Fontconfig
    FcPattern* pattern = FcPatternCreate();
    FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<const FcChar8*>("Raster Fonts 6x8"));  // Replace with your font family name
    FcObjectSet* objectSet = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, nullptr);

    FcFontSet* matched_fonts = FcFontList(config, pattern, objectSet);
    if( matched_fonts ){
        std::cout << "Matched Fonts:" << std::endl;
        for( int i = 0; i < matched_fonts->nfont; ++i ){
            FcPattern* font = matched_fonts->fonts[i];
            char* family = nullptr;
            char* style = nullptr;
            char* file = nullptr;
            if( FcPatternGetString(font, FC_FAMILY, 0, reinterpret_cast<FcChar8**>(&family)) == FcResultMatch &&
                FcPatternGetString(font, FC_STYLE, 0, reinterpret_cast<FcChar8**>(&style)) == FcResultMatch &&
                FcPatternGetString(font, FC_FILE, 0, reinterpret_cast<FcChar8**>(&file)) == FcResultMatch ){
                    std::cout << "Family: " << family << ", Style: " << style << ", File: " << file << std::endl;
            }else{
                    std::cerr << "Failed to retrieve properties for a matched font." << std::endl;
            };
        };
        FcFontSetDestroy(matched_fonts);
    };
    return config;  // Return the custom configuration for use in Pango
};

/*** SIGNAL ***/
void Gx_module::attach_signals(){};
void Gx_module::dettach_signals(){};

/*** MOD ***/
/* Add/remove */
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
void Gx_module::unset_mod()	{ LOG_IN(); 
	/*dettach_signals();
	if ( mod.extpath.ext == "la" ){
		delete module;
	};*/
    LOG_OUT(); 
};
/* extpath */
void Gx_module::set_module_name(Glib::ustring name) { mod.extpath.name = name; };	
Glib::ustring  Gx_module::get_name(){	return mod.extpath.name ; };
Glib::ustring  Gx_module::get_file(){	return mod.extpath.file ; };
Glib::ustring  Gx_module::get_ext(){	return mod.extpath.ext ; };
Glib::ustring  Gx_module::get_filename(){	return mod.extpath.filename ; };
Glib::ustring  Gx_module::get_path(){	return mod.extpath.path ; };

/*** APP ***/
/* mod */
/* Set icon */
void Gx_module::set_icon_file(Glib::ustring icon_file){
    LOG_IN();
    if( icon_file != ""){
        if( std::filesystem::exists(icon_file.c_str()) ){
            mod_options.icon=icon_file;
            std::cout << _("Icon file: ")<< mod_options.icon << std::endl;
        }else{
            std::cerr<< _("Warning: the icon file ")<< icon_file << _(" doesn't exist or is not readable")<< std::endl;
            mod_options.icon="";
        };
    };
    LOG_OUT();
};
void Gx_module::set_app_icon(Gtk::Window* main_window){ // UNUSED
    // only for the windows icon on title, doesn't change the panel icon
    if( mod_options.icon != "" ){
        auto icon_theme = Gtk::IconTheme::get_for_display(Gdk::Display::get_default());
        int start = mod_options.icon.find_last_of(DS);
        int end = mod_options.icon.find_last_of(".");
        Glib::ustring icon_path = mod_options.icon.substr( 0, start );
        Glib::ustring icon_name = mod_options.icon.substr( start+1 , end - (start+1) );
        icon_theme->add_search_path( icon_path );
        if (icon_theme->has_icon(icon_name)) {
            main_window->set_default_icon_name(icon_name);
            main_window->set_icon_name(icon_name);
        }else{
            std::cout << "No icon in theme have this name: "<< icon_name << std::endl ;
        };
    };
};
/* set name */
void Gx_module::set_app_name(Glib::ustring name){
    if(main_window){
      main_window->set_title(name);
    };
    mod.name=name;
};
void Gx_module::set_app_name(unsigned int mod_index){
    set_app_name(get_app_name() + " (" + tostr<unsigned int>(mod_index) +")");
};
Glib::ustring Gx_module::get_app_name(){ return mod.name ; };
Glib::ustring  Gx_module::get_type(){	return mod.type ; };
Glib::ustring  Gx_module::get_cat(){	return mod.cat ; };
Glib::ustring  Gx_module::get_desc(){	return mod.desc ; };

#endif  /* gxmod_CC */
