/* ----------------------------------------------------------------------------
 * gxinterface -- Gtk+ Extended Interface
 * Main source
 *
 * ----------------------------------------------------------------------------
 * copyright © 2006, 2007, 2008, 2009, 2010 Jérôme BENHAÏM <benhaimjerome@gmail.com>
 *
 * ----------------------------------------------------------------------------
 *
 *   This program is free software: you can redistribute it and/or modify
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

/* TODO : change interface selector to getops :p */
#include "main.h"

int main (int argc, char *argv[]){
    LOG_IN ();
    setlocale (LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE,PROGRAMNAME_LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
    //gchar* filename;
    char interface='g';
    /* test param */

    for ( int i = 0 ; i < argc ; i++){
        if ( (argc > 1) && std::string(argv[i]) == "-u" && (argv[i+1] != NULL) && (std::string(argv[i+1]) != "") ){
            interface=gchar(argv[i+1][0]);
            std::cout << "interface mode : " << interface << std::endl;
            break;
        };
    };
    if ( interface == 0 ){
        interface='g';
        std::cout << "interface mode : " << interface << std::endl;
    };
    switch (interface) {
                // choose interface gnome kde x11 ..
                case 'g' :
                    try{
                        //Gx_interface *g_app;
                        //auto app = Gtk::Application::create("");
                        auto g_app = Gx_interface::create();
                        //g_app = new Gx_interface(argc, argv);
                        return g_app->run(argc, argv);
                        //delete g_app;
                    }catch(const std::exception& ex){
                        std::cerr << ex.what() << std::endl;
                        //std::cerr << ex.domain() << std::endl;
                        //std::cerr << ex.code() << std::endl;
                        LOG_OUT();
                        return 1;
                    };
                break;
                /*case 'k' :
                    Kinterface *k_app_interface;
                    k_app_interface = new Kinterface(argc, argv);
                    delete k_app_interface;
                break;
                case 'x' :
                    Xinterface *x_app_interface;
                    x_app_interface = new Xinterface(argc, argv);
                    delete x_app_interface;
                break;
                case 'w' :
                    Winterface *w_app_interface;
                    w_app_interface = new Winterface(argc, argv);
                    delete w_app_interface;
                break;
                case 'p' :
                    Pinterface *p_app_interface;
                    p_app_interface = new Pinterface(argc, argv);
                    delete p_app_interface;
                break;*/
                default :
                    std::cout << "L'option : "<< interface << " pour une interface n'est pas reconnue." << std::endl;
                break;
};
  LOG_OUT ();
  return 0;
};


