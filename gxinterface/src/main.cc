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
    // Initialize logging system
    LogManager::instance().add_handler(
        std::make_shared<Logger>( "app.log", 1 )
    );
    /* 
     *   Debug LOG manager
     *   auto& lm = LogManager::instance();
     *   std::cout << "LogManager address (main): " << &lm << std::endl;
     */
    LOG("*************************** Starting ***************************** ");
    LOG("\t" + get_time());
    LOG("****************************************************************** ");

    LOG(LOG_IN());
    init_nls();
    char interface=0;
    std::string msg, error_msg;
    /* check params */
    for ( int i = 0 ; i < argc ; i++){
        if ( (argc > 1) && std::string(argv[i]) == "-u" && (argv[i+1] != NULL) && (std::string(argv[i+1]) != "") ){
            interface=gchar(argv[i+1][0]);
            msg = _("Interface graphic mode : ") + interface;
            LOG( msg );
        };
    };
    if ( interface == 0 ){
        interface='g'; //force only supported mode
        std::string msg = _("Interface graphic mode : ") + interface;
        LOG( msg );
    };
    try{
        switch (interface) {
                    // choose interface gnome kde x11 ..
                    case 'g' :{
                            auto g_app = Gx_interface::create();
                            return g_app->run(argc, argv);
                            break;
                    };
                    default:{
                        std::string msg = _("The option : ") + interface + _(" is not valid for an interface type");
                        LOG(msg);
                        break;
                    };
        };
    }catch(const std::exception& ex){
        err_msg = error( __PRETTY_FUNCTION__, _("Error: in application start -> ") , ex.what() );
        LOG_ERR( err_msg );
        LOG(LOG_OUT());
        return 1;
    };
    LOG(LOG_OUT());
    return 0;
};
