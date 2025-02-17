/* ----------------------------------------------------------------------------
* gxinterface -- Gtk+ Extended Interface
* GxInterface source
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
#include "gxinterface.h"
/* class Ge_interface */
Gx_interface::Gx_interface(): Gtk::Application("", Gio::Application::Flags::HANDLES_COMMAND_LINE) {
    LOG_IN();
    try{
        #ifdef ENABLE_NLS
            std::locale::global(std::locale(""));
            bindtextdomain(GETTEXT_PACKAGE,PROGRAMNAME_LOCALEDIR);
            bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
            textdomain(GETTEXT_PACKAGE);
        #endif
        /* init of error code */
        error = NULL;
        /* analyse of command line parameters */
        signal_command_line().connect(sigc::mem_fun(*this, &Gx_interface::on_command_line), false);
        //setup_log_handlers ();
        /* interface init */
    }catch(const std::exception& ex){
        LOG_OUT();
        throw;
    };
    LOG_OUT();
};

Gx_interface::~Gx_interface(){
    LOG_IN();
    delete module_manager;
    LOG_OUT();
};

Glib::RefPtr<Gx_interface> Gx_interface::create(){
    LOG_IN();
    return Glib::make_refptr_for_instance<Gx_interface>(new Gx_interface());
    LOG_OUT();
};

void Gx_interface::on_activate(){
    LOG_IN();
    try{
        if( itype == Glib::ustring("interface")) {
            module_manager=new Gemod(iname);
            if(module_manager){
                if(module_manager->get_main()){
                    add_window(*module_manager->get_main());
                    (*module_manager->get_main()).set_default_size(1024, 768);
                    (*module_manager->get_main()).set_title(module_manager->get_app_name());
                    (*module_manager->get_main()).set_visible(true);
                };
            };
        };
        if(itype == Glib::ustring("module")) {
            // Create your window here
            module_manager=new Gemod(iname,0);
            if(module_manager){
                if(module_manager->get_window()){
                    add_window(*module_manager->get_window());
                    (*module_manager->get_window()).set_default_size(1024,768);
                    (*module_manager->get_window()).set_title(module_manager->get_app_name());
                    (*module_manager->get_window()).set_visible(true);
                };
            };
        };
    }catch(const std::exception& ex){
        std::cerr << _("Error: in application activate -> ") << ex.what() << std::endl;
    };
    LOG_OUT();
};


int Gx_interface::on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line){
    LOG_IN();
    int argc;
    char** argv = command_line->get_arguments(argc);
    int i;
    /* TODO: use C++ getopts */
    /* analyse argument of command line */
    itype="interface";
    iname=UI_FILE;
    for ( i = 1; i <= argc; i++) {
        if ( (argv[i] != NULL) && ( Glib::ustring(argv[i]) == "-i" || (Glib::ustring(argv[i]) == "-m") )
        && (argv[i+1] != NULL) && ( Glib::ustring(argv[i+1]) != "" )
        ){  // -i and interface filename as argument
            FILE *file = fopen(argv[i+1],"r");
            if ( file == NULL ) {						// try open fil
                iname=UI_FILE;
                if ( Glib::ustring(argv[i]) == "-m" ){
                    std::cout << _("!!! Module ") << argv[i+1] << _(" doesn't exist or could not be read !!!") << std::endl;
                }else{
                    std::cout << _("!!! Interface file ") << argv[i+1] << _(" does not exist or could not be read !!!") << std::endl;
                };
                std::cout << _("Loading default interface file.") << std::endl;
            }else{
                if ( Glib::ustring(argv[i]) == "-m" ){
                    itype="module";
                    std::cout << _("Loading module.") <<std::endl;
                };
                iname=Glib::ustring(argv[i+1]);
                fclose(file);
            };
        };
    };
    std::cout << _("Loading file: ")<< iname <<std::endl;
    LOG_OUT();
    activate();
    return 0;
};
