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
    #ifdef ENABLE_NLS
        //std::setlocale(LC_ALL, "");
       // std::locale::global(std::locale(""));
        textdomain(GETTEXT_PACKAGE);
        bindtextdomain(GETTEXT_PACKAGE, PROGRAMNAME_LOCALEDIR);
        bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    #endif

    char interface=0;
    /* check params */
    for ( int i = 0 ; i < argc ; i++){
        if ( (argc > 1) && std::string(argv[i]) == "-u" && (argv[i+1] != NULL) && (std::string(argv[i+1]) != "") ){
            interface=gchar(argv[i+1][0]);
            std::cout << _("Interface graphic mode : ") << interface << std::endl;
            break;
        };
    };
    if ( interface == 0 ){
        interface='g'; //force only supported mode
        std::cout << _("Interface graphic mode : ") << interface << std::endl;
    };
    try{
        switch (interface) {
                    // choose interface gnome kde x11 ..
                    case 'g' :{
                            auto g_app = Gx_interface::create();
                            return g_app->run(argc, argv);
                            break;
                    };
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
                    default:{
                            std::cout << _("The option : ")<< interface << _(" is not valid for an interface type") << std::endl;
                        break;
                    };
        };
    }catch(const std::exception& ex){
        std::cerr << _("Error: in application start -> ") << ex.what() << std::endl;
        LOG_OUT();
        return 1;
    };
    LOG_OUT ();
    return 0;
};
