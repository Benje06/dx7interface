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

#include "main.h"
#if(defined(__WIN32) || defined(__MINGW32__))
    #include <gtk/gtk.h>
#endif
int main (int argc, char *argv[]){
    char interface='g'; // Unic existant mode g as gtk
    std::string msg="", err_msg="";
    std::string log_file="gxinterface.log";
    unsigned int log_lvl=2;
    bool log_lvl_set=false;
    bool help_requested=false;
    // Initialize logging system
    #if(defined(__WIN32) || defined(__MINGW32__))
        const char* localAppData = std::getenv("LOCALAPPDATA");
        if (localAppData && *localAppData) {
            log_file = std::string(localAppData) + "\\gxinterface\\" + log_file;
        }
    #endif
    LogManager::instance().add_handler(
        std::make_shared<Logger>( log_file, log_lvl )
    );
    /* 
     *   Debug LOG manager
     *   auto& lm = LogManager::instance();
     *   std::cout << "LogManager address (main): " << &lm << std::endl;
     */
    LOG("**************************** Starting *****************************");
    LOG("\t\t\t\t\t\t" + get_time());
    LOG("*******************************************************************");

    LOG(LOG_IN());
    static const std::vector<help_options> opts = {
        { 'h', "help",      "",
            { _("display this help and exit") }
        },
        { 'g', "gui-mode",  "g",
            { _("graphic interface mode (supported: g, default: g)") }
        },
        { 'l', "log-level", "[0/1/2]",
            { _("log level: 0=no log, 1=log to file,"),
                _("2=log to file and console (default: 2)") }
        }
    };
    static const struct option long_options[] = {
        { "gui-mode",  required_argument, nullptr, 'g' },
        { "log-level", required_argument, nullptr, 'l' },
        { "help",      no_argument,       nullptr, 'h' },
        { nullptr,     0,                 nullptr,  0  }
    };
    /* check params */
    optind = 0;   /* reentrancy: full reset of getopt internal state */
    opterr = 0;   /* silence getopt's own stderr; we log via LogManager */

    std::vector<char*> argv_copy(argc + 1);
    try{
        init_nls();
        for(int i = 0; i < argc; ++i) {
            argv_copy[i] = strdup(argv[i]);   // deep copy
        }
        argv_copy[argc] = nullptr;

        int opt;
        while ((opt = getopt_long(argc, argv_copy.data(), "g:l:h", long_options, nullptr)) != -1) {
            switch (opt) {
                case 'g':
                    if (optarg && optarg[0] != '\0') {
                        interface = char(optarg[0]);
                    };
                    break;
                case 'l':
                    log_lvl = std::stoi(optarg);
                    log_lvl_set = true;
                    break;
                case 'h':
                    help_requested = true;
                    break;
                case '?':
                default:
                    /* Unknown option or missing argument: ignore here so that
                     * options targeted at gxinterface or the module are
                     * preserved for them to parse later. */
                    break;
            };
        };
        // Free memory
        for(int i = 0; i < argc; ++i) {
            free(argv_copy[i]);
            argv_copy[i] = nullptr;
        }
        /* apply parameters */
        if (log_lvl_set) {
            LogManager::instance().set_log_level(log_lvl);
        };
        msg = _("Interface graphic mode: ");
        msg += interface;
        LOG( msg );
        if (help_requested) {
            LOG( help_format(argv[0], opts) );
        };
        switch (interface) {
            // choose interface gnome kde x11 ..
            case 'g' :{
                #if defined(__WIN32) || defined(__MINGW32__)
                    gtk_disable_setlocale();
                #endif
                auto g_app = Gx_interface::create();
                return g_app->run(argc, argv);
                break;
            };
            default:{
                msg = _("The option: ");
                msg += interface;
                msg +=_(" is not valid for an interface type");
                LOG(msg);
                break;
            };
        };
    }catch(const std::exception& ex){
        for(size_t i = 0; i < argv_copy.size(); ++i){
            free(argv_copy[i]);
            argv_copy[i] = nullptr;
        }
        err_msg = error( __PRETTY_FUNCTION__, _("Error: in application start -> ") , ex.what() );
        LOG_ERR( err_msg );
        LOG(LOG_OUT());
        return 1;
    };
    LOG(LOG_OUT());
    return 0;
};
