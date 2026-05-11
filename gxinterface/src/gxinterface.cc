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
    LOG(LOG_IN());
    try{
        init_nls();
        /* analyse of command line parameters */
        signal_command_line().connect(sigc::mem_fun(*this, &Gx_interface::on_command_line), false);
        /* TODO : set log handler */
        //setup_log_handlers();
    }catch(const std::exception& ex){
        msg_err = error( __PRETTY_FUNCTION__, _("Failed to construct gxinterface"), ex.what() );
        LOG_ERR( msg_err );
        LOG(LOG_OUT());
        throw;
    };
    LOG(LOG_OUT());
};

Gx_interface::~Gx_interface(){
    LOG(LOG_IN());
    if (argv) {
        g_strfreev(argv);
        argv = nullptr;
    };
    delete module_manager;
    LOG(LOG_OUT());
};

Glib::RefPtr<Gx_interface> Gx_interface::create(){
    LOG(LOG_IN());
    return Glib::make_refptr_for_instance<Gx_interface>(new Gx_interface());
    LOG(LOG_OUT());
};

void Gx_interface::on_activate(){
    LOG(LOG_IN());
    try{
        if( itype == Glib::ustring("interface" )) {
            module_manager=new Gemod(iname);
        }else if( itype == Glib::ustring("module") ) {
            module_manager=new Gemod(iname,0,argv,argc);
        };
        if(module_manager){
            add_window(*module_manager->get_window());
            (*module_manager->get_window()).set_default_size(1024, 768);
            (*module_manager->get_window()).set_title(module_manager->get_app_name());
            (*module_manager->get_window()).set_visible(true);
        };
    }catch(const std::exception& ex){
        msg_err = error( __PRETTY_FUNCTION__, _("Error: in application activate -> "), ex.what() );
        LOG_ERR( msg_err );
    };
    LOG(LOG_OUT());
};

int Gx_interface::on_command_line(const Glib::RefPtr<Gio::ApplicationCommandLine>& command_line){
    LOG(LOG_IN());
    itype = "interface";
    iname = UI_FILE;
    bool i_set = false;
    bool m_set = false;
    bool help_requested = false;
    std::string path;
    int opt;
    std::vector<char*> argv_copy(argc + 1);

    static const std::vector<help_options> opts = {
        { 'i', "interface", "FILE",
            { _("Specific interface file (.ui) to be loaded") }
        },
        { 'm', "module",    "FILE",
            { _("load a module (.dll/.so/.la) and run it") }
        }
    };
    static const struct option long_options[] = {
        { "interface", required_argument, nullptr, 'i' },
        { "module",    required_argument, nullptr, 'm' },
        { "help",      no_argument,       nullptr, 'h' },
        { nullptr,     0,                 nullptr,  0  }
    };

    try{
        /* Free previous argv if reentry (e.g. DBus reactivation, second instance) */
        if (argv) {
            g_strfreev(argv);
            argv = nullptr;
        };
        argv = command_line->get_arguments(argc);
        argv_copy.resize(argc + 1);

        for(int i = 0; i < argc; ++i) {
            argv_copy[i] = strdup(argv[i]);   // deep copy
        }
        argv_copy[argc] = nullptr;

        /* Phase 1: parse and collect */
        optind = 0;   /* reentrancy: full reset of getopt internal state */
        opterr = 0;   /* silence getopt's own stderr; we log via LogManager */
        while ((opt = getopt_long(argc, argv_copy.data(), "i:m:h", long_options, nullptr)) != -1) {
            switch (opt) {
                case 'i':
                    i_set = true;
                    if (optarg) path = optarg;
                    break;
                case 'm':
                    m_set = true;
                    if (optarg) path = optarg;
                    break;
                case 'h':
                    help_requested = true;
                    break;
                case '?':
                default:
                    /* Unknown option or missing argument: ignore here so that
                    * options targeted at main or the module are preserved
                    * for them to parse later. */
                    break;
            };
        };
        // Free memory
        for(size_t i = 0; i < argv_copy.size(); ++i){
            free(argv_copy[i]);
            argv_copy[i] = nullptr;
        }
        /* Phase 2: apply effects */
        /* mutual exclusion: -i and -m cannot be used together */
        if (i_set && m_set) {
            msg_err = error( __PRETTY_FUNCTION__,
                            _("Conflicting options"),
                            _("-i and -m are mutually exclusive") );
            LOG_ERR( msg_err );
            LOG( help_format(argv[0], opts) );
            LOG(LOG_OUT());
            return 1;
        };
        /* unified file load + fallback for -i / -m */
        if (i_set || m_set) {
            FILE* file = fopen(path.c_str(), "r");
            if (file == NULL) {
                iname = UI_FILE;
                if (m_set) {
                    msg_log= _("Module ") + path + _(" doesn't exist or could not be read !!!");
                } else {
                    msg_log= _("Interface file ") + path + _(" doesn't exist or could not be read !!!");
                };
                LOG( msg_log);
                msg_log= _("Loading default interface file.");
                LOG( msg_log);
            } else {
                if (m_set) {
                    itype = "module";
                    msg_log= _("Loading module.");
                    LOG( msg_log);
                };
                iname = path;
                fclose(file);
            };
        };
        if (help_requested) {
            LOG( help_format(argv[0], opts) );
            if (!m_set) {
                LOG(LOG_OUT());
                return 0;
            };
        };
        msg_log= _("Loading file: ") + iname;
        LOG( msg_log);
        LOG(LOG_OUT());
        activate();
        return 0;
    }catch(const std::exception& ex){
        for(size_t i = 0; i < argv_copy.size(); ++i){
            free(argv_copy[i]);
            argv_copy[i] = nullptr;
        }
        msg_err = error( __PRETTY_FUNCTION__ , _("The analyse of command line parameter failed.") , ex.what());
        LOG_ERR( msg_err );
        LOG(LOG_OUT());
        throw std::runtime_error(msg_err);
		return 1;
    };    
};
