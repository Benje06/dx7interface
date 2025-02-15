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
/*    // Create and show your main window here
    auto window = new Gtk::ApplicationWindow();
    window->set_default_size(400, 300);
    add_window(*window);
    window->show();
*/
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
        if ( (argv[i] != NULL) && (Glib::ustring(argv[i]) == "-i")
        && (argv[i+1] != NULL) && (Glib::ustring(argv[i+1]) != "") ){  // -f and name as argument
            FILE *file = fopen(argv[i+1],"r");
            if ( file == NULL ) {						// try open fil
                iname=UI_FILE;
                std::cout << "!!! Le fichier d'interface "<<argv[i+1]<<" n'existe pas !!!" << std::endl;
                std::cout << "Chargement du fichier d'interface par defaut. " << std::endl;
            }else{
            iname=Glib::ustring(argv[i+1]);
            fclose(file);
            };
        };
        if ( (argv[i] != NULL) && (Glib::ustring(argv[i]) == "-m")
        && (argv[i+1] != NULL) && (Glib::ustring(argv[i+1]) != "") ){  // -m and name as argument
            FILE *file = fopen(argv[i+1],"r");
            if ( file == NULL) {						// try open fil
                iname=UI_FILE;
                std::cout << "!!! Le module " << argv[i+1] << " n'existe pas !!!" << std::endl;
                std::cout << "Chargement du fichier d'interface par defaut. " << std::endl;
            }else{
            itype="module";
            iname=Glib::ustring(argv[i+1]);
            std::cout << "Chargement d'un module." <<std::endl;
            fclose(file);
            };
        };
    };
    std::cout << "Chargement du fichier: "<< iname <<" ."<<std::endl;
    LOG_OUT();
    activate();
    return 0;
    //return Gtk::Application::on_command_line(command_line);
};

//Glib::ustring Gx_interface::get_itype(){ LOG_IN(); return itype; LOG_OUT(); };

