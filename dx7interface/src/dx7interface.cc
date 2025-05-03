/* ----------------------------------------------------------------------------
 * Dx7interface.c -- DX7 Graphic interface
 * dx7 interface Headers                                             header
 * ----------------------------------------------------------------------------
 * copyright © 2006, 2007, 2008, 2009, 2010  Jérôme BENHAÏM <benhaimjerome@gmail.com>,
 *
 * ----------------------------------------------------------------------------
 * This program is free software ; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation ;
 * either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY ; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * ----------------------------------------------------------------------------
 */

#ifndef Dx7interface_CC
        #define Dx7interface_CC
        #include "dx7interface.h"

extern "C" {
    std::tuple<std::shared_ptr<void>, Gx_module::St_mod_options> LoadPlug(uint8_t index){
        auto dx7interface = std::make_shared<Dx7interface>(UI,index);
        return std::make_tuple(dx7interface, dx7interface->get_module_options());
    };
};

Dx7interface::Dx7interface(Glib::ustring ui, uint8_t index) :  Gx_module(ui,MODULE_NAME), Synth(MODULE_NAME) {
    /*basic constructor */
    /* 
     * Debug for logmanager
     * auto& lm = LogManager::instance();
     * std::cout << "LogManager address (main): " << &lm << std::endl;
     */
    LOG( LOG_IN() );
    block_midi();
    block_ui();
    init_nls();
    /* I/O init */
    Gio::init();
    if (index <= 0){
        set_app_name(MODULE_NAME);
    }else{
        set_app_name(MODULE_NAME+index);
    };
    action_type = ACT_OPEN;
    mod_options.cssfile = CSSFILE;
    mod_options.custom_font = FONT;
    mod_options.icon = ICON;
    /* MIDI */
    /* Yamaha specific */
    Synth::id_fabricant=id_fabricant;
    /* set channel & sub_status */
    Synth::channel_send=0x00;
    Synth::channel_receive=0x00;
    Synth::sub_status=0x10;
    #ifdef __linux__
        seq_handle=get_seq_handler();
        ev=get_seq_event_handler();
    #endif
    #if (defined(__WIN32) || defined(__MINGW32__) && defined(__RtMidi__))
    
    #endif
    /* UI */
    // mouse gesture for drawing area
    init_gesture_controller();
    // create store for voices column list view
    create_bank_voices_list();
    // create list function
    create_param_list();
    // Save dialog
    create_dialogs();
    // Create rigth click menu
    create_popover_menu();
    // attach GUI signals
    attach_signals();
    /* */
    init_global_fonction_parameter();
    set_default_values();
    // start thread
    S_Thread();
    unblock_ui();
    unblock_midi();
    LOG( LOG_OUT() );
};

Dx7interface::~Dx7interface(){
    LOG( LOG_IN() );
    /* basic destructor*/
    /* terminate thread */
    T_Thread();
    dettach_signals();
    LOG( LOG_OUT() );
};

void Dx7interface::set_default_values(){
    Glib::RefPtr<Gio::File> init_voice = Gio::File::create_for_path( DATA_DIR"cfg/DX7_INIT_VOICE.syx" );
    set_bank(0, init_voice);

    bank_1_modif.sound->extra.mute.val=0x7F; // all unmuted
    bank_1_origin.sound->extra.mute.val=0x7F;
    Glib::RefPtr<Gio::File> param_file = Gio::File::create_for_path( DATA_DIR"cfg/midi_learn_default_config.cfg" );
    read_midi_learned_param(param_file);
};

bool Dx7interface::Run(){
    #ifdef __linux__
        listen_midi();
    #endif
    #if (defined(__WIN32) || defined(__MINGW32__) && defined(__RtMidi__))
        while (nanosleep(&ts, NULL) == -1 && errno == EINTR) {
            // Retry if interrupted by a signal
        }
    #endif
    return true;
};
/* Midi Learn */
void Dx7interface::create_param_list(){
    /* create specific data structure model for voice bank list */
    param_data_model = Gio::ListStore<ParamItem>::create();
    /* set model to GUI */
    param_selection_model=Glib::RefPtr<Gtk::SingleSelection>(get_gwidget<Gtk::SingleSelection>("selection_param"));
    param_selection_model->set_autoselect(false);
    param_selection_model->set_model(param_data_model);
    get_gwidget<Gtk::DropDown>("dropdown_affect_param")->set_sensitive(true);
    auto param_list_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("dropdown_affect_param"));
    // auto factory_param=Glib::RefPtr<Gtk::SignalListItemFactory>(get_gwidget<Gtk::SignalListItemFactory>("factory_param"));
    (get_gwidget<Gtk::SignalListItemFactory>("factory_param"))->signal_setup().connect(
        sigc::bind(sigc::mem_fun(*this, &Dx7interface::on_setup_param_label), Gtk::Align::START));
    (get_gwidget<Gtk::SignalListItemFactory>("factory_param"))->signal_bind().connect(
        sigc::mem_fun(*this, &Dx7interface::on_bind_param_name));

    midi_learned.resize(max_param_nb);
    for(int i=0; i<max_param_nb; i++){
        //param_data_model->append(ParamItem::create(function_list[i]));
        update_param_data_model<ParamItem>(param_data_model, i, function_list[i]);
    };
};
void Dx7interface::add_midi_learned(int param_number, int function_index){
        //LOG( LOG_IN() );
        if( param_number >= static_cast<int>(midi_learned.size()) ){
            midi_learned.resize(param_number + 1);
        };
        midi_learned[param_number].push_back(function_index);
        //LOG( LOG_OUT() );
};
void Dx7interface::rem_midi_learned(int param_number, int function_index){
    //LOG( LOG_IN() );
    if( param_number < static_cast<int>(midi_learned.size()) ){
        auto& vec = midi_learned[param_number];
        vec.erase(std::remove(vec.begin(), vec.end(), function_index), vec.end());
    };
    //LOG( LOG_OUT() );
};
void Dx7interface::add_midi_learn_param_widget(Glib::ustring function_name, Glib::ustring param_number, int selected_item){
    //LOG( LOG_IN() );
    // TODO: check param_number is a numeric
    auto box = get_gwidget<Gtk::Box>("box_listen_affect_param");
    int midi_param_value = std::stoi(param_number.raw());
    /* affected param name widget */
    auto text_fct = Gtk::make_managed<Gtk::Text>();
    text_fct->set_name(function_name);
    text_fct->set_text(function_name);
    text_fct->set_editable(false);
    /* midi param number widget */
    auto text_param = Gtk::make_managed<Gtk::Text>();
    text_param->set_name(param_number);
    text_param->set_text(param_number);
    text_param->set_editable(false);
    /* delete button for the entry */
    auto btn_delete = Gtk::make_managed<Gtk::Button>("delete" +param_number);
    btn_delete->set_name("delete_" + param_number);
    /* box to group created widget */
    auto box_funct = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
    box_funct->set_name("box_midi_param"+param_number);
    box_funct->append(*text_fct);
    box_funct->append(*text_param);
    box_funct->append(*btn_delete);
    /* ui box to attach result */
    box->add_tick_callback([this, box, box_funct,midi_param_value,selected_item](const Glib::RefPtr<Gdk::FrameClock>&) -> bool {
        //LOG( LOG_IN() );
        if(*pending_remove == 0){
            //msg = " ADD box triggered" ;
            add_midi_learned(midi_param_value, selected_item);
            box->append(*box_funct);
            //LOG( LOG_OUT() );
            return false; // false: Remove the tick callback after execution
        }else{
            return true;
        }
    });
    #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
        /* scoped slot to be autoremove */
        auto slot_btn_delete_params = std::make_shared<sigc::scoped_connection>();
        *slot_btn_delete_params = btn_delete->signal_clicked().connect([this, box_funct, text_fct, text_param, btn_delete, slot_btn_delete_params, selected_item, param_number]() {
            //LOG( LOG_IN() );
            //msg = " REM box triggered" ;
            if(*pending_remove > 0){
                (*pending_remove)--;
            };
            int p_number = std::stoi(param_number.raw());
            rem_midi_learned(p_number, selected_item);
            auto box = get_gwidget<Gtk::Box>("box_listen_affect_param");
            box_funct->remove(*text_param);
            box_funct->remove(*text_fct);
            box_funct->remove(*btn_delete);
            box->remove(*box_funct);
            //LOG( LOG_OUT() );
        });
    #else
        auto slot_btn_delete_params = std::make_shared<sigc::connection>();
        *slot_btn_delete_params = btn_delete->signal_clicked().connect(
            [this, box_funct, text_fct, text_param, btn_delete, selected_item, param_number, slot_btn_delete_params]() {
                //LOG( LOG_IN() );
                //msg = "REM box triggered" ;
                if(*pending_remove > 0){
                    (*pending_remove)--;
                };
                int p_number = std::stoi(param_number.raw());
                rem_midi_learned(p_number, selected_item);
                auto box = get_gwidget<Gtk::Box>("box_listen_affect_param");
                box_funct->remove(*text_param);
                box_funct->remove(*text_fct);
                box_funct->remove(*btn_delete);
                box->remove(*box_funct);
                if (slot_btn_delete_params->connected()) {
                    slot_btn_delete_params->disconnect();
                }
                //LOG( LOG_OUT() );
            }
        );
    #endif
    //box->queue_draw();
    //LOG( LOG_OUT() );
};
void Dx7interface::clean_midi_learn(){
    //LOG( LOG_IN() );
    try{
        Gtk::Box* parent = get_gwidget<Gtk::Box>("box_listen_affect_param");
        Gtk::Widget* child = parent->get_first_child();
        if(child){
            Glib::RefPtr<Glib::Regex> regex_box= Glib::Regex::create("^box_midi_param.*");
            Glib::RefPtr<Glib::Regex> regex_function = Glib::Regex::create("^delete.*");
            while (child){
                if( regex_box->match(child->get_name()) ){ // si match box_midi_param
                    //msg = "child name: " + child->get_name() ;
                    Gtk::Widget* subchild = child->get_first_child();
                    while (subchild){
                        //msg = "sub child name: " + subchild->get_name() ;
                        if( regex_function->match(subchild->get_name()) ){
                            //msg = "sub child MATCH" + subchild->get_name() ;
                            (*pending_remove)++;
                            subchild->add_tick_callback([this, parent, child, subchild](const Glib::RefPtr<Gdk::FrameClock>&) -> bool {
                                //LOG( LOG_IN() );
                                //msg = "SUBCHILD activate triggered" ;
                                subchild->activate();
                                //subchild->queue_draw();
                                //LOG( LOG_OUT() );
                                return false;
                            });
                            break;
                        };
                        subchild = subchild->get_next_sibling();
                    };
                };
                child = child->get_next_sibling();
            };
        };
    }catch( const std::exception & ex ){
        std::string err_msg = error( __PRETTY_FUNCTION__, "???", ex.what() );
        LOG_ERR( err_msg );
    };
    //LOG( LOG_OUT() );
};
void Dx7interface::read_midi_learned_param(Glib::RefPtr<Gio::File> param_file){
    LOG( LOG_IN() );
    clean_midi_learn();
    auto data_stream = Gio::DataInputStream::create(param_file->read());
    std::string line;
    Glib::ustring function_name;
    Glib::ustring param_number;
    std::regex pattern("[^0-9]+");
    data_stream->read_line(line);
    while(data_stream->read_line(line)){
        function_name = line.substr(0,line.find_last_of(","));
        param_number = line.substr(line.find_last_of(",")+1,line.length());

        param_number = std::regex_replace(param_number.c_str(), pattern, "");
        if( param_number != "" ){
            for( int selected_item = 0 ; selected_item < max_param_nb; selected_item++){
                if( function_list[selected_item] == function_name ){
                    add_midi_learn_param_widget(function_name, param_number, selected_item);
                    break;
                };
            };
        };
    };
    data_stream->close();
    LOG( LOG_OUT() );
};
void Dx7interface::save_midi_learned_param(Glib::RefPtr<Gio::File> param_file){
    auto output_stream = param_file->replace();
    auto data_stream = Gio::DataOutputStream::create(output_stream);
    std::string line;
    Glib::ustring function;
    Glib::ustring midi_param_number;
    Glib::ustring first_line = _("# Function, midi controller number");
    line = first_line;
    #if (defined(__WIN32) || defined(__MINGW32__) && defined(__RtMidi__))
        line.append(tostr<const char>(*EOL));
    #else
        line.append(tostr<const char>(EOL));
    #endif
    data_stream->put_string(line);
    for( int function_index = 0 ; function_index < max_param_nb; function_index++){
        midi_param_number = "";
        function = function_list[function_index];
        for(int param_number = 0; param_number < 128; param_number++){
            if( !midi_learned[param_number].empty() && midi_learned[param_number][0] == function_index ){
                midi_param_number = std::to_string(param_number);
            };
        };
        line = function + "," + midi_param_number;
        #if (defined(__WIN32) || defined(__MINGW32__) && defined(__RtMidi__))
            line.append(tostr<const char>(*EOL));
        #else
            line.append(tostr<const char>(EOL));
        #endif
        data_stream->put_string(line);
    };
    data_stream->flush();
    data_stream->close();
    output_stream->close();
};
void Dx7interface::on_midi_learn_param_save(){
    file_dialog_param_save->set_title(_("Select file"));
    Glib::ustring filename;
    unsigned int index = 0;
    #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
        file_dialog_param_save->set_initial_name("midi_learn_config.cfg");
        if(initial_folder_save_param==nullptr){
            initial_folder_save_param=initial_folder_open_param;
        }
        if(initial_folder_save_param!=nullptr){
            file_dialog_param_save->set_initial_folder(initial_folder_save_param);
        }
        file_dialog_param_save->save( *(get_window()), [this,index](const Glib::RefPtr<Gio::AsyncResult>& result) {
            try {
                Glib::RefPtr<Gio::File> file = file_dialog_param_save->save_finish(result);
                if (file) {
                    initial_folder_save_param = Gio::File::create_for_path(file->get_parent()->get_path());
                    save_midi_learned_param(file);
                };
            }catch( const std::exception & ex ){
                std::string err_msg = error( __PRETTY_FUNCTION__, _("Cannot write file as raw"), ex.what() );
                LOG_ERR( err_msg );
            }
        });
    #else
        file_dialog_param_save->set_transient_for(*(get_window()));
        file_dialog_param_save->set_current_name("midi_learn_config.cfg");
        if(initial_folder_save_param==nullptr){
            initial_folder_save_param=initial_folder_open_param;
        }
        if(initial_folder_save_param!=nullptr){
            file_dialog_param_save->set_current_folder(initial_folder_save_param);
        };
        file_dialog_param_save->signal_response().connect([this,index](int response) {
            try {
                if (response == Gtk::ResponseType::ACCEPT) {
                    auto file = file_dialog_param_save->get_file();
                    if (file) {
                        initial_folder_save_param = Gio::File::create_for_path(file->get_parent()->get_path());
                        save_midi_learned_param(file);
                    };
                }
                file_dialog_param_save->hide();
            }catch( const std::exception & ex ){
                std::string err_msg = error( __PRETTY_FUNCTION__, "???", ex.what() );
                LOG_ERR( err_msg );
            };
        });
        file_dialog_param_save->show();
    #endif
};
void Dx7interface::on_midi_learn_param_select(){
    //LOG( LOG_IN() );
    try{
        file_dialog_param_select->set_title(_("Select config"));
        #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
            file_dialog_param_select->set_initial_folder(initial_folder_open_param);
            file_dialog_param_select->open( *(get_window()), [this](const Glib::RefPtr<Gio::AsyncResult>& result ) {
                try {
                    Glib::RefPtr<Gio::File> config_file = file_dialog_param_select->open_finish(result);
                    if (config_file) {
                        read_midi_learned_param(config_file);
                        initial_folder_open_param = Gio::File::create_for_path(config_file->get_parent()->get_path());
                    };
                } catch (const std::exception & ex) {
                    std::string err_msg = error( __PRETTY_FUNCTION__, "???", ex.what() );
                    LOG_ERR( err_msg );
                };
            }); /* end dialog open function */
        #else
            file_dialog_param_select->set_transient_for(*(get_window()));
            file_dialog_param_select->signal_response().connect([this](int response) {
                try {
                    if (response == Gtk::ResponseType::ACCEPT) {
                        auto config_file = file_dialog_param_select->get_file();
                        if (config_file) {
                            read_midi_learned_param(config_file);
                            initial_folder_open_param= Gio::File::create_for_path(config_file->get_path());;
                        };
                    };
                    file_dialog_param_select->hide();
                } catch (const std::exception & ex) {
                    std::string err_msg = error( __PRETTY_FUNCTION__, "???", ex.what() );
                    LOG_ERR( err_msg );
                };
            });
            file_dialog_param_select->show();
        #endif
    }catch( const std::exception & ex ){
        std::string err_msg = error( __PRETTY_FUNCTION__, "???", ex.what() );
        LOG_ERR( err_msg );
        //throw std::runtime_error(err_msg);
    };
    //LOG( LOG_OUT() );
};
/* Threaded loop run */
#ifdef __linux__ 
    void Dx7interface::listen_midi(){
        snd_seq_event_input(seq_handle, &ev);
        Synth::print_event_info(ev);
        int length_mask;
        //if( uncomplete || ((int)ev->dest.client == Synth::get_client_id() && ((int)(ev->data.control.channel) +1) == (int)Synth::channel_receive ) ) {
        if( uncomplete || (int)ev->dest.client == Synth::get_client_id() ){
            std::string msg;
            switch( ev->type ){
                case SND_SEQ_EVENT_NOTEON:
                    //Synth::print_event_info(ev);
                    msg = _("Channel: ")  + std::to_string( (int(ev->data.control.channel) +1) ) + " \t"
                    + _("value: ") + std::to_string( int(ev->data.note.note) );
                    LOG( msg );
                    break;
                case SND_SEQ_EVENT_NOTEOFF:
                    msg = _("Note OFF") ;
                    LOG( msg );
                    //Synth::print_event_info(ev);
                    msg = _("Channel: ")  + std::to_string( (int(ev->data.control.channel) +1) ) + " \t"
                    + _("value: ") +  std::to_string( int(ev->data.note.note) );
                    LOG( msg );
                    break;
                case SND_SEQ_EVENT_CONTROLLER:
                    /*typedef union snd_seq_event_data {
                        snd_seq_ev_raw8_t raw8;
                        snd_seq_ev_raw32_t raw32;
                        snd_seq_ev_queue_control_t queue;
                        snd_seq_timestamp_t time;
                        snd_seq_addr_t addr;
                        snd_seq_connect_t connect;
                        snd_seq_result_t result;
                    } snd_seq_event_data_t;*/
                    //Synth::print_event_info(ev);
                    msg = _("Channel: ") + std::to_string( (int(ev->data.control.channel) + 1) ) + " \t"
                        + _("param: ") + std::to_string(ev->data.control.param) + " "
                        + _("value: ") + std::to_string( int(ev->data.control.value) );
                    LOG( msg );
                    if(midi_learn){
                        auto param = std::make_shared<int>(ev->data.control.param);
                        (get_gwidget<Gtk::Entry>("entry_affect_param"))->add_tick_callback([this,param](const Glib::RefPtr<Gdk::FrameClock>&) {
                            (get_gwidget<Gtk::Entry>("entry_affect_param"))->set_text(std::to_string(*param));
                            return false;
                        });
                    }else{
                        if ( ev->data.control.param < max_param_nb && !midi_learned[ev->data.control.param].empty()) {
                            (this->*list_ui_parameters_functions[midi_learned[ev->data.control.param][0]])(ev->data.control.value);
                        };
                    };
                    break;
                case SND_SEQ_EVENT_PITCHBEND:
                    //Synth::print_event_info(ev);
                    msg = _("Channel: ") + std::to_string( (int(ev->data.control.channel) +1) ) + " \t"
                        + _("value: ") + std::to_string( int(ev->data.control.value) );
                    LOG( msg );
                    break;
                case SND_SEQ_EVENT_PGMCHANGE:
                    //Synth::print_event_info(ev);
                    /*event data type = snd_seq_ev_ctrl_t */
                    msg =  _("Channel : ")  + std::to_string( (int(ev->data.control.channel) +1) ) + " \t"
                        + _("param : ")  + std::to_string(ev->data.control.param) + " "
                        + _("value : ") + std::to_string( int(ev->data.control.value) );
                    LOG( msg );
                    if( ev->data.control.param == 0 && ( (unsigned int)ev->data.control.value < bank_nb_sound ) ){
                     	unsigned int value = (unsigned int)ev->data.control.value;
                        select_voice(value);
                    }
                    break;
                case SND_SEQ_EVENT_SYSEX:
                    //Synth::print_event_info(ev);
                    //SND_SEQ_EVENT_SYSEX 	system exclusive data (variable length);
                    // event data type = snd_seq_ev_ext_t
                    /*msg =  "Channel : "  + (int(ev->data.control.channel) +1) + '\t'
                    + "length : "  + int(ev->data.ext.len) + " "
                    + "ptr : " + ev->data.ext.ptr
                    ;*/
                    // 09 32 SoundBankItem
                    // 02 32 son + function
                    //TODO: to review
                    receive = true;
                    length_mask = ev->type & SND_SEQ_EVENT_LENGTH_MASK;
                    if (length_mask == SND_SEQ_EVENT_LENGTH_FIXED) {
                        msg = _("Event has fixed length.") ;
                        LOG( msg );
                    } else if (length_mask == SND_SEQ_EVENT_LENGTH_VARIABLE) {
                        msg = _("Event has variable length.") ;
                        LOG( msg );
                    } else {
                        msg = _("Unknown length mask.") ;
                        LOG( msg );
                    }
                    msg = _("Length:") + std::to_string( int(ev->data.ext.len) );
                    if( (ev->data.ext.len > 8 || uncomplete) && receive ){
                        uncomplete = true;
                        uint8_t* byte_ptr = static_cast<uint8_t*>(ev->data.ext.ptr);
                        sysex_buffer.insert(sysex_buffer.end(), byte_ptr, byte_ptr + ev->data.ext.len);
                        if (!sysex_buffer.empty() && sysex_buffer.back() == 0xF7) {
                            uncomplete = false;
                            if(sysex_buffer.size() > 163){
                                receive_bank(sysex_buffer);
                            }else if(sysex_buffer.size() == 163){
                                //receive_sound(sysex_buffer);
                            }else{
                                //receive sysex message
                            };
                            sysex_buffer.clear();
                        };
                    };
                    break;
                case SND_SEQ_EVENT_SENSING:
                    // CLOCK REQUEST
                    break;
            };
        };
        snd_seq_free_event(ev);
    };
#endif
#if (defined(__WIN32) || defined(__MINGW32__) && defined(__RtMidi__))
    void Dx7interface::listen_midi(double timestamp, std::vector<unsigned char>* _message, void* userData){
        LOG( LOG_IN() );
        message.assign(_message->begin(),_message->end());
        Synth::print_event_info();
        /*
        //if( uncomplete || ((int)ev->dest.client == Synth::get_client_id() && ((int)(ev->data.control.channel) +1) == (int)Synth::channel_receive ) ) {
        if( uncomplete || (int)ev->dest.client == Synth::get_client_id() ) {
            switch (ev->type) {
                case SND_SEQ_EVENT_NOTEON:
                    msg = "Channel: "  + (int(ev->data.control.channel) +1) + " " + '\t'
                    + "value: " + int(ev->data.note.note) ;;
                    break;
                case SND_SEQ_EVENT_NOTEOFF:
                    msg = "Channel: "  + (int(ev->data.control.channel) +1) + " " + '\t'
                    + "value: " +  int(ev->data.note.note) ;
                    break;
                case SND_SEQ_EVENT_CONTROLLER:
                    msg = "Channel: " + ( (int)(ev->data.control.channel) +1) + " " + '\t'
                    + "param: "  + ev->data.control.param + " "
                    + "value: " + int(ev->data.control.value) ;
                    if(midi_learn){
                        auto param = std::make_shared<int>(ev->data.control.param);
                        (get_gwidget<Gtk::Entry>("entry_affect_param"))->add_tick_callback([this,param](const Glib::RefPtr<Gdk::FrameClock>&) {
                            (get_gwidget<Gtk::Entry>("entry_affect_param"))->set_text(std::to_string(*param));
                            return false;
                        });
                    }else{
                        if ( ev->data.control.param < max_param_nb && !midi_learned[ev->data.control.param].empty()) {
                            (this->*list_ui_parameters_functions[midi_learned[ev->data.control.param][0]])(ev->data.control.value);
                        };
                    };
                    break;
                case SND_SEQ_EVENT_PITCHBEND:
                    msg = "Channel: " + (int(ev->data.control.channel) +1)+ " " + '\t'
                    + "value: " + int(ev->data.control.value) ;
                    break;
                case SND_SEQ_EVENT_PGMCHANGE:
                    //
                    msg =  "Channel : "  + (int(ev->data.control.channel) +1) + '\t'
                    + "param : "  + ev->data.control.param + " "
                    + "value : " + int(ev->data.control.value)
                    ;
                    if ( ev->data.control.param == 0 && ( (unsigned int)ev->data.control.value < bank_nb_sound ) ){
                        #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 12)
                            // get_gwidget<Gtk::ColumnView>("columnview_bank")->scroll_to((unsigned int)ev->data.control.value,nullptr,Gtk::ListScrollFlags::SELECT);
                           // bank_selection_model->set_selected((unsigned int)ev->data.control.value);
                             unsigned int value = (unsigned int)ev->data.control.value;
                            (get_gwidget<Gtk::ColumnView>("columnview_bank"))->add_tick_callback([this,value](const Glib::RefPtr<Gdk::FrameClock>&) {
                                (get_gwidget<Gtk::ColumnView>("columnview_bank"))->scroll_to((unsigned int)value,nullptr,Gtk::ListScrollFlags::SELECT);
                                return false; // Return false to remove the callback after one executio
                            });
                        #else
                            auto adjustment = get_gwidget<Gtk::ColumnView>("columnview_bank")->get_vadjustment();
                            adjustment->set_value((double)ev->data.control.value);
                            bank_selection_model->set_selected((unsigned int)ev->data.control.value);
                        #endif
                    }
                    break;
                case SND_SEQ_EVENT_SYSEX:
                    //SND_SEQ_EVENT_SYSEX 	system exclusive data (variable length);
                    // event data type = snd_seq_ev_ext_t
                    // msg =  "Channel : "  + (int(ev->data.control.channel) +1) + '\t'
                    // + "length : "  + int(ev->data.ext.len) + " "
                    // + "ptr : " + ev->data.ext.ptr
                    // ;
                    // 09 32 SoundBankItem
                    // 02 32 son + function
                    //TODO: to review
                    receive = true;
                    length_mask = ev->type & SND_SEQ_EVENT_LENGTH_MASK;
                    if (length_mask == SND_SEQ_EVENT_LENGTH_FIXED) {
                        msg = "Event has fixed length." ;
                    } else if (length_mask == SND_SEQ_EVENT_LENGTH_VARIABLE) {
                        msg = "Event has variable length." ;
                    } else {
                        msg = "Unknown length mask." ;
                    }
                    msg = "Length:" + (int(ev->data.ext.len)) ;
                    if( (ev->data.ext.len > 8 || uncomplete) && receive ){
                        uncomplete = true;
                        uint8_t* byte_ptr = static_cast<uint8_t*>(ev->data.ext.ptr);
                        sysex_buffer.insert(sysex_buffer.end(), byte_ptr, byte_ptr + ev->data.ext.len);
                        if (!sysex_buffer.empty() && sysex_buffer.back() == 0xF7) {
                            uncomplete = false;
                            receive_bank(sysex_buffer);
                            sysex_buffer.clear();
                        };
                    };
                    break;
                case SND_SEQ_EVENT_SENSING:
                    // CLOCK REQUEST
                    break;
            };
        };
        */
        LOG( LOG_OUT() );
    };
#endif

bool Dx7interface::Run2(){
    // UNUSED for test
    //sleep_for(std::chrono::milliseconds(5000));
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000000;
    nanosleep(&ts, NULL);
    //msg = "coucou" ;
    while(lock){};
    lock=true;
    for(unsigned long i=0; i < midi_param.size(); i++){
        if ( midi_param[i] != -1) {
            int val = midi_param[i];
            (this->*list_ui_parameters_functions[midi_learned[i][0]])(val);
            std::string msg = _(" Param: ") + std::to_string(i) + _(" Value: ") + std::to_string(midi_param[i]);
            LOG( msg );
            midi_param[i] = -1;
        };
    };
    lock=false;
    return true;
};

/* Dialogs */
void Dx7interface::create_dialogs() {
    #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
        dialog_file_select = get_gwidget<Gtk::FileDialog>("FileDialog_select");
        dialog_file_save = get_gwidget<Gtk::FileDialog>("FileDialog_save");
    #else
        dialog_file_select = get_gwidget<Gtk::FileChooserDialog>("FileDialog_select");
        dialog_file_select->add_button("_Open", Gtk::ResponseType::ACCEPT);
        dialog_file_select->add_button("_Cancel", Gtk::ResponseType::CANCEL);
        dialog_file_select->set_action(Gtk::FileChooser::Action::OPEN);
        dialog_file_save = get_gwidget<Gtk::FileChooserDialog>("FileDialog_save");
        dialog_file_save->add_button("_Save", Gtk::ResponseType::ACCEPT);
        dialog_file_save->add_button("_Cancel", Gtk::ResponseType::CANCEL);
        dialog_file_save->set_action(Gtk::FileChooser::Action::SAVE);
    #endif
    // dialogs for
    dialog_file_select->set_modal(true);
    dialog_file_save->set_modal(true);
    // dialog for parameters
    btn_dialog_param = get_gwidget<Gtk::Button>("btn_dialog_param");
    dialog_param = get_gwidget<Gtk::Window>("dialog_param");
    dialog_param->set_default_size(20, 10);
    dialog_param->set_hide_on_close(true);
    dialog_param->set_modal(true);
    get_gwidget<Gtk::CheckButton>("checkbutton_128")->set_group(*(get_gwidget<Gtk::CheckButton>("checkbutton_32")));
    get_gwidget<Gtk::CheckButton>("checkbutton_extra_parameters_by_bank")->set_group(*(get_gwidget<Gtk::CheckButton>("checkbutton_extra_parameters_by_sound")));
};

void Dx7interface::set_param(){
    if( action_type == ACT_SAVE ){
        Glib::ustring filename;
        save_index = 0;
        bool is_32 = false;
        bool is_128 = false;
        as_raw = get_gwidget<Gtk::CheckButton>("checkbutton_as_raw")->get_active();
        if(bank_nb_sound > 32){
            is_32 = get_gwidget<Gtk::CheckButton>("checkbutton_32")->get_active();
            is_128 = get_gwidget<Gtk::CheckButton>("checkbutton_128")->get_active();
        }else{
            is_32=true;
        };
        write_extra_params = get_gwidget<Gtk::CheckButton>("checkbutton_add_extra_parameters")->get_active();
        if(save_type == SOUND){
            export_config = DX7_1;
            filename = bank_1_modif.sound->name;
        }else if(save_type == BANK){
            filename = bank_1_modif.name;
            if(is_32){
                export_config = DX7_32;
                save_index = get_gwidget<Gtk::SpinButton>("spinbutton_save_start")->get_value();
            }else if(is_128){
                export_config = DX7_128;
            }else{
                export_config = DX7_1;
            };
        };

        dialog_file_save->set_title(dialog_param->get_title());
        if(initial_folder_save==nullptr){
            initial_folder_save=initial_folder_open;
        }
        #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
            if(as_raw){
                dialog_file_save->set_initial_name(filename+".dx7");
                export_config = DX7_RAW;
            }else{
                dialog_file_save->set_initial_name(filename+".syx");
            };
        #else
            if(as_raw){
                dialog_file_save->set_current_name(filename+".dx7");
                export_config = DX7_RAW;
            }else{
                dialog_file_save->set_current_name(filename+".syx");
            };
        #endif
        std::string msg = _("base filename: ") + filename ;
        LOG( msg );
        msg = _("with format: ");
        if( as_raw ){
            msg +="Raw";
        }else{
            msg +="Bulk";
        }
        LOG( msg );
    };
};
void Dx7interface::set_dialog(Glib::ustring title){
    LOG( LOG_IN() );
    try{
        if( action_type == ACT_SAVE ){
            get_gwidget<Gtk::Box>("box_save")->set_visible(true);
            get_gwidget<Gtk::Box>("box_insert")->set_visible(false);
            Glib::ustring name;
            if( save_type == BANK ){
                name=bank_1_modif.name;
            }else{
                name=bank_1_modif.sound->name;
            }
            Glib::ustring label_name= title +": "+ name ;
            get_gwidget<Gtk::Label>("label_name")->set_label(label_name);

            auto spinbutton_save_start = get_gwidget<Gtk::SpinButton>("spinbutton_save_start");
            /* set the upper limit to bank_nb_sound less 32 */
            double lower, upper;
            spinbutton_save_start->get_range(lower, upper);
            spinbutton_save_start->set_range(lower, (bank_nb_sound-32));
            /* set visible for bank save */
            if(save_type == BANK && bank_nb_sound > 32){
                get_gwidget<Gtk::Box>("box_save_bank")->set_visible(true);
                get_gwidget<Gtk::Label>("label_bulk")->set_visible(true);
            }else{
                get_gwidget<Gtk::Box>("box_save_bank")->set_visible(false);
                get_gwidget<Gtk::Label>("label_bulk")->set_visible(false);
            };
        }else if( action_type == ACT_INSERT ){
            get_gwidget<Gtk::Box>("box_save")->set_visible(false);
            get_gwidget<Gtk::Box>("box_insert")->set_visible(true);
            auto spinbutton_insert_start = get_gwidget<Gtk::SpinButton>("spinbutton_insert_start");
            /* set the upper limit to bank_nb_sound less 32 */
            double lower, upper;
            spinbutton_insert_start->get_range(lower, upper);
            spinbutton_insert_start->set_range(lower, (bank_nb_sound));
            spinbutton_insert_start->set_value(snum);

            Glib::ustring label_insert=title+": "+snum;
            get_gwidget<Gtk::Label>("label_name")->set_label(label_insert);
        };
    }catch( const std::exception & ex ){
        std::string err_msg = error( __PRETTY_FUNCTION__, "???", ex.what() );
        LOG_ERR( err_msg );
        //throw std::runtime_error(err_msg);
    };
    LOG( LOG_OUT() );
};

void Dx7interface::OpenDialogFileSave(unsigned int data_stream_index, std::function<void(unsigned int, Glib::RefPtr<Gio::File>)> funct){

    Gx_module::OpenDialogFileSave(data_stream_index, funct);
};
void Dx7interface::on_file_save(unsigned int data_stream_index, Glib::RefPtr<Gio::File> file){
    try{
        dialog_param->close();
        if(bank_nb_sound != 0){
            if(save_type == SOUND){
                if(as_raw){
                    write_voice_as_raw(data_stream_index, file);
                }else{
                    write_voice_as_sysex(data_stream_index, file);
                };
            }else if(save_type == BANK){
                write_bank(data_stream_index, file, save_index);
            };
        };
    }catch( const std::exception& ex){
        //std::string err_msg = "!!! " + std::string(__PRETTY_FUNCTION__) + _(" Failed to save file: !!!\n") + file->get_path() + "\n" + _("Reason => ") + ex.what();
        std::string err_msg = error( __PRETTY_FUNCTION__, _("Failed to save file: "), ex.what() );
        LOG_ERR( err_msg );
        LOG( LOG_OUT() );
        throw std::runtime_error(err_msg);
    };
};

/*** BANK ***/
void Dx7interface::create_bank_voices_list(){
    /* create specific data structure model for voice bank list */
    bank_data_model = Gio::ListStore<SoundBankItem>::create();
    /* set model to GUI */
    bank_selection_model=Glib::RefPtr<Gtk::SingleSelection>(get_gwidget<Gtk::SingleSelection>("selection_bank"));
    bank_selection_model->set_autoselect(false);
    bank_selection_model->set_model(bank_data_model);

    /* Sound Select */
    auto factory_num=get_gwidget<Gtk::SignalListItemFactory>("factory_num");
    factory_num->signal_setup().connect(
        sigc::bind(
            sigc::mem_fun(*this, &Dx7interface::on_setup_sound_number_label), Gtk::Align::CENTER));
    factory_num->signal_bind().connect(
        sigc::mem_fun(*this, &Dx7interface::on_bind_num));

    auto factory_name=get_gwidget<Gtk::SignalListItemFactory>("factory_name");
    factory_name->signal_setup().connect(
        sigc::bind(
            sigc::mem_fun(*this, &Dx7interface::on_setup_sound_name_label), Gtk::Align::CENTER));
    factory_name->signal_bind().connect(
        sigc::mem_fun(*this, &Dx7interface::on_bind_name));

    slot_selected_sound_change = bank_selection_model->signal_selection_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_selected_sound_change));

    /* Bank load */
    slot_bank_select = (get_gwidget<Gtk::Button>("bank_select"))->signal_clicked().connect(
        sigc::bind(
            sigc::mem_fun(*this, &Dx7interface::OpenDialogFileSelect),
                0,
                std::bind(&Dx7interface::set_bank, this, std::placeholders::_1, std::placeholders::_2) )
    );
    slot_bank_reveal = (get_gwidget<Gtk::Button>("btn_toolbar_reveal_bank"))->signal_clicked().connect(
        sigc::mem_fun(*this, &Dx7interface::on_bank_reveal));
};
void Dx7interface::create_popover_menu(){
    auto menu = Gio::Menu::create();
    menu->append("_Save sound", "menu.save_sound");
    menu->append("_Save bank", "menu.save_bank");
    menu->append("_Send bank", "menu.send_bank");
    menu->append("_Restore sound", "menu.restore_sound");
    menu->append("_Restore bank", "menu.restore_bank");
    menu->append("_Insert At", "menu.insert_at");
    menu->append("_Replace", "menu.replace_sound");
    menu->append("_Delete", "menu.delete_sound");

    // Create popover menu
    m_popover_menu = Gtk::make_managed<Gtk::PopoverMenu>();
    m_popover_menu->set_parent(*get_gwidget<Gtk::ColumnView>("columnview_bank"));
    m_popover_menu->set_menu_model(menu);
    m_popover_menu->set_has_arrow(false);

    // Add right-click gesture
    auto gesture = Gtk::GestureClick::create();
    gesture->set_button(GDK_BUTTON_SECONDARY);
    gesture->signal_pressed().connect(sigc::mem_fun(*this, &Dx7interface::on_columnview_right_click));
    get_gwidget<Gtk::ColumnView>("columnview_bank")->add_controller(gesture);

    // Create action group
    action_group = Gio::SimpleActionGroup::create();
    get_gwidget<Gtk::ColumnView>("columnview_bank")->insert_action_group("menu", action_group);
};
void Dx7interface::on_columnview_right_click(int n_press, double x, double y){
    // GENERIC except it an event
    LOG( LOG_IN() );
        m_popover_menu->set_pointing_to(Gdk::Rectangle(x, y, 1, 1));
        m_popover_menu->popup();
    LOG( LOG_OUT() );
};
/* set/load */
void Dx7interface::OpenDialogFileSelect(unsigned int data_stream_index, std::function<void(unsigned int, Glib::RefPtr<Gio::File>)> funct){

    Gx_module::OpenDialogFileSelect(data_stream_index, funct);
};

void Dx7interface::clean_bank(){
    LOG( LOG_IN() );
    set_init_voice_in_origin();
    auto [bank_origin_src, bank_modif_src] = get_banks_source();
    std::visit([&](auto& bank_origin, auto& bank_modif) {
        for( unsigned int i = 0 ; i < bank_nb_sound ; i++ ){
            bank_origin.get().sound[i] = bank_1_origin.sound[0];        // write init_voice to all sounds
        };
    }, bank_origin_src, bank_modif_src);
    restore_origin(BANK);                                               // restore origin to write in modif

    unsigned int n_items = bank_data_model->get_n_items();              // clear all listview entry
    if (n_items != 0) {
        bank_data_model->remove_all();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::set_init_voice_in_origin(){
    // store actual bank values
    unsigned int bank_nb_sound_origin = bank_nb_sound;
    Glib::ustring bank_name = bank_1_origin.name;
    // Read init_voice
    Glib::RefPtr<Gio::File> init_voice_file = Gio::File::create_for_path( DATA_DIR"cfg/DX7_INIT_VOICE.syx" );
    read_file_as_datastream( 1, init_voice_file,
                            std::bind(&Dx7interface::set_bank_sounds, this,
                                      std::placeholders::_1, std::placeholders::_2,
                                      std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
    // restore bank_1 values
    bank_1_origin.name = bank_name;
    bank_nb_sound = bank_nb_sound_origin;
};

void Dx7interface::read_file_as_datastream(unsigned int stream_index, Glib::RefPtr<Gio::File> file, std::function<void(unsigned int, Glib::RefPtr<Gio::File>, Glib::ustring, Glib::ustring, unsigned int)> funct){
    Gx_module::read_file_as_datastream(stream_index, file, funct);
};

void Dx7interface::set_bank(unsigned int data_stream_index, Glib::RefPtr<Gio::File> bank_file){
    LOG( LOG_IN() );
    Synth::set_bank(data_stream_index, bank_file, std::bind(&Dx7interface::set_bank_sounds, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
    restore_origin(BANK);
    slot_selected_sound_change.unblock();
    LOG( LOG_OUT() );
};

void Dx7interface::set_bank_sounds(unsigned int data_stream_index, Glib::RefPtr<Gio::File> file, Glib::ustring file_name, Glib::ustring file_base, unsigned int file_size){
    // set read voice in Origin
    std::string msg = _("Bank name: ") + file_name ;
    LOG( msg );
    parse_sysex(file, data_stream.at(data_stream_index), file_size);
    // moove to seek_paraemters
    if( std::filesystem::exists( (file_base+"_fct.syx").c_str() ) ){
        Glib::RefPtr<Gio::File> file_fct=Gio::File::create_for_path( (file_base+"_fct.syx").c_str() );
        data_stream_param = Gio::DataInputStream::create(file_fct->read());
    }
    Bank_ptr bank_ptr;
    switch( file_size ){
        case 128: /* one voice */
            bank_nb_sound = 1;
            bank_1_origin.name = file_name;
            bank_ptr = reinterpret_cast<Bank_ptr>(&bank_1_origin);
            break;
        case 155: /* one voice Dx7 bulk 1 */
            bank_nb_sound = 1;
            bank_1_origin.name = file_name;
            bank_ptr = reinterpret_cast<Bank_ptr>(&bank_1_origin);
            break;
        case 4096: /* 32 voices Dx7 bulk 32 */
            bank_nb_sound = 32;
            bank_32_origin.name = file_name;
            bank_ptr = reinterpret_cast<Bank_ptr>(&bank_32_origin);
            //new(&reinterpret_cast<Bank>(bank_1_origin)->name) Glib::ustring("");
            break;
        case 8200: /* 32 voices TF1 bulk 32 */
            bank_nb_sound = 32;
            bank_32_origin.name = file_name;
            bank_ptr = reinterpret_cast<Bank_ptr>(&bank_32_origin);
            break;

        case 16384: /* 128 voices */
            bank_nb_sound = 128;
            bank_128_origin.name = file_name;
            bank_ptr = reinterpret_cast<Bank_ptr>(&bank_128_origin);
            break;
    };

    if( file_size == 155 ){
        seek_voice_by_byte(data_stream.at(data_stream_index), &bank_ptr->sound[0]);
        seek_parameters(file_base, &bank_ptr->sound[snum]);
    }else{
        for( snum=0; snum < bank_nb_sound; snum++ ){
            seek_voice(data_stream.at(data_stream_index), &bank_ptr->sound[snum]);
            seek_parameters(file_base, &bank_ptr->sound[snum]);
        };
    };
    bank_1_origin.name = file_name;

    old_snum=0;
    snum=0; // reset to first element
    /*if(!isStreamClosed(data_stream_param)){
        data_stream_param->close();
    };*/
};

void Dx7interface::set_bank_name(Glib::ustring name){
    get_gwidget<Gtk::Button>("bank_select")->set_label(name);
};
/* receive */
void Dx7interface::receive_bank(std::vector<uint8_t> sysex_buffer){
    old_snum=0;
    snum = 0;
    Glib::ustring bank_name = _("Received");
    std::string msg = _("Buffer size of received bank: " + sysex_buffer.size()) ;
    LOG( msg );
    /*for (uint8_t byte : sysex_buffer) {
     *      msg = std::hex + std::setw(2) + std::setfill('0') + static_cast<int>(byte) + " ";
    }*/
    switch( sysex_buffer.size() ){
        case 136:
            bank_nb_sound = 1;
            clean_bank();
            receive_voice(&bank_1_origin.sound[snum], sysex_buffer);
            bank_1_origin.name = bank_name;
            bank_1_modif=bank_1_origin;
            break;
        case 163:
            bank_nb_sound = 1;
            clean_bank();
            receive_voice_by_byte(&bank_1_origin.sound[snum], sysex_buffer);
            bank_1_origin.name = bank_name;
            bank_1_modif=bank_1_origin;
            break;
        case 4104:
            bank_nb_sound = 32;
            if( sysex_buffer[3] == 0x09 ){
                clean_bank();
                for (; snum < 32; snum++){
                    receive_voice(&bank_32_origin.sound[snum], sysex_buffer);
                };
            }
            if ( sysex_buffer[3] == 0x02 ){
                snum = 0;
                // TODO : check 0x02 position
                for (; snum < 32; snum++){
                    receive_paramters(&bank_32_origin.sound[snum], sysex_buffer);
                };
            };
            snum = 0; // set selected to 0
            bank_32_origin.name = bank_name;
            bank_32_modif=bank_32_origin;
            bank_1_origin.sound[0]=bank_32_origin.sound[0];
            bank_1_origin.name = bank_name;
            bank_1_modif=bank_1_origin;
            break;
    }
    select_voice(snum);
    get_gwidget<Gtk::Button>("bank_select")->set_label(bank_name);
};
/* restore */
void Dx7interface::restore_origin(unsigned int type){
    LOG( LOG_IN() );
    block_ui();
    std::string msg;
    auto [bank_origin_src, bank_modif_src] = get_banks_source();
    std::visit([&](auto& bank_origin, auto& bank_modif) {
        if(type == BANK){
            msg = _("Restore bank: ") + bank_modif.get().name + _(" from origin bank.") ;
            LOG( msg );

            for ( snum = 0 ; snum < bank_nb_sound; snum++ ){
                bank_modif.get().sound[snum] = bank_origin.get().sound[snum];
                update_data_model<SoundBankItem>(bank_data_model, bank_modif.get().sound[snum].name);
            };
            snum = old_snum;
        }else{
            msg = _("Restore sound: ") + bank_modif.get().sound[snum].name + _(" from origin bank.") ;
            LOG( msg );
            bank_modif.get().sound[snum] = bank_origin.get().sound[snum];
            update_data_model<SoundBankItem>(bank_data_model, bank_modif.get().sound[snum].name);
        }
        if( bank_nb_sound != 1){
            bank_1_origin.sound[0] = bank_origin.get().sound[snum];
            bank_1_modif.sound[0] = bank_1_origin.sound[0];
        };
    }, bank_origin_src, bank_modif_src);

    select_voice(snum);
    LOG( LOG_OUT() );
};
// TODO: to factorise on_restore_sound and bank
void Dx7interface::on_restore_bank(){
    LOG( LOG_IN() );
    restore_origin(BANK);
    LOG( LOG_OUT() );
};
void Dx7interface::on_restore_sound(){
    LOG( LOG_IN() );
    restore_origin(SOUND);
    LOG( LOG_OUT() );
};

/*** INSERT / REPLACE / DELETE / MOOVE ***/
/* REPLACE */
void Dx7interface::on_replace_sound(unsigned int data_stream_index, Glib::RefPtr<Gio::File> file){
    read_file_as_datastream(data_stream_index, file,std::bind(&Dx7interface::replace_sound, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,std::placeholders::_4,std::placeholders::_5));
}
void Dx7interface::replace_sound(unsigned int data_stream_index, Glib::RefPtr<Gio::File> file, Glib::ustring file_name, Glib::ustring file_base, unsigned int file_size){
    LOG( LOG_IN() );
    block_ui(); // needed
    std::string msg = _("Replaced sound: ") + bank_1_modif.sound->name;
    LOG( msg );
    parse_sysex(file, data_stream.at(data_stream_index), file_size);
    if( file_size == 128 ){
        seek_voice(data_stream.at(data_stream_index), &bank_1_modif.sound[0]);
    }else{
        seek_voice_by_byte(data_stream.at(data_stream_index), &bank_1_modif.sound[0]);
    }
    seek_parameters(file_base, &bank_1_modif.sound[0]);
    msg = _(" With: ") + bank_1_modif.sound->name ;
    LOG( msg );
    update_bank_modif();
    select_voice(snum);
    LOG( LOG_OUT() );
};

/** INSERT AT **/
std::tuple<unsigned int,std::pair<Dx7interface::BankVariant, Dx7interface::BankVariant>> Dx7interface::get_banks_dest(unsigned int pos_end_ins){
    if( pos_end_ins > 32 || bank_nb_sound == 128 ){          // switch to 128 sounds
        return std::make_tuple(
            128,
            std::make_pair(std::ref(bank_128_origin), std::ref(bank_128_modif))
        );
    }else if( pos_end_ins > 1 ){    // switch to 32 sounds
        return std::make_tuple(
            32,
            std::make_pair(std::ref(bank_32_origin), std::ref(bank_32_modif))
        );
    }else{
        return std::make_tuple(
            1,
            std::make_pair(std::ref(bank_1_origin), std::ref(bank_1_modif))
        );
    };
};
std::pair<Dx7interface::BankVariant, Dx7interface::BankVariant> Dx7interface::get_banks_source(){
    // TODO: check the throw
    switch( bank_nb_sound ){
        case 1:{
            return std::make_pair(std::ref(bank_1_origin), std::ref(bank_1_modif));
            break;
        };
        case 32:{
            return std::make_pair(std::ref(bank_32_origin), std::ref(bank_32_modif));
            break;
        };
        case 128:{
            return std::make_pair(std::ref(bank_128_origin), std::ref(bank_128_modif));
            break;
        };
        default:
            throw std::runtime_error("Invalid value for bank_nb_sound");
    };
};
void Dx7interface::copy_bank(BankVariant& bank_origin_src, BankVariant& bank_origin_dest, BankVariant& bank_modif_src, BankVariant& bank_modif_dest, unsigned int src_size, unsigned int dst_size){
    // ok & for a non copy
    std::visit([&](auto src_origin, auto& dest_origin, auto src_modif, auto& dest_modif) {
        // copy sound bank name
        dest_origin.get().name = src_origin.get().name;
        dest_modif.get().name = src_origin.get().name;
        // copy sounds
        unsigned int i = 0;
        for (; i < dst_size && i < src_size; i++) {
            dest_origin.get().sound[i] = src_origin.get().sound[i];
            dest_modif.get().sound[i] = src_modif.get().sound[i];
        };
        if( i < dst_size){
            set_init_voice_in_origin();
            for (; i < dst_size ; i++) {
                dest_origin.get().sound[i] = bank_1_origin.sound[0];
                dest_modif.get().sound[i] = bank_1_origin.sound[0];
            };
        };
    }, bank_origin_src, bank_origin_dest, bank_modif_src, bank_modif_dest);
};
void Dx7interface::moove_sound(BankVariant& bank_origin_dest, BankVariant& bank_modif_dest, unsigned int end_insert,int nb_snd){
    std::visit([&](auto& bank_origin, auto& bank_modif) {
        // do not moove origin bank
        if( nb_snd > 0){
            for( unsigned int i = bank_nb_sound-1; i > 0 && i >= end_insert ; i--){
                //bank_origin.get().sound[i] = bank_origin.get().sound[i-nb_snd];
                bank_modif.get().sound[i] = bank_modif.get().sound[i-nb_snd];
            };
        }else if( nb_snd < 0 ){
            for( unsigned int i = end_insert; i < bank_nb_sound-1 ; i++){
                //bank_origin.get().sound[i] = bank_origin.get().sound[i-nb_snd];
                bank_modif.get().sound[i] = bank_modif.get().sound[i-nb_snd];
            };
        };
    }, bank_origin_dest, bank_modif_dest);
};
void Dx7interface::read_voice(unsigned int data_stream_index, BankVariant& bank_origin_dest, BankVariant& bank_modif_dest, unsigned int max_write_pos, bool byte_flag){
    std::visit([&](auto& bank_origin, auto& bank_modif) {
        unsigned int pos=get_gwidget<Gtk::SpinButton>("spinbutton_insert_start")->get_value();;
        if(byte_flag){
            for( ; pos < bank_nb_sound && pos < max_write_pos; pos++ ){
                seek_voice_by_byte(data_stream.at(data_stream_index), &bank_origin.get().sound[pos]);
                bank_modif.get().sound[pos] = bank_origin.get().sound[pos];
            };
        }else{
            for( ; pos < bank_nb_sound && pos < max_write_pos; pos++ ){
                seek_voice(data_stream.at(data_stream_index), &bank_origin.get().sound[pos]);
                bank_modif.get().sound[pos] = bank_origin.get().sound[pos];
            };
        };
    }, bank_origin_dest, bank_modif_dest);
};
void Dx7interface::prepare_bank(unsigned int data_stream_index, unsigned int nb_snd_in_file, unsigned int pos_end_ins, bool byte_flag){
    unsigned int bank_nb_sound_src = bank_nb_sound;

    auto [bank_origin_src, bank_modif_src] = get_banks_source();
    auto [bank_nb_sound_dest, banks_pair] = get_banks_dest(pos_end_ins+1); // return the destination bank depending of total_snd
    auto [bank_origin_dest, bank_modif_dest] = banks_pair;
    if(bank_nb_sound_src != bank_nb_sound_dest){ //copy bank in new bank
        copy_bank(
            bank_origin_src,
            bank_origin_dest,
            bank_modif_src,
            bank_modif_dest,
            bank_nb_sound_src,bank_nb_sound_dest);
    }
    bank_nb_sound = bank_nb_sound_dest;
    moove_sound(bank_origin_dest, bank_modif_dest, pos_end_ins, nb_snd_in_file);
    read_voice(data_stream_index, bank_origin_dest, bank_modif_dest, pos_end_ins, byte_flag);

    update_data_model_full<SoundBankItem>(bank_data_model, bank_modif_dest);
};
void Dx7interface::insert_at(unsigned int data_stream_index, Glib::RefPtr<Gio::File> file,Glib::ustring file_name,Glib::ustring file_base,unsigned int file_size){
    /*
    * modes :
    *  default push other sound to next position
    *  si taille banque = 32 et position 32 => concatene -> banque 128 sons ( 32 + x sons + x init sounds)
    *  si taille banque = 1 => concatene -> banque 32/128 sons ( 1 + x sons + x init )
    *  si taille banque 128 => insert at and push out others
    */
    unsigned int nb_snd_in_file = 0;
    unsigned int pos_end_ins = 0;
    bool byte_flag = false;
    unsigned int insert_at_pos = get_gwidget<Gtk::SpinButton>("spinbutton_insert_start")->get_value();

    block_ui(); // needed

    update_bank_modif(); // to save change

    parse_sysex(file, data_stream.at(data_stream_index), file_size);
    if( (file_size % 128) == 0 ){                    // file is multiple of 128
        nb_snd_in_file = (file_size / 128);
        byte_flag=false;
    }else if( (file_size % 128) != 0 ){              // file is multiple of 155
        nb_snd_in_file = (file_size / 155);
        byte_flag=true;
    };
    pos_end_ins = insert_at_pos + nb_snd_in_file;
    prepare_bank(data_stream_index, nb_snd_in_file, pos_end_ins, byte_flag);
    seek_parameters(file_base, &bank_1_modif.sound[0]);

    if (old_snum >= insert_at_pos){ // si le son a ete deplacé
        // positionner la selection a la nouvelle position de la voix en cours pour ne pas changer le son
        unsigned int new_pos_selected_snd = old_snum + nb_snd_in_file;
        if ( new_pos_selected_snd < bank_nb_sound){ // si l'ancien son encore present dans la banque
            old_snum = new_pos_selected_snd;
            snum = new_pos_selected_snd;
        }else{ // sinon selectionner le premier son de l'insert et mettre a jour bank_1_X
            old_snum = insert_at_pos;
            snum = insert_at_pos;
        };
        unmooved_sound = false;
        update_bank_modif();
        unmooved_sound = true;
    }
    select_voice(snum);
};

void Dx7interface::on_insert_sound(unsigned int data_stream_index, Glib::RefPtr<Gio::File> file){
    dialog_param->close();
    read_file_as_datastream(data_stream_index, file,std::bind(&Dx7interface::insert_at, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
};
void Dx7interface::on_insert_at(){
    // Open dialog insert at position
    try{
        action_type = ACT_INSERT;
        slot_btn_dialog_param = btn_dialog_param->signal_clicked().connect(
            sigc::bind(
                sigc::mem_fun(*this, &Dx7interface::OpenDialogFileSelect),
                0,
                std::bind(&Dx7interface::on_insert_sound, this, std::placeholders::_1,std::placeholders::_2) ));
        OpenDialogParam(_("Insert Sound(s) at"));
    }catch( const std::exception& ex ){
        std::string err_msg = error( __PRETTY_FUNCTION__, "???", ex.what() );
        LOG_ERR( err_msg );
        LOG( LOG_OUT() );
    };
};

/* DELETE */
void Dx7interface::on_delete_sound(){
    LOG( LOG_IN() );
    auto pos_snd = bank_selection_model->get_selected();
    auto [bank_origin_src, bank_modif_src] = get_banks_source();
    moove_sound(bank_origin_src,bank_modif_src,pos_snd,-1);
    old_snum = bank_nb_sound-1;
    snum = bank_nb_sound-1;
    Glib::RefPtr<Gio::File> init_voice_file = Gio::File::create_for_path( DATA_DIR"cfg/DX7_INIT_VOICE.syx" );
    read_file_as_datastream(0, init_voice_file,std::bind(&Dx7interface::replace_sound, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,std::placeholders::_5));
    update_data_model_full<SoundBankItem>(bank_data_model, bank_modif_src);
    LOG( LOG_OUT() );
};

/* SEND / SAVE */
void Dx7interface::on_send_bank(){
    send_bank=true;
    write_bank_as_sysex(0, nullptr, 0);
};
void Dx7interface::on_save_bank(){
    try{
        action_type = ACT_SAVE;
        save_type = BANK;
        save_modif_sound();

        slot_btn_dialog_param = btn_dialog_param->signal_clicked().connect(
            sigc::bind(
                sigc::mem_fun(*this, &Dx7interface::OpenDialogFileSave),
                    0,
                    std::bind(&Dx7interface::on_file_save, this, std::placeholders::_1,std::placeholders::_2)));
        OpenDialogParam("Saving Bank");
    }catch( const std::exception & ex ){
        std::string err_msg = error( __PRETTY_FUNCTION__, "???", ex.what() );
        LOG_ERR( err_msg );
    };
};
void Dx7interface::write_bank(unsigned int data_stream_index, Glib::RefPtr<Gio::File> file, unsigned int index){
    LOG( LOG_IN() );
    switch( export_config ){
        case DX7_1:        /* write bulk1 bank */
        case DX7_32:       /* write bulk32 bank */
            write_bank_as_sysex(data_stream_index, file, index);
            break;
        case DX7_128: {    /* write 4x bulk32 bank */
            Glib::ustring filename = file->get_path();
            Glib::ustring bank_file_base = filename.substr(0,filename.find_last_of("."));
            for(unsigned int i = 0; i < 127; i=i+32){
                file = Gio::File::create_for_path( (bank_file_base+"_"+tostr<int>(i)+"_"+tostr<int>(i+31)+".syx").c_str() );
                write_bank_as_sysex(data_stream_index, file, i);
            }
            break;
        }
        case DX7_RAW: {     /* write raw bank */
            /* sound without sysex headers (HEXTER 128) */
            write_bank_as_raw(data_stream_index, file, index);
            break;
        }
    };
    LOG( LOG_OUT() );
};
/* Dx7 format bulk sysex */
void Dx7interface::write_bank_as_sysex(unsigned int data_stream_index, Glib::RefPtr<Gio::File> file,unsigned int index){
    LOG( LOG_IN() );
    /*
     1 1*110000  F0   Status byte - start sysex
     0iiiiiii  43   ID # (i=67; Yamaha)
     0SSSNNNN  00   Sub-status (s=0) & channel number (n=0; ch 1)
     0FFFFFFF  00   format number (F=0; 1 voice)(F=9; 32 voices)
     0BBBBBBB  01   byte count MS byte
     0BBBBBBB  1B   byte count LS byte (B=155; 1 voice)(B=4096; 32voices)
     0DDDDDDD  **   data byte 1
     |       |       |
     0DDDDDDD  **   data byte 155
     0EEEEEEE  **   checksum (masked 2's complement of sum of 155 bytes)
     11110111  F7   Status - end sysex
     */
    try{
        uint8_t voice_checksum = 0;
        unsigned int l=6, msg_size;
        Bank_ptr bank_ptr;
        unsigned char msb,lsb,format_nb;
        switch( bank_nb_sound ){
            case 1:{
                msg_size = 163;
                format_nb = 0x00;
                msb = 0x01;
                lsb = 0x1B;
                bank_ptr = reinterpret_cast<Bank_ptr>(&bank_1_modif);
                break;
            }
            case 32:{
                msg_size = 4104;
                format_nb = 0x09;
                msb = 0x20;
                lsb = 0x00;
                bank_ptr = reinterpret_cast<Bank_ptr>(&bank_32_modif);
                break;
            }
            case 128:{
                msg_size = 4104;
                format_nb = 0x09;
                msb = 0x20;
                lsb = 0x00;
                bank_ptr = reinterpret_cast<Bank_ptr>(&bank_128_modif);
                break;
            }
        };
        unsigned char msg[msg_size];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=0x00 + channel_send;
        msg[3]=format_nb;
        msg[4]=msb;
        msg[5]=lsb;
        if(bank_nb_sound == 1){
            write_voice_bulk1(&l, msg, &bank_ptr->sound[0], &voice_checksum );
        }else{
            for (unsigned int i=index; i<(32+index); i++){
                write_voice_bulk32(&l, msg, &bank_ptr->sound[i], &voice_checksum );
            };
        };
        msg[l++]=(unsigned char)(voice_checksum & 0x7F) ;
        msg[l]=0xF7;
        if( send_bank ){
            if( bank_nb_sound > 0){
                send_midi(SND_SEQ_EVENT_SYSEX, msg_size, msg);
                send_bank=false;
            };
        }else{
            write_file_as_datastream(data_stream_index, file, msg, msg_size);
        };
    }catch( const std::exception& ex ){
        std::string err_msg = error( __PRETTY_FUNCTION__, _(" Error writing to file: \n") + file->get_path(), ex.what() );
        LOG_ERR( err_msg );
    }
    /*
    string msg = "varaiable l: " + (int)l  ;
    LOG( msg );
    for (unsigned int i = 0 ; i < size ; i++){
        msg = std::hex + std::setw(2) + std::setfill('0') + (int)(msg[i] & 0xFF)+ " " ;
        LOG( msg );
    };
    msg = std::dec ;
    */
    LOG( LOG_OUT() );
};
void Dx7interface::write_bank_as_raw(unsigned int data_stream_index, Glib::RefPtr<Gio::File> file, unsigned int index){
    /* index to export from a sound number */
    LOG( LOG_IN() );
    try{
        uint8_t voice_checksum = 0;
        unsigned int l=0, msg_size, msg_extra_size, msg_extra_index=0;
        Bank_ptr bank_ptr;
        switch( bank_nb_sound ){
            case 1:{
                msg_size = 155;
                msg_extra_size = 98;
                bank_ptr=reinterpret_cast<Bank_ptr>(&bank_1_modif);
                break;
            }
            case 32:{
                msg_size = 4096;
                msg_extra_size = 3136;
                bank_ptr=reinterpret_cast<Bank_ptr>(&bank_32_modif);
                break;
            }
            case 128:{
                bank_ptr=reinterpret_cast<Bank_ptr>(&bank_128_modif);
                if (get_gwidget<Gtk::CheckButton>("checkbutton_32")->get_active()){
                    msg_size = 4096;
                    msg_extra_size = 3136;
                }else{
                    msg_size = 16384;
                    msg_extra_size = 12544;
                };
                break;
            }
        };
        unsigned char msg[msg_size];
        unsigned char msg_extra[msg_extra_size];
        if(bank_nb_sound == 1){
            write_voice_bulk1(&l, msg, &bank_ptr->sound[0], &voice_checksum );
            write_voice_extra_parameters(&bank_ptr->sound[0],msg_extra,&msg_extra_index);
        }else{
            for (unsigned int i=index, msg_extra_index=0; i<((msg_size/128)+index); i++){
                write_voice_bulk32(&l, msg, &bank_ptr->sound[i], &voice_checksum );
                write_voice_extra_parameters(&bank_ptr->sound[i],msg_extra,&msg_extra_index);
            };
        };
        write_file_as_datastream(data_stream_index, file, msg, msg_size);
        if(write_extra_params){
            Glib::ustring file_full = file->get_path();
            Glib::ustring file_base = file_full.substr(0,file_full.find_last_of("."));
            Glib::RefPtr<Gio::File> file_extra=Gio::File::create_for_path( (file_base+"_fct.syx").c_str() );
            write_file_as_datastream(data_stream_index, file_extra, msg_extra, msg_extra_size);
        };
    }catch( const std::exception& ex ){;
        std::string err_msg = error( __PRETTY_FUNCTION__, _(" Error writing to file: \n") + file->get_path(), ex.what() );
        LOG_ERR( err_msg );
        LOG( LOG_OUT() );
    }
    /*for (unsigned int i = 0 ; i < size ; i++){
        msg = std::hex + std::setw(2) + std::setfill('0') + (int)(msg[i] & 0xFF)+ " " ;
    };
    msg = std::dec ;
    msg = "varaiable l: " + (int)l  ;
    msg = "DX_: " + (int)export_config  ;
    msg = "index: " + (int)index  ;
    msg = "file: " + file->get_path()  ;
    msg = "size: " + size  ;*/
    LOG( LOG_OUT() );
};
/*** VOICE ***/
/* Sound Name modification */
Glib::ustring Dx7interface::check_sound_name(Glib::ustring sound_name){
    // LOG( LOG_IN() );
    sound_name = sound_name.substr(0, 10);
    unsigned int missing_char = 10 - sound_name.length();
    for( unsigned int i=0; i < missing_char ;i++){
        sound_name += ' ';
    }
    sound_name = str_to_ascii(sound_name);
    // LOG( LOG_OUT() );
    return sound_name;
};
template<class ListStoreType>
void Dx7interface::update_param_data_model(Glib::RefPtr<Gio::ListStore<ListStoreType>> data_model, unsigned int index_element, Glib::ustring name){
    auto param = ListStoreType::create(name);
    if( !data_model->get_item(index_element) ){
        data_model->insert(index_element, param);
    }else{
        data_model->splice(index_element, 1, {param});  // Replace item at same position
    };
};
template<class ListStoreType>
void Dx7interface::update_data_model(Glib::RefPtr<Gio::ListStore<ListStoreType>> data_model, Glib::ustring sound_name){
    auto sound = ListStoreType::create(snum,sound_name);
    if( !data_model->get_item(snum) ){
        data_model->insert(snum, sound);
    }else{
        data_model->splice(snum, 1, {sound});  // Replace item at same position
    };
};
template<class ListStoreType>
void Dx7interface::update_data_model_full(Glib::RefPtr<Gio::ListStore<ListStoreType>> data_model, BankVariant& bank_modif_dest){
    unsigned int temp_snum = snum;
    std::visit([&]( auto& bank_modif ) { // update the datamodel
        for ( snum = 0 ; snum < bank_nb_sound; snum++ ){
            update_data_model<ListStoreType>(data_model, bank_modif.get().sound[snum].name);
        };
    }, bank_modif_dest );
    snum=temp_snum;
};
void Dx7interface::set_sound_name(Glib::ustring sound_name){
    // LOG( LOG_IN() );
    bank_1_modif.sound->name = sound_name;
    update_data_model<SoundBankItem>(bank_data_model, sound_name);
    // LOG( LOG_OUT() );
};
void Dx7interface::on_sound_name_event(){
    // LOG( LOG_IN() );
    std::string sound_name = (get_gwidget<Gtk::Entry>("entry_sound_name"))->get_text();
    if( sound_name != (bank_1_modif.sound->name).c_str() ){
        set_sound_name(check_sound_name(sound_name));
    };
    // LOG( LOG_OUT() );
};

void Dx7interface::select_voice(unsigned int position){
    slot_selected_sound_change.unblock();
    #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 12)
        (get_gwidget<Gtk::ColumnView>("columnview_bank"))->add_tick_callback([this, position](const Glib::RefPtr<Gdk::FrameClock>&) {
            (get_gwidget<Gtk::ColumnView>("columnview_bank"))->scroll_to(position, nullptr, Gtk::ListScrollFlags::SELECT);
            return false; // Return false to remove the callback after one executio
        });
    #else
        bool first_run = true;
        (get_gwidget<Gtk::ColumnView>("columnview_bank"))->add_tick_callback([this, position, first_run](const Glib::RefPtr<Gdk::FrameClock>&) mutable {
            if(first_run) { // need to skip first ticks callback to select element
                first_run = false;
                return true;
            };
            auto adjustment = get_gwidget<Gtk::ColumnView>("columnview_bank")->get_vadjustment();
            adjustment->set_value((double)position);
            bank_selection_model->set_selected((unsigned int)position);
            return false; // Return false to remove the callback after one executio
        });
    #endif
};
void Dx7interface::write_voice_bulk1(unsigned int* l, unsigned char* msg, St_dx7sysex_1* sound, uint8_t* voice_checksum){
    try{
        unsigned int j, k;
        for ( j = 6; j-- != 0 ; ){
            /* OP[J] EG RATE[k] */
            for ( k = 0; k < 4 ; k++ ){
                msg[(*l)++]=sound->op[j].eg_rt[k].val & 0x7F;
                *voice_checksum -= msg[(*l)-1];
            };
            /* OP[J] EG LVL[k] */
            for ( k = 0; k < 4 ; k++  ){
                msg[(*l)++]=sound->op[j].eg_lvl[k].val & 0x7F;
                *voice_checksum -= msg[(*l)-1];
            };
            msg[(*l)++] = sound->op[j].kls.brk_pt.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++] = sound->op[j].kls.lft_dpth.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++] = sound->op[j].kls.rght_dpth.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++]= sound->op[j].kls.lft_curve.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++]= sound->op[j].kls.rght_curve.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];

            msg[(*l)++]= sound->op[j].krs.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++] = sound->op[j].ams.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++] = sound->op[j].kvs.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];

            msg[(*l)++] = sound->op[j].lvl.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];

            msg[(*l)++] = sound->op[j].freq_mode.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++] = sound->op[j].freq_coarse.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++] = sound->op[j].freq_fine.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++]= sound->op[j].dtun.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
        };
        for(j=0 ; j <= 3; j++ ){
            msg[(*l)++] = sound->pitch.eg_rt[j].val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
        };
        for(j=0 ; j <= 3; j++ ){
            msg[(*l)++] = sound->pitch.eg_lvl[j].val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
        };
        msg[(*l)++] = sound->algo.algo.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = sound->algo.feedback.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = sound->algo.oks.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];

        msg[(*l)++] = sound->lfo.speed.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = sound->lfo.delay.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = sound->lfo.pmd.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = sound->lfo.amd.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = sound->lfo.sync.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = sound->lfo.wave.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = sound->lfo.pms.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];

        msg[(*l)++] = sound->algo.transpose.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];
        for (uint8_t carac = 0 ; carac <= 9 ; carac++ ){
            msg[(*l)++] = sound->name.data()[carac];
            *voice_checksum -= msg[(*l)-1];
        };
    }catch( const std::exception& ex ){
        /*
         * std::string err_msg = _("From: ") + std::string(__PRETTY_FUNCTION__) + "\n\t"
         *                     + _("Cannot construct bulk1") + "\n\t"
         *                     + _("Reason: ") + ex.what();
         */
        std::string err_msg = error( __PRETTY_FUNCTION__, _("Cannot construct bulk1"), ex.what() );
        LOG_ERR( err_msg );
    };
};
void Dx7interface::write_voice_bulk32(unsigned int* l, unsigned char* msg, St_dx7sysex_1* sound, uint8_t* voice_checksum){
    try {
        /* TODO : check original sound format and other kind */
        unsigned int j, k;
        /* operator j */
        for ( j = 6; j-- != 0 ; ){
            /* OP[J] EG RATE[k] */
            for ( k = 0; k < 4 ; k++ ){
                msg[(*l)++]=sound->op[j].eg_rt[k].val & 0x7F;
                *voice_checksum -= msg[(*l)-1];
            };
            /* OP[J] EG LVL[k] */
            for ( k = 0; k < 4 ; k++  ){
                msg[(*l)++]=sound->op[j].eg_lvl[k].val & 0x7F;
                *voice_checksum -= msg[(*l)-1];
            };
            msg[(*l)++] = sound->op[j].kls.brk_pt.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++] = sound->op[j].kls.lft_dpth.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++] = sound->op[j].kls.rght_dpth.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++] = ( (sound->op[j].kls.rght_curve.val + 2) + (sound->op[j].kls.lft_curve.val & 0x03) ) & 0x0F ;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++] = ( (sound->op[j].dtun.val + 3) + (sound->op[j].krs.val & 0x07) ) & 0x7F;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++] = ( (sound->op[j].kvs.val + 2) + (sound->op[j].ams.val & 0x03) ) & 0x1F;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++] = sound->op[j].lvl.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++] = ( ( sound->op[j].freq_coarse.val + 1) + (sound->op[j].freq_mode.val & 0x01)  ) & 0x3F;
            *voice_checksum -= msg[(*l)-1];
            msg[(*l)++] = sound->op[j].freq_fine.val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
        };
        for(j=0 ; j < 4; j++ ){
            msg[(*l)++] = sound->pitch.eg_rt[j].val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
        };
        for(j=0 ; j < 4; j++ ){
            msg[(*l)++] = sound->pitch.eg_lvl[j].val & 0x7F;
            *voice_checksum -= msg[(*l)-1];
        };
        msg[(*l)++] = sound->algo.algo.val & 0x1F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = ( (sound->algo.oks.val + 3) + (sound->algo.feedback.val & 0x07) ) & 0x0F;
        *voice_checksum -= msg[(*l)-1];

        msg[(*l)++] = sound->lfo.speed.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = sound->lfo.delay.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = sound->lfo.pmd.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = sound->lfo.amd.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = ( ( sound->lfo.pms.val + 4) + ( (sound->lfo.wave.val & 0x07) +1 ) + (sound->lfo.sync.val & 0x01) ) & 0x7F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = sound->algo.transpose.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];

        for (uint8_t carac = 0 ; carac <= 9 ; carac++ ){
            msg[(*l)++] = sound->name.data()[carac];
        };
        *voice_checksum -= msg[(*l)-1];
    }catch( const std::exception& ex ){
        std::string err_msg = error( __PRETTY_FUNCTION__, _("Cannot construct bulk32"), ex.what() );
        LOG_ERR( err_msg );
    };
};
void Dx7interface::write_voice_as_sysex(unsigned int data_stream_index, Glib::RefPtr<Gio::File> file){
    try {
        uint8_t voice_checksum=0;
        unsigned int l = 6;
        unsigned int msg_size=163;
        unsigned char msg[msg_size];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=0x00 + channel_send;
        msg[3]=0x00;
        msg[4]=0x01;
        msg[5]=0x1B;
        write_voice_bulk1(&l, msg, &bank_1_modif.sound[0], &voice_checksum );
        msg[161]=(unsigned char)(voice_checksum & 0x7F);
        msg[162]=0xF7;
        write_file_as_datastream(data_stream_index, file, msg, msg_size);
    }catch( const std::exception& ex ){
        std::string err_msg = error( __PRETTY_FUNCTION__, _("Cannot write file as sysex"), ex.what() );
        LOG_ERR( err_msg );
    }
};
void Dx7interface::write_voice_as_raw(unsigned int data_stream_index, Glib::RefPtr<Gio::File> file){
    try {
        uint8_t voice_checksum=0;
        unsigned int l = 0;
        unsigned int msg_size = 128;
        unsigned char msg[msg_size];
        write_voice_bulk32(&l, msg, &bank_1_modif.sound[0], &voice_checksum );
        write_file_as_datastream(data_stream_index, file, msg, msg_size);
    }catch( const std::exception& ex ){
        std::string err_msg = error( __PRETTY_FUNCTION__, _("Cannot write file as raw"), ex.what() );
        LOG_ERR( err_msg );
    }
};
void Dx7interface::write_voice_extra_parameters(st_dx7sysex_1* sound,unsigned char* msg, unsigned int* index){
    LOG( LOG_IN() );
    if(write_extra_params){
        try{
            // size = 98
            unsigned int i, j;
            unsigned int nb_elem = 14;
            uint8_t val[] = {
                sound->extra.functions.poly_mono.val,
                sound->extra.functions.ptch_bnd_rng.val,
                sound->extra.functions.ptch_bnd_stp.val,
                sound->extra.functions.portamento_md.val,
                sound->extra.functions.portamento_glss.val,
                sound->extra.functions.portamento_tm.val,
                sound->extra.functions.md_whl_rng.val,
                sound->extra.functions.md_whl_assgn.val,
                sound->extra.functions.foot_rng.val,
                sound->extra.functions.foot_assgn.val,
                sound->extra.functions.brth_rng.val,
                sound->extra.functions.brth_assgn.val,
                sound->extra.functions.aftrtch_rng.val,
                sound->extra.functions.aftrtch_assgn.val,
            };
            for ( i=0, j=64; i< nb_elem; i++,j++){
                /*
                 *              msg = "char j: " + std::hex + (int)j + std::dec ;
                 *              msg = "i: " + (int)i ;
                 *              msg = "sound val: " + std::hex + (int)val[i] + std::dec ;
                 */
                msg[(*index)++]=0xF0;
                msg[(*index)++]=id_fabricant;
                msg[(*index)++]=sub_status + channel_send;
                msg[(*index)++]=0x08;
                msg[(*index)++]=j;
                msg[(*index)++]=val[i];
                msg[(*index)++]=0xF7;
            };
        }catch( const std::exception& ex ){
            std::string err_msg = error( __PRETTY_FUNCTION__, _("Cannot write extra parameters"), ex.what() );
            LOG_ERR( err_msg );
        };
    };
    LOG( LOG_OUT() );
};
void Dx7interface::save_modif_sound(){
    // save current sound (0) in bank modif (old_snum)
    // comment => Et origin
    std::string msg = _("Saved voice: ") + bank_1_modif.sound->name + _(" in modif bank.") ;
    LOG( msg );
    // bank_1_origin.sound[0] = bank_1_modif.sound[0];
    switch( bank_nb_sound ){
        case 32:
            bank_32_modif.sound[old_snum] = bank_1_modif.sound[0];
            // bank_32_origin.sound[old_snum] = bank_1_origin.sound[0];
            break;
        case 128:
            bank_128_modif.sound[old_snum] = bank_1_modif.sound[0];
            // bank_128_origin.sound[old_snum] = bank_1_origin.sound[0];
            break;
    };
};
void Dx7interface::on_save_sound(){
    LOG( LOG_IN() );
    try{
        action_type = ACT_SAVE;
        save_type = SOUND;
        save_modif_sound();

        slot_btn_dialog_param = btn_dialog_param->signal_clicked().connect(
            sigc::bind(
                sigc::mem_fun(*this, &Dx7interface::OpenDialogFileSave),
                       0,
                       std::bind(&Dx7interface::on_file_save, this, std::placeholders::_1, std::placeholders::_2) ));
        OpenDialogParam("Saving sound");
    }catch( const std::exception & ex ){
        std::string err_msg = error( __PRETTY_FUNCTION__, _(" ??? "), ex.what() );
        LOG_ERR( err_msg );
    };
    LOG( LOG_OUT() );
};
/* extended sysex */
void Dx7interface::write_voices_as_n_sysex(St_dx7sysex_1* sound){  // UNUSED
    /* TODO : check original sound format and other kind */
    uint8_t l=0, j, k;
    unsigned char msg[128];
    /* operator j */
    for ( j = 6; j-- != 0 ; ){
        /* OP[J] EG RATE[k] */
        for ( k = 0; k < 4 ; k++ ){
            msg[l++]=sound->op[j].eg_rt[k].val & 0x7F;
        };
        /* OP[J] EG LVL[k] */
        for ( k = 0; k < 4 ; k++  ){
            msg[l++]=sound->op[j].eg_lvl[k].val & 0x7F;
        };
        msg[l++] = sound->op[j].kls.brk_pt.val & 0x7F;
        msg[l++] = sound->op[j].kls.lft_dpth.val & 0x7F;
        msg[l++] = sound->op[j].kls.rght_dpth.val & 0x7F;
        msg[l++] = ( (sound->op[j].kls.lft_curve.val + 2) + (sound->op[j].kls.rght_curve.val & 0x03) ) & 0x0F ;
        msg[l++] = ( (sound->op[j].dtun.val + 3) + (sound->op[j].krs.val & 0x07) ) & 0x7F;
        msg[l++] = ( (sound->op[j].kvs.val + 2) + (sound->op[j].ams.val & 0x03) ) & 0x1F;
        msg[l++] = sound->op[j].lvl.val & 0x7F;
        msg[l++] = ( ( sound->op[j].freq_coarse.val + 1) + (sound->op[j].freq_mode.val & 0x01)  ) & 0x3F;
        msg[l++] = sound->op[j].freq_fine.val & 0x7F;
    };
    for(j=0 ; j < 4; j++ ){
        msg[l++] = sound->pitch.eg_rt[j].val & 0x7F;
    };
    for(j=0 ; j < 4; j++ ){
        msg[l++] = sound->pitch.eg_lvl[j].val & 0x7F;
    };
    msg[l++] = sound->algo.algo.val & 0x1F;
    msg[l++] = ( (sound->algo.oks.val + 3) + (sound->algo.feedback.val & 0x07) ) & 0x0F;

    msg[l++] = sound->lfo.speed.val & 0x7F;
    msg[l++] = sound->lfo.delay.val & 0x7F;
    msg[l++] = sound->lfo.pmd.val & 0x7F;
    msg[l++] = sound->lfo.amd.val & 0x7F;
    msg[l++] = ( ( sound->lfo.pms.val + 4) + ( (sound->lfo.wave.val & 0x07) +1 ) + (sound->lfo.sync.val & 0x01) ) & 0x7F;
    msg[l++] = sound->algo.transpose.val & 0x7F;

    for (uint8_t carac = 0 ; carac <= 9 ; carac++ ){
        msg[l++] = sound->name.data()[carac];
    };
    /*
     * std::string msg = "Voice Output: ";
     * LOG( msg );
     *
     * for (int i = 0 ; i < 128 ; i++){
     *   msg = std::hex + std::setw(2) + std::setfill('0') + (int)(msg[i] & 0x7F)+ " " ;
     * };
     * std::cout << std::dec ;
     */

};

/** Save current sound on bank_X_modif and load set_selected sound from bank_X_modif**/
void Dx7interface::update_bank_modif(){
    switch( bank_nb_sound ){
        case 32:
            // save current sound (0) in bank modif (old_snum)
            if(unmooved_sound){
                bank_32_modif.sound[old_snum] = bank_1_modif.sound[0];
            };
            // copy actual sound index in current
            bank_1_modif.sound[0]= bank_32_modif.sound[snum];
            bank_1_origin.sound[0]= bank_32_origin.sound[snum];
            break;
        case 128:
            // save current sound (0) in bank modif (old_snum)
            if(unmooved_sound){
                bank_128_modif.sound[old_snum] = bank_1_modif.sound[0];
            };
            // copy actual sound index in current
            bank_1_modif.sound[0]= bank_128_modif.sound[snum];
            bank_1_origin.sound[0]= bank_128_origin.sound[snum];
            break;
    };
};
void Dx7interface::on_selected_sound_change(unsigned int num, unsigned int nb_elmnt){
    // to be trigerred on by click or by select_voice().
    // his slot in ui blocking must not be remove:
    //      if so, scrolling will not work as expected
    //      use unblock() his slot in functions that need to trigger
    //      set_voice(). (cf. set_bank())
    // avoid calling it directly as it will stack the calls.
    // prefer use select_voice(). (cf. replace_sound())
    LOG( LOG_IN() );
    snum = bank_selection_model->get_selected();
    update_bank_modif();
    old_snum = snum;
    (get_gwidget<Gtk::ToggleButton>("btn_compare"))->set_active(false);
    set_voice(&bank_1_modif.sound[0]);
    LOG( LOG_OUT() );
};

/* read: seek (in bank file)*/
void Dx7interface::seek_voice(Glib::RefPtr<Gio::DataInputStream> data_stream, St_dx7sysex_1* sound){ /* BULK 32 */
    //LOG( LOG_IN() );
    uint8_t val,j,k;
    /* operator j */
    for ( j = 6; j-- != 0 ; ){
        /* OP[J] EG RATE[k] */
        for ( k = 0; k < 4 ; k++ ){
            sound->op[j].eg_rt[k].val=data_stream->read_byte() & 0x7F;
        };
        /* OP[J] EG LVL[k] */
        for ( k = 0; k < 4 ; k++  ){
            sound->op[j].eg_lvl[k].val=data_stream->read_byte() & 0x7F;
        };
        sound->op[j].kls.brk_pt.val=data_stream->read_byte() & 0x7F;
        sound->op[j].kls.lft_dpth.val=data_stream->read_byte() & 0x7F;
        sound->op[j].kls.rght_dpth.val=data_stream->read_byte() & 0x7F;
        /* set out first 4 unused bit by mask (addr bit 11)*/
        val=data_stream->read_byte() & 0x0F ;
        /* first two bit to rc and last two bit to lc*/
        sound->op[j].kls.lft_curve.val=val & 0x03 ;
        sound->op[j].kls.rght_curve.val=val >> 2;
        /* set out first 1 unused bit by mask  (addr bit 12)*/
        val=data_stream->read_byte() & 0x7F;
        sound->op[j].krs.val=val &  0x07;
        sound->op[j].dtun.val=val >> 3 ;
        /* set out first 3 unused bit by mask  (addr bit 13)*/
        val=data_stream->read_byte() & 0x1F;
        sound->op[j].ams.val=val & 0x03;
        sound->op[j].kvs.val=val >> 2;

        sound->op[j].lvl.val=data_stream->read_byte() & 0x7F;
        /* set out first 2 unused bit by mask  (addr bit 15)*/
        val=data_stream->read_byte() & 0x3F;
        sound->op[j].freq_mode.val=val & 0x01;
        sound->op[j].freq_coarse.val=val >> 1;
        sound->op[j].freq_fine.val=data_stream->read_byte() & 0x7F;
    };
    /* addr 102 */
    for( j=0 ; j < 4; j++ ){
        sound->pitch.eg_rt[j].val=data_stream->read_byte() & 0x7F;
    };
    for( j=0 ; j < 4; j++ ){
        sound->pitch.eg_lvl[j].val=data_stream->read_byte() & 0x7F;
    };
    /* addr 110 */
    sound->algo.algo.val=data_stream->read_byte() & 0x1F ;
    val=data_stream->read_byte() & 0x0F;
    sound->algo.feedback.val=val & 0x07;
    sound->algo.oks.val=val >> 3;
    sound->lfo.speed.val=data_stream->read_byte() & 0x7F;
    sound->lfo.delay.val=data_stream->read_byte() & 0x7F;
    sound->lfo.pmd.val=data_stream->read_byte() & 0x7F;
    sound->lfo.amd.val=data_stream->read_byte() & 0x7F;
    /* (addr bit 116) */
    val=data_stream->read_byte() & 0x7F;
    sound->lfo.sync.val=val & 0x01;
    sound->lfo.wave.val=(val >> 1)&0x07;
    sound->lfo.pms.val=val >> 4;
    sound->algo.transpose.val=data_stream->read_byte() & 0x7F;
    std::ostringstream strm;
    for( j=0; j <= 9; j++ ){
        strm << (data_stream->read_byte());
    };
    sound->name=strm.str();
    sound->extra.mute.val=0x7F;
    /* add voice name to liststore */
    update_data_model<SoundBankItem>(bank_data_model, sound->name);
    //LOG( LOG_OUT() );
};
void Dx7interface::seek_voice_by_byte(Glib::RefPtr<Gio::DataInputStream> data_stream, St_dx7sysex_1* sound){ /* BULK 1 */
    //LOG( LOG_IN() );
    uint8_t j,k;
    /* operator j */
    for ( j = 6; j-- != 0 ; ){
        /* OP[J] EG RATE[k] */
        for ( k = 0; k < 4 ; k++ ){
            sound->op[j].eg_rt[k].val=data_stream->read_byte() & 0x7F;
        };
        /* OP[J] EG LVL[k] */
        for ( k = 0; k < 4 ; k++  ){
            sound->op[j].eg_lvl[k].val=data_stream->read_byte() & 0x7F;
        };
        sound->op[j].kls.brk_pt.val=data_stream->read_byte() & 0x7F;
        sound->op[j].kls.lft_dpth.val=data_stream->read_byte() & 0x7F;
        sound->op[j].kls.rght_dpth.val=data_stream->read_byte() & 0x7F;
        sound->op[j].kls.lft_curve.val=data_stream->read_byte() & 0x7F;
        sound->op[j].kls.rght_curve.val=data_stream->read_byte() & 0x7F;
        sound->op[j].krs.val=data_stream->read_byte() & 0x7F;
        sound->op[j].ams.val=data_stream->read_byte() & 0x7F;
        sound->op[j].kvs.val=data_stream->read_byte() & 0x7F;
        sound->op[j].lvl.val=data_stream->read_byte() & 0x7F;
        sound->op[j].freq_mode.val=data_stream->read_byte() & 0x7F;
        sound->op[j].freq_coarse.val=data_stream->read_byte() & 0x7F;
        sound->op[j].freq_fine.val=data_stream->read_byte() & 0x7F;
        sound->op[j].dtun.val=data_stream->read_byte() & 0x7F;
    };
    for( j=0 ; j < 4; j++ ){
        sound->pitch.eg_rt[j].val=data_stream->read_byte() & 0x7F;
    };
    for( j=0 ; j < 4; j++ ){
        sound->pitch.eg_lvl[j].val=data_stream->read_byte() & 0x7F;
    };
    sound->algo.algo.val=data_stream->read_byte() & 0x7F ;
    sound->algo.feedback.val=data_stream->read_byte() & 0x7F;
    sound->algo.oks.val=data_stream->read_byte() & 0x7F;
    sound->lfo.speed.val=data_stream->read_byte() & 0x7F;
    sound->lfo.delay.val=data_stream->read_byte() & 0x7F;
    sound->lfo.pmd.val=data_stream->read_byte() & 0x7F;
    sound->lfo.amd.val=data_stream->read_byte() & 0x7F;
    sound->lfo.sync.val=data_stream->read_byte() & 0x7F;
    sound->lfo.wave.val=data_stream->read_byte() & 0x7F;
    sound->lfo.pms.val=data_stream->read_byte() & 0x7F;
    sound->algo.transpose.val=data_stream->read_byte() & 0x7F;
    std::ostringstream strm;
    for( j=0; j <= 9; j++ ){
        strm << (data_stream->read_byte());
    };
    sound->name=strm.str();
    sound->extra.mute.val=0x7F;
    /* add voice name to liststore */
    update_data_model<SoundBankItem>(bank_data_model, sound->name);
    //LOG( LOG_OUT() );
};

void Dx7interface::seek_parameters(Glib::ustring bank_file_base, St_dx7sysex_1* sound){
    if( isStreamClosed(data_stream_param)){ // case fct for one sound
        Glib::ustring file_path = bank_file_base + sound->name + "_fct.syx";
        if( std::filesystem::exists( file_path.c_str() )){ 
            Glib::RefPtr<Gio::File> file_fct = Gio::File::create_for_path( file_path.c_str() );
            std::string msg = _("For voice: ") + sound->name + _("\n\tParameters of function reads from file: ") + file_fct->get_path() ;
            LOG( msg );
            data_stream_param = Gio::DataInputStream::create(file_fct->read());
            seek_voice_parameters(sound);
            data_stream_param->close();
        };
    }else{
        seek_voice_parameters(sound);
    };
};
void Dx7interface::seek_voice_parameters(St_dx7sysex_1* sound){
    if(!isStreamClosed(data_stream_param)){
        /* skip */
        //for (unsigned int i=0; i<(pos*98); data_stream_param->read_byte(),i++);
        for(uint8_t i=0; i<14; i++){
            for(uint8_t j=0; j<4; j++){
                data_stream_param->read_byte();
            };
            switch(data_stream_param->read_byte()){
                case 0x40:
                    sound->extra.functions.poly_mono.val = data_stream_param->read_byte() & sound->extra.functions.poly_mono.mask;
                    break;
                case 0x41:
                    sound->extra.functions.ptch_bnd_rng.val = data_stream_param->read_byte() & sound->extra.functions.ptch_bnd_rng.mask;
                    break;
                case 0x42:
                    sound->extra.functions.ptch_bnd_stp.val = data_stream_param->read_byte() & sound->extra.functions.ptch_bnd_stp.mask;
                    break;
                case 0x43:
                    sound->extra.functions.portamento_md.val = data_stream_param->read_byte() & sound->extra.functions.portamento_md.mask;
                    break;
                case 0x44:
                    sound->extra.functions.portamento_glss.val = data_stream_param->read_byte() & sound->extra.functions.portamento_glss.mask;
                    break;
                case 0x45:
                    sound->extra.functions.portamento_tm.val = data_stream_param->read_byte() & sound->extra.functions.portamento_tm.mask;
                    break;
                case 0x46:
                    sound->extra.functions.md_whl_rng.val = data_stream_param->read_byte() & sound->extra.functions.md_whl_rng.mask;
                    break;
                case 0x47:
                    sound->extra.functions.md_whl_assgn.val = data_stream_param->read_byte() & sound->extra.functions.md_whl_assgn.mask;
                    break;
                case 0x48:
                    sound->extra.functions.foot_rng.val = data_stream_param->read_byte() & sound->extra.functions.foot_rng.mask;
                    break;
                case 0x49:
                    sound->extra.functions.foot_assgn.val = data_stream_param->read_byte() & sound->extra.functions.foot_assgn.mask;
                    break;
                case 0x4A:
                    sound->extra.functions.brth_rng.val = data_stream_param->read_byte() & sound->extra.functions.brth_rng.mask;
                    break;
                case 0x4B:
                    sound->extra.functions.brth_assgn.val = data_stream_param->read_byte() & sound->extra.functions.brth_assgn.mask;
                    break;
                case 0x4C:
                    sound->extra.functions.aftrtch_rng.val = data_stream_param->read_byte() & sound->extra.functions.aftrtch_rng.mask;
                    break;
                case 0x4D:
                    sound->extra.functions.aftrtch_assgn.val = data_stream_param->read_byte() & sound->extra.functions.aftrtch_assgn.mask;
                    break;
            };
            data_stream_param->read_byte();
        };
    }else{
        sound->extra.functions.poly_mono.val = 0x00;
        sound->extra.functions.ptch_bnd_rng.val = 0x00;
        sound->extra.functions.ptch_bnd_stp.val = 0x00;
        sound->extra.functions.portamento_md.val = 0x00;
        sound->extra.functions.portamento_glss.val = 0x00;
        sound->extra.functions.portamento_tm.val = 0x00;
        sound->extra.functions.md_whl_rng.val = 0x00;
        sound->extra.functions.md_whl_assgn.val = 0x00;
        sound->extra.functions.foot_rng.val = 0x00;
        sound->extra.functions.foot_assgn.val = 0x00;
        sound->extra.functions.brth_rng.val = 0x00;
        sound->extra.functions.brth_assgn.val = 0x00;
        sound->extra.functions.aftrtch_rng.val = 0x00;
        sound->extra.functions.aftrtch_assgn.val = 0x00;
    };
};

void Dx7interface::receive_voice(St_dx7sysex_1* sound, std::vector<uint8_t> data){
    // TODO: ADD RECEIVE
    /* BULK 32 */
    //LOG( LOG_IN() );
        int i;
        i=6 + (128 * snum);
        uint8_t val,j,k;
        /* operator j */
        for ( j = 6; j-- != 0 ; ){
            /* OP[J] EG RATE[k] */
            for ( k = 0; k < 4 ; k++ ){
                sound->op[j].eg_rt[k].val=data[i++] & 0x7F;
            };
            /* OP[J] EG LVL[k] */
            for ( k = 0; k < 4 ; k++  ){
                sound->op[j].eg_lvl[k].val=data[i++] & 0x7F;
            };
            sound->op[j].kls.brk_pt.val=data[i++] & 0x7F;
            sound->op[j].kls.lft_dpth.val=data[i++] & 0x7F;
            sound->op[j].kls.rght_dpth.val=data[i++] & 0x7F;
            /* set out first 4 unused bit by mask (addr bit 11)*/
            val=data[i++] & 0x7F;
            /* first two bit to rc and last two bit to lc*/
            sound->op[j].kls.lft_curve.val=val & 0x03 ;
            sound->op[j].kls.rght_curve.val=val >> 2;
            /* set out first 1 unused bit by mask  (addr bit 12)*/
            val=data[i++] & 0x7F;
            sound->op[j].krs.val=val &  0x07;
            sound->op[j].dtun.val=val >> 3 ;
            /* set out first 3 unused bit by mask  (addr bit 13)*/
            val=data[i++] & 0x1F;
            sound->op[j].ams.val=val & 0x03;
            sound->op[j].kvs.val=val >> 2;

            sound->op[j].lvl.val=data[i++] & 0x7F;
            /* set out first 2 unused bit by mask  (addr bit 15)*/
            val=data[i++] & 0x3F;
            sound->op[j].freq_mode.val=val & 0x01;
            sound->op[j].freq_coarse.val=val >> 1;
            sound->op[j].freq_fine.val=data[i++] & 0x7F;
        };
        /* addr 102 */
        for( j=0 ; j < 4; j++ ){
            sound->pitch.eg_rt[j].val=data[i++] & 0x7F;
        };
        for( j=0 ; j < 4; j++ ){
            sound->pitch.eg_lvl[j].val=data[i++] & 0x7F;
        };
        /* addr 110 */
        sound->algo.algo.val=data[i++] & 0x1F ;
        val=data[i++] & 0x7F;
        sound->algo.feedback.val=val & 0x07;
        sound->algo.oks.val=val >> 3;
        sound->lfo.speed.val=data[i++] & 0x7F;
        sound->lfo.delay.val=data[i++] & 0x7F;
        sound->lfo.pmd.val=data[i++] & 0x7F;
        sound->lfo.amd.val=data[i++] & 0x7F;
        /* (addr bit 116) */
        val=data[i++] & 0x7F;
        sound->lfo.sync.val=val & 0x01;
        sound->lfo.wave.val=(val >> 1)&0x07;
        sound->lfo.pms.val=val >> 4;
        sound->algo.transpose.val=data[i++] & 0x7F;
        std::ostringstream strm;
        for( j=0; j <= 9; j++ ){
            strm << (data[i++]);
        };
        sound->name=strm.str();
        sound->extra.mute.val=0x7F;
        /* add voice name to liststore */
        update_data_model<SoundBankItem>(bank_data_model, sound->name);
    //LOG( LOG_OUT() );
};
void Dx7interface::receive_voice_by_byte(St_dx7sysex_1* sound, std::vector<uint8_t> data){
    /* BULK 1 */
    //LOG( LOG_IN() );
    uint8_t j,k;
    int i = 6 ;
    /* operator j */
    for ( j = 6; j-- != 0 ; ){
        /* OP[J] EG RATE[k] */
        for ( k = 0; k < 4 ; k++ ){
            sound->op[j].eg_rt[k].val=data[i++] & 0x7F;
        };
        /* OP[J] EG LVL[k] */
        for ( k = 0; k < 4 ; k++  ){
            sound->op[j].eg_lvl[k].val=data[i++] & 0x7F;
        };
        sound->op[j].kls.brk_pt.val=data[i++] & 0x7F;
        sound->op[j].kls.lft_dpth.val=data[i++] & 0x7F;
        sound->op[j].kls.rght_dpth.val=data[i++] & 0x7F;
        sound->op[j].kls.lft_curve.val=data[i++] & 0x7F;
        sound->op[j].kls.rght_curve.val=data[i++] & 0x7F;
        sound->op[j].krs.val=data[i++] & 0x7F;
        sound->op[j].ams.val=data[i++] & 0x7F;
        sound->op[j].kvs.val=data[i++] & 0x7F;
        sound->op[j].lvl.val=data[i++] & 0x7F;
        sound->op[j].freq_mode.val=data[i++] & 0x7F;
        sound->op[j].freq_coarse.val=data[i++] & 0x7F;
        sound->op[j].freq_fine.val=data[i++] & 0x7F;
        sound->op[j].dtun.val=data[i++] & 0x7F;
    };
    for( j=0 ; j < 4; j++ ){
        sound->pitch.eg_rt[j].val=data[i++] & 0x7F;
    };
    for( j=0 ; j < 4; j++ ){
        sound->pitch.eg_lvl[j].val=data[i++] & 0x7F;
    };
    sound->algo.algo.val=data[i++] & 0x7F ;
    sound->algo.feedback.val=data[i++] & 0x7F;
    sound->algo.oks.val=data[i++] & 0x7F;
    sound->lfo.speed.val=data[i++] & 0x7F;
    sound->lfo.delay.val=data[i++] & 0x7F;
    sound->lfo.pmd.val=data[i++] & 0x7F;
    sound->lfo.amd.val=data[i++] & 0x7F;
    sound->lfo.sync.val=data[i++] & 0x7F;
    sound->lfo.wave.val=data[i++] & 0x7F;
    sound->lfo.pms.val=data[i++] & 0x7F;
    sound->algo.transpose.val=data[i++] & 0x7F;
    std::ostringstream strm;
    for( j=0; j <= 9; j++ ){
        strm << (data[i++]);
    };
    sound->name=strm.str();
    sound->extra.mute.val=0x7F;
    /* add voice name to liststore */
    update_data_model<SoundBankItem>(bank_data_model, sound->name);
    //LOG( LOG_OUT() );
};

void Dx7interface::receive_paramters(St_dx7sysex_1* sound, std::vector<uint8_t> data){
        int i;
        i=6 + (64 * snum);
        sound->extra.functions.poly_mono.val = (data[i++]>>6) & sound->extra.functions.poly_mono.mask;
        sound->extra.functions.ptch_bnd_rng.val = (data[i]) & sound->extra.functions.ptch_bnd_rng.mask;
        sound->extra.functions.ptch_bnd_stp.val = ( (data[i]>>4) + ((data[i+14]>>6)+3) ) & sound->extra.functions.ptch_bnd_stp.mask;
        i++;
        sound->extra.functions.portamento_tm.val = (data[i++]) & sound->extra.functions.portamento_tm.mask;
        sound->extra.functions.portamento_glss.val = (data[i]) & sound->extra.functions.portamento_glss.mask;
        sound->extra.functions.portamento_md.val = (data[i++]>>1) & sound->extra.functions.portamento_md.mask;
        sound->extra.functions.md_whl_rng.val = (uint8_t)(( (data[i]) & 0x10 )* 6.6);
        sound->extra.functions.md_whl_assgn.val = (data[i++]>>4) & sound->extra.functions.md_whl_assgn.mask;
        sound->extra.functions.foot_rng.val = (uint8_t)(( (data[i]) & 0x10 )* 6.6);
        sound->extra.functions.foot_assgn.val = (data[i++]>>4) & sound->extra.functions.foot_assgn.mask;
        sound->extra.functions.aftrtch_rng.val = (uint8_t)(( (data[i]) & 0x10 )* 6.6);
        sound->extra.functions.aftrtch_assgn.val = (data[i++]>>4) & sound->extra.functions.aftrtch_assgn.mask;
        sound->extra.functions.brth_rng.val = (uint8_t)(( (data[i]) & 0x10 )* 6.6);
        sound->extra.functions.brth_assgn.val = (data[i++]>>4) & sound->extra.functions.brth_assgn.mask;
};

/* read: set (in ui from struct) */
void Dx7interface::set_voice(St_dx7sysex_1* sound){ LOG( LOG_IN() );
    /* set value in each widget from modif */
    /* general */
    block_ui(); // have to be here,
                // if inside the callback some ui event are trigerred
                // must be place the soonest ???

    (get_window())->add_tick_callback([this,sound] (const Glib::RefPtr<Gdk::FrameClock>&) -> bool {
        block_midi();
        //block_ui();
        uint8_t j,k;
        /* Voice name */
        (get_gwidget<Gtk::Entry>("entry_sound_name"))->set_text(sound->name);
        /* ALGO */
        (get_gwidget<Gtk::SpinButton>("algo_number"))->set_value(sound->algo.algo.val+1);
        (get_gwidget<Gtk::SpinButton>("feedback"))->set_value(sound->algo.feedback.val);
        /*	[0-11] + (([1-5]-1)*12)	*/
        (get_gwidget<Gtk::DropDown>("note_transpose"))->set_selected(sound->algo.transpose.val % 12);
        (get_gwidget<Gtk::SpinButton>("octv_transpose"))->set_value( (sound->algo.transpose.val / 12)+1 );
        (get_gwidget<Gtk::CheckButton>("oks"))->set_active(sound->algo.oks.val);
        /* LFO */
        (get_gwidget<Gtk::DropDown>("lfo_wav"))->set_selected(sound->lfo.wave.val);

        /*set image */
        (get_gwidget<Gtk::CheckButton>("lfo_sync"))->set_active(sound->lfo.sync.val);
        (get_gwidget<Gtk::SpinButton>("lfo_speed"))->set_value(sound->lfo.speed.val);
        (get_gwidget<Gtk::SpinButton>("lfo_delay"))->set_value(sound->lfo.delay.val);
        (get_gwidget<Gtk::SpinButton>("lfo_pmd"))->set_value(sound->lfo.pmd.val);
        (get_gwidget<Gtk::SpinButton>("lfo_amd"))->set_value(sound->lfo.amd.val);
        /* LFO modulation */
        (get_gwidget<Gtk::Scale>("pms"))->set_value(sound->lfo.pms.val);
        /* PITCH EG */
        for ( k = 0; k < 4 ; k++ ){
            get_gwidget<Gtk::SpinButton>("eg_rt" + tostr<unsigned int>(k+1) + "_pitch")->set_value(sound->pitch.eg_rt[k].val);
        };
        for ( k = 0; k < 4 ; k++ ){
            get_gwidget<Gtk::SpinButton>("eg_lvl"+tostr<unsigned int>(k+1)+"_pitch")->set_value(sound->pitch.eg_lvl[k].val);
        };
        /* OPERATEUR j+1 */
        uint8_t mute_val=sound->extra.mute.val & 0x7F; // get mute status from extra struct;
        for ( j=0;j<6;j++){
            /* AMS */
            (get_gwidget<Gtk::Scale>("ams_op"+tostr<unsigned int>(j+1)))->set_value(sound->op[j].ams.val);
            /* FREQUENCE */
            (get_gwidget<Gtk::DropDown>("freq_mode_op"+tostr<unsigned int>(j+1)))->set_selected(sound->op[j].freq_mode.val);
            (get_gwidget<Gtk::SpinButton>("freq_coarse_op"+tostr<unsigned int>(j+1)))->set_value(sound->op[j].freq_coarse.val);
            (get_gwidget<Gtk::SpinButton>("freq_fine_op"+tostr<unsigned int>(j+1)))->set_value(sound->op[j].freq_fine.val);
            (get_gwidget<Gtk::Scale>("dtun_op"+tostr<unsigned int>(j+1)))->set_value(sound->op[j].dtun.val-7);
            /* DRAWING AREA */

            /* OP[J] EG RT[k] */
            for ( k = 0; k < 4 ; k++ ){
                (get_gwidget<Gtk::SpinButton>("eg_rt"+tostr<unsigned int>(k+1)+"_op"+tostr<unsigned int>(j+1)))->set_value(sound->op[j].eg_rt[k].val);
            };
            /* OP[J] EG LVL[k] */
            for ( k = 0; k < 4 ; k++ ){
                (get_gwidget<Gtk::SpinButton>("eg_lvl"+tostr<unsigned int>(k+1)+"_op"+tostr<unsigned int>(j+1)))->set_value(sound->op[j].eg_lvl[k].val);
            };
            /* KRS */
            (get_gwidget<Gtk::Scale>("krs_op"+tostr<unsigned int>(j+1)))->set_value(sound->op[j].krs.val);
            /* KVS */
            (get_gwidget<Gtk::Scale>("kvs_op"+tostr<unsigned int>(j+1)))->set_value(sound->op[j].kvs.val);
            /* LVL */
            (get_gwidget<Gtk::SpinButton>("lvl_op"+tostr<unsigned int>(j+1)))->set_value(sound->op[j].lvl.val);
            /* MUTE */
            /* NO MUTE VALUE IN STD SYSEX CAN BE ADD IN LEFT SPACE */
            uint8_t muted = mute_val;
            muted = ( muted >> ( 5 - j ) );
            (get_gwidget<Gtk::ToggleButton>("mute_op"+tostr<unsigned int>(j+1)))->set_active(!(muted & 0x01));
            /* KLS */
            (get_gwidget<Gtk::DropDown>("kls_lft_curve_op"+tostr<unsigned int>(j+1)))->set_selected(sound->op[j].kls.lft_curve.val);
            (get_gwidget<Gtk::DropDown>("kls_rght_curve_op"+tostr<unsigned int>(j+1)))->set_selected(sound->op[j].kls.rght_curve.val);
            (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op"+tostr<unsigned int>(j+1)))->set_value(sound->op[j].kls.lft_dpth.val);
            (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op"+tostr<unsigned int>(j+1)))->set_value(sound->op[j].kls.rght_dpth.val);
            (get_gwidget<Gtk::DropDown>("note_brk_pt_op"+tostr<unsigned int>(j+1)))->set_selected(sound->op[j].kls.brk_pt.val % 12);
            int val = ((sound->op[j].kls.brk_pt.val) -3);
            if(val < 0){
                (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op"+tostr<unsigned int>(j+1)))->set_value( -1 );
            }else{
                (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op"+tostr<unsigned int>(j+1)))->set_value( val / 12 );
            };
            set_voice_parameters(sound);
        };

        redraw_all_curve();
        on_txt_freq_op_event();
        unblock_midi();

        if(compare){
            send_voice(&bank_1_origin.sound[0]);
            block_midi();
        }else{
            unblock_ui();
            send_voice(&bank_1_modif.sound[0]);
        };
        (get_window())->set_title(Glib::ustring(MODULE_NAME) + ": "+sound->name.c_str());

        return false; // false: Remove the tick callback after execution
    });
    LOG( LOG_OUT() );
};
void Dx7interface::set_voice_parameters(St_dx7sysex_1* sound){
    // called from set_voice in the callback
    if( mode_tf1 && !compare ) {
        (get_gwidget<Gtk::ToggleButton>("btn_poly_mono"))->set_active(sound->extra.functions.poly_mono.val);
        (get_gwidget<Gtk::Scale>("ptch_bnd_rng"))->set_value(sound->extra.functions.ptch_bnd_rng.val);
        (get_gwidget<Gtk::Scale>("ptch_bnd_stp"))->set_value(sound->extra.functions.ptch_bnd_stp.val);
        (get_gwidget<Gtk::ToggleButton>("btn_portamento_md"))->set_active(sound->extra.functions.portamento_md.val);
        (get_gwidget<Gtk::ToggleButton>("btn_portamento_glss"))->set_active(sound->extra.functions.portamento_glss.val);
        (get_gwidget<Gtk::SpinButton>("portamento_tm"))->set_value(sound->extra.functions.portamento_tm.val);

        (get_gwidget<Gtk::SpinButton>("md_whl_rng"))->set_value(sound->extra.functions.md_whl_rng.val);
        unsigned char val = sound->extra.functions.md_whl_assgn.val;
        (get_gwidget<Gtk::CheckButton>("md_whl_ptch"))->set_active( (val & 0x01) );
        (get_gwidget<Gtk::CheckButton>("md_whl_mp"))->set_active( (val & 0x02)>>1 );
        (get_gwidget<Gtk::CheckButton>("md_whl_gbs"))->set_active( (val & 0x04)>>2 );

        (get_gwidget<Gtk::SpinButton>("foot_rng"))->set_value(sound->extra.functions.foot_rng.val);
        val = sound->extra.functions.foot_assgn.val;
        (get_gwidget<Gtk::CheckButton>("foot_ptch"))->set_active(val & 0x01);
        (get_gwidget<Gtk::CheckButton>("foot_mp"))->set_active((val & 0x02)>>1);
        (get_gwidget<Gtk::CheckButton>("foot_gbs"))->set_active((val & 0x04)>>2);

        (get_gwidget<Gtk::SpinButton>("brth_rng"))->set_value(sound->extra.functions.brth_rng.val);
        val = sound->extra.functions.brth_assgn.val;
        (get_gwidget<Gtk::CheckButton>("brth_ptch"))->set_active(val & 0x01);
        (get_gwidget<Gtk::CheckButton>("brth_mp"))->set_active((val & 0x02)>>1);
        (get_gwidget<Gtk::CheckButton>("brth_gbs"))->set_active((val & 0x04)>>2);

        (get_gwidget<Gtk::SpinButton>("aftrtch_rng"))->set_value(sound->extra.functions.aftrtch_rng.val);
        val = sound->extra.functions.aftrtch_assgn.val;
        (get_gwidget<Gtk::CheckButton>("aftrtch_ptch"))->set_active(val & 0x01);
        (get_gwidget<Gtk::CheckButton>("aftrtch_mp"))->set_active((val & 0x02)>>1);
        (get_gwidget<Gtk::CheckButton>("aftrtch_gbs"))->set_active((val & 0x04)>>2);
    };
};

/* write: send (to midi) */
void Dx7interface::send_voice(st_dx7sysex_1* sound){
    LOG( LOG_IN() );
    /* TODO : check one voice send each parameter alone 155 bytes or as bulk format 128 BYtes */
    /*
    11110000  F0   Status byte - start sysex
    0iiiiiii  43   ID # (i=67; Yamaha)
    0sssnnnn  00   Sub-status (s=0) & channel number (n=0; ch 1)
    0fffffff  00   format number (f=0; 1 voice)
    0bbbbbbb  01   byte count MS byte
    0bbbbbbb  1B   byte count LS byte (b=155; 1 voice)
    0ddddddd  **   data byte 1
        |       |       |
    0ddddddd  **   data byte 155
    0eeeeeee  **   checksum (masked 2's complement of sum of 155 bytes)
    11110111  F7   Status - end sysex
    */
    uint8_t voice_checksum = 0;
    /* voice msg, index of sysex value in message, operator index, eg index */
    uint8_t l=6, j, k;
    unsigned char msg[163];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=0x00 + channel_send;
    msg[3]=0x00;
    msg[4]=0x01;
    msg[5]=0x1B;
    /* operator j */
    for ( j = 6; j-- != 0 ; ){
        /* OP[J] EG RATE[k] */
        for ( k = 0; k < 4 ; k++ ){
            msg[l++]=sound->op[j].eg_rt[k].val;
            voice_checksum -= msg[l-1];
        };
        /* OP[J] EG LVL[k] */
        for ( k = 0; k < 4 ; k++  ){
            msg[l++]=sound->op[j].eg_lvl[k].val;
            voice_checksum -= msg[l-1];
        };
        msg[l++] = sound->op[j].kls.brk_pt.val;
            voice_checksum -= msg[l-1];
        msg[l++] = sound->op[j].kls.lft_dpth.val;
            voice_checksum -= msg[l-1];
        msg[l++] = sound->op[j].kls.rght_dpth.val;
            voice_checksum -= msg[l-1];
        msg[l++]= sound->op[j].kls.lft_curve.val;
            voice_checksum -= msg[l-1];
        msg[l++]= sound->op[j].kls.rght_curve.val;
            voice_checksum -= msg[l-1];

        msg[l++]= sound->op[j].krs.val;
            voice_checksum -= msg[l-1];
        msg[l++] = sound->op[j].ams.val;
            voice_checksum -= msg[l-1];
        msg[l++] = sound->op[j].kvs.val;
            voice_checksum -= msg[l-1];

        msg[l++] = sound->op[j].lvl.val;
            voice_checksum -= msg[l-1];

        msg[l++] = sound->op[j].freq_mode.val;
            voice_checksum -= msg[l-1];
        msg[l++] = sound->op[j].freq_coarse.val;
            voice_checksum -= msg[l-1];
        msg[l++] = sound->op[j].freq_fine.val;
            voice_checksum -= msg[l-1];
        msg[l++]= sound->op[j].dtun.val;
            voice_checksum -= msg[l-1];
    };
    for(j=0 ; j <= 3; j++ ){
        msg[l++] = sound->pitch.eg_rt[j].val;
        voice_checksum -= msg[l-1];
    };
    for(j=0 ; j <= 3; j++ ){
        msg[l++] = sound->pitch.eg_lvl[j].val;
        voice_checksum -= msg[l-1];
    };
    msg[l++] = sound->algo.algo.val;
        voice_checksum -= msg[l-1];
    msg[l++] = sound->algo.feedback.val;
        voice_checksum -= msg[l-1];
    msg[l++] = sound->algo.oks.val;
        voice_checksum -= msg[l-1];

    msg[l++] = sound->lfo.speed.val;
        voice_checksum -= msg[l-1];
    msg[l++] = sound->lfo.delay.val;
        voice_checksum -= msg[l-1];
    msg[l++] = sound->lfo.pmd.val;
        voice_checksum -= msg[l-1];
    msg[l++] = sound->lfo.amd.val;
        voice_checksum -= msg[l-1];
    msg[l++] = sound->lfo.sync.val;
        voice_checksum -= msg[l-1];
    msg[l++] = sound->lfo.wave.val;
        voice_checksum -= msg[l-1];
    msg[l++] = sound->lfo.pms.val;
        voice_checksum -= msg[l-1];

    msg[l++] = sound->algo.transpose.val;
        voice_checksum -= msg[l-1];
    for (uint8_t carac = 0 ; carac <= 9 ; carac++ ){
        msg[l++] = sound->name.data()[carac];
        voice_checksum -= msg[l-1];
    };
    /* compute checksum */
    sound->sum = (unsigned char)(voice_checksum & 0x7F) ;
    msg[161]=sound->sum;
    msg[162]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 163, msg);
    /* extra parameters */
    //send_extra_parameters(sound);
    /* send mute for dx and hexter */
    //on_mute_op_event();
    LOG( LOG_OUT() );
};
void Dx7interface::send_extra_parameters(st_dx7sysex_1* sound){
    LOG( LOG_IN() );
    if(send_extra_params){
        uint8_t i, j;
        unsigned int nb_elem = 14;
        unsigned char msg[7];
        uint8_t val[] = {
            sound->extra.functions.poly_mono.val,
            sound->extra.functions.ptch_bnd_rng.val,
            sound->extra.functions.ptch_bnd_stp.val,
            sound->extra.functions.portamento_md.val,
            sound->extra.functions.portamento_glss.val,
            sound->extra.functions.portamento_tm.val,
            sound->extra.functions.md_whl_rng.val,
            sound->extra.functions.md_whl_assgn.val,
            sound->extra.functions.foot_rng.val,
            sound->extra.functions.foot_assgn.val,
            sound->extra.functions.brth_rng.val,
            sound->extra.functions.brth_assgn.val,
            sound->extra.functions.aftrtch_rng.val,
            sound->extra.functions.aftrtch_assgn.val,
        };
        for ( i=0, j=64 ; i< nb_elem; i++,j++){
            /*
                msg = "char j: " + std::hex + (int)j + std::dec ;
                msg = "i: " + (int)i ;
                msg = "sound val: " + std::hex + (int)val[i] + std::dec ;
            */
            msg[0]=0xF0;
            msg[1]=id_fabricant;
            msg[2]=sub_status + channel_send;
            msg[3]=0x08;
            msg[4]=j;
            msg[5]=val[i];
            msg[6]=0xF7;
            send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        };
    };
    LOG( LOG_OUT() );
};

/**** UI SIGNALS CONNECTION ****/
/* fold/unfold bank's sounds list */
void Dx7interface::on_bank_reveal(){
    LOG( LOG_IN() );
    (get_gwidget<Gtk::Revealer>("revealer_bank"))->set_reveal_child(!(get_gwidget<Gtk::Revealer>("revealer_bank"))->get_reveal_child());
    LOG( LOG_OUT() );
};

/* bank view / columnview population functions */
void Dx7interface::on_bind_num(const Glib::RefPtr<Gtk::ListItem>& list_item){
    auto label = dynamic_cast<Gtk::Label*>(list_item->get_child());
    if(label ){
        auto item = std::dynamic_pointer_cast<SoundBankItem>(list_item->get_item());
        if( item ){
            label->set_text(Glib::ustring::format(item->get_number()));
        };
    };
};
void Dx7interface::on_bind_name(const std::shared_ptr<Gtk::ListItem>& list_item){
    //LOG( LOG_IN() );
    auto label = dynamic_cast<Gtk::Label*>(list_item->get_child());
    auto item =  std::dynamic_pointer_cast<SoundBankItem>(list_item->get_item());
    if( label && item){
            label->set_text(item->get_name());
    };
    //LOG( LOG_OUT() );
};
void Dx7interface::on_setup_sound_name_label(const Glib::RefPtr<Gtk::ListItem>& list_item, Gtk::Align halign){
    auto label = Gtk::make_managed<Gtk::Label>("", halign);
    #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
        label->add_css_class("sound_label");
    #else
        label->get_style_context()->add_class("sound_label");  // Add custom CSS class
    #endif
    list_item->set_child(*label);
};
void Dx7interface::on_setup_sound_number_label(const Glib::RefPtr<Gtk::ListItem>& list_item, Gtk::Align halign){
    auto label = Gtk::make_managed<Gtk::Label>("", halign);
    #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
        label->add_css_class("sound_label");
    #else
        label->get_style_context()->add_class("sound_label");  // Add custom CSS class
    #endif
    list_item->set_child(*label);
};

void Dx7interface::on_bind_param_name(const Glib::RefPtr<Gtk::ListItem>& list_item){
    auto label = dynamic_cast<Gtk::Label*>(list_item->get_child());
    auto item =  std::dynamic_pointer_cast<ParamItem>(list_item->get_item());
    if( label && item){
        label->set_text(item->get_name());
    };
};
void Dx7interface::on_setup_param_label(const Glib::RefPtr<Gtk::ListItem>& list_item, Gtk::Align halign){
    auto label = Gtk::make_managed<Gtk::Label>("", halign);
    #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
        label->add_css_class("param_name");
    #else
        label->get_style_context()->add_class("param_name");  // Add custom CSS class
    #endif
    list_item->set_child(*label);
};

void Dx7interface::on_midi_learn_event(){ // set the bool for midi learn
    if( (get_gwidget<Gtk::ToggleButton>("toggle_midi_learn"))->get_active() ){
        (get_gwidget<Gtk::ToggleButton>("toggle_midi_learn"))->add_css_class("blink");
         midi_learn=true;
    }else{
        (get_gwidget<Gtk::ToggleButton>("toggle_midi_learn"))->remove_css_class("blink");
         midi_learn=false;
    };
};
void Dx7interface::on_add_midi_learn_event(){
    LOG( LOG_IN() );
    try{
        auto selected_item = get_gwidget<Gtk::DropDown>("dropdown_affect_param")->get_selected_item(); // return selected item
        auto selected_function = get_gwidget<Gtk::DropDown>("dropdown_affect_param")->get_selected(); // return index of selected item
        if (selected_item) {
            Glib::ustring function_name = (std::dynamic_pointer_cast<ParamItem>(selected_item))->get_name();
            Glib::ustring st_midi_param_number = get_gwidget<Gtk::Entry>("entry_affect_param")->get_text();
            if ( st_midi_param_number != "") {
                unsigned int param_value = std::stoi(st_midi_param_number.raw());
                if (param_value < 128){
                    add_midi_learned(param_value, selected_function);
                    add_midi_learn_param_widget(function_name, st_midi_param_number, selected_function);
                };
            };
        };
    }catch( const std::exception & ex ){
        std::string err_msg = error( __PRETTY_FUNCTION__, "???", ex.what());
        LOG_ERR( err_msg );
    };
    LOG( LOG_OUT() );
};

/* Init all Gesture controller */
void Dx7interface::init_gesture_controller(){
    controller_mouse_moove_op1 = Gtk::EventControllerMotion::create();
    controller_mouse_button_op1 = Gtk::GestureClick::create();
    controller_mouse_button_op1->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    controller_mouse_button_op1->set_button(1); // bouton gauche souris
    controller_mouse_moove_op2 = Gtk::EventControllerMotion::create();
    controller_mouse_button_op2 = Gtk::GestureClick::create();
    controller_mouse_button_op2->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    controller_mouse_button_op2->set_button(1); // bouton gauche souris
    controller_mouse_moove_op3 = Gtk::EventControllerMotion::create();
    controller_mouse_button_op3 = Gtk::GestureClick::create();
    controller_mouse_button_op3->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    controller_mouse_button_op3->set_button(1); // bouton gauche souris
    controller_mouse_moove_op4 = Gtk::EventControllerMotion::create();
    controller_mouse_button_op4 = Gtk::GestureClick::create();
    controller_mouse_button_op4->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    controller_mouse_button_op4->set_button(1); // bouton gauche souris
    controller_mouse_moove_op5 = Gtk::EventControllerMotion::create();
    controller_mouse_button_op5 = Gtk::GestureClick::create();
    controller_mouse_button_op5->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    controller_mouse_button_op5->set_button(1); // bouton gauche souris
    controller_mouse_moove_op6 = Gtk::EventControllerMotion::create();
    controller_mouse_button_op6 = Gtk::GestureClick::create();
    controller_mouse_button_op6->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    controller_mouse_button_op6->set_button(1); // bouton gauche souris
    controller_mouse_moove_pitch = Gtk::EventControllerMotion::create();
    controller_mouse_button_pitch = Gtk::GestureClick::create();
    controller_mouse_button_pitch->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    controller_mouse_button_pitch->set_button(1); // bouton gauche souris
};

void Dx7interface::init_global_fonction_parameter(){
    LOG( LOG_IN() );
    on_mono_poly_event();
    on_portamento_md_event();
    on_txt_freq_op_event();
    LOG( LOG_OUT() );
};

/* Attach all signals */
void Dx7interface::attach_action_group_signals(){
    action_group->add_action("save_sound", sigc::mem_fun(*this, &Dx7interface::on_save_sound));
    action_group->add_action("save_bank", sigc::mem_fun(*this, &Dx7interface::on_save_bank));
    action_group->add_action("send_bank", sigc::mem_fun(*this, &Dx7interface::on_send_bank));
    action_group->add_action("restore_sound", sigc::mem_fun(*this, &Dx7interface::on_restore_sound));
    action_group->add_action("restore_bank", sigc::mem_fun(*this, &Dx7interface::on_restore_bank));
    action_group->add_action("insert_at", sigc::mem_fun(*this, &Dx7interface::on_insert_at));
    action_group->add_action("replace_sound", sigc::bind(
        sigc::mem_fun(*this, &Dx7interface::OpenDialogFileSelect),
                      0,
                      std::bind(&Dx7interface::on_replace_sound, this, std::placeholders::_1, std::placeholders::_2) )
    );
    action_group->add_action("delete_sound", sigc::mem_fun(*this, &Dx7interface::on_delete_sound));
};
void Dx7interface::attach_drawarea_signals(){

    /* Drawing area for pitch */
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->set_draw_func(
        sigc::mem_fun(*this, &Dx7interface::on_draw_pitch_event) );
    controller_mouse_moove_pitch->signal_motion().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_mooves), Glib::ustring("pitch") ));
    controller_mouse_button_pitch->signal_pressed().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_click), Glib::ustring("pitch") ));
    controller_mouse_button_pitch->signal_released().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_click_release), Glib::ustring("pitch") )    );
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->add_controller(controller_mouse_button_pitch);
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->add_controller(controller_mouse_moove_pitch);

    /* Drawing area for each operator */
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->set_draw_func(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::on_draw_op_event), Glib::ustring("1") ));
    controller_mouse_moove_op1->signal_motion().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_mooves), Glib::ustring("op1") ));
    controller_mouse_button_op1->signal_pressed().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_click), Glib::ustring("op1") ));
    controller_mouse_button_op1->signal_released().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_click_release), Glib::ustring("op1") ));
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->add_controller(controller_mouse_button_op1);
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->add_controller(controller_mouse_moove_op1);

    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->set_draw_func(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::on_draw_op_event), Glib::ustring("2") ));
    controller_mouse_moove_op2->signal_motion().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_mooves), Glib::ustring("op2") ));
    controller_mouse_button_op2->signal_pressed().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_click), Glib::ustring("op2") ));
    controller_mouse_button_op2->signal_released().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_click_release), Glib::ustring("op2") ));
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->add_controller(controller_mouse_button_op2);
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->add_controller(controller_mouse_moove_op2);

    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->set_draw_func(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::on_draw_op_event), Glib::ustring("3") ));
    controller_mouse_moove_op3->signal_motion().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_mooves), Glib::ustring("op3") ));
    controller_mouse_button_op3->signal_pressed().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_click), Glib::ustring("op3") ));
    controller_mouse_button_op3->signal_released().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_click_release), Glib::ustring("op3") ));
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->add_controller(controller_mouse_button_op3);
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->add_controller(controller_mouse_moove_op3);

    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->set_draw_func(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::on_draw_op_event), Glib::ustring("4") ));
    controller_mouse_moove_op4->signal_motion().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_mooves), Glib::ustring("op4") ));
    controller_mouse_button_op4->signal_pressed().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_click), Glib::ustring("op4") ));
    controller_mouse_button_op4->signal_released().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_click_release), Glib::ustring("op4") ));
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->add_controller(controller_mouse_button_op4);
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->add_controller(controller_mouse_moove_op4);

    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->set_draw_func(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::on_draw_op_event), Glib::ustring("5") ));
    controller_mouse_moove_op5->signal_motion().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_mooves), Glib::ustring("op5") ));
    controller_mouse_button_op5->signal_pressed().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_click), Glib::ustring("op5") ));
    controller_mouse_button_op5->signal_released().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_click_release), Glib::ustring("op5") ));
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->add_controller(controller_mouse_button_op5);
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->add_controller(controller_mouse_moove_op5);

    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->set_draw_func(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::on_draw_op_event), Glib::ustring("6") ));
    controller_mouse_moove_op6->signal_motion().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_mooves), Glib::ustring("op6") ));
    controller_mouse_button_op6->signal_pressed().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_click), Glib::ustring("op6") ));
    controller_mouse_button_op6->signal_released().connect(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::mouse_click_release), Glib::ustring("op6") ));
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->add_controller(controller_mouse_button_op6);
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->add_controller(controller_mouse_moove_op6);

    /* Drawing area for keyboard scaling */
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op1"))->set_draw_func(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::on_draw_kls_event), Glib::ustring("1") ) );
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op2"))->set_draw_func(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::on_draw_kls_event), Glib::ustring("2") ) );
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op3"))->set_draw_func(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::on_draw_kls_event), Glib::ustring("3") ) );
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op4"))->set_draw_func(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::on_draw_kls_event), Glib::ustring("4") ) );
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op5"))->set_draw_func(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::on_draw_kls_event), Glib::ustring("5") ) );
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op6"))->set_draw_func(
        sigc::bind( sigc::mem_fun(*this, &Dx7interface::on_draw_kls_event), Glib::ustring("6") ) );
};

void Dx7interface::attach_signals(){
    LOG( LOG_IN() );
    //slot_OBJECT_NAME = (je recupere l'object)->sur le signal de l'evenement.je connecte( le signal de ( la fonction ));
    // (i get the ui object)->on_signal_of_event.I_connect(the_signal_of(the_function));

    /*  prototype bind button
    m_button1.signal_clicked().connect(
        sigc::bind<Glib::ustring>( sigc::mem_fun(*this, &HelloWorld::on_button_clicked), "button 1") );

    void on_button_clicked(Glib::ustring data){
        //
    };
    */
    /*** generale  ***/
    /* menu bank view */
    attach_action_group_signals();

    /* Sound Name modification */
    slot_sound_name_activate =(get_gwidget<Gtk::Entry>("entry_sound_name"))->signal_activate().connect( sigc::mem_fun(*this, &Dx7interface::on_sound_name_event));
    slot_sound_name_change =(get_gwidget<Gtk::Entry>("entry_sound_name"))->signal_changed().connect( sigc::mem_fun(*this, &Dx7interface::on_sound_name_event));

    (get_gwidget<Gtk::CheckButton>("checkbutton_add_extra_parameters"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_extra_param_event));

    /* Midi learn */
    slot_midi_learn_load = (get_gwidget<Gtk::Button>("btn_midi_learn_load"))->signal_clicked().connect(
        sigc::mem_fun(*this, &Dx7interface::on_midi_learn_param_select));

    slot_midi_learn_save = (get_gwidget<Gtk::Button>("btn_midi_learn_save"))->signal_clicked().connect(
        sigc::mem_fun(*this, &Dx7interface::on_midi_learn_param_save));

    (get_gwidget<Gtk::ToggleButton>("toggle_midi_learn"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_midi_learn_event));
    (get_gwidget<Gtk::Button>("button_add_param"))->signal_clicked().connect(
        sigc::mem_fun(*this, &Dx7interface::on_add_midi_learn_event));

    slot_midi_channel_send = (get_gwidget<Gtk::SpinButton>("midi_channel_send"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_midi_channel_send_event));
    slot_midi_channel_receive = (get_gwidget<Gtk::SpinButton>("midi_channel_receive"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_midi_channel_receive_event));

    /* FUNCTIONS */
    slot_poly = (get_gwidget<Gtk::ToggleButton>("btn_poly_mono"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mono_poly_event));
    slot_portamento_md = (get_gwidget<Gtk::ToggleButton>("btn_portamento_md"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_portamento_md_event));
    slot_portamento_glss = (get_gwidget<Gtk::ToggleButton>("btn_portamento_glss"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_portamento_glss_event));
    slot_portamento_tm = (get_gwidget<Gtk::SpinButton>("portamento_tm"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_portamento_tm_event));
    slot_ptch_bnd_rng = (get_gwidget<Gtk::Scale>("ptch_bnd_rng"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ptch_bnd_rng_event));
    slot_ptch_bnd_stp = (get_gwidget<Gtk::Scale>("ptch_bnd_stp"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ptch_bnd_stp_event));

    /* tableau des controleurs */
    slot_md_whl_rng = (get_gwidget<Gtk::SpinButton>("md_whl_rng"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_md_whl_rng_event));
    slot_md_whl_ptch = (get_gwidget<Gtk::CheckButton>("md_whl_ptch"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_md_whl_assgn_event));
    slot_md_whl_mp = (get_gwidget<Gtk::CheckButton>("md_whl_mp"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_md_whl_assgn_event));
    slot_md_whl_gbs = (get_gwidget<Gtk::CheckButton>("md_whl_gbs"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_md_whl_assgn_event));
    slot_foot_rng = (get_gwidget<Gtk::SpinButton>("foot_rng"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_foot_rng_event));
    slot_foot_ptch = (get_gwidget<Gtk::CheckButton>("foot_ptch"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_foot_assgn_event));
    slot_foot_mp = (get_gwidget<Gtk::CheckButton>("foot_mp"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_foot_assgn_event));
    slot_foot_gbs = (get_gwidget<Gtk::CheckButton>("foot_gbs"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_foot_assgn_event));
    slot_brth_rng = (get_gwidget<Gtk::SpinButton>("brth_rng"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_brth_rng_event));
    slot_brth_ptch = (get_gwidget<Gtk::CheckButton>("brth_ptch"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_brth_assgn_event));
    slot_brth_mp = (get_gwidget<Gtk::CheckButton>("brth_mp"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_brth_assgn_event));
    slot_brth_gbs = (get_gwidget<Gtk::CheckButton>("brth_gbs"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_brth_assgn_event));
    slot_aftrtch_rng = (get_gwidget<Gtk::SpinButton>("aftrtch_rng"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_aftrtch_rng_event));
    slot_aftrtch_ptch = (get_gwidget<Gtk::CheckButton>("aftrtch_ptch"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_aftrtch_assgn_event));
    slot_aftrtch_mp = (get_gwidget<Gtk::CheckButton>("aftrtch_mp"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_aftrtch_assgn_event));
    slot_aftrtch_gbs = (get_gwidget<Gtk::CheckButton>("aftrtch_gbs"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_aftrtch_assgn_event));

    /* compare */
    slot_btn_compare = (get_gwidget<Gtk::ToggleButton>("btn_compare"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_compare_event));
    /* send_extra_parameters */
    slot_btn_send_extra_parameters = (get_gwidget<Gtk::CheckButton>("btn_send_extra_parameters"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_send_extra_parameters_event));
    slot_btn_mode_tf1 = (get_gwidget<Gtk::CheckButton>("btn_mode_tf1"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mode_tf1_event));

    /* panic */
    slot_btn_panic = (get_gwidget<Gtk::Button>("btn_panic"))->signal_clicked().connect(
        sigc::mem_fun(*this, &Dx7interface::on_panic_event));

    /* Algo */
    (get_gwidget<Gtk::DrawingArea>("drawingarea_algo"))->set_draw_func(
        sigc::mem_fun(*this, &Dx7interface::on_draw_algo) );

    slot_algo = (get_gwidget<Gtk::SpinButton>("algo_number"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_algo_event));


    (get_gwidget<Gtk::SpinButton>("algo_number"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        unsigned int ui_val = ((get_gwidget<Gtk::SpinButton>("algo_number"))->get_value() - 1);
        if (compare) {
            if( ui_val != bank_1_origin.sound->algo.algo.val ){
                int value = bank_1_origin.sound->algo.algo.val;
                (get_gwidget<Gtk::SpinButton>("algo_number"))->set_value(value + 1);
            };
        }else{
            if( ui_val != bank_1_modif.sound->algo.algo.val ){
                int value = bank_1_modif.sound->algo.algo.val;
                (get_gwidget<Gtk::SpinButton>("algo_number"))->set_value(value + 1);
            };
        };
        return true; // Return false to remove the callback after one executio
    });

    slot_feedback = (get_gwidget<Gtk::SpinButton>("feedback"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_feedback_event));
    (get_gwidget<Gtk::SpinButton>("feedback"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->algo.feedback.val;
        }else{
            value = bank_1_modif.sound->algo.feedback.val;
        };
        (get_gwidget<Gtk::SpinButton>("feedback"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });

    auto note_transpose_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("note_transpose"));

    slot_note_transpose = (get_gwidget<Gtk::DropDown>("note_transpose"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_transpose_event));
    (get_gwidget<Gtk::DropDown>("note_transpose"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->algo.transpose.val;
        }else{
            value = bank_1_modif.sound->algo.transpose.val;
        };
        (get_gwidget<Gtk::SpinButton>("octv_transpose"))->set_value( (value / 12)+1 );
        (get_gwidget<Gtk::DropDown>("note_transpose"))->set_selected(value % 12);
        return true; // Return false to remove the callback after one executio
    });

    slot_octv_transpose = (get_gwidget<Gtk::SpinButton>("octv_transpose"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_transpose_event));
    (get_gwidget<Gtk::SpinButton>("octv_transpose"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->algo.transpose.val;
        }else{
            value = bank_1_modif.sound->algo.transpose.val;
        };
        (get_gwidget<Gtk::SpinButton>("octv_transpose"))->set_value( (value / 12)+1 );
        (get_gwidget<Gtk::DropDown>("note_transpose"))->set_selected(value % 12);
        return true; // Return false to remove the callback after one executio
    });

    slot_oks = (get_gwidget<Gtk::CheckButton>("oks"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_oks_event));
    (get_gwidget<Gtk::CheckButton>("oks"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->algo.oks.val;
        }else{
            value = bank_1_modif.sound->algo.oks.val;
        };
        (get_gwidget<Gtk::CheckButton>("oks"))->set_active(value);
        return true; // Return false to remove the callback after one executio
    });

    /* lfo */
    (get_gwidget<Gtk::DrawingArea>("drawingarea_lfo_wav"))->set_draw_func(
        sigc::mem_fun(*this, &Dx7interface::on_draw_lfo) );

    slot_lfo_wav = (get_gwidget<Gtk::DropDown>("lfo_wav"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lfo_wav_event));
    (get_gwidget<Gtk::DropDown>("lfo_wav"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->lfo.wave.val;
        }else{
            value = bank_1_modif.sound->lfo.wave.val;
        };
        (get_gwidget<Gtk::DropDown>("lfo_wav"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto lfo_wav_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("lfo_wav"));

    slot_lfo_sync = (get_gwidget<Gtk::CheckButton>("lfo_sync"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lfo_sync_event));
    (get_gwidget<Gtk::CheckButton>("lfo_sync"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->lfo.sync.val;
        }else{
            value = bank_1_modif.sound->lfo.sync.val;
        };
        (get_gwidget<Gtk::CheckButton>("lfo_sync"))->set_active(value);
        return true; // Return false to remove the callback after one executio
    });

    slot_lfo_speed = (get_gwidget<Gtk::SpinButton>("lfo_speed"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lfo_speed_event));
    (get_gwidget<Gtk::SpinButton>("lfo_speed"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->lfo.speed.val;
        }else{
            value = bank_1_modif.sound->lfo.speed.val;
        };
        (get_gwidget<Gtk::SpinButton>("lfo_speed"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_lfo_delay = (get_gwidget<Gtk::SpinButton>("lfo_delay"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lfo_delay_event));
    (get_gwidget<Gtk::SpinButton>("lfo_delay"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->lfo.delay.val;
        }else{
            value = bank_1_modif.sound->lfo.delay.val;
        };
        (get_gwidget<Gtk::SpinButton>("lfo_delay"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_lfo_pmd = (get_gwidget<Gtk::SpinButton>("lfo_pmd"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lfo_pmd_event));
    (get_gwidget<Gtk::SpinButton>("lfo_pmd"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->lfo.pmd.val;
        }else{
            value = bank_1_modif.sound->lfo.pmd.val;
        };
        (get_gwidget<Gtk::SpinButton>("lfo_pmd"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });

    slot_lfo_amd = (get_gwidget<Gtk::SpinButton>("lfo_amd"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lfo_amd_event));
    (get_gwidget<Gtk::SpinButton>("lfo_amd"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->lfo.amd.val;
        }else{
            value = bank_1_modif.sound->lfo.amd.val;
        };
        (get_gwidget<Gtk::SpinButton>("lfo_amd"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });

    /* lfo modulation */
    slot_pms = (get_gwidget<Gtk::Scale>("pms"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pms_event));
    (get_gwidget<Gtk::Scale>("pms"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->lfo.pms.val;
        }else{
            value = bank_1_modif.sound->lfo.pms.val;
        };
        (get_gwidget<Gtk::Scale>("pms"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });

    /* pitch eg */
    slot_pitch_rt1 = (get_gwidget<Gtk::SpinButton>("eg_rt1_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_rt1_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt1_pitch"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->pitch.eg_rt[0].val;
        }else{
            value = bank_1_modif.sound->pitch.eg_rt[0].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt1_pitch"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_pitch_rt2 = (get_gwidget<Gtk::SpinButton>("eg_rt2_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_rt2_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt2_pitch"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->pitch.eg_rt[1].val;
        }else{
            value = bank_1_modif.sound->pitch.eg_rt[1].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt2_pitch"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_pitch_rt3 = (get_gwidget<Gtk::SpinButton>("eg_rt3_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_rt3_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt3_pitch"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->pitch.eg_rt[2].val;
        }else{
            value = bank_1_modif.sound->pitch.eg_rt[2].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt3_pitch"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_pitch_rt4 = (get_gwidget<Gtk::SpinButton>("eg_rt4_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_rt4_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt4_pitch"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->pitch.eg_rt[3].val;
        }else{
            value = bank_1_modif.sound->pitch.eg_rt[3].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt4_pitch"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_pitch_lvl1 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_lvl1_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_pitch"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->pitch.eg_lvl[0].val;
        }else{
            value = bank_1_modif.sound->pitch.eg_lvl[0].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl1_pitch"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_pitch_lvl2 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_lvl2_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_pitch"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->pitch.eg_lvl[1].val;
        }else{
            value = bank_1_modif.sound->pitch.eg_lvl[1].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl2_pitch"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_pitch_lvl3 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_lvl3_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_pitch"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->pitch.eg_lvl[2].val;
        }else{
            value = bank_1_modif.sound->pitch.eg_lvl[2].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl3_pitch"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_pitch_lvl4 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_lvl4_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_pitch"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->pitch.eg_lvl[3].val;
        }else{
            value = bank_1_modif.sound->pitch.eg_lvl[3].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl4_pitch"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });

    attach_drawarea_signals();

    /* OPERATEUR 1 */
    slot_ams_op1 = (get_gwidget<Gtk::Scale>("ams_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op1_event)); //frame lfo
    (get_gwidget<Gtk::Scale>("ams_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].ams.val;
        }else{
            value = bank_1_modif.sound->op[0].ams.val;
        };
        (get_gwidget<Gtk::Scale>("ams_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP1 FREQUENCE */
    slot_freq_mode_op1 = (get_gwidget<Gtk::DropDown>("freq_mode_op1"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op1_event));
    (get_gwidget<Gtk::DropDown>("freq_mode_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].freq_mode.val;
        }else{
            value = bank_1_modif.sound->op[0].freq_mode.val;
        };
        (get_gwidget<Gtk::DropDown>("freq_mode_op1"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto freq_mode_op1_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("freq_mode_op1"));

    slot_freq_coarse_op1 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op1_event));
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].freq_coarse.val;
        }else{
            value = bank_1_modif.sound->op[0].freq_coarse.val;
        };
        (get_gwidget<Gtk::SpinButton>("freq_coarse_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_freq_fine_op1 = (get_gwidget<Gtk::SpinButton>("freq_fine_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op1_event));
    (get_gwidget<Gtk::SpinButton>("freq_fine_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].freq_fine.val;
        }else{
            value = bank_1_modif.sound->op[0].freq_fine.val;
        };
        (get_gwidget<Gtk::SpinButton>("freq_fine_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_dtun_op1 = (get_gwidget<Gtk::Scale>("dtun_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op1_event));
    (get_gwidget<Gtk::Scale>("dtun_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        unsigned int ui_val = ((get_gwidget<Gtk::Scale>("dtun_op1"))->get_value() - 7);
        if (compare) {
            if( ui_val != bank_1_origin.sound->algo.algo.val ){
                int value = bank_1_origin.sound->op[0].dtun.val-7;
                (get_gwidget<Gtk::Scale>("dtun_op1"))->set_value(value);
            };
        }else{
            if( ui_val != bank_1_modif.sound->algo.algo.val ){
                int value = bank_1_modif.sound->op[0].dtun.val-7;
                (get_gwidget<Gtk::Scale>("dtun_op1"))->set_value(value);
            };
        };
        return true; // Return false to remove the callback after one executio
    });
    /* OP1 EG */
    slot_eg_rt1_op1 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op1_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].eg_rt[0].val;
        }else{
            value = bank_1_modif.sound->op[0].eg_rt[0].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt1_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt2_op1 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op1_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].eg_rt[1].val;
        }else{
            value = bank_1_modif.sound->op[0].eg_rt[1].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt2_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt3_op1 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op1_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].eg_rt[2].val;
        }else{
            value = bank_1_modif.sound->op[0].eg_rt[2].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt3_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt4_op1 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op1_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].eg_rt[3].val;
        }else{
            value = bank_1_modif.sound->op[0].eg_rt[3].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt4_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl1_op1 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op1_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].eg_lvl[0].val;
        }else{
            value = bank_1_modif.sound->op[0].eg_lvl[0].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl1_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl2_op1 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op1_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].eg_lvl[1].val;
        }else{
            value = bank_1_modif.sound->op[0].eg_lvl[1].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl2_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl3_op1 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op1_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].eg_lvl[2].val;
        }else{
            value = bank_1_modif.sound->op[0].eg_lvl[2].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl3_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl4_op1 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op1_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].eg_lvl[3].val;
        }else{
            value = bank_1_modif.sound->op[0].eg_lvl[3].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl4_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP1 VOLUME */
    slot_krs_op1 = (get_gwidget<Gtk::Scale>("krs_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op1_event));
    (get_gwidget<Gtk::Scale>("krs_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].krs.val;
        }else{
            value = bank_1_modif.sound->op[0].krs.val;
        };
        (get_gwidget<Gtk::Scale>("krs_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kvs_op1 = (get_gwidget<Gtk::Scale>("kvs_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op1_event));
    (get_gwidget<Gtk::Scale>("kvs_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].kvs.val;
        }else{
            value = bank_1_modif.sound->op[0].kvs.val;
        };
        (get_gwidget<Gtk::Scale>("kvs_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_lvl_op1 = (get_gwidget<Gtk::SpinButton>("lvl_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op1_event));
    (get_gwidget<Gtk::SpinButton>("lvl_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].lvl.val;
        }else{
            value = bank_1_modif.sound->op[0].lvl.val;
        };
        (get_gwidget<Gtk::SpinButton>("lvl_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_mute_op1 = (get_gwidget<Gtk::ToggleButton>("mute_op1"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    (get_gwidget<Gtk::ToggleButton>("mute_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->extra.mute.val >>5;
        }else{
            value = bank_1_modif.sound->extra.mute.val >>5;
        };
        (get_gwidget<Gtk::ToggleButton>("mute_op1"))->set_active(!(value & 0x01));
        return true; // Return false to remove the callback after one executio
    });

    /* OP1 KLS */
    slot_kls_lft_curve_op1 = (get_gwidget<Gtk::DropDown>("kls_lft_curve_op1"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op1_event));
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].kls.lft_curve.val;
        }else{
            value = bank_1_modif.sound->op[0].kls.lft_curve.val;
        };
        (get_gwidget<Gtk::DropDown>("kls_lft_curve_op1"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_lft_curve_op1_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_lft_curve_op1"));

    slot_kls_rght_curve_op1 = (get_gwidget<Gtk::DropDown>("kls_rght_curve_op1"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op1_event));
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].kls.rght_curve.val;
        }else{
            value = bank_1_modif.sound->op[0].kls.rght_curve.val;
        };
        (get_gwidget<Gtk::DropDown>("kls_rght_curve_op1"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_rght_curve_op1_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_rght_curve_op1"));

    slot_kls_lft_depth_op1 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op1_event));
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].kls.lft_dpth.val;
        }else{
            value = bank_1_modif.sound->op[0].kls.lft_dpth.val;
        };
        (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_rght_depth_op1 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op1_event));
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].kls.rght_dpth.val;
        }else{
            value = bank_1_modif.sound->op[0].kls.rght_dpth.val;
        };
        (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_note_brk_pt_op1 = (get_gwidget<Gtk::DropDown>("note_brk_pt_op1"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op1_event));
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[0].kls.brk_pt.val;
        }else{
            value = bank_1_modif.sound->op[0].kls.brk_pt.val;
        };
        (get_gwidget<Gtk::DropDown>("note_brk_pt_op1"))->set_selected(value % 12);
        int val = (value -3);
        if(val < 0){
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->set_value( -1 );
        }else{
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->set_value( val / 12 );
        };
        return true; // Return false to remove the callback after one executio
    });
    auto note_brk_pt_op1_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("note_brk_pt_op1"),get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"),slot_kls_octv_brk_pt_op1);

    slot_kls_octv_brk_pt_op1 = (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op1_event));
    /*(get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].kls.brk_pt.val;
        (get_gwidget<Gtk::DropDown>("note_brk_pt_op1"))->set_selected(value % 12);
        int val = (value -3);
        if(val < 0){
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->set_value( -1 );
        }else{
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->set_value( val / 12 );
        };
        return true; // Return false to remove the callback after one executio
    });*/

    /* OPERATEUR 2 */
    slot_ams_op2 = (get_gwidget<Gtk::Scale>("ams_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op2_event)); //frame lfo
    (get_gwidget<Gtk::Scale>("ams_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].ams.val;
        }else{
            value = bank_1_modif.sound->op[1].ams.val;
        };
        (get_gwidget<Gtk::Scale>("ams_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP2 FREQUENCE */
    slot_freq_mode_op2 = (get_gwidget<Gtk::DropDown>("freq_mode_op2"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op2_event));
    (get_gwidget<Gtk::DropDown>("freq_mode_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].freq_mode.val;
        }else{
            value = bank_1_modif.sound->op[1].freq_mode.val;
        };
        (get_gwidget<Gtk::DropDown>("freq_mode_op2"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto freq_mode_op2_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("freq_mode_op2"));

    slot_freq_coarse_op2 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op2_event));
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].freq_coarse.val;
        }else{
            value = bank_1_modif.sound->op[1].freq_coarse.val;
        };
        (get_gwidget<Gtk::SpinButton>("freq_coarse_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_freq_fine_op2 = (get_gwidget<Gtk::SpinButton>("freq_fine_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op2_event));
    (get_gwidget<Gtk::SpinButton>("freq_fine_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].freq_fine.val;
        }else{
            value = bank_1_modif.sound->op[1].freq_fine.val;
        };
        (get_gwidget<Gtk::SpinButton>("freq_fine_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_dtun_op2 = (get_gwidget<Gtk::Scale>("dtun_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op2_event));
    (get_gwidget<Gtk::Scale>("dtun_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].dtun.val-7;
        }else{
            value = bank_1_modif.sound->op[1].dtun.val-7;
        };
        (get_gwidget<Gtk::Scale>("dtun_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP2 EG */
    slot_eg_rt1_op2 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op2_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].eg_rt[0].val;
        }else{
            value = bank_1_modif.sound->op[1].eg_rt[0].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt1_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt2_op2 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op2_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].eg_rt[1].val;
        }else{
            value = bank_1_modif.sound->op[1].eg_rt[1].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt2_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt3_op2 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op2_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].eg_rt[2].val;
        }else{
            value = bank_1_modif.sound->op[1].eg_rt[2].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt3_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt4_op2 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op2_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].eg_rt[3].val;
        }else{
            value = bank_1_modif.sound->op[1].eg_rt[3].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt4_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl1_op2 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op2_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].eg_lvl[0].val;
        }else{
            value = bank_1_modif.sound->op[1].eg_lvl[0].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl1_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl2_op2 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op2_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].eg_lvl[1].val;
        }else{
            value = bank_1_modif.sound->op[1].eg_lvl[1].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl2_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl3_op2 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op2_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].eg_lvl[2].val;
        }else{
            value = bank_1_modif.sound->op[1].eg_lvl[2].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl3_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl4_op2 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op2_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].eg_lvl[3].val;
        }else{
            value = bank_1_modif.sound->op[1].eg_lvl[3].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl4_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP2 VOLUME */
    slot_krs_op2 = (get_gwidget<Gtk::Scale>("krs_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op2_event));
    (get_gwidget<Gtk::Scale>("krs_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].krs.val;
        }else{
            value = bank_1_modif.sound->op[1].krs.val;
        };
        (get_gwidget<Gtk::Scale>("krs_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kvs_op2 = (get_gwidget<Gtk::Scale>("kvs_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op2_event));
    (get_gwidget<Gtk::Scale>("kvs_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].kvs.val;
        }else{
            value = bank_1_modif.sound->op[1].kvs.val;
        };
        (get_gwidget<Gtk::Scale>("kvs_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_lvl_op2 = (get_gwidget<Gtk::SpinButton>("lvl_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op2_event));
    (get_gwidget<Gtk::SpinButton>("lvl_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].lvl.val;
        }else{
            value = bank_1_modif.sound->op[1].lvl.val;
        };
        (get_gwidget<Gtk::SpinButton>("lvl_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_mute_op2 = (get_gwidget<Gtk::ToggleButton>("mute_op2"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    (get_gwidget<Gtk::ToggleButton>("mute_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->extra.mute.val >>4;
        }else{
            value = bank_1_modif.sound->extra.mute.val >>4;
        };
        (get_gwidget<Gtk::ToggleButton>("mute_op2"))->set_active(!(value & 0x01));
        return true; // Return false to remove the callback after one executio
    });

    /* OP2 KLS */
    slot_kls_lft_curve_op2 = (get_gwidget<Gtk::DropDown>("kls_lft_curve_op2"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op2_event));
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].kls.lft_curve.val;
        }else{
            value = bank_1_modif.sound->op[1].kls.lft_curve.val;
        };
        (get_gwidget<Gtk::DropDown>("kls_lft_curve_op2"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_lft_curve_op2_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_lft_curve_op2"));

    slot_kls_rght_curve_op2 = (get_gwidget<Gtk::DropDown>("kls_rght_curve_op2"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op2_event));
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].kls.rght_curve.val;
        }else{
            value = bank_1_modif.sound->op[1].kls.rght_curve.val;
        };
        (get_gwidget<Gtk::DropDown>("kls_rght_curve_op2"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_rght_curve_op2_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_rght_curve_op2"));

    slot_kls_lft_depth_op2 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op2_event));
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].kls.lft_dpth.val;
        }else{
            value = bank_1_modif.sound->op[1].kls.lft_dpth.val;
        };
        (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_rght_depth_op2 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op2_event));
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].kls.rght_dpth.val;
        }else{
            value = bank_1_modif.sound->op[1].kls.rght_dpth.val;
        };
        (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_note_brk_pt_op2 = (get_gwidget<Gtk::DropDown>("note_brk_pt_op2"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op2_event));
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[1].kls.brk_pt.val;
        }else{
            value = bank_1_modif.sound->op[1].kls.brk_pt.val;
        };
        (get_gwidget<Gtk::DropDown>("note_brk_pt_op2"))->set_selected(value % 12);
        int val = (value -3);
        if(val < 0){
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op2"))->set_value( -1 );
        }else{
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op2"))->set_value( val / 12 );
        };
        return true; // Return false to remove the callback after one executio
    });
    auto note_brk_pt_op2_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("note_brk_pt_op2"),get_gwidget<Gtk::SpinButton>("octv_brk_pt_op2"),slot_kls_octv_brk_pt_op2);

    slot_kls_octv_brk_pt_op2 = (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op2_event));


    /* OPERATEUR 3 */
    slot_ams_op3 = (get_gwidget<Gtk::Scale>("ams_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op3_event)); //frame lfo
    (get_gwidget<Gtk::Scale>("ams_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].ams.val;
        }else{
            value = bank_1_modif.sound->op[2].ams.val;
        };
        (get_gwidget<Gtk::Scale>("ams_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP3 FREQUENCE */
    slot_freq_mode_op3 = (get_gwidget<Gtk::DropDown>("freq_mode_op3"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op3_event));
    (get_gwidget<Gtk::DropDown>("freq_mode_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].freq_mode.val;
        }else{
            value = bank_1_modif.sound->op[2].freq_mode.val;
        };
        (get_gwidget<Gtk::DropDown>("freq_mode_op3"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto freq_mode_op3_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("freq_mode_op3"));

    slot_freq_coarse_op3 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op3_event));
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].freq_coarse.val;
        }else{
            value = bank_1_modif.sound->op[2].freq_coarse.val;
        };
        (get_gwidget<Gtk::SpinButton>("freq_coarse_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_freq_fine_op3 = (get_gwidget<Gtk::SpinButton>("freq_fine_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op3_event));
    (get_gwidget<Gtk::SpinButton>("freq_fine_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].freq_fine.val;
        }else{
            value = bank_1_modif.sound->op[2].freq_fine.val;
        };
        (get_gwidget<Gtk::SpinButton>("freq_fine_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_dtun_op3 = (get_gwidget<Gtk::Scale>("dtun_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op3_event));
    (get_gwidget<Gtk::Scale>("dtun_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].dtun.val-7;
        }else{
            value = bank_1_modif.sound->op[2].dtun.val-7;
        };
        (get_gwidget<Gtk::Scale>("dtun_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP3 EG */
    slot_eg_rt1_op3 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op3_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].eg_rt[0].val;
        }else{
            value = bank_1_modif.sound->op[2].eg_rt[0].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt1_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt2_op3 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op3_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].eg_rt[1].val;
        }else{
            value = bank_1_modif.sound->op[2].eg_rt[1].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt2_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt3_op3 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op3_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].eg_rt[2].val;
        }else{
            value = bank_1_modif.sound->op[2].eg_rt[2].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt3_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt4_op3 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op3_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].eg_rt[3].val;
        }else{
            value = bank_1_modif.sound->op[2].eg_rt[3].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt4_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl1_op3 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op3_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].eg_lvl[0].val;
        }else{
            value = bank_1_modif.sound->op[2].eg_lvl[0].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl1_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl2_op3 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op3_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].eg_lvl[1].val;
        }else{
            value = bank_1_modif.sound->op[2].eg_lvl[1].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl2_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl3_op3 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op3_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].eg_lvl[2].val;
        }else{
            value = bank_1_modif.sound->op[2].eg_lvl[2].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl3_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl4_op3 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op3_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].eg_lvl[3].val;
        }else{
            value = bank_1_modif.sound->op[2].eg_lvl[3].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl4_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP3 VOLUME */
    slot_krs_op3 = (get_gwidget<Gtk::Scale>("krs_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op3_event));
    (get_gwidget<Gtk::Scale>("krs_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].krs.val;
        }else{
            value = bank_1_modif.sound->op[2].krs.val;
        };
        (get_gwidget<Gtk::Scale>("krs_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kvs_op3 = (get_gwidget<Gtk::Scale>("kvs_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op3_event));
    (get_gwidget<Gtk::Scale>("kvs_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].kvs.val;
        }else{
            value = bank_1_modif.sound->op[2].kvs.val;
        };
        (get_gwidget<Gtk::Scale>("kvs_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_lvl_op3 = (get_gwidget<Gtk::SpinButton>("lvl_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op3_event));
    (get_gwidget<Gtk::SpinButton>("lvl_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].lvl.val;
        }else{
            value = bank_1_modif.sound->op[2].lvl.val;
        };
        (get_gwidget<Gtk::SpinButton>("lvl_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_mute_op3 = (get_gwidget<Gtk::ToggleButton>("mute_op3"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    (get_gwidget<Gtk::ToggleButton>("mute_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->extra.mute.val >>3;
        }else{
            value = bank_1_modif.sound->extra.mute.val >>3;
        };
        (get_gwidget<Gtk::ToggleButton>("mute_op3"))->set_active(!(value & 0x01));
        return true; // Return false to remove the callback after one executio
    });

    /* OP3 KLS */
    slot_kls_lft_curve_op3 = (get_gwidget<Gtk::DropDown>("kls_lft_curve_op3"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op3_event));
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].kls.lft_curve.val;
        }else{
            value = bank_1_modif.sound->op[2].kls.lft_curve.val;
        };
        (get_gwidget<Gtk::DropDown>("kls_lft_curve_op3"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_lft_curve_op3_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_lft_curve_op3"));

    slot_kls_rght_curve_op3 = (get_gwidget<Gtk::DropDown>("kls_rght_curve_op3"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op3_event));
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].kls.rght_curve.val;
        }else{
            value = bank_1_modif.sound->op[2].kls.rght_curve.val;
        };
        (get_gwidget<Gtk::DropDown>("kls_rght_curve_op3"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_rght_curve_op3_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_rght_curve_op3"));

    slot_kls_lft_depth_op3 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op3_event));
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].kls.lft_dpth.val;
        }else{
            value = bank_1_modif.sound->op[2].kls.lft_dpth.val;
        };
        (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_rght_depth_op3 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op3_event));
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].kls.rght_dpth.val;
        }else{
            value = bank_1_modif.sound->op[2].kls.rght_dpth.val;
        };
        (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_note_brk_pt_op3 = (get_gwidget<Gtk::DropDown>("note_brk_pt_op3"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op3_event));
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[2].kls.brk_pt.val;
        }else{
            value = bank_1_modif.sound->op[2].kls.brk_pt.val;
        };
        (get_gwidget<Gtk::DropDown>("note_brk_pt_op3"))->set_selected(value % 12);
        int val = (value -3);
        if(val < 0){
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op3"))->set_value( -1 );
        }else{
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op3"))->set_value( val / 12 );
        };
        return true; // Return false to remove the callback after one executio
    });
    auto note_brk_pt_op3_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("note_brk_pt_op3"),get_gwidget<Gtk::SpinButton>("octv_brk_pt_op3"),slot_kls_octv_brk_pt_op3);

    slot_kls_octv_brk_pt_op3 = (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op3_event));


    /* OPERATEUR 4 */
    slot_ams_op4 = (get_gwidget<Gtk::Scale>("ams_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op4_event)); //frame lfo
    (get_gwidget<Gtk::Scale>("ams_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].ams.val;
        }else{
            value = bank_1_modif.sound->op[3].ams.val;
        };
        (get_gwidget<Gtk::Scale>("ams_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP4 FREQUENCE */
    slot_freq_mode_op4 = (get_gwidget<Gtk::DropDown>("freq_mode_op4"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op4_event));
    (get_gwidget<Gtk::DropDown>("freq_mode_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].freq_mode.val;
        }else{
            value = bank_1_modif.sound->op[3].freq_mode.val;
        };
        (get_gwidget<Gtk::DropDown>("freq_mode_op4"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto freq_mode_op4_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("freq_mode_op4"));

    slot_freq_coarse_op4 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op4_event));
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].freq_coarse.val;
        }else{
            value = bank_1_modif.sound->op[3].freq_coarse.val;
        };
        (get_gwidget<Gtk::SpinButton>("freq_coarse_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_freq_fine_op4 = (get_gwidget<Gtk::SpinButton>("freq_fine_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op4_event));
    (get_gwidget<Gtk::SpinButton>("freq_fine_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].freq_fine.val;
        }else{
            value = bank_1_modif.sound->op[3].freq_fine.val;
        };
        (get_gwidget<Gtk::SpinButton>("freq_fine_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_dtun_op4 = (get_gwidget<Gtk::Scale>("dtun_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op4_event));
    (get_gwidget<Gtk::Scale>("dtun_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].dtun.val-7;
        }else{
            value = bank_1_modif.sound->op[3].dtun.val-7;
        };
        (get_gwidget<Gtk::Scale>("dtun_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP4 EG */
    slot_eg_rt1_op4 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op4_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].eg_rt[0].val;
        }else{
            value = bank_1_modif.sound->op[3].eg_rt[0].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt1_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt2_op4 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op4_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].eg_rt[1].val;
        }else{
            value = bank_1_modif.sound->op[3].eg_rt[1].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt2_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt3_op4 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op4_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].eg_rt[2].val;
        }else{
            value = bank_1_modif.sound->op[3].eg_rt[2].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt3_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt4_op4 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op4_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].eg_rt[3].val;
        }else{
            value = bank_1_modif.sound->op[3].eg_rt[3].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt4_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl1_op4 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op4_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].eg_lvl[0].val;
        }else{
            value = bank_1_modif.sound->op[3].eg_lvl[0].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl1_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl2_op4 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op4_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].eg_lvl[1].val;
        }else{
            value = bank_1_modif.sound->op[3].eg_lvl[1].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl2_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl3_op4 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op4_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].eg_lvl[2].val;
        }else{
            value = bank_1_modif.sound->op[3].eg_lvl[2].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl3_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl4_op4 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op4_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].eg_lvl[3].val;
        }else{
            value = bank_1_modif.sound->op[3].eg_lvl[3].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl4_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP4 VOLUME */
    slot_krs_op4 = (get_gwidget<Gtk::Scale>("krs_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op4_event));
    (get_gwidget<Gtk::Scale>("krs_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].krs.val;
        }else{
            value = bank_1_modif.sound->op[3].krs.val;
        };
        (get_gwidget<Gtk::Scale>("krs_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kvs_op4 = (get_gwidget<Gtk::Scale>("kvs_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op4_event));
    (get_gwidget<Gtk::Scale>("kvs_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].kvs.val;
        }else{
            value = bank_1_modif.sound->op[3].kvs.val;
        };
        (get_gwidget<Gtk::Scale>("kvs_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_lvl_op4 = (get_gwidget<Gtk::SpinButton>("lvl_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op4_event));
    (get_gwidget<Gtk::SpinButton>("lvl_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].lvl.val;
        }else{
            value = bank_1_modif.sound->op[3].lvl.val;
        };
        (get_gwidget<Gtk::SpinButton>("lvl_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_mute_op4 = (get_gwidget<Gtk::ToggleButton>("mute_op4"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    (get_gwidget<Gtk::ToggleButton>("mute_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->extra.mute.val >>2;
        }else{
            value = bank_1_modif.sound->extra.mute.val >>2;
        };
        (get_gwidget<Gtk::ToggleButton>("mute_op4"))->set_active(!(value & 0x01));
        return true; // Return false to remove the callback after one executio
    });

    /* OP4 KLS */
    slot_kls_lft_curve_op4 = (get_gwidget<Gtk::DropDown>("kls_lft_curve_op4"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op4_event));
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].kls.lft_curve.val;
        }else{
            value = bank_1_modif.sound->op[3].kls.lft_curve.val;
        };
        (get_gwidget<Gtk::DropDown>("kls_lft_curve_op4"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_lft_curve_op4_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_lft_curve_op4"));

    slot_kls_rght_curve_op4 = (get_gwidget<Gtk::DropDown>("kls_rght_curve_op4"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op4_event));
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].kls.rght_curve.val;
        }else{
            value = bank_1_modif.sound->op[3].kls.rght_curve.val;
        };
        (get_gwidget<Gtk::DropDown>("kls_rght_curve_op4"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_rght_curve_op4_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_rght_curve_op4"));

    slot_kls_lft_depth_op4 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op4_event));
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].kls.lft_dpth.val;
        }else{
            value = bank_1_modif.sound->op[3].kls.lft_dpth.val;
        };
        (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_rght_depth_op4 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op4_event));
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].kls.rght_dpth.val;
        }else{
            value = bank_1_modif.sound->op[3].kls.rght_dpth.val;
        };
        (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_note_brk_pt_op4 = (get_gwidget<Gtk::DropDown>("note_brk_pt_op4"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op4_event));
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[3].kls.brk_pt.val;
        }else{
            value = bank_1_modif.sound->op[3].kls.brk_pt.val;
        };
        (get_gwidget<Gtk::DropDown>("note_brk_pt_op4"))->set_selected(value % 12);
        int val = (value -3);
        if(val < 0){
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op4"))->set_value( -1 );
        }else{
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op4"))->set_value( val / 12 );
        };
        return true; // Return false to remove the callback after one executio
    });
    auto note_brk_pt_op4_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("note_brk_pt_op4"),get_gwidget<Gtk::SpinButton>("octv_brk_pt_op4"),slot_kls_octv_brk_pt_op4);

    slot_kls_octv_brk_pt_op4 = (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op4_event));


    /* OPERATEUR 5 */
    slot_ams_op5 = (get_gwidget<Gtk::Scale>("ams_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op5_event)); //frame lfo
    (get_gwidget<Gtk::Scale>("ams_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].ams.val;
        }else{
            value = bank_1_modif.sound->op[4].ams.val;
        };
        (get_gwidget<Gtk::Scale>("ams_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP5 FREQUENCE */
    slot_freq_mode_op5 = (get_gwidget<Gtk::DropDown>("freq_mode_op5"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op5_event));
    (get_gwidget<Gtk::DropDown>("freq_mode_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].freq_mode.val;
        }else{
            value = bank_1_modif.sound->op[4].freq_mode.val;
        };
        (get_gwidget<Gtk::DropDown>("freq_mode_op5"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto freq_mode_op5_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("freq_mode_op5"));

    slot_freq_coarse_op5 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op5_event));
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].freq_coarse.val;
        }else{
            value = bank_1_modif.sound->op[4].freq_coarse.val;
        };
        (get_gwidget<Gtk::SpinButton>("freq_coarse_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_freq_fine_op5 = (get_gwidget<Gtk::SpinButton>("freq_fine_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op5_event));
    (get_gwidget<Gtk::SpinButton>("freq_fine_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].freq_fine.val;
        }else{
            value = bank_1_modif.sound->op[4].freq_fine.val;
        };
        (get_gwidget<Gtk::SpinButton>("freq_fine_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_dtun_op5 = (get_gwidget<Gtk::Scale>("dtun_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op5_event));
    (get_gwidget<Gtk::Scale>("dtun_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].dtun.val-7;
        }else{
            value = bank_1_modif.sound->op[4].dtun.val-7;
        };
        (get_gwidget<Gtk::Scale>("dtun_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP5 EG */
    slot_eg_rt1_op5 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op5_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].eg_rt[0].val;
        }else{
            value = bank_1_modif.sound->op[4].eg_rt[0].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt1_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt2_op5 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op5_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].eg_rt[1].val;
        }else{
            value = bank_1_modif.sound->op[4].eg_rt[1].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt2_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt3_op5 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op5_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].eg_rt[2].val;
        }else{
            value = bank_1_modif.sound->op[4].eg_rt[2].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt3_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt4_op5 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op5_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].eg_rt[3].val;
        }else{
            value = bank_1_modif.sound->op[4].eg_rt[3].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt4_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl1_op5 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op5_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].eg_lvl[0].val;
        }else{
            value = bank_1_modif.sound->op[4].eg_lvl[0].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl1_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl2_op5 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op5_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].eg_lvl[1].val;
        }else{
            value = bank_1_modif.sound->op[4].eg_lvl[1].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl2_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl3_op5 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op5_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].eg_lvl[2].val;
        }else{
            value = bank_1_modif.sound->op[4].eg_lvl[2].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl3_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl4_op5 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op5_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].eg_lvl[3].val;
        }else{
            value = bank_1_modif.sound->op[4].eg_lvl[3].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl4_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP5 VOLUME */
    slot_krs_op5 = (get_gwidget<Gtk::Scale>("krs_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op5_event));
    (get_gwidget<Gtk::Scale>("krs_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].krs.val;
        }else{
            value = bank_1_modif.sound->op[4].krs.val;
        };
        (get_gwidget<Gtk::Scale>("krs_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kvs_op5 = (get_gwidget<Gtk::Scale>("kvs_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op5_event));
    (get_gwidget<Gtk::Scale>("kvs_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].kvs.val;
        }else{
            value = bank_1_modif.sound->op[4].kvs.val;
        };
        (get_gwidget<Gtk::Scale>("kvs_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_lvl_op5 = (get_gwidget<Gtk::SpinButton>("lvl_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op5_event));
    (get_gwidget<Gtk::SpinButton>("lvl_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].lvl.val;
        }else{
            value = bank_1_modif.sound->op[4].lvl.val;
        };
        (get_gwidget<Gtk::SpinButton>("lvl_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_mute_op5 = (get_gwidget<Gtk::ToggleButton>("mute_op5"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    (get_gwidget<Gtk::ToggleButton>("mute_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->extra.mute.val >>1;
        }else{
            value = bank_1_modif.sound->extra.mute.val >>1;
        };
        (get_gwidget<Gtk::ToggleButton>("mute_op5"))->set_active(!(value & 0x01));
        return true; // Return false to remove the callback after one executio
    });

    /* OP5 KLS */
    slot_kls_lft_curve_op5 = (get_gwidget<Gtk::DropDown>("kls_lft_curve_op5"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op5_event));
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].kls.lft_curve.val;
        }else{
            value = bank_1_modif.sound->op[4].kls.lft_curve.val;
        };
        (get_gwidget<Gtk::DropDown>("kls_lft_curve_op5"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_lft_curve_op5_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_lft_curve_op5"));

    slot_kls_rght_curve_op5 = (get_gwidget<Gtk::DropDown>("kls_rght_curve_op5"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op5_event));
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].kls.rght_curve.val;
        }else{
            value = bank_1_modif.sound->op[4].kls.rght_curve.val;
        };
        (get_gwidget<Gtk::DropDown>("kls_rght_curve_op5"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_rght_curve_op5_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_rght_curve_op5"));

    slot_kls_lft_depth_op5 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op5_event));
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].kls.lft_dpth.val;
        }else{
            value = bank_1_modif.sound->op[4].kls.lft_dpth.val;
        };
        (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_rght_depth_op5 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op5_event));
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].kls.rght_dpth.val;
        }else{
            value = bank_1_modif.sound->op[4].kls.rght_dpth.val;
        };
        (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_note_brk_pt_op5 = (get_gwidget<Gtk::DropDown>("note_brk_pt_op5"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op5_event));
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[4].kls.brk_pt.val;
        }else{
            value = bank_1_modif.sound->op[4].kls.brk_pt.val;
        };
        (get_gwidget<Gtk::DropDown>("note_brk_pt_op5"))->set_selected(value % 12);
        int val = (value -3);
        if(val < 0){
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op5"))->set_value( -1 );
        }else{
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op5"))->set_value( val / 12 );
        };
        return true; // Return false to remove the callback after one executio
    });
    auto note_brk_pt_op5_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("note_brk_pt_op5"),get_gwidget<Gtk::SpinButton>("octv_brk_pt_op5"),slot_kls_octv_brk_pt_op5);

    slot_kls_octv_brk_pt_op5 = (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op5_event));

    /* OPERATEUR 6 */
    slot_ams_op6 = (get_gwidget<Gtk::Scale>("ams_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op6_event)); //frame lfo
    (get_gwidget<Gtk::Scale>("ams_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].ams.val;
        }else{
            value = bank_1_modif.sound->op[5].ams.val;
        };
        (get_gwidget<Gtk::Scale>("ams_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP6 FREQUENCE */
    slot_freq_mode_op6 = (get_gwidget<Gtk::DropDown>("freq_mode_op6"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op6_event));
    (get_gwidget<Gtk::DropDown>("freq_mode_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].freq_mode.val;
        }else{
            value = bank_1_modif.sound->op[5].freq_mode.val;
        };
        (get_gwidget<Gtk::DropDown>("freq_mode_op6"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto freq_mode_op6_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("freq_mode_op6"));

    slot_freq_coarse_op6 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op6_event));
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].freq_coarse.val;
        }else{
            value = bank_1_modif.sound->op[5].freq_coarse.val;
        };
        (get_gwidget<Gtk::SpinButton>("freq_coarse_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_freq_fine_op6 = (get_gwidget<Gtk::SpinButton>("freq_fine_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op6_event));
    (get_gwidget<Gtk::SpinButton>("freq_fine_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].freq_fine.val;
        }else{
            value = bank_1_modif.sound->op[5].freq_fine.val;
        };
        (get_gwidget<Gtk::SpinButton>("freq_fine_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_dtun_op6 = (get_gwidget<Gtk::Scale>("dtun_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op6_event));
    (get_gwidget<Gtk::Scale>("dtun_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].dtun.val-7;
        }else{
            value = bank_1_modif.sound->op[5].dtun.val-7;
        };
        (get_gwidget<Gtk::Scale>("dtun_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP6 EG */
    slot_eg_rt1_op6 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op6_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].eg_rt[0].val;
        }else{
            value = bank_1_modif.sound->op[5].eg_rt[0].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt1_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt2_op6 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op6_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].eg_rt[1].val;
        }else{
            value = bank_1_modif.sound->op[5].eg_rt[1].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt2_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt3_op6 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op6_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].eg_rt[2].val;
        }else{
            value = bank_1_modif.sound->op[5].eg_rt[2].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt3_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt4_op6 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op6_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].eg_rt[3].val;
        }else{
            value = bank_1_modif.sound->op[5].eg_rt[3].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_rt4_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl1_op6 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op6_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].eg_lvl[0].val;
        }else{
            value = bank_1_modif.sound->op[5].eg_lvl[0].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl1_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl2_op6 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op6_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].eg_lvl[1].val;
        }else{
            value = bank_1_modif.sound->op[5].eg_lvl[1].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl2_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl3_op6 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op6_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].eg_lvl[2].val;
        }else{
            value = bank_1_modif.sound->op[5].eg_lvl[2].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl3_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl4_op6 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op6_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].eg_lvl[3].val;
        }else{
            value = bank_1_modif.sound->op[5].eg_lvl[3].val;
        };
        (get_gwidget<Gtk::SpinButton>("eg_lvl4_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP6 VOLUME */
    slot_krs_op6 = (get_gwidget<Gtk::Scale>("krs_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op6_event));
    (get_gwidget<Gtk::Scale>("krs_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].krs.val;
        }else{
            value = bank_1_modif.sound->op[5].krs.val;
        };
        (get_gwidget<Gtk::Scale>("krs_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kvs_op6 = (get_gwidget<Gtk::Scale>("kvs_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op6_event));
    (get_gwidget<Gtk::Scale>("kvs_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].kvs.val;
        }else{
            value = bank_1_modif.sound->op[5].kvs.val;
        };
        (get_gwidget<Gtk::Scale>("kvs_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_lvl_op6 = (get_gwidget<Gtk::SpinButton>("lvl_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op6_event));
    (get_gwidget<Gtk::SpinButton>("lvl_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].lvl.val;
        }else{
            value = bank_1_modif.sound->op[5].lvl.val;
        };
        (get_gwidget<Gtk::SpinButton>("lvl_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_mute_op6 = (get_gwidget<Gtk::ToggleButton>("mute_op6"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    (get_gwidget<Gtk::ToggleButton>("mute_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->extra.mute.val;
        }else{
            value = bank_1_modif.sound->extra.mute.val;
        };
        (get_gwidget<Gtk::ToggleButton>("mute_op6"))->set_active(!(value & 0x01));
        return true; // Return false to remove the callback after one executio
    });

    /* OP6 KLS */
    slot_kls_lft_curve_op6 = (get_gwidget<Gtk::DropDown>("kls_lft_curve_op6"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op6_event));
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].kls.lft_curve.val;
        }else{
            value = bank_1_modif.sound->op[5].kls.lft_curve.val;
        };
        (get_gwidget<Gtk::DropDown>("kls_lft_curve_op6"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_lft_curve_op6_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_lft_curve_op6"));

    slot_kls_rght_curve_op6 = (get_gwidget<Gtk::DropDown>("kls_rght_curve_op6"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op6_event));
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].kls.rght_curve.val;
        }else{
            value = bank_1_modif.sound->op[5].kls.rght_curve.val;
        };
        (get_gwidget<Gtk::DropDown>("kls_rght_curve_op6"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_rght_curve_op6_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_rght_curve_op6"));

    slot_kls_lft_depth_op6 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op6_event));
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].kls.lft_dpth.val;
        }else{
            value = bank_1_modif.sound->op[5].kls.lft_dpth.val;
        };
        (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_rght_depth_op6 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op6_event));
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].kls.rght_dpth.val;
        }else{
            value = bank_1_modif.sound->op[5].kls.rght_dpth.val;
        };
        (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_note_brk_pt_op6 = (get_gwidget<Gtk::DropDown>("note_brk_pt_op6"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op6_event));
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value;
        if (compare) {
            value = bank_1_origin.sound->op[5].kls.brk_pt.val;
        }else{
            value = bank_1_modif.sound->op[5].kls.brk_pt.val;
        };
        (get_gwidget<Gtk::DropDown>("note_brk_pt_op6"))->set_selected(value % 12);
        int val = (value -3);
        if(val < 0){
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op6"))->set_value( -1 );
        }else{
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op6"))->set_value( val / 12 );
        };
        return true; // Return false to remove the callback after one executio
    });
    auto note_brk_pt_op6_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("note_brk_pt_op6"),get_gwidget<Gtk::SpinButton>("octv_brk_pt_op6"),slot_kls_octv_brk_pt_op6);

    slot_kls_octv_brk_pt_op6 = (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op6_event));

    LOG( LOG_OUT() );
};

/*** Mouse gesture on drawingarea ***/
void Dx7interface::mouse_mooves(double x, double y, Glib::ustring name){
    if(p_drag != -1){
        double width = (double)get_gwidget<Gtk::DrawingArea>("drawingarea_eg_"+name)->get_width();
        double height = (double)get_gwidget<Gtk::DrawingArea>("drawingarea_eg_"+name)->get_height();
        double x_ratio=( (width ) /400.0);
        double y_ratio=( (height) /99.0);
        int val_x = 0;
        int val_y = int( (height - y) - r_point ) / y_ratio;

        if(p_drag == 0){
            (get_gwidget<Gtk::SpinButton>("eg_lvl4_"+name))->set_value(val_y);
            return;
        }else{
            if (p_drag > 0 && p_drag < 4){
                val_x  = (x - drawarea[name][p_drag-1].first) / x_ratio;
            }else{
                double x_noteoff=(width)*3.0/4.0;
                val_x = (x - x_noteoff) / x_ratio;
            };
        };
        val_x = std::max(0, (100 - int(val_x) ) );
        (get_gwidget<Gtk::SpinButton>("eg_rt"+tostr<int>(p_drag)+"_"+name))->set_value(val_x);
        (get_gwidget<Gtk::SpinButton>("eg_lvl"+tostr<int>(p_drag)+"_"+name))->set_value(val_y);
    };
};
void Dx7interface::mouse_click(int n_press, double x, double y, Glib::ustring name){
    //double width = (double)get_gwidget<Gtk::DrawingArea>("drawingarea_eg_"+name)->get_width();
    double height = (double)get_gwidget<Gtk::DrawingArea>("drawingarea_eg_"+name)->get_height();
    y = std::abs(height - y);
    for( int i = 0; i <= 4 ;i++){
        if(std::hypot(abs(drawarea[name][i].first - x), abs(drawarea[name][i].second - y)) <= r_point_shadow +1 ){
            p_drag=i;
            break;
        };
    };
};
void Dx7interface::mouse_click_release(int n_press, double x, double y, Glib::ustring name){
    p_drag=-1;
};

/*** DRAW FUNCTIONS ***/
int* Dx7interface::get_cr_visible_size(const Cairo::RefPtr<Cairo::Context>& cr, Glib::ustring name){
    int* size = nullptr;
    GdkSurface* surface = (get_gwidget<Gtk::DrawingArea>(name))->get_native()->get_surface()->gobj();

    if (surface) {
        // Get the visible region
        GdkDevice* device = gdk_seat_get_pointer(gdk_display_get_default_seat(gdk_surface_get_display(surface)));
        double x, y;
        GdkModifierType mask;
        gdk_surface_get_device_position(surface, device, &x, &y, &mask);

        // Get the surface dimensions
        size[0] = gdk_surface_get_width(surface);
        size[1] = gdk_surface_get_height(surface);
    };
    return size;
};
/* ADSR */
void Dx7interface::draw_background(const Cairo::RefPtr<Cairo::Context>& cr){
    //LOG( LOG_IN() );
    cr->save();
    cr->set_source_rgba(bg_color[0], bg_color[1], bg_color[2], bg_color[3]);
    cr->paint();    // fill with color
    cr->restore();
    //LOG( LOG_OUT() );
};

void Dx7interface::draw_grid(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height){
    //LOG( LOG_IN() );
    double x=0.0, y=0.0;                // coordonee du point
    double x_step=(width/grid_step_x);  // representation d'un pas
    double y_step=(height/grid_step_y);
    cr->save();
    cr->set_source_rgba(dash_color[0], dash_color[1], dash_color[2], dash_color[3]);
    cr->set_dash(dash_pattern,dash_offset);
    cr->set_line_width(dash_width);
    for(x=0.0;x<=(width - dash_width); x+=x_step) { //draw verticals lines
        cr->move_to(x, y);
        cr->line_to(x, height);
    };
    cr->move_to( (width - dash_width), y);
    cr->line_to( (width - dash_width), height);
    x=0.0;
    for(y=0.0;y<=(height - dash_width); y+=y_step) {  //draw horizontals lines
        cr->move_to(x, y);
        cr->line_to(width, y);
    };
    cr->move_to(x , (height - dash_width) );
    cr->line_to(width, (height - dash_width));
    cr->stroke();  // trace le contour en preservant ceux deja tracés
    cr->restore();
};

void Dx7interface::draw_point(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y,double width, bool orange){
    //LOG( LOG_IN() );
    double r,g,b;
    r=line_color[0];
    g=line_color[1];
    b=line_color[2];
    // Orange dx7 0.88,0.35,0.27,1.0
    if( orange ){
        r=0.88;
        g=0.35;
        b=0.27;
    };
    double radius;
    cr->save();
    cr->set_source_rgba(r,g,b,1.0);
    radius = r_point/2.0;       // Radius of the point
    cr->arc(x, y, radius, 0.0, 2.0 * M_PI);
    cr->fill();
    cr->set_source_rgba(r,g,b,0.8);
    radius = r_point/1.33;       // Radius of the point
    cr->arc(x, y, radius, 0.0, 2.0 * M_PI);
    cr->fill();
    cr->set_source_rgba(r,g,b,0.8);
    radius = r_point;       // Radius of the point
    cr->arc(x, y, radius, 0.0, 2.0 * M_PI);
    cr->stroke();
    cr->set_source_rgba(r,g,b,0.1);
    radius = r_point_shadow;       // Radius of the point
    cr->arc(x, y, radius, 0.0, 2.0 * M_PI);
    cr->stroke();
    cr->restore();
    //LOG( LOG_OUT() );
};

void Dx7interface::draw_note_off(const Cairo::RefPtr<Cairo::Context>& cr,double x_noteoff, double height){
    /* set the note Off line */
    cr->save();
    cr->set_source_rgba(dash_color[0], dash_color[1], dash_color[2], dash_color[3]);
    cr->set_dash(dash_pattern,dash_offset);
    cr->set_line_width(dash_width+2.0);
    cr->move_to(x_noteoff, 0);
    cr->line_to(x_noteoff, height);
    cr->stroke();
    cr->restore();
};

void Dx7interface::draw_adsr(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height, Glib::ustring name){
    //LOG( LOG_IN() );
    //set origin to bottom left
    cr->translate(0, height);
    cr->scale(1, -1);
    double x_noteoff=(width)*3.0/4.0;
    draw_note_off(cr, x_noteoff, height);

    // Draw ADSR
    // scale factor, to compute x
    height-=2.0*r_point;
    width-=2.0*r_point;
    double x_ratio=( (width ) /400.0);
    double y_ratio=( (height) /99.0);

    /* Draw curve */
    bool r_flag = false;      //draw_point flag for First and last point switch color
    double x=r_point, y=r_point;     // coordonee du point

    for(uint8_t i=1;i<=4;i++) {
        cr->save();
        cr->set_source_rgba(line_color[0],line_color[1],line_color[2],1.0);
        cr->set_line_width(line_width);
        r_flag = false;
        if (i==1) {
            cr->move_to( x,
                       ( y + ( (double)(get_gwidget<Gtk::SpinButton>("eg_lvl4_"+name))->get_value() * y_ratio) )
            );
        }else{
            cr->move_to(x, y);    // deplace le curseur
        };
        if (i==4){
            x = x_noteoff;
            cr->line_to(x, y);      // trace une ligne
            cr->move_to(x, y);
            r_flag = true;
        };
        x += ( ( x_ratio * (
            std::abs( 100.0 - (double)( (get_gwidget<Gtk::SpinButton>("eg_rt"+tostr<unsigned int>(i)+"_"+name))->get_value() + 1.0 ) )
        ) ) ) ;
        y = (r_point) + ( ( (double)(get_gwidget<Gtk::SpinButton>("eg_lvl"+tostr<unsigned int>(i)+"_"+name))->get_value() ) * y_ratio );
        cr->line_to(x, y);      // trace une ligne
        cr->move_to(x, y);
        cr->stroke();
        cr->restore();
        drawarea[name][i]={x,y};
        draw_point(cr,x,y, width,r_flag);
    };
    //draw first point at last to covert line
    x = r_point;
    y = r_point + ( (double)(get_gwidget<Gtk::SpinButton>("eg_lvl4_"+name))->get_value() * y_ratio );
    drawarea[name][0]={x,y};
    draw_point(cr, x, y, width, true);
    //LOG( LOG_OUT() );
};
/* KLS */
void Dx7interface::draw_keyboard(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height, Glib::ustring num_op){
    //LOG( LOG_IN() );
    /* key touch */
    if (std::filesystem::exists(std::string(PROGRAMNAME_IMG_DIR"/touche_b.png"))
        && std::filesystem::exists(std::string(PROGRAMNAME_IMG_DIR"/touche_w.png"))
        && std::filesystem::exists(std::string(PROGRAMNAME_IMG_DIR"/keyboard_background.png"))
        && std::filesystem::exists(std::string(PROGRAMNAME_IMG_DIR"/keyboard.png"))
    ){
        Cairo::RefPtr<Cairo::ImageSurface> touch;
        Cairo::RefPtr<Cairo::ImageSurface> touch_b = Cairo::ImageSurface::create_from_png(PROGRAMNAME_IMG_DIR"/touche_b.png");
        Cairo::RefPtr<Cairo::ImageSurface> touch_w = Cairo::ImageSurface::create_from_png(PROGRAMNAME_IMG_DIR"/touche_w.png");
        double width_w = (double)touch_w->get_width();
        /* keyboard */
        Cairo::RefPtr<Cairo::ImageSurface> keyboard_bg_image_surface = Cairo::ImageSurface::create_from_png(PROGRAMNAME_IMG_DIR"/keyboard_background.png");
        Cairo::RefPtr<Cairo::ImageSurface> keyboard_image_surface = Cairo::ImageSurface::create_from_png(PROGRAMNAME_IMG_DIR"/keyboard.png");
        double keyboard_width = (double)keyboard_image_surface->get_width();
        double keyboard_heigth = (double)keyboard_image_surface->get_height();
        /* UI values */
        Glib::ustring note = (std::dynamic_pointer_cast<Gtk::StringObject>((get_gwidget<Gtk::DropDown>("kls_rght_curve_op"+num_op))->get_selected_item()))->get_string();
        unsigned int num_note = get_gwidget<Gtk::DropDown>("note_brk_pt_op"+num_op)->get_selected();
        unsigned int octv = get_gwidget<Gtk::SpinButton>("octv_brk_pt_op"+num_op)->get_value();
        // note_ = (get_gwidget<Gtk::DropDown>("note_transpose"))->get_selected();
        //octv_ = (((get_gwidget<Gtk::SpinButton>("octv_transpose"))->get_value() - 1) * 12);
        /* Key touch */
        //C 4 = note=3 oct=4
        //A 4 = note=0 oct=4
        // progression  -2  1 3  6 8 noir
        //             -3-10 2 45 7  blanche
        /* compute step */
        int note_ref = 3; // C
        int dec = num_note - note_ref;
        int octv_base = 0;
        int dep_octv = octv_base - octv;
        double dep = (2.0* width_w) - ((double)dep_octv)*(7.0*width_w);
        switch(dec){
            case 0:{ // C
                dep = dep;
                touch = touch_w;
                break;
            }
            case 1:{ // C#
                dep = dep + (width_w /2.0);
                touch = touch_b;
                break;
            };
            case 2:{ // D
                dep = dep + (width_w);
                touch = touch_w;
                break;
            };
            case 3:{ // D#
                dep = dep + ( ( width_w ) + ( width_w/2.0 ) );
                touch = touch_b;

                break;
            };
            case 4:{ // E
                dep = dep + ( 2.0 * width_w ) ;
                touch = touch_w;
                break;
            };
            case 5:{ // F
                dep = dep + ( 3.0 * width_w );
                touch = touch_w;
                break;
            };
            case 6:{ // F#
                dep = dep + ( ( 3.0 * width_w ) + ( width_w/2.0 ) );
                touch = touch_b;
                break;
            };
            case 7:{ // G
                dep = dep + ( 4.0 * width_w );
                touch = touch_w;
                break;
            };
            case 8:{ // G#
                dep = dep +( ( 4.0 * width_w) + (width_w/2.0) ) ;
                touch = touch_b;
                break;
            };
            case -3:{ // A
                dep = dep + ( ( 5.0 * width_w ) );
                touch = touch_w;
                break;
            };
            case -2:{ // A#
                dep = dep + ( ( 5.0 * width_w ) + (width_w/2.0) ) ;
                touch = touch_b;
                break;
            };
            case -1:{ // B
                dep = dep + ( ( 6.0 * width_w ) );
                touch = touch_w;
                break;
            };
        };
        /* Keyboard */
        double keyboard_pos = height - keyboard_heigth;
        double keyboard_start = (width /2.0) - (0.5*width_w) - dep;
        double pos_key = (width /2.0) - (touch->get_width() / 2.0);

        /* Keyboard bg */
        cr->save();
        cr->set_source(keyboard_bg_image_surface, keyboard_start, keyboard_pos);
        cr->paint();
        cr->set_source(keyboard_bg_image_surface, keyboard_start + keyboard_width - (3*width_w), keyboard_pos);
        cr->paint();
        if( touch == touch_b ){
            /* Keyboard under */
            cr->set_source(keyboard_image_surface, keyboard_start, keyboard_pos);
            cr->paint();
            cr->set_source(keyboard_image_surface, keyboard_start + keyboard_width - (3*width_w), keyboard_pos);
            cr->paint();
            /* Touch */
            cr->set_source(touch, pos_key, keyboard_pos );
            cr->paint();
        }else{
            /* Touch */
            cr->set_source(touch, pos_key, keyboard_pos );
            cr->paint();
            /* Keyboard over */
            cr->set_source(keyboard_image_surface, keyboard_start, keyboard_pos);
            cr->paint();
            cr->set_source(keyboard_image_surface, keyboard_start + keyboard_width - (3*width_w), keyboard_pos);
            cr->paint();
        };
        cr->restore();
    }else{
        std::string msg_err = _("File not found: ") + std::string(PROGRAMNAME_IMG_DIR"/ keyboard or touch .png");
        LOG( msg_err );
    };
    //LOG( LOG_OUT() );
};

void Dx7interface::draw_axis(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height){
    //LOG( LOG_IN() );
    double x=(width/2.0), y=(height/2.0);
    //set origin to bottom left
    double r,g,b,a;
    // Orange dx7 0.88,0.35,0.27,1.0
    r=0.88;
    g=0.35;
    b=0.27;
    a=1.0;

    // Draw AXIS
    cr->save();
    cr->set_source_rgba(r,g,b,a);
    cr->set_line_width(1.0);
    cr->move_to(x, 0);
    cr->line_to(x, height);
    cr->move_to(0, y);
    cr->line_to(width, y);
    cr->stroke();
    cr->restore();
    //LOG( LOG_OUT() );
};

void Dx7interface::draw_kls_curve(const Cairo::RefPtr<Cairo::Context>& cr, Glib::ustring type_curve, double width, double height, double dpth, Glib::ustring dir, double lvl){
    double half_width  = width/2.0;
    double half_height = (height/2.0) + ( (height)*(lvl - 50.0)/160.0 );
    double scale_factor = (100.0 - dpth) +25 ; // +25, to get 85 at max ( 85==100 depth)
    switch( str_const_hash(type_curve.c_str()) ){
        case "EXP+"_hash:{
            // EXP+ rigth
            cr->move_to(half_width, half_height);
            for (double x = 0.0, y = 0.0; x <= half_width && y <= half_height ; x +=5.0) {
                // -1 facteur correctif à l'origine
                y = std::exp( x / scale_factor ) - 1 ;
                if (dir == "lft"){ x = -x; };
                cr->line_to( half_width + x, half_height + y );
                if (dir == "lft"){ x = -x; };
            }
            break;
        };
        case "EXP-"_hash:{
            // EXP- right
            cr->move_to(half_width, half_height);
            for (double x = 0.0, y = 0.0; x <= half_width && y < half_height ; x +=5.0) {
                y = std::exp( x / scale_factor ) - 1;
                if (dir == "lft"){ x = -x; };
                cr->line_to( half_width + x, half_height - y );
                if (dir == "lft"){ x = -x; };
            }
            break;
        };
        case "LIN+"_hash:{
            //LIN+ rigth
            cr->move_to(half_width,half_height);
            if (dir == "lft"){ width = 0; };
            cr->line_to( width, half_height + ( (half_height)*(dpth/100) ) );
            break;
        };
        case "LIN-"_hash:{
            //LIN- rigth
            cr->move_to(half_width,half_height);
            if (dir == "lft"){ width = 0; };
            cr->line_to( width, half_height - ( (half_height)*(dpth/100) ) );
            break;
        };
    };
};

void Dx7interface::draw_kls(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height, Glib::ustring num_op){
    //LOG( LOG_IN() );
    Glib::ustring rght_curve =(
        std::dynamic_pointer_cast<Gtk::StringObject>(
            (get_gwidget<Gtk::DropDown>("kls_rght_curve_op"+num_op))->get_selected_item())
                               )->get_string() ;
    double rght_dpth =(double)(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op"+num_op))->get_value()+1 ;
    Glib::ustring lft_curve =(
        std::dynamic_pointer_cast<Gtk::StringObject>(
            (get_gwidget<Gtk::DropDown>("kls_lft_curve_op"+num_op))->get_selected_item())
                              )->get_string() ;
    double lft_dpth =(double)(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op"+num_op))->get_value()+1 ;
    double lvl =(get_gwidget<Gtk::SpinButton>("lvl_op"+num_op))->get_value()+1;
    cr->save();
    cr->translate(0, height);
    cr->scale(1, -1);
    cr->set_source_rgba(line_color[0],line_color[1],line_color[2],1.0);
    cr->set_line_width(line_width);
    draw_kls_curve(cr,rght_curve,width,height,rght_dpth,"rght", lvl);
    cr->stroke();
    draw_kls_curve(cr,lft_curve,width,height,lft_dpth,"lft", lvl);
    cr->stroke();
    cr->restore();
    //LOG( LOG_OUT() );
};

/** EVENTS **/
/* Draw */
void Dx7interface::on_draw_kls_event(const Cairo::RefPtr<Cairo::Context>& cr,int width, int height, Glib::ustring num_op){
    // LOG( LOG_IN() );
    if( cr && (width != 0) && (height != 0) ){
        double wdth=(double)width, hght=(double)height;
        draw_background(cr);
        draw_axis(cr,wdth, hght );
        draw_kls( cr, wdth, hght, num_op);
        draw_keyboard( cr, wdth, hght, num_op );
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op"+num_op))->queue_draw();
    };
    // LOG( LOG_OUT() );
};

void Dx7interface::on_draw_op_event(const Cairo::RefPtr<Cairo::Context>& cr,int width, int height, Glib::ustring num_op){
    // LOG( LOG_IN() );
    if( cr && (width != 0) && (height != 0) ){
        double wdth=(double)width, hght=(double)height;
        draw_background(cr);
        draw_grid( cr, wdth, hght);
        draw_adsr( cr, wdth, hght, "op"+num_op );
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op"+num_op))->queue_draw();
    };
    // LOG( LOG_OUT() );
};

void Dx7interface::on_draw_pitch_event(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height){
    // LOG( LOG_IN() );
    if (cr && (width != 0) && (height != 0) ){
        double wdth=(double)width, hght=(double)height;
        draw_background(cr);
        draw_grid( cr, wdth, hght );
        draw_adsr( cr, wdth, hght, "pitch" );
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    };
    // LOG( LOG_OUT() );
};

void Dx7interface::on_draw_algo(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height){
    // LOG( LOG_IN() );
    std::string img;
    if(compare){
        img = std::string(PROGRAMNAME_IMG_DIR"/algo"+tostr<unsigned int>(bank_1_origin.sound->algo.algo.val+1)+".png");
    }else{
        img = std::string(PROGRAMNAME_IMG_DIR"/algo"+tostr<unsigned int>(bank_1_modif.sound->algo.algo.val+1)+".png");
    }
    if ( std::filesystem::exists(img) ){
        Cairo::RefPtr<Cairo::ImageSurface> algo_image_surface = Cairo::ImageSurface::create_from_png(img);
        double scale_factor = width / algo_image_surface->get_width();
        cr->save();
        cr->scale(scale_factor,scale_factor);
        cr->set_source(algo_image_surface, 0, 0);
        cr->paint();
        cr->restore();
    }else{
        std::string msg_err = _("File not found: ") + img;
        LOG_ERR( msg_err );
    };
    // LOG( LOG_OUT() );
};

void Dx7interface::on_draw_lfo(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height){
    // LOG( LOG_IN() );
    Glib::ustring img = ( std::dynamic_pointer_cast<Gtk::StringObject>((get_gwidget<Gtk::DropDown>("lfo_wav"))->get_selected_item()) )->get_string();
    if (img == "S/HOLD"){
        img = PROGRAMNAME_IMG_DIR"/S_HOLD.png";
    }else{
        img = PROGRAMNAME_IMG_DIR"/"+img+".png";
    };
    if( std::filesystem::exists(img.c_str()) ){
        Cairo::RefPtr<Cairo::ImageSurface> lfo_image_surface = Cairo::ImageSurface::create_from_png(img);
        double scale_factor = width / lfo_image_surface->get_width();
        cr->save();
        cr->scale(scale_factor,scale_factor);
        cr->set_source(lfo_image_surface, 0,0);
        cr->paint();
        cr->restore();
    }else{
        std::string msg_err = _("File not found: ") + img;
        LOG_ERR( msg_err );
    };
    // LOG( LOG_OUT() );
};

void Dx7interface::redraw_all_curve(){
    /* all drawingarea redraw */
    (get_gwidget<Gtk::DrawingArea>("drawingarea_lfo_wav"))->queue_draw();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_algo"))->queue_draw();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op1"))->queue_draw();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op2"))->queue_draw();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op3"))->queue_draw();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op4"))->queue_draw();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op5"))->queue_draw();
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op6"))->queue_draw();
};

/* MIDI Functions */
void Dx7interface::on_midi_channel_send_event(){
    // LOG( LOG_IN() );
    channel_send = (get_gwidget<Gtk::SpinButton>("midi_channel_send"))->get_value()-1;
    // LOG( LOG_OUT() );
};
void Dx7interface::on_midi_channel_receive_event(){
    // LOG( LOG_IN() );
    channel_receive = (get_gwidget<Gtk::SpinButton>("midi_channel_receive"))->get_value()-1;
    // LOG( LOG_OUT() );
};


/* Compare */
void Dx7interface::on_compare_event(){
    // TODO: add set mute/unmute from struct
    // BUG: compare on empty object with load
    if ( (get_gwidget<Gtk::ToggleButton>("btn_compare"))->get_active() ) {
        LOG("compare on");
        compare=true;
        get_gwidget<Gtk::ToggleButton>("btn_compare")->add_css_class("blink");
        set_voice(&bank_1_origin.sound[0]);
    }else{
        LOG("compare off");
        compare=false;
        get_gwidget<Gtk::ToggleButton>("btn_compare")->remove_css_class("blink");
        set_voice(&bank_1_modif.sound[0]);
    };
};

/* Extra parameters */
void Dx7interface::on_send_extra_parameters_event(){
    if ( (get_gwidget<Gtk::CheckButton>("btn_send_extra_parameters"))->get_active() ) {
        send_extra_params=true;
    }else{
        send_extra_params=false;
    };
};

void Dx7interface::on_mode_tf1_event(){
    if ( (get_gwidget<Gtk::CheckButton>("btn_mode_tf1"))->get_active() ) {
        mode_tf1=true;
        set_voice_parameters(&bank_1_modif.sound[0]);
    }else{
        mode_tf1=false;
    };
};

/* Save box */
void Dx7interface::on_as_raw_event(){
    if(get_gwidget<Gtk::CheckButton>("checkbutton_as_raw")->get_active()){
        get_gwidget<Gtk::CheckButton>("checkbutton_128")->set_sensitive(true);
    }else{
        get_gwidget<Gtk::CheckButton>("checkbutton_32")->set_active(true);
        get_gwidget<Gtk::CheckButton>("checkbutton_128")->set_sensitive(false);
    };
};
void Dx7interface::on_extra_param_event(){
    if(get_gwidget<Gtk::CheckButton>("checkbutton_add_extra_parameters")->get_active()){
        get_gwidget<Gtk::CheckButton>("checkbutton_extra_parameters_by_bank")->set_sensitive(true);
        get_gwidget<Gtk::CheckButton>("checkbutton_extra_parameters_by_sound")->set_sensitive(true);
    }else{
        get_gwidget<Gtk::CheckButton>("checkbutton_extra_parameters_by_bank")->set_sensitive(false);
        get_gwidget<Gtk::CheckButton>("checkbutton_extra_parameters_by_sound")->set_sensitive(false);
    };
};

/* Panic */
void Dx7interface::on_panic_event(){
    unsigned char msg[3];
    msg[0]=0xB0;
    msg[1]=0x7B;
    msg[2]=0x00;
    send_midi(SND_SEQ_EVENT_CONTROLLER, 3, msg);
};

/* Extra Functions */
void Dx7interface::on_mono_poly_event(){
	LOG( LOG_IN() );
    unsigned char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status + channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x40;                        // 40
                                        // bit0 0=poly/bit0 1=mono
    msg[5]=(get_gwidget<Gtk::ToggleButton>("btn_poly_mono"))->get_active();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    bank_1_modif.sound->extra.functions.poly_mono.val = msg[5];
    if ( (get_gwidget<Gtk::ToggleButton>("btn_poly_mono"))->get_active() ) {
        (get_gwidget<Gtk::ToggleButton>("btn_poly_mono"))->set_label(_("Monophonic"));
    }else{
        (get_gwidget<Gtk::ToggleButton>("btn_poly_mono"))->set_label(_("Polyphonic"));
    };
	LOG( LOG_OUT() );
};

void Dx7interface::on_ptch_bnd_rng_event(){
    LOG( LOG_IN() );
    unsigned char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status + channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x41;                        // 41
                                        // 0111 1000
    msg[5]=(get_gwidget<Gtk::Scale>("ptch_bnd_rng"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    bank_1_modif.sound->extra.functions.ptch_bnd_rng.val = msg[5];
    LOG( LOG_OUT() );
};

void Dx7interface::on_ptch_bnd_stp_event(){
    LOG( LOG_IN() );
    unsigned char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status + channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x42;                        // 42
    msg[5]=(get_gwidget<Gtk::Scale>("ptch_bnd_stp"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    bank_1_modif.sound->extra.functions.ptch_bnd_stp.val = msg[5];
    LOG( LOG_OUT() );
};

void Dx7interface::on_portamento_md_event(){
    LOG( LOG_IN() );
    unsigned char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status + channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x43;                        // 43
                                        // 0-3 bit0=retain; bit1=follow
    msg[5]=(get_gwidget<Gtk::ToggleButton>("btn_portamento_md"))->get_active();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    bank_1_modif.sound->extra.functions.portamento_md.val = msg[5];
    if ( (get_gwidget<Gtk::ToggleButton>("btn_portamento_md"))->get_active() ) {
        (get_gwidget<Gtk::ToggleButton>("btn_portamento_md"))->set_label(_("Follow"));
    }else{
        (get_gwidget<Gtk::ToggleButton>("btn_portamento_md"))->set_label(_("Retain"));
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_portamento_glss_event(){
    LOG( LOG_IN() );
    unsigned char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status + channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x44;                        // 44
                                        // 0-3 bit 0= 1=gliss ???
    msg[5]=(get_gwidget<Gtk::ToggleButton>("btn_portamento_glss"))->get_active();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    bank_1_modif.sound->extra.functions.portamento_glss.val = msg[5];
    LOG( LOG_OUT() );
};

void Dx7interface::on_portamento_tm_event(){
    LOG( LOG_IN() );
    unsigned char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status + channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x45;                        // 45
    msg[5]=(get_gwidget<Gtk::SpinButton>("portamento_tm"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    bank_1_modif.sound->extra.functions.portamento_tm.val = msg[5];
    LOG( LOG_OUT() );
};

void Dx7interface::on_md_whl_rng_event(){
    LOG( LOG_IN() );
    unsigned char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status + channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x46;                        // 46
    msg[5]=(get_gwidget<Gtk::SpinButton>("md_whl_rng"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    bank_1_modif.sound->extra.functions.md_whl_rng.val = msg[5];
    LOG( LOG_OUT() );
};

void Dx7interface::on_md_whl_assgn_event(){
    LOG( LOG_IN() );
    unsigned char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status + channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x47;                        // 47
    msg[5]= (get_gwidget<Gtk::CheckButton>("md_whl_ptch"))->get_active()
          +((get_gwidget<Gtk::CheckButton>("md_whl_mp"))->get_active()*2)
          +((get_gwidget<Gtk::CheckButton>("md_whl_gbs"))->get_active()*4);
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    bank_1_modif.sound->extra.functions.md_whl_assgn.val = msg[5];
    LOG( LOG_OUT() );
};

void Dx7interface::on_foot_rng_event(){
    LOG( LOG_IN() );
    unsigned char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status + channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x48;                        // 48
    msg[5]=(get_gwidget<Gtk::SpinButton>("foot_rng"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    bank_1_modif.sound->extra.functions.foot_rng.val = msg[5];
    LOG( LOG_OUT() );
};

void Dx7interface::on_foot_assgn_event(){
    LOG( LOG_IN() );
    unsigned char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status + channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x49;                        // 49
    msg[5]= (get_gwidget<Gtk::CheckButton>("foot_ptch"))->get_active()
          +((get_gwidget<Gtk::CheckButton>("foot_mp"))->get_active()*2)
          +((get_gwidget<Gtk::CheckButton>("foot_gbs"))->get_active()*4);
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    bank_1_modif.sound->extra.functions.foot_assgn.val = msg[5];
    LOG( LOG_OUT() );
};

void Dx7interface::on_brth_rng_event(){
    LOG( LOG_IN() );
    unsigned char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status + channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x4A;                        // 4A
    msg[5]=(get_gwidget<Gtk::SpinButton>("brth_rng"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    bank_1_modif.sound->extra.functions.brth_rng.val = msg[5];
    LOG( LOG_OUT() );
};

void Dx7interface::on_brth_assgn_event(){
    LOG( LOG_IN() );
    unsigned char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status + channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x4B;                        // 4B
    msg[5]= (get_gwidget<Gtk::CheckButton>("brth_ptch"))->get_active()
          +((get_gwidget<Gtk::CheckButton>("brth_mp"))->get_active()*2)
          +((get_gwidget<Gtk::CheckButton>("brth_gbs"))->get_active()*4);
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    bank_1_modif.sound->extra.functions.brth_assgn.val = msg[5];
    LOG( LOG_OUT() );
};

void Dx7interface::on_aftrtch_rng_event(){
    LOG( LOG_IN() );
    unsigned char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status + channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x4C;                        // 4C
    msg[5]=(get_gwidget<Gtk::SpinButton>("aftrtch_rng"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    bank_1_modif.sound->extra.functions.aftrtch_rng.val = msg[5];
    LOG( LOG_OUT() );
};

void Dx7interface::on_aftrtch_assgn_event(){
    LOG( LOG_IN() );
    unsigned char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status + channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x4D;                        // 4D
    msg[5]= (get_gwidget<Gtk::CheckButton>("aftrtch_ptch"))->get_active()
          +((get_gwidget<Gtk::CheckButton>("aftrtch_mp"))->get_active()*2)
          +((get_gwidget<Gtk::CheckButton>("aftrtch_gbs"))->get_active()*4);
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    bank_1_modif.sound->extra.functions.aftrtch_assgn.val = msg[5];
    LOG( LOG_OUT() );
};

/* ALGO */
void Dx7interface::on_algo_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x01;
        msg[4]=0x06;
        msg[5]=(get_gwidget<Gtk::SpinButton>("algo_number"))->get_value()-1;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->algo.algo.val = msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_algo"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_feedback_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x01;
        msg[4]=0x07;
        msg[5]=(get_gwidget<Gtk::SpinButton>("feedback"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->algo.feedback.val = msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_transpose_event() {
    LOG( LOG_IN() );
    if (!compare){
        char val =  (get_gwidget<Gtk::DropDown>("note_transpose"))->get_selected()
                 +( ((get_gwidget<Gtk::SpinButton>("octv_transpose"))->get_value()-1)*12 );
        if( val >= 0 && val <= 48){
            unsigned char msg[7];
            msg[0]=0xF0;
            msg[1]=id_fabricant;
            msg[2]=sub_status + channel_send;
            msg[3]=0x01;
            msg[4]=0x10;
            msg[5]=val;
            msg[6]=0xF7;
            send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
            bank_1_modif.sound->algo.transpose.val = msg[5];
        };
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_oks_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x01;
        msg[4]=0x08;
        if ( (get_gwidget<Gtk::CheckButton>("oks"))->get_active() ) {
            msg[5]=0x01;
        }else{
            msg[5]=0x00;
        };
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->algo.oks.val = msg[5];
    };
    LOG( LOG_OUT() );
};

/* LFO */
void Dx7interface::on_lfo_wav_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x01;
        msg[4]=0x0E;
        msg[5]=(get_gwidget<Gtk::DropDown>("lfo_wav"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->lfo.wave.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_lfo_wav"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_lfo_sync_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x01;
        msg[4]=0x0D;
        if ( (get_gwidget<Gtk::CheckButton>("lfo_sync"))->get_active() ) {
            msg[5]=0x01;
        }else{
            msg[5]=0x00;
        }
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->lfo.sync.val=msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_lfo_speed_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x01;
        msg[4]=0x09;
        msg[5]=(get_gwidget<Gtk::SpinButton>("lfo_speed"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->lfo.speed.val = msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_lfo_delay_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x01;
        msg[4]=0x0A;
        msg[5]=(get_gwidget<Gtk::SpinButton>("lfo_delay"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->lfo.delay.val = msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_lfo_pmd_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x01;
        msg[4]=0x0B;
        msg[5]=(get_gwidget<Gtk::SpinButton>("lfo_pmd"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->lfo.pmd.val = msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_lfo_amd_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x01;
        msg[4]=0x0C;
        msg[5]=(get_gwidget<Gtk::SpinButton>("lfo_amd"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->lfo.amd.val = msg[5];
    };
    LOG( LOG_OUT() );
};

/*LFO MODULATION */
void Dx7interface::on_pms_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x01;
        msg[4]=0x0F;
        msg[5]=(get_gwidget<Gtk::Scale>("pms"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->lfo.pms.val = msg[5];
    };
    LOG( LOG_OUT() );
};

/* PITCH EG */
void Dx7interface::on_pitch_rt1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x7E;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_pitch"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->pitch.eg_rt[0].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_pitch_rt2_event() {
    LOG( LOG_IN() );
    if (!compare){
    unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x7F;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_pitch"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->pitch.eg_rt[1].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_pitch_rt3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x01;
        msg[4]=0x00;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_pitch"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->pitch.eg_rt[2].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_pitch_rt4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x01;
        msg[4]=0x01;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_pitch"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->pitch.eg_rt[3].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_pitch_lvl1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x01;
        msg[4]=0x02;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_pitch"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->pitch.eg_lvl[0].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_pitch_lvl2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x01;
        msg[4]=0x03;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_pitch"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->pitch.eg_lvl[1].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_pitch_lvl3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x01;
        msg[4]=0x04;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_pitch"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->pitch.eg_lvl[2].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_pitch_lvl4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x01;
        msg[4]=0x05;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_pitch"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->pitch.eg_lvl[3].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

/* MUTE FOR EACH OPERATOR IN DX7 */
void Dx7interface::on_mute_op_event() {
    
    unsigned char msg[7];
    uint8_t i,mute_val=0x00;

    for(i=1; i<=6;i++){
        bool widget_active = (bool)((get_gwidget<Gtk::ToggleButton>("mute_op"+tostr<unsigned int>(i)))->get_active());
        mute_val=( mute_val | (!widget_active) ) ;
        if (i!=6){
            mute_val=mute_val + 1;
        };
        //msg = "on_mute_op mute_val : " + std::bitset<8>(mute_val) +std::endl;
    };
    if(!compare){
        bank_1_modif.sound->extra.mute.val=mute_val;
    };
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status + channel_send;
    msg[3]=0x01;
    msg[4]=0x1B;
    msg[5]=mute_val;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);

    for(i=1; i<=6;i++){
        (this->*mute_hexter_functions[i-1])();
    };
};

/* OP1 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op1_event(){
    LOG( LOG_IN() );
    uint8_t mute_val;
    
    if(!compare){
        mute_val = bank_1_modif.sound->extra.mute.val;
    }else{
        mute_val = bank_1_origin.sound->extra.mute.val;
    };
    
    mute_val = mute_val >>5;
    
    if ( !(mute_val & 0x01) ) {
        (get_gwidget<Gtk::Label>("label_general_op1"))->set_label(_("/* OP1 */"));
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x79;        // volume op1
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{        
        (get_gwidget<Gtk::Label>("label_general_op1"))->set_label(_(" OP1 "));
        on_lvl_op1_event();        
    };
    LOG( LOG_OUT() );
};
/* OP2 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op2_event() {
    LOG( LOG_IN() );
    uint8_t mute_val;
    
    if(!compare){
        mute_val = bank_1_modif.sound->extra.mute.val;
    }else{
        mute_val = bank_1_origin.sound->extra.mute.val;
    };
    
    mute_val = mute_val >>4;
    
    if ( !(mute_val & 0x01) ) {
        (get_gwidget<Gtk::Label>("label_general_op2"))->set_label(_("/* OP2 */"));
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x64;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op2"))->set_label(_(" OP2 "));
        on_lvl_op2_event();
    };
    LOG( LOG_OUT() );
};
/* OP3 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op3_event() {
    LOG( LOG_IN() );    
    uint8_t mute_val;
    
    if(!compare){
        mute_val = bank_1_modif.sound->extra.mute.val;
    }else{
        mute_val = bank_1_origin.sound->extra.mute.val;
    };
    
    mute_val = mute_val >>3;
    
    if ( !(mute_val & 0x01) ) {
        (get_gwidget<Gtk::Label>("label_general_op3"))->set_label(_("/* OP3 */"));
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x4F;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op3"))->set_label(_(" OP3 "));
        on_lvl_op3_event();
    };
    LOG( LOG_OUT() );
};
/* OP4 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op4_event() {
    LOG( LOG_IN() );
    uint8_t mute_val;
    
    if(!compare){
        mute_val = bank_1_modif.sound->extra.mute.val;
    }else{
        mute_val = bank_1_origin.sound->extra.mute.val;
    };
    
    mute_val = mute_val >>2;
    
    if ( !(mute_val & 0x01) ) {
        (get_gwidget<Gtk::Label>("label_general_op4"))->set_label(_("/* OP4 */"));
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x3A;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op4"))->set_label(_(" OP4 "));
        on_lvl_op4_event();
    };
    LOG( LOG_OUT() );
};
/* OP5 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op5_event() {
    LOG( LOG_IN() );
    uint8_t mute_val;
    
    if(!compare){
        mute_val = bank_1_modif.sound->extra.mute.val;
    }else{
        mute_val = bank_1_origin.sound->extra.mute.val;
    };
    
    mute_val = mute_val >>1;
    
    if ( !(mute_val & 0x01) ) {
        (get_gwidget<Gtk::Label>("label_general_op5"))->set_label(_("/* OP5 */"));
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x25;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op5"))->set_label(_(" OP5 "));
        on_lvl_op5_event();
    };
    LOG( LOG_OUT() );
};
/* OP6 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op6_event() {
    LOG( LOG_IN() );
    uint8_t mute_val;
    
    if(!compare){
        mute_val = bank_1_modif.sound->extra.mute.val;
    }else{
        mute_val = bank_1_origin.sound->extra.mute.val;
    };
    
    if ( !(mute_val & 0x01) ) {
        (get_gwidget<Gtk::Label>("label_general_op6"))->set_label(_("/* OP6 */"));
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x10;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op6"))->set_label(_(" OP6 "));
        on_lvl_op6_event();
    };
    LOG( LOG_OUT() );
};

/* view frequence for each operator  */
void Dx7interface::on_txt_freq_op_event()   {
    gdouble freq_val,ff,fc;
    unsigned int i ;
    for ( i=1;i<=6;i++){
        fc=(get_gwidget<Gtk::SpinButton>("freq_coarse_op"+tostr<unsigned int>(i)))->get_value();
        ff=(get_gwidget<Gtk::SpinButton>("freq_fine_op"+tostr<unsigned int>(i)))->get_value();
        if ( (get_gwidget<Gtk::DropDown>("freq_mode_op"+tostr<unsigned int>(i)))->get_selected() ) {
            (get_gwidget<Gtk::Label>("label_view_freq_op"+tostr<unsigned int>(i)))->set_label("Hz");
            // calcul termitor thanks ^^
            gdouble A = exp(log(9.772)/99);
            freq_val = pow(A,ff);
            switch( (unsigned int)(fc) & 3 ){
                case 1: freq_val *=  10;
                    break;
                case 2: freq_val *=  100;
                    break;
                case 3: freq_val *=  1000;
                    break;
            };
        }else{
            (get_gwidget<Gtk::Label>("label_view_freq_op"+tostr<unsigned int>(i)))->set_label("Rate");
            if (fc==0) {
                freq_val = 0.500+(0.005*ff);
            }else{
                freq_val = fc+((fc/100)*ff);
            };
        };
        (get_gwidget<Gtk::Entry>("entry_freq_op"+tostr<unsigned int>(i)))->set_text(tostr(freq_val));
    };
};

/* OP1 */
void Dx7interface::on_ams_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x77;
        msg[5]=(get_gwidget<Gtk::Scale>("ams_op1"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].ams.val=msg[5];
    };
    LOG( LOG_OUT() );
};

/* OP1 FREQ */
void Dx7interface::on_freq_mode_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x7A;
        msg[5]=(get_gwidget<Gtk::DropDown>("freq_mode_op1"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].freq_mode.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_freq_coarse_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x7B;
        msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op1"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].freq_coarse.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_freq_fine_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x7C;
        msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op1"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].freq_fine.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_dtun_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x7D;
        msg[5]=(get_gwidget<Gtk::Scale>("dtun_op1"))->get_value()+7;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].dtun.val=msg[5];
    };
    LOG( LOG_OUT() );
};

/* OP1 EG */
void Dx7interface::on_eg_rt1_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x69;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op1"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].eg_rt[0].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt2_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x6A;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op1"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].eg_rt[1].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt3_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x6B;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op1"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].eg_rt[2].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt4_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x6C;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op1"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].eg_rt[3].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl1_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x6D;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op1"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].eg_lvl[0].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl2_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x6E;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op1"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].eg_lvl[1].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl3_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x6F;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op1"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].eg_lvl[2].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl4_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x70;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op1"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].eg_lvl[3].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

/* OP1 FRAME VOLUME */
void Dx7interface::on_krs_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x76;
        msg[5]=(get_gwidget<Gtk::Scale>("krs_op1"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].krs.val=msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kvs_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x78;
        msg[5]=(get_gwidget<Gtk::Scale>("kvs_op1"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].kvs.val=msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_lvl_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x79;
        msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op1"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].lvl.val=msg[5];
        if ( (get_gwidget<Gtk::ToggleButton>("mute_op1"))->get_active() ) {
            (get_gwidget<Gtk::ToggleButton>("mute_op1"))->set_active(false);
        };
    };
    LOG( LOG_OUT() );
};

/* OP1 KLS */
void Dx7interface::on_kls_lft_curve_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x74;
        msg[5]=(get_gwidget<Gtk::DropDown>("kls_lft_curve_op1"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].kls.lft_curve.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op1"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_rght_curve_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x75;
        msg[5]=(get_gwidget<Gtk::DropDown>("kls_rght_curve_op1"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].kls.rght_curve.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op1"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_lft_dpth_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x72;
        msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op1"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].kls.lft_dpth.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op1"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_rght_dpth_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x73;
        msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op1"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[0].kls.rght_dpth.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op1"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_brk_pt_op1_event() {
    LOG( LOG_IN() );
    if (!compare){
        char val_note =(get_gwidget<Gtk::DropDown>("note_brk_pt_op1"))->get_selected();
        char val_octv =(get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->get_value();
        if(val_note < 3){
            val_note = val_note+12;
        };
        char val = ((val_note) + (val_octv * 12));
        if( val >= 0 && val <= 99 ){
            unsigned char msg[7];
            msg[0]=0xF0;
            msg[1]=id_fabricant;
            msg[2]=sub_status + channel_send;
            msg[3]=0x00;
            msg[4]=0x71;
            msg[5]=val;
            msg[6]=0xF7;
            send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
            bank_1_modif.sound->op[0].kls.brk_pt.val=msg[5];
            (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op1"))->queue_draw();
        };
    };
    LOG( LOG_OUT() );
};

/* OP2 */
void Dx7interface::on_ams_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x62;
        msg[5]=(get_gwidget<Gtk::Scale>("ams_op2"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].ams.val=msg[5];
    };
    LOG( LOG_OUT() );
};

/* OP2 FREQ */
void Dx7interface::on_freq_mode_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x65;
        msg[5]=(get_gwidget<Gtk::DropDown>("freq_mode_op2"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].freq_mode.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_freq_coarse_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x66;
        msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op2"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].freq_coarse.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_freq_fine_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x67;
        msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op2"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].freq_fine.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_dtun_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x68;
        msg[5]=(get_gwidget<Gtk::Scale>("dtun_op2"))->get_value()+7;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].dtun.val=msg[5];
    };
    LOG( LOG_OUT() );
};

/* OP2 EG */
void Dx7interface::on_eg_rt1_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x54;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op2"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].eg_rt[0].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt2_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x55;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op2"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].eg_rt[1].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt3_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x56;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op2"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].eg_rt[2].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt4_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x57;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op2"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].eg_rt[3].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl1_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x58;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op2"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].eg_lvl[0].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl2_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x59;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op2"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].eg_lvl[1].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl3_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x5A;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op2"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].eg_lvl[2].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl4_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x5B;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op2"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].eg_lvl[3].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

/* OP2 FRAME VOLUME */
void Dx7interface::on_krs_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x61;
        msg[5]=(get_gwidget<Gtk::Scale>("krs_op2"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].krs.val=msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kvs_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x63;
        msg[5]=(get_gwidget<Gtk::Scale>("kvs_op2"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].kvs.val=msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_lvl_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x64;
        msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op2"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].lvl.val=msg[5];
        if ( (get_gwidget<Gtk::ToggleButton>("mute_op2"))->get_active() ) {
            (get_gwidget<Gtk::ToggleButton>("mute_op2"))->set_active(false);
        };
    };
    LOG( LOG_OUT() );
};

/* OP2 KLS */
void Dx7interface::on_kls_lft_curve_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x5F;
        msg[5]=(get_gwidget<Gtk::DropDown>("kls_lft_curve_op2"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].kls.lft_curve.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op2"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_rght_curve_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x60;
        msg[5]=(get_gwidget<Gtk::DropDown>("kls_rght_curve_op2"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].kls.rght_curve.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op2"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_lft_dpth_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x5D;
        msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op2"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].kls.lft_dpth.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op2"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_rght_dpth_op2_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x5E;
        msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op2"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[1].kls.rght_dpth.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op2"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_brk_pt_op2_event() {
    if (!compare){
        char val_note =(get_gwidget<Gtk::DropDown>("note_brk_pt_op2"))->get_selected();
        char val_octv =(get_gwidget<Gtk::SpinButton>("octv_brk_pt_op2"))->get_value();
        if(val_note < 3){
            val_note = val_note+12;
        };
        char val = ((val_note) + (val_octv * 12));
        if( val >= 0 && val <= 99 ){
            unsigned char msg[7];
            msg[0]=0xF0;
            msg[1]=id_fabricant;
            msg[2]=sub_status + channel_send;
            msg[3]=0x00;
            msg[4]=0x5C;
            msg[5]=val;
            msg[6]=0xF7;
            send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
            bank_1_modif.sound->op[1].kls.brk_pt.val=msg[5];
            (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op2"))->queue_draw();
        };
    };
};


/* OP3 */
void Dx7interface::on_ams_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x4D;
        msg[5]=(get_gwidget<Gtk::Scale>("ams_op3"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].ams.val=msg[5];
    };
    LOG( LOG_OUT() );
};

/* OP3 FREQ */
void Dx7interface::on_freq_mode_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x50;
        msg[5]=(get_gwidget<Gtk::DropDown>("freq_mode_op3"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].freq_mode.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_freq_coarse_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x51;
        msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op3"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].freq_coarse.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_freq_fine_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x52;
        msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op3"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].freq_fine.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_dtun_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x53;
        msg[5]=(get_gwidget<Gtk::Scale>("dtun_op3"))->get_value()+7;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].dtun.val=msg[5];
    };
    LOG( LOG_OUT() );
};

/* OP3 EG */
void Dx7interface::on_eg_rt1_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x3F;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op3"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].eg_rt[0].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt2_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x40;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op3"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].eg_rt[1].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt3_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x41;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op3"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].eg_rt[2].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt4_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x42;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op3"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].eg_rt[3].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl1_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x43;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op3"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].eg_lvl[0].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl2_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x44;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op3"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].eg_lvl[1].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl3_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x45;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op3"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].eg_lvl[2].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl4_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x46;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op3"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].eg_lvl[3].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

/* OP3 FRAME VOLUME */
void Dx7interface::on_krs_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x4C;
        msg[5]=(get_gwidget<Gtk::Scale>("krs_op3"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].krs.val=msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kvs_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x4E;
        msg[5]=(get_gwidget<Gtk::Scale>("kvs_op3"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].kvs.val=msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_lvl_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x4F;
        msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op3"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].lvl.val=msg[5];
        if ( (get_gwidget<Gtk::ToggleButton>("mute_op3"))->get_active() ) {
            (get_gwidget<Gtk::ToggleButton>("mute_op3"))->set_active(false);
        };
    };
    LOG( LOG_OUT() );
};


/* OP3 KLS */
void Dx7interface::on_kls_lft_curve_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x4A;
        msg[5]=(get_gwidget<Gtk::DropDown>("kls_lft_curve_op3"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].kls.lft_curve.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op3"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_rght_curve_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x4B;
        msg[5]=(get_gwidget<Gtk::DropDown>("kls_rght_curve_op3"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].kls.rght_curve.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op3"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_lft_dpth_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x48;
        msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op3"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].kls.lft_dpth.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op3"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_rght_dpth_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x49;
        msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op3"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[2].kls.rght_dpth.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op3"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_brk_pt_op3_event() {
    LOG( LOG_IN() );
    if (!compare){
        char val_note =(get_gwidget<Gtk::DropDown>("note_brk_pt_op3"))->get_selected();
        char val_octv =(get_gwidget<Gtk::SpinButton>("octv_brk_pt_op3"))->get_value();
        if(val_note < 3){
            val_note = val_note+12;
        };
        char val = ((val_note) + (val_octv * 12));
        if( val >= 0 && val <= 99 ){
            unsigned char msg[7];
            msg[0]=0xF0;
            msg[1]=id_fabricant;
            msg[2]=sub_status + channel_send;
            msg[3]=0x00;
            msg[4]=0x47;
            msg[5]=val;
            msg[6]=0xF7;
            send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
            bank_1_modif.sound->op[2].kls.brk_pt.val=msg[5];
            (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op3"))->queue_draw();
        };
    };
};


/* OP4 */
void Dx7interface::on_ams_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x38;
        msg[5]=(get_gwidget<Gtk::Scale>("ams_op4"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].ams.val=msg[5];
    };
    LOG( LOG_OUT() );
};

/* OP4 FREQ*/
void Dx7interface::on_freq_mode_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x3B;
        msg[5]=(get_gwidget<Gtk::DropDown>("freq_mode_op4"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].freq_mode.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_freq_coarse_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x3C;
        msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op4"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].freq_coarse.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_freq_fine_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x3D;
        msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op4"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].freq_fine.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_dtun_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x3E;
        msg[5]=(get_gwidget<Gtk::Scale>("dtun_op4"))->get_value()+7;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].dtun.val=msg[5];
    };
    LOG( LOG_OUT() );
};

/* OP4 EG*/
void Dx7interface::on_eg_rt1_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x2A;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op4"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].eg_rt[0].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt2_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x2B;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op4"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].eg_rt[1].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt3_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x2C;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op4"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].eg_rt[2].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt4_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x2D;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op4"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].eg_rt[3].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl1_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x2E;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op4"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].eg_lvl[0].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl2_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x2F;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op4"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].eg_lvl[1].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl3_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x30;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op4"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].eg_lvl[2].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl4_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x31;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op4"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].eg_lvl[3].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    };
    LOG( LOG_OUT() );
};


/* OP4 FRAME VOLUME */
void Dx7interface::on_krs_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x37;
        msg[5]=(get_gwidget<Gtk::Scale>("krs_op4"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].krs.val=msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kvs_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
    unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x39;
        msg[5]=(get_gwidget<Gtk::Scale>("kvs_op4"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].kvs.val=msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_lvl_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x3A;
        msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op4"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].lvl.val=msg[5];
        if ( (get_gwidget<Gtk::ToggleButton>("mute_op4"))->get_active() ) {
            (get_gwidget<Gtk::ToggleButton>("mute_op4"))->set_active(false);
        };
    };
    LOG( LOG_OUT() );
};

/* OP4 KLS*/
void Dx7interface::on_kls_lft_curve_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x35;
        msg[5]=(get_gwidget<Gtk::DropDown>("kls_lft_curve_op4"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].kls.lft_curve.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op4"))->queue_draw();   
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_rght_curve_op4_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x36;
        msg[5]=(get_gwidget<Gtk::DropDown>("kls_rght_curve_op4"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].kls.rght_curve.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op4"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_lft_dpth_op4_event() {
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x33;
        msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op4"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].kls.lft_dpth.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op4"))->queue_draw();
    };
};

void Dx7interface::on_kls_rght_dpth_op4_event() {
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x34;
        msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op4"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[3].kls.rght_dpth.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op4"))->queue_draw();
    };
};

void Dx7interface::on_kls_brk_pt_op4_event(){
    if (!compare){
        char val_note =(get_gwidget<Gtk::DropDown>("note_brk_pt_op4"))->get_selected();
        char val_octv =(get_gwidget<Gtk::SpinButton>("octv_brk_pt_op4"))->get_value();
        if(val_note < 3){
            val_note = val_note+12;
        };
        char val = ((val_note) + (val_octv * 12));
        if( val >= 0 && val <= 99 ){
            unsigned char msg[7];
            msg[0]=0xF0;
            msg[1]=id_fabricant;
            msg[2]=sub_status + channel_send;
            msg[3]=0x00;
            msg[4]=0x32;
            msg[5]=val;
            msg[6]=0xF7;
            send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
            bank_1_modif.sound->op[3].kls.brk_pt.val=msg[5];
            (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op4"))->queue_draw();
        };
    };
};

/* OP5 */
void Dx7interface::on_ams_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x23;
        msg[5]=(get_gwidget<Gtk::Scale>("ams_op5"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].ams.val=msg[5];
    };
    LOG( LOG_OUT() );
};

/* OP5 FREQ */
void Dx7interface::on_freq_mode_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x26;
        msg[5]=(get_gwidget<Gtk::DropDown>("freq_mode_op5"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].freq_mode.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_freq_coarse_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x27;
        msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op5"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].freq_coarse.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_freq_fine_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x28;
        msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op5"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].freq_fine.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_dtun_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x29;
        msg[5]=(get_gwidget<Gtk::Scale>("dtun_op5"))->get_value()+7;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].dtun.val=msg[5];
    };
    LOG( LOG_OUT() );
};

/* OP5 EG */
void Dx7interface::on_eg_rt1_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x15;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op5"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].eg_rt[0].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt2_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x16;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op5"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].eg_rt[1].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt3_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x17;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op5"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].eg_rt[2].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt4_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x18;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op5"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].eg_rt[3].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl1_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
    unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x19;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op5"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].eg_lvl[0].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl2_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
    unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x1A;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op5"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].eg_lvl[1].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl3_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x1B;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op5"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].eg_lvl[2].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl4_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x1C;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op5"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].eg_lvl[3].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

/* OP5 FRAME VOLUME */
void Dx7interface::on_krs_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x22;
        msg[5]=(get_gwidget<Gtk::Scale>("krs_op5"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].krs.val=msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kvs_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x24;
        msg[5]=(get_gwidget<Gtk::Scale>("kvs_op5"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].kvs.val=msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_lvl_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x25;
        msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op5"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].lvl.val=msg[5];
        if ( (get_gwidget<Gtk::ToggleButton>("mute_op5"))->get_active() ) {
            (get_gwidget<Gtk::ToggleButton>("mute_op5"))->set_active(false);
        };
    };
    LOG( LOG_OUT() );
};

/* OP5 KLS */
void Dx7interface::on_kls_lft_curve_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x20;
        msg[5]=(get_gwidget<Gtk::DropDown>("kls_lft_curve_op5"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].kls.lft_curve.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op5"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_rght_curve_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x21;
        msg[5]=(get_gwidget<Gtk::DropDown>("kls_rght_curve_op5"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].kls.rght_curve.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op5"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_lft_dpth_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x1E;
        msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op5"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].kls.lft_dpth.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op5"))->queue_draw();   
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_rght_dpth_op5_event(){
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x1F;
        msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op5"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[4].kls.rght_dpth.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op5"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_brk_pt_op5_event() {
    LOG( LOG_IN() );
    if (!compare){
        char val_note =(get_gwidget<Gtk::DropDown>("note_brk_pt_op5"))->get_selected();
        char val_octv =(get_gwidget<Gtk::SpinButton>("octv_brk_pt_op5"))->get_value();
        if(val_note < 3){
            val_note = val_note+12;
        };
        char val = ((val_note) + (val_octv * 12));
        if( val >= 0 && val <= 99 ){
            unsigned char msg[7];
            msg[0]=0xF0;
            msg[1]=id_fabricant;
            msg[2]=sub_status + channel_send;
            msg[3]=0x00;
            msg[4]=0x1D;
            msg[5]=val;
            msg[6]=0xF7;
            send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
            bank_1_modif.sound->op[4].kls.brk_pt.val=msg[5];
            (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op5"))->queue_draw();
        };
    };
    LOG( LOG_OUT() );
};

/* OP6 */
void Dx7interface::on_ams_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x0E;
        msg[5]=(get_gwidget<Gtk::Scale>("ams_op6"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].ams.val=msg[5];
    };
    LOG( LOG_OUT() );
};

/* OP6 FREQ */
void Dx7interface::on_freq_mode_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x11;
        msg[5]=(get_gwidget<Gtk::DropDown>("freq_mode_op6"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].freq_mode.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_freq_coarse_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x12;
        msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op6"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].freq_coarse.val=msg[5];
        on_txt_freq_op_event(); 
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_freq_fine_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x13;
        msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op6"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].freq_fine.val=msg[5];
        on_txt_freq_op_event();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_dtun_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x14;
        msg[5]=(get_gwidget<Gtk::Scale>("dtun_op6"))->get_value()+7;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].dtun.val=msg[5];
    };
    LOG( LOG_OUT() );
};

/* OP6 EG */
void Dx7interface::on_eg_rt1_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x00;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op6"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].eg_rt[0].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt2_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x01;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op6"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].eg_rt[1].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt3_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x02;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op6"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].eg_rt[2].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_rt4_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x03;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op6"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].eg_rt[3].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl1_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x04;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op6"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].eg_lvl[0].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl2_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x05;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op6"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].eg_lvl[1].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl3_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x06;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op6"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].eg_lvl[2].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_eg_lvl4_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x07;
        msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op6"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].eg_lvl[3].val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

/* OP6 FRAME VOLUME */
void Dx7interface::on_krs_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x0D;
        msg[5]=(get_gwidget<Gtk::Scale>("krs_op6"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].krs.val=msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kvs_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x0F;
        msg[5]=(get_gwidget<Gtk::Scale>("kvs_op6"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].kvs.val=msg[5];
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_lvl_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x10;
        msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op6"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].lvl.val=msg[5]; 
        if ( (get_gwidget<Gtk::ToggleButton>("mute_op6"))->get_active() ) {
            (get_gwidget<Gtk::ToggleButton>("mute_op6"))->set_active(false);
        };
    };
    LOG( LOG_OUT() );
};

/* OP6 KLS */
void Dx7interface::on_kls_lft_curve_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x0B;
        msg[5]=(get_gwidget<Gtk::DropDown>("kls_lft_curve_op6"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].kls.lft_curve.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op6"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_rght_curve_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x0C;
        msg[5]=(get_gwidget<Gtk::DropDown>("kls_rght_curve_op6"))->get_selected();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].kls.rght_curve.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op6"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_lft_dpth_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x09;
        msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op6"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].kls.lft_dpth.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op6"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_rght_dpth_op6_event() {
    LOG( LOG_IN() );
    if (!compare){
        unsigned char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status + channel_send;
        msg[3]=0x00;
        msg[4]=0x0A;
        msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op6"))->get_value();
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        bank_1_modif.sound->op[5].kls.rght_dpth.val=msg[5];
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op6"))->queue_draw();
    };
    LOG( LOG_OUT() );
};

void Dx7interface::on_kls_brk_pt_op6_event() {
    if (!compare){
        char val_note =(get_gwidget<Gtk::DropDown>("note_brk_pt_op6"))->get_selected();
        char val_octv =(get_gwidget<Gtk::SpinButton>("octv_brk_pt_op6"))->get_value();
        if(val_note < 3){
            val_note = val_note+12;
        };
        char val = ((val_note) + (val_octv * 12));
        if( val >= 0 && val <= 99 ){
            unsigned char msg[7];
            msg[0]=0xF0;
            msg[1]=id_fabricant;
            msg[2]=sub_status + channel_send;
            msg[3]=0x00;
            msg[4]=0x08;
            msg[5]=val;
            msg[6]=0xF7;
           	send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
            bank_1_modif.sound->op[5].kls.brk_pt.val=msg[5];
            (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op6"))->queue_draw();
        };
    };
};

/** UI EVENTs **/
void Dx7interface::set_aftrtch_assgn_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->extra.functions.aftrtch_assgn.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->extra.functions.aftrtch_assgn.max){
        value=bank_1_modif.sound->extra.functions.aftrtch_assgn.max;
    };
    bank_1_modif.sound->extra.functions.aftrtch_assgn.val=(int)value;
    /*int val = value;
    (get_gwidget<Gtk::CheckButton>("aftrtch_ptch"))->set_active(val & 0x01);
    (get_gwidget<Gtk::CheckButton>("aftrtch_mp"))->set_active((val & 0x02)>>1);
    (get_gwidget<Gtk::CheckButton>("aftrtch_gbs"))->set_active((val & 0x04)>>2);
    */
};
void Dx7interface::set_aftrtch_rng_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->extra.functions.aftrtch_rng.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->extra.functions.aftrtch_rng.max){
        value=bank_1_modif.sound->extra.functions.aftrtch_rng.max;
    };
    bank_1_modif.sound->extra.functions.aftrtch_rng.val=(int)value;
};
void Dx7interface::set_algo_event(int value){
    //LOG( LOG_IN() );
    value = (double)value/(127.0/(double)bank_1_modif.sound->algo.algo.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->algo.algo.max){
        value=bank_1_modif.sound->algo.algo.max;
    };
    bank_1_modif.sound->algo.algo.val=(int)value;
    //LOG( LOG_OUT() );
};
void Dx7interface::set_ams_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].ams.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].ams.max){
        value=bank_1_modif.sound->op[0].ams.max;
    };
    bank_1_modif.sound->op[0].ams.val = (int)value;
};

void Dx7interface::set_ams_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].ams.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].ams.max){
        value=bank_1_modif.sound->op[1].ams.max;
    };
    bank_1_modif.sound->op[1].ams.val=(int)value;
};
void Dx7interface::set_ams_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].ams.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].ams.max){
        value=bank_1_modif.sound->op[2].ams.max;
    };
    bank_1_modif.sound->op[2].ams.val=(int)value;
};
void Dx7interface::set_ams_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].ams.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].ams.max){
        value=bank_1_modif.sound->op[3].ams.max;
    };
    bank_1_modif.sound->op[3].ams.val=(int)value;
};
void Dx7interface::set_ams_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].ams.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].ams.max){
        value=bank_1_modif.sound->op[4].ams.max;
    };
    bank_1_modif.sound->op[4].ams.val=(int)value;
};
void Dx7interface::set_ams_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].ams.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].ams.max){
        value=bank_1_modif.sound->op[5].ams.max;
    };
    bank_1_modif.sound->op[5].ams.val=(int)value;
};
void Dx7interface::set_brth_assgn_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->extra.functions.brth_assgn.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->extra.functions.brth_assgn.max){
        value=bank_1_modif.sound->extra.functions.brth_assgn.max;
    };
    bank_1_modif.sound->extra.functions.brth_assgn.val=(int)value;
    /*int val = value;
    (get_gwidget<Gtk::CheckButton>("brth_ptch"))->set_active(val & 0x01);
    (get_gwidget<Gtk::CheckButton>("brth_mp"))->set_active((val & 0x02)>>1);
    (get_gwidget<Gtk::CheckButton>("brth_gbs"))->set_active((val & 0x04)>>2);
    */
};
void Dx7interface::set_brth_rng_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->extra.functions.brth_rng.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->extra.functions.brth_rng.max){
        value=bank_1_modif.sound->extra.functions.brth_rng.max;
    };
    bank_1_modif.sound->extra.functions.brth_rng.val=(int)value;
};
void Dx7interface::set_compare_event(int value){
    //value = (double)value/(127.0/(double)bank_1_modif.sound->;
};

void Dx7interface::set_dtun_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].dtun.max);
    if( value < 0 ){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].dtun.max){
        value=bank_1_modif.sound->op[0].dtun.max;
    };
    bank_1_modif.sound->op[0].dtun.val=(int)value;
};
void Dx7interface::set_dtun_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].dtun.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].dtun.max){
        value=bank_1_modif.sound->op[1].dtun.max;
    };
    bank_1_modif.sound->op[1].dtun.val=(int)value;
};
void Dx7interface::set_dtun_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].dtun.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].dtun.max){
        value=bank_1_modif.sound->op[2].dtun.max;
    };
    bank_1_modif.sound->op[2].dtun.val=(int)value;
};
void Dx7interface::set_dtun_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].dtun.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].dtun.max){
        value=bank_1_modif.sound->op[3].dtun.max;
    };
    bank_1_modif.sound->op[3].dtun.val=(int)value;
};
void Dx7interface::set_dtun_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].dtun.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].dtun.max){
        value=bank_1_modif.sound->op[4].dtun.max;
    };
    bank_1_modif.sound->op[4].dtun.val=(int)value;
};
void Dx7interface::set_dtun_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].dtun.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].dtun.max){
        value=bank_1_modif.sound->op[5].dtun.max;
    };
    bank_1_modif.sound->op[5].dtun.val=(int)value;
};
void Dx7interface::set_eg_lvl1_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].eg_lvl[0].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].eg_lvl[0].max){
        value=bank_1_modif.sound->op[0].eg_lvl[0].max;
    };
    bank_1_modif.sound->op[0].eg_lvl[0].val=(int)value;
};
void Dx7interface::set_eg_lvl1_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].eg_lvl[0].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].eg_lvl[0].max){
        value=bank_1_modif.sound->op[1].eg_lvl[0].max;
    };
    bank_1_modif.sound->op[1].eg_lvl[0].val=(int)value;
};
void Dx7interface::set_eg_lvl1_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].eg_lvl[0].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].eg_lvl[0].max){
        value=bank_1_modif.sound->op[2].eg_lvl[0].max;
    };
    bank_1_modif.sound->op[2].eg_lvl[0].val=(int)value;
};
void Dx7interface::set_eg_lvl1_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].eg_lvl[0].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].eg_lvl[0].max){
        value=bank_1_modif.sound->op[3].eg_lvl[0].max;
    };
    bank_1_modif.sound->op[3].eg_lvl[0].val=(int)value;
};
void Dx7interface::set_eg_lvl1_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].eg_lvl[0].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].eg_lvl[0].max){
        value=bank_1_modif.sound->op[4].eg_lvl[0].max;
    };
    bank_1_modif.sound->op[4].eg_lvl[0].val=(int)value;
};
void Dx7interface::set_eg_lvl1_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].eg_lvl[0].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].eg_lvl[0].max){
        value=bank_1_modif.sound->op[5].eg_lvl[0].max;
    };
    bank_1_modif.sound->op[5].eg_lvl[0].val=(int)value;
};
void Dx7interface::set_eg_lvl2_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].eg_lvl[1].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].eg_lvl[1].max){
        value=bank_1_modif.sound->op[0].eg_lvl[1].max;
    };
    bank_1_modif.sound->op[0].eg_lvl[1].val=(int)value;
};
void Dx7interface::set_eg_lvl2_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].eg_lvl[1].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].eg_lvl[1].max){
        value=bank_1_modif.sound->op[1].eg_lvl[1].max;
    };
    bank_1_modif.sound->op[1].eg_lvl[1].val=(int)value;
};
void Dx7interface::set_eg_lvl2_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].eg_lvl[1].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].eg_lvl[1].max){
        value=bank_1_modif.sound->op[2].eg_lvl[1].max;
    };
    bank_1_modif.sound->op[2].eg_lvl[1].val=(int)value;
};
void Dx7interface::set_eg_lvl2_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].eg_lvl[1].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].eg_lvl[1].max){
        value=bank_1_modif.sound->op[3].eg_lvl[1].max;
    };
    bank_1_modif.sound->op[3].eg_lvl[1].val=(int)value;
};
void Dx7interface::set_eg_lvl2_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].eg_lvl[1].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].eg_lvl[1].max){
        value=bank_1_modif.sound->op[4].eg_lvl[1].max;
    };
    bank_1_modif.sound->op[4].eg_lvl[1].val=(int)value;
};
void Dx7interface::set_eg_lvl2_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].eg_lvl[1].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].eg_lvl[1].max){
        value=bank_1_modif.sound->op[5].eg_lvl[1].max;
    };
    bank_1_modif.sound->op[5].eg_lvl[1].val=(int)value;
};
void Dx7interface::set_eg_lvl3_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].eg_lvl[2].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].eg_lvl[2].max){
        value=bank_1_modif.sound->op[0].eg_lvl[2].max;
    };
    bank_1_modif.sound->op[0].eg_lvl[2].val=(int)value;
};
void Dx7interface::set_eg_lvl3_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].eg_lvl[2].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].eg_lvl[2].max){
        value=bank_1_modif.sound->op[1].eg_lvl[2].max;
    };
    bank_1_modif.sound->op[1].eg_lvl[2].val=(int)value;
};
void Dx7interface::set_eg_lvl3_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].eg_lvl[2].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].eg_lvl[2].max){
        value=bank_1_modif.sound->op[2].eg_lvl[2].max;
    };
    bank_1_modif.sound->op[2].eg_lvl[2].val=(int)value;
};
void Dx7interface::set_eg_lvl3_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].eg_lvl[2].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].eg_lvl[2].max){
        value=bank_1_modif.sound->op[3].eg_lvl[2].max;
    };
    bank_1_modif.sound->op[3].eg_lvl[2].val=(int)value;
};
void Dx7interface::set_eg_lvl3_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].eg_lvl[2].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].eg_lvl[2].max){
        value=bank_1_modif.sound->op[4].eg_lvl[2].max;
    };
    bank_1_modif.sound->op[4].eg_lvl[2].val=(int)value;
};
void Dx7interface::set_eg_lvl3_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].eg_lvl[2].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].eg_lvl[2].max){
        value=bank_1_modif.sound->op[5].eg_lvl[2].max;
    };
    bank_1_modif.sound->op[5].eg_lvl[2].val=(int)value;
};
void Dx7interface::set_eg_lvl4_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].eg_lvl[3].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].eg_lvl[3].max){
        value=bank_1_modif.sound->op[0].eg_lvl[3].max;
    };
    bank_1_modif.sound->op[0].eg_lvl[3].val=(int)value;
};
void Dx7interface::set_eg_lvl4_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].eg_lvl[3].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].eg_lvl[3].max){
        value=bank_1_modif.sound->op[1].eg_lvl[3].max;
    };
    bank_1_modif.sound->op[1].eg_lvl[3].val=(int)value;
};
void Dx7interface::set_eg_lvl4_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].eg_lvl[3].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].eg_lvl[3].max){
        value=bank_1_modif.sound->op[2].eg_lvl[3].max;
    };
    bank_1_modif.sound->op[2].eg_lvl[3].val=(int)value;
};
void Dx7interface::set_eg_lvl4_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].eg_lvl[3].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].eg_lvl[3].max){
        value=bank_1_modif.sound->op[3].eg_lvl[3].max;
    };
    bank_1_modif.sound->op[3].eg_lvl[3].val=(int)value;
};
void Dx7interface::set_eg_lvl4_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].eg_lvl[3].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].eg_lvl[3].max){
        value=bank_1_modif.sound->op[4].eg_lvl[3].max;
    };
    bank_1_modif.sound->op[4].eg_lvl[3].val=(int)value;
};
void Dx7interface::set_eg_lvl4_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].eg_lvl[3].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].eg_lvl[3].max){
        value=bank_1_modif.sound->op[5].eg_lvl[3].max;
    };
    bank_1_modif.sound->op[5].eg_lvl[3].val=(int)value;
};
void Dx7interface::set_eg_rt1_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].eg_rt[0].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].eg_rt[0].max){
        value=bank_1_modif.sound->op[0].eg_rt[0].max;
    };
    bank_1_modif.sound->op[0].eg_rt[0].val=(int)value;
};
void Dx7interface::set_eg_rt1_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].eg_rt[0].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].eg_rt[0].max){
        value=bank_1_modif.sound->op[1].eg_rt[0].max;
    };
    bank_1_modif.sound->op[1].eg_rt[0].val=(int)value;
};
void Dx7interface::set_eg_rt1_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].eg_rt[0].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].eg_rt[0].max){
        value=bank_1_modif.sound->op[2].eg_rt[0].max;
    };
    bank_1_modif.sound->op[2].eg_rt[0].val=(int)value;
};
void Dx7interface::set_eg_rt1_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].eg_rt[0].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].eg_rt[0].max){
        value=bank_1_modif.sound->op[3].eg_rt[0].max;
    };
    bank_1_modif.sound->op[3].eg_rt[0].val=(int)value;
};
void Dx7interface::set_eg_rt1_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].eg_rt[0].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].eg_rt[0].max){
        value=bank_1_modif.sound->op[4].eg_rt[0].max;
    };
    bank_1_modif.sound->op[4].eg_rt[0].val=(int)value;
};
void Dx7interface::set_eg_rt1_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].eg_rt[0].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].eg_rt[0].max){
        value=bank_1_modif.sound->op[5].eg_rt[0].max;
    };
    bank_1_modif.sound->op[5].eg_rt[0].val=(int)value;
};
void Dx7interface::set_eg_rt2_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].eg_rt[1].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].eg_rt[1].max){
        value=bank_1_modif.sound->op[0].eg_rt[1].max;
    };
    bank_1_modif.sound->op[0].eg_rt[1].val=(int)value;
};
void Dx7interface::set_eg_rt2_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].eg_rt[1].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].eg_rt[1].max){
        value=bank_1_modif.sound->op[1].eg_rt[1].max;
    };
    bank_1_modif.sound->op[1].eg_rt[1].val=(int)value;
};
void Dx7interface::set_eg_rt2_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].eg_rt[1].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].eg_rt[1].max){
        value=bank_1_modif.sound->op[2].eg_rt[1].max;
    };
    bank_1_modif.sound->op[2].eg_rt[1].val=(int)value;
};
void Dx7interface::set_eg_rt2_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].eg_rt[1].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].eg_rt[1].max){
        value=bank_1_modif.sound->op[3].eg_rt[1].max;
    };
    bank_1_modif.sound->op[3].eg_rt[1].val=(int)value;
};
void Dx7interface::set_eg_rt2_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].eg_rt[1].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].eg_rt[1].max){
        value=bank_1_modif.sound->op[4].eg_rt[1].max;
    };
    bank_1_modif.sound->op[4].eg_rt[1].val=(int)value;
};
void Dx7interface::set_eg_rt2_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].eg_rt[1].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].eg_rt[1].max){
        value=bank_1_modif.sound->op[5].eg_rt[1].max;
    };
    bank_1_modif.sound->op[5].eg_rt[1].val=(int)value;
};
void Dx7interface::set_eg_rt3_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].eg_rt[2].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].eg_rt[2].max){
        value=bank_1_modif.sound->op[0].eg_rt[2].max;
    };
    bank_1_modif.sound->op[0].eg_rt[2].val=(int)value;
};
void Dx7interface::set_eg_rt3_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].eg_rt[2].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].eg_rt[2].max){
        value=bank_1_modif.sound->op[1].eg_rt[2].max;
    };
    bank_1_modif.sound->op[1].eg_rt[2].val=(int)value;
};
void Dx7interface::set_eg_rt3_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].eg_rt[2].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].eg_rt[2].max){
        value=bank_1_modif.sound->op[2].eg_rt[2].max;
    };
    bank_1_modif.sound->op[2].eg_rt[2].val=(int)value;
};
void Dx7interface::set_eg_rt3_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].eg_rt[2].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].eg_rt[2].max){
        value=bank_1_modif.sound->op[3].eg_rt[2].max;
    };
    bank_1_modif.sound->op[3].eg_rt[2].val=(int)value;
};
void Dx7interface::set_eg_rt3_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].eg_rt[2].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].eg_rt[2].max){
        value=bank_1_modif.sound->op[4].eg_rt[2].max;
    };
    bank_1_modif.sound->op[4].eg_rt[2].val=(int)value;
};
void Dx7interface::set_eg_rt3_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].eg_rt[2].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].eg_rt[2].max){
        value=bank_1_modif.sound->op[5].eg_rt[2].max;
    };
    bank_1_modif.sound->op[5].eg_rt[2].val=(int)value;
};
void Dx7interface::set_eg_rt4_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].eg_rt[3].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].eg_rt[3].max){
        value=bank_1_modif.sound->op[0].eg_rt[3].max;
    };
    bank_1_modif.sound->op[0].eg_rt[3].val=(int)value;
};
void Dx7interface::set_eg_rt4_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].eg_rt[3].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].eg_rt[3].max){
        value=bank_1_modif.sound->op[1].eg_rt[3].max;
    };
    bank_1_modif.sound->op[1].eg_rt[3].val=(int)value;
};
void Dx7interface::set_eg_rt4_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].eg_rt[3].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].eg_rt[3].max){
        value=bank_1_modif.sound->op[2].eg_rt[3].max;
    };
    bank_1_modif.sound->op[2].eg_rt[3].val=(int)value;
};
void Dx7interface::set_eg_rt4_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].eg_rt[3].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].eg_rt[3].max){
        value=bank_1_modif.sound->op[3].eg_rt[3].max;
    };
    bank_1_modif.sound->op[3].eg_rt[3].val=(int)value;
};
void Dx7interface::set_eg_rt4_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].eg_rt[3].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].eg_rt[3].max){
        value=bank_1_modif.sound->op[4].eg_rt[3].max;
    };
    bank_1_modif.sound->op[4].eg_rt[3].val=(int)value;
};
void Dx7interface::set_eg_rt4_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].eg_rt[3].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].eg_rt[3].max){
        value=bank_1_modif.sound->op[5].eg_rt[3].max;
    };
    bank_1_modif.sound->op[5].eg_rt[3].val=(int)value;
};
void Dx7interface::set_feedback_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->algo.feedback.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->algo.feedback.max){
        value=bank_1_modif.sound->algo.feedback.max;
    };
    bank_1_modif.sound->algo.feedback.val=(int)value;
};
void Dx7interface::set_foot_assgn_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->extra.functions.foot_assgn.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->extra.functions.foot_assgn.max){
        value=bank_1_modif.sound->extra.functions.foot_assgn.max;
    };
    bank_1_modif.sound->extra.functions.foot_assgn.val=(int)value;
    /*int val = value;
    (get_gwidget<Gtk::CheckButton>("foot_ptch"))->set_active(val & 0x01);
    (get_gwidget<Gtk::CheckButton>("foot_mp"))->set_active((val & 0x02)>>1);
    (get_gwidget<Gtk::CheckButton>("foot_gbs"))->set_active((val & 0x04)>>2);*/
};
void Dx7interface::set_foot_rng_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->extra.functions.foot_rng.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->extra.functions.foot_rng.max){
        value=bank_1_modif.sound->extra.functions.foot_rng.max;
    };
    bank_1_modif.sound->extra.functions.foot_rng.val=(int)value;
};
void Dx7interface::set_freq_coarse_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].freq_coarse.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].freq_coarse.max){
        value=bank_1_modif.sound->op[0].freq_coarse.max;
    };
    bank_1_modif.sound->op[0].freq_coarse.val=(int)value;
};
void Dx7interface::set_freq_coarse_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].freq_coarse.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].freq_coarse.max){
        value=bank_1_modif.sound->op[1].freq_coarse.max;
    };
    bank_1_modif.sound->op[1].freq_coarse.val=(int)value;
};
void Dx7interface::set_freq_coarse_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].freq_coarse.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].freq_coarse.max){
        value=bank_1_modif.sound->op[2].freq_coarse.max;
    };
    bank_1_modif.sound->op[2].freq_coarse.val=(int)value;
};
void Dx7interface::set_freq_coarse_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].freq_coarse.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].freq_coarse.max){
        value=bank_1_modif.sound->op[3].freq_coarse.max;
    };
    bank_1_modif.sound->op[3].freq_coarse.val=(int)value;
};
void Dx7interface::set_freq_coarse_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].freq_coarse.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].freq_coarse.max){
        value=bank_1_modif.sound->op[4].freq_coarse.max;
    };
    bank_1_modif.sound->op[4].freq_coarse.val=(int)value;
};
void Dx7interface::set_freq_coarse_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].freq_coarse.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].freq_coarse.max){
        value=bank_1_modif.sound->op[5].freq_coarse.max;
    };
    bank_1_modif.sound->op[5].freq_coarse.val=(int)value;
};
void Dx7interface::set_freq_fine_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].freq_fine.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].freq_fine.max){
        value=bank_1_modif.sound->op[0].freq_fine.max;
    };
    bank_1_modif.sound->op[0].freq_fine.val=(int)value;
};
void Dx7interface::set_freq_fine_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].freq_fine.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].freq_fine.max){
        value=bank_1_modif.sound->op[1].freq_fine.max;
    };
    bank_1_modif.sound->op[1].freq_fine.val=(int)value;
};
void Dx7interface::set_freq_fine_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].freq_fine.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].freq_fine.max){
        value=bank_1_modif.sound->op[2].freq_fine.max;
    };
    bank_1_modif.sound->op[2].freq_fine.val=(int)value;
};
void Dx7interface::set_freq_fine_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].freq_fine.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].freq_fine.max){
        value=bank_1_modif.sound->op[3].freq_fine.max;
    };
    bank_1_modif.sound->op[3].freq_fine.val=(int)value;
};
void Dx7interface::set_freq_fine_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].freq_fine.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].freq_fine.max){
        value=bank_1_modif.sound->op[4].freq_fine.max;
    };
    bank_1_modif.sound->op[4].freq_fine.val=(int)value;
};
void Dx7interface::set_freq_fine_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].freq_fine.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].freq_fine.max){
        value=bank_1_modif.sound->op[5].freq_fine.max;
    };
    bank_1_modif.sound->op[5].freq_fine.val=(int)value;
};
void Dx7interface::set_freq_mode_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].freq_mode.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].freq_mode.max){
        value=bank_1_modif.sound->op[0].freq_mode.max;
    };
    bank_1_modif.sound->op[0].freq_mode.val=(int)value;
};
void Dx7interface::set_freq_mode_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].freq_mode.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].freq_mode.max){
        value=bank_1_modif.sound->op[1].freq_mode.max;
    };
    bank_1_modif.sound->op[1].freq_mode.val=(int)value;
};
void Dx7interface::set_freq_mode_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].freq_mode.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].freq_mode.max){
        value=bank_1_modif.sound->op[2].freq_mode.max;
    };
    bank_1_modif.sound->op[2].freq_mode.val=(int)value;
};
void Dx7interface::set_freq_mode_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].freq_mode.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].freq_mode.max){
        value=bank_1_modif.sound->op[3].freq_mode.max;
    };
    bank_1_modif.sound->op[3].freq_mode.val=(int)value;
};
void Dx7interface::set_freq_mode_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].freq_mode.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].freq_mode.max){
        value=bank_1_modif.sound->op[4].freq_mode.max;
    };
    bank_1_modif.sound->op[4].freq_mode.val=(int)value;;
};
void Dx7interface::set_freq_mode_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].freq_mode.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].freq_mode.max){
        value=bank_1_modif.sound->op[5].freq_mode.max;
    };
    bank_1_modif.sound->op[5].freq_mode.val=(int)value;
};
void Dx7interface::set_kls_brk_pt_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].kls.brk_pt.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].kls.brk_pt.max){
        value=bank_1_modif.sound->op[0].kls.brk_pt.max;
    };
    bank_1_modif.sound->op[0].kls.brk_pt.val=(int)value;
    /*
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op1"))->set_selected(value % 12);
    int val = (value -3);
    if(val < 0){
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->set_value( -1 );
    }else{
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->set_value( val / 12 );
    };*/
};
void Dx7interface::set_kls_brk_pt_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].kls.brk_pt.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].kls.brk_pt.max){
        value=bank_1_modif.sound->op[1].kls.brk_pt.max;
    };
    bank_1_modif.sound->op[1].kls.brk_pt.val=(int)value;
    /*(get_gwidget<Gtk::DropDown>("note_brk_pt_op2"))->set_selected(value % 12);
    int val = (value -3);
    if(val < 0){
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op2"))->set_value( -1 );
    }else{
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op2"))->set_value( val / 12 );
    };*/
};
void Dx7interface::set_kls_brk_pt_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].kls.brk_pt.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].kls.brk_pt.max){
        value=bank_1_modif.sound->op[2].kls.brk_pt.max;
    };
    bank_1_modif.sound->op[2].kls.brk_pt.val=(int)value;
    /*(get_gwidget<Gtk::DropDown>("note_brk_pt_op3"))->set_selected(value % 12);
    int val = (value -3);
    if(val < 0){
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op3"))->set_value( -1 );
    }else{
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op3"))->set_value( val / 12 );
    };*/
};
void Dx7interface::set_kls_brk_pt_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].kls.brk_pt.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].kls.brk_pt.max){
        value=bank_1_modif.sound->op[3].kls.brk_pt.max;
    };
    bank_1_modif.sound->op[3].kls.brk_pt.val=(int)value;
    /*(get_gwidget<Gtk::DropDown>("note_brk_pt_op4"))->set_selected(value % 12);
    int val = (value -3);
    if(val < 0){
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op4"))->set_value( -1 );
    }else{
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op4"))->set_value( val / 12 );
    };*/
};
void Dx7interface::set_kls_brk_pt_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].kls.brk_pt.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].kls.brk_pt.max){
        value=bank_1_modif.sound->op[4].kls.brk_pt.max;
    };
    bank_1_modif.sound->op[4].kls.brk_pt.val=(int)value;
    /*(get_gwidget<Gtk::DropDown>("note_brk_pt_op5"))->set_selected(value % 12);
    int val = (value -3);
    if(val < 0){
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op5"))->set_value( -1 );
    }else{
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op5"))->set_value( val / 12 );
    };
    */
};
void Dx7interface::set_kls_brk_pt_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].kls.brk_pt.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].kls.brk_pt.max){
        value=bank_1_modif.sound->op[5].kls.brk_pt.max;
    };
    bank_1_modif.sound->op[5].kls.brk_pt.val=(int)value;
    /*(get_gwidget<Gtk::DropDown>("note_brk_pt_op6"))->set_selected(value % 12);
    int val = (value -3);
    if(val < 0){
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op6"))->set_value( -1 );
    }else{
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op6"))->set_value( val / 12 );
    };
    */
};
void Dx7interface::set_kls_lft_curve_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].kls.lft_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].kls.lft_curve.max){
        value=bank_1_modif.sound->op[0].kls.lft_curve.max;
    };
    bank_1_modif.sound->op[0].kls.lft_curve.val=(int)value;
};
void Dx7interface::set_kls_lft_curve_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].kls.lft_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].kls.lft_curve.max){
        value=bank_1_modif.sound->op[1].kls.lft_curve.max;
    };
     bank_1_modif.sound->op[1].kls.lft_curve.val=(int)value;
};
void Dx7interface::set_kls_lft_curve_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].kls.lft_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].kls.lft_curve.max){
        value=bank_1_modif.sound->op[2].kls.lft_curve.max;
    };
     bank_1_modif.sound->op[2].kls.lft_curve.val=(int)value;
};
void Dx7interface::set_kls_lft_curve_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].kls.lft_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].kls.lft_curve.max){
        value=bank_1_modif.sound->op[3].kls.lft_curve.max;
    };
    bank_1_modif.sound->op[3].kls.lft_curve.val=(int)value;
};
void Dx7interface::set_kls_lft_curve_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].kls.lft_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].kls.lft_curve.max){
        value=bank_1_modif.sound->op[4].kls.lft_curve.max;
    };
    bank_1_modif.sound->op[4].kls.lft_curve.val=(int)value;
};
void Dx7interface::set_kls_lft_curve_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].kls.lft_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].kls.lft_curve.max){
        value=bank_1_modif.sound->op[5].kls.lft_curve.max;
    };
     bank_1_modif.sound->op[5].kls.lft_curve.val=(int)value;
};
void Dx7interface::set_kls_lft_dpth_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].kls.lft_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].kls.lft_dpth.max){
        value=bank_1_modif.sound->op[0].kls.lft_dpth.max;
    };
    bank_1_modif.sound->op[0].kls.lft_dpth.val=(int)value;
};
void Dx7interface::set_kls_lft_dpth_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].kls.lft_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].kls.lft_dpth.max){
        value=bank_1_modif.sound->op[1].kls.lft_dpth.max;
    };
    bank_1_modif.sound->op[1].kls.lft_dpth.val=(int)value;
};
void Dx7interface::set_kls_lft_dpth_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].kls.lft_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].kls.lft_dpth.max){
        value=bank_1_modif.sound->op[2].kls.lft_dpth.max;
    };
    bank_1_modif.sound->op[2].kls.lft_dpth.val=(int)value;
};
void Dx7interface::set_kls_lft_dpth_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].kls.lft_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].kls.lft_dpth.max){
        value=bank_1_modif.sound->op[3].kls.lft_dpth.max;
    };
    bank_1_modif.sound->op[3].kls.lft_dpth.val=(int)value;
};
void Dx7interface::set_kls_lft_dpth_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].kls.lft_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].kls.lft_dpth.max){
        value=bank_1_modif.sound->op[4].kls.lft_dpth.max;
    };
    bank_1_modif.sound->op[4].kls.lft_dpth.val=(int)value;
};
void Dx7interface::set_kls_lft_dpth_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].kls.lft_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].kls.lft_dpth.max){
        value=bank_1_modif.sound->op[5].kls.lft_dpth.max;
    };
    bank_1_modif.sound->op[5].kls.lft_dpth.val=(int)value;
};
void Dx7interface::set_kls_rght_curve_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].kls.rght_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].kls.rght_curve.max){
        value=bank_1_modif.sound->op[0].kls.rght_curve.max;
    };
    bank_1_modif.sound->op[0].kls.rght_curve.val=(int)value;
};
void Dx7interface::set_kls_rght_curve_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].kls.rght_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].kls.rght_curve.max){
        value=bank_1_modif.sound->op[1].kls.rght_curve.max;
    };
     bank_1_modif.sound->op[1].kls.rght_curve.val=(int)value;
};
void Dx7interface::set_kls_rght_curve_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].kls.rght_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].kls.rght_curve.max){
        value=bank_1_modif.sound->op[2].kls.rght_curve.max;
    };
    bank_1_modif.sound->op[2].kls.rght_curve.val=(int)value;
};
void Dx7interface::set_kls_rght_curve_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].kls.rght_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].kls.rght_curve.max){
        value=bank_1_modif.sound->op[3].kls.rght_curve.max;
    };
    bank_1_modif.sound->op[3].kls.rght_curve.val=(int)value;
};
void Dx7interface::set_kls_rght_curve_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].kls.rght_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].kls.rght_curve.max){
        value=bank_1_modif.sound->op[4].kls.rght_curve.max;
    };
     bank_1_modif.sound->op[4].kls.rght_curve.val=(int)value;
};
void Dx7interface::set_kls_rght_curve_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].kls.rght_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].kls.rght_curve.max){
        value=bank_1_modif.sound->op[5].kls.rght_curve.max;
    };
     bank_1_modif.sound->op[5].kls.rght_curve.val=(int)value;
};
void Dx7interface::set_kls_rght_dpth_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].kls.rght_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].kls.rght_dpth.max){
        value=bank_1_modif.sound->op[0].kls.rght_dpth.max;
    };
    bank_1_modif.sound->op[0].kls.rght_dpth.val=(int)value;
};
void Dx7interface::set_kls_rght_dpth_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].kls.rght_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].kls.rght_dpth.max){
        value=bank_1_modif.sound->op[1].kls.rght_dpth.max;
    };
    bank_1_modif.sound->op[1].kls.rght_dpth.val=(int)value;
};
void Dx7interface::set_kls_rght_dpth_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].kls.rght_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].kls.rght_dpth.max){
        value=bank_1_modif.sound->op[2].kls.rght_dpth.max;
    };
    bank_1_modif.sound->op[2].kls.rght_dpth.val=(int)value;
};
void Dx7interface::set_kls_rght_dpth_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].kls.rght_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].kls.rght_dpth.max){
        value=bank_1_modif.sound->op[3].kls.rght_dpth.max;
    };
    bank_1_modif.sound->op[3].kls.rght_dpth.val=(int)value;
};
void Dx7interface::set_kls_rght_dpth_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].kls.rght_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].kls.rght_dpth.max){
        value=bank_1_modif.sound->op[4].kls.rght_dpth.max;
    };
    bank_1_modif.sound->op[4].kls.rght_dpth.val=(int)value;
};
void Dx7interface::set_kls_rght_dpth_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].kls.rght_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].kls.rght_dpth.max){
        value=bank_1_modif.sound->op[5].kls.rght_dpth.max;
    };
    bank_1_modif.sound->op[5].kls.rght_dpth.val=(int)value;
};
void Dx7interface::set_krs_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].krs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].krs.max){
        value=bank_1_modif.sound->op[0].krs.max;
    };
    bank_1_modif.sound->op[0].krs.val=(int)value;
};
void Dx7interface::set_krs_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].krs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].krs.max){
        value=bank_1_modif.sound->op[1].krs.max;
    };
    bank_1_modif.sound->op[1].krs.val=(int)value;
};
void Dx7interface::set_krs_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].krs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].krs.max){
        value=bank_1_modif.sound->op[2].krs.max;
    };
    bank_1_modif.sound->op[2].krs.val=(int)value;
};
void Dx7interface::set_krs_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].krs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].krs.max){
        value=bank_1_modif.sound->op[3].krs.max;
    };
    bank_1_modif.sound->op[3].krs.val=(int)value;
};
void Dx7interface::set_krs_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].krs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].krs.max){
        value=bank_1_modif.sound->op[4].krs.max;
    };
    bank_1_modif.sound->op[4].krs.val=(int)value;
};
void Dx7interface::set_krs_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].krs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].krs.max){
        value=bank_1_modif.sound->op[5].krs.max;
    };
    bank_1_modif.sound->op[5].krs.val = (int)value;
};
void Dx7interface::set_kvs_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].kvs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].kvs.max){
        value=bank_1_modif.sound->op[0].kvs.max;
    };
    bank_1_modif.sound->op[0].kvs.val=(int)value;
};
void Dx7interface::set_kvs_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].kvs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].kvs.max){
        value=bank_1_modif.sound->op[1].kvs.max;
    };
    bank_1_modif.sound->op[1].kvs.val=(int)value;
};
void Dx7interface::set_kvs_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].kvs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].kvs.max){
        value=bank_1_modif.sound->op[2].kvs.max;
    };
    bank_1_modif.sound->op[2].kvs.val=(int)value;
};
void Dx7interface::set_kvs_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].kvs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].kvs.max){
        value=bank_1_modif.sound->op[3].kvs.max;
    };
    bank_1_modif.sound->op[3].kvs.val=(int)value;
};
void Dx7interface::set_kvs_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].kvs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].kvs.max){
        value=bank_1_modif.sound->op[4].kvs.max;
    };
    bank_1_modif.sound->op[4].kvs.val=(int)value;
};
void Dx7interface::set_kvs_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].kvs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].kvs.max){
        value=bank_1_modif.sound->op[5].kvs.max;
    };
    bank_1_modif.sound->op[5].kvs.val=(int)value;
};
void Dx7interface::set_lfo_amd_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->lfo.amd.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->lfo.amd.max){
        value=bank_1_modif.sound->lfo.amd.max;
    };
    bank_1_modif.sound->lfo.amd.val = (int)value;
};
void Dx7interface::set_lfo_delay_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->lfo.delay.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->lfo.delay.max){
        value=bank_1_modif.sound->lfo.delay.max;
    };
    bank_1_modif.sound->lfo.delay.val=(int)value;
};
void Dx7interface::set_lfo_pmd_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->lfo.pmd.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->lfo.pmd.max){
        value=bank_1_modif.sound->lfo.pmd.max;
    };
    bank_1_modif.sound->lfo.pmd.val=(int)value;
};
void Dx7interface::set_lfo_speed_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->lfo.speed.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->lfo.speed.max){
        value=bank_1_modif.sound->lfo.speed.max;
    };
    bank_1_modif.sound->lfo.speed.val=(int)value;
};
void Dx7interface::set_lfo_sync_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->lfo.sync.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->lfo.sync.max){
        value=bank_1_modif.sound->lfo.sync.max;
    };
     bank_1_modif.sound->lfo.sync.val=(int)value;
};
void Dx7interface::set_lfo_wav_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->lfo.wave.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->lfo.wave.max){
        value=bank_1_modif.sound->lfo.wave.max;
    };
     bank_1_modif.sound->lfo.wave.val=(int)value;
};
void Dx7interface::set_lvl_op1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[0].lvl.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].lvl.max){
        value=bank_1_modif.sound->op[0].lvl.max;
    };
    bank_1_modif.sound->op[0].lvl.val=(int)value;
};
void Dx7interface::set_lvl_op2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[1].lvl.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].lvl.max){
        value=bank_1_modif.sound->op[1].lvl.max;
    };
    bank_1_modif.sound->op[1].lvl.val=(int)value;
};
void Dx7interface::set_lvl_op3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[2].lvl.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].lvl.max){
        value=bank_1_modif.sound->op[2].lvl.max;
    };
    bank_1_modif.sound->op[2].lvl.val=(int)value;
};
void Dx7interface::set_lvl_op4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[3].lvl.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].lvl.max){
        value=bank_1_modif.sound->op[3].lvl.max;
    };
    bank_1_modif.sound->op[3].lvl.val=(int)value;
};
void Dx7interface::set_lvl_op5_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[4].lvl.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].lvl.max){
        value=bank_1_modif.sound->op[4].lvl.max;
    };
    bank_1_modif.sound->op[4].lvl.val=(int)value;
};
void Dx7interface::set_lvl_op6_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->op[5].lvl.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].lvl.max){
        value=bank_1_modif.sound->op[5].lvl.max;
    };
    bank_1_modif.sound->op[5].lvl.val=(int)value;
};
void Dx7interface::set_md_whl_assgn_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->extra.functions.md_whl_assgn.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->extra.functions.md_whl_assgn.max){
        value=bank_1_modif.sound->extra.functions.md_whl_assgn.max;
    };
    bank_1_modif.sound->extra.functions.md_whl_assgn.val=(int)value;
    /*unsigned char val = value;
    (get_gwidget<Gtk::CheckButton>("md_whl_ptch"))->set_active( (val & 0x01) );
    (get_gwidget<Gtk::CheckButton>("md_whl_mp"))->set_active( (val & 0x02)>>1 );
    (get_gwidget<Gtk::CheckButton>("md_whl_gbs"))->set_active( (val & 0x04)>>2 );
    */
};
void Dx7interface::set_md_whl_rng_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->extra.functions.md_whl_rng.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->extra.functions.md_whl_rng.max){
        value=bank_1_modif.sound->extra.functions.md_whl_rng.max;
    };
    bank_1_modif.sound->extra.functions.md_whl_rng.val=(int)value;
};
void Dx7interface::set_mono_poly_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->extra.functions.poly_mono.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->extra.functions.poly_mono.max){
        value=bank_1_modif.sound->extra.functions.poly_mono.max;
    };
    bank_1_modif.sound->extra.functions.poly_mono.val=(int)value;
};

void Dx7interface::set_mute_op1_event(int value){
    int val_ori = bank_1_modif.sound->extra.mute.val;
    val_ori = (val_ori >> 5) & 0x01;
    if( ( value >= 63 && val_ori ) || ( value < 63  && !val_ori ) ){
        value=0x20;
    }else{
        value=0x00;
    };
    bank_1_modif.sound->extra.mute.val=bank_1_modif.sound->extra.mute.val ^ value;
};
void Dx7interface::set_mute_op2_event(int value){
    int val_ori = bank_1_modif.sound->extra.mute.val;
    val_ori = (val_ori >> 4) & 0x01;
    if( ( value >= 63 && val_ori ) || ( value < 63  && !val_ori ) ){
        value=0x10;
    }else{
        value=0x00;
    };
    bank_1_modif.sound->extra.mute.val=bank_1_modif.sound->extra.mute.val ^ value;
};
void Dx7interface::set_mute_op3_event(int value){
    int val_ori = bank_1_modif.sound->extra.mute.val;
    val_ori = (val_ori >> 3) & 0x01;
    if( ( value >= 63 && val_ori ) || ( value < 63  && !val_ori ) ){
        value=0x08;
    }else{
        value=0x00;
    };
    bank_1_modif.sound->extra.mute.val=bank_1_modif.sound->extra.mute.val ^ value;
};
void Dx7interface::set_mute_op4_event(int value){
    int val_ori = bank_1_modif.sound->extra.mute.val;
    val_ori = (val_ori >> 2) & 0x01;
    if( ( value >= 63 && val_ori ) || ( value < 63  && !val_ori ) ){
        value=0x04;
    }else{
        value=0x00;
    };
    bank_1_modif.sound->extra.mute.val=bank_1_modif.sound->extra.mute.val ^ value;
};
void Dx7interface::set_mute_op5_event(int value){
    int val_ori = bank_1_modif.sound->extra.mute.val;
    val_ori = (val_ori >> 1) & 0x01;
    if( ( value >= 63 && val_ori ) || ( value < 63  && !val_ori ) ){
        value=0x02;
    }else{
        value=0x00;
    };
    bank_1_modif.sound->extra.mute.val=bank_1_modif.sound->extra.mute.val ^ value;
};
void Dx7interface::set_mute_op6_event(int value){
    int val_ori = bank_1_modif.sound->extra.mute.val;
    val_ori = val_ori & 0x01;
    if( ( value >= 63 && val_ori ) || ( value < 63  && !val_ori ) ){
        value=0x01;
    }else{
        value=0x00;
    };
    bank_1_modif.sound->extra.mute.val = bank_1_modif.sound->extra.mute.val ^ value;
};

void Dx7interface::set_oks_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->algo.oks.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->algo.oks.max){
        value=bank_1_modif.sound->algo.oks.max;
    };
    bank_1_modif.sound->algo.oks.val=(int)value;
};
void Dx7interface::set_panic_event(int value){
    //value = (double)value/(127.0/(double)bank_1_modif.sound->max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
};
void Dx7interface::set_pitch_lvl1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->pitch.eg_lvl[0].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->pitch.eg_lvl[0].max){
        value=bank_1_modif.sound->pitch.eg_lvl[0].max;
    };
    bank_1_modif.sound->pitch.eg_lvl[0].val=(int)value;
};
void Dx7interface::set_pitch_lvl2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->pitch.eg_lvl[1].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->pitch.eg_lvl[1].max){
        value=bank_1_modif.sound->pitch.eg_lvl[1].max;
    };
    bank_1_modif.sound->pitch.eg_lvl[1].val=(int)value;
};
void Dx7interface::set_pitch_lvl3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->pitch.eg_lvl[2].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->pitch.eg_lvl[2].max){
        value=bank_1_modif.sound->pitch.eg_lvl[2].max;
    };
    bank_1_modif.sound->pitch.eg_lvl[2].val=(int)value;
};
void Dx7interface::set_pitch_lvl4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->pitch.eg_lvl[3].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->pitch.eg_lvl[3].max){
        value=bank_1_modif.sound->pitch.eg_lvl[3].max;
    };
    bank_1_modif.sound->pitch.eg_lvl[3].val=(int)value;
};
void Dx7interface::set_pitch_rt1_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->pitch.eg_rt[0].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->pitch.eg_rt[0].max){
        value=bank_1_modif.sound->pitch.eg_rt[0].max;
    };
    bank_1_modif.sound->pitch.eg_rt[0].val=(int)value;
};
void Dx7interface::set_pitch_rt2_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->pitch.eg_rt[1].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->pitch.eg_rt[1].max){
        value=bank_1_modif.sound->pitch.eg_rt[1].max;
    };
    bank_1_modif.sound->pitch.eg_rt[1].val=(int)value;
};
void Dx7interface::set_pitch_rt3_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->pitch.eg_rt[2].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->pitch.eg_rt[2].max){
        value=bank_1_modif.sound->pitch.eg_rt[2].max;
    };
    bank_1_modif.sound->pitch.eg_rt[2].val=(int)value;
};
void Dx7interface::set_pitch_rt4_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->pitch.eg_rt[3].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->pitch.eg_rt[3].max){
        value=bank_1_modif.sound->pitch.eg_rt[3].max;
    };
    bank_1_modif.sound->pitch.eg_rt[3].val=(int)value;
};

void Dx7interface::set_pms_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->lfo.pms.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->lfo.pms.max){
        value=bank_1_modif.sound->lfo.pms.max;
    };
    bank_1_modif.sound->lfo.pms.val=(int)value;
};
void Dx7interface::set_portamento_glss_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->extra.functions.portamento_glss.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->extra.functions.portamento_glss.max){
        value=bank_1_modif.sound->extra.functions.portamento_glss.max;
    };
    bank_1_modif.sound->extra.functions.portamento_glss.val=(int)value;
};
void Dx7interface::set_portamento_md_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->extra.functions.portamento_md.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->extra.functions.portamento_md.max){
        value=bank_1_modif.sound->extra.functions.portamento_md.max;
    };
    bank_1_modif.sound->extra.functions.portamento_md.val=(int)value;
};
void Dx7interface::set_portamento_tm_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->extra.functions.portamento_tm.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->extra.functions.portamento_tm.max){
        value=bank_1_modif.sound->extra.functions.portamento_tm.max;
    };
    bank_1_modif.sound->extra.functions.portamento_tm.val=(int)value;
};
void Dx7interface::set_ptch_bnd_rng_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->extra.functions.ptch_bnd_rng.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->extra.functions.ptch_bnd_rng.max){
        value=bank_1_modif.sound->extra.functions.ptch_bnd_rng.max;
    };
    bank_1_modif.sound->extra.functions.ptch_bnd_rng.val=(int)value;
};
void Dx7interface::set_ptch_bnd_stp_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->extra.functions.ptch_bnd_stp.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->extra.functions.ptch_bnd_stp.max){
        value=bank_1_modif.sound->extra.functions.ptch_bnd_stp.max;
    };
    bank_1_modif.sound->extra.functions.ptch_bnd_stp.val=(int)value;
};
void Dx7interface::set_send_extra_parameters_event(int value){
};

void Dx7interface::set_transpose_event(int value){
    value = (double)value/(127.0/(double)bank_1_modif.sound->algo.transpose.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->algo.transpose.max){
        value=bank_1_modif.sound->algo.transpose.max;
    };
    bank_1_modif.sound->algo.transpose.val=(int)value;
};

void Dx7interface::block_ui(){
    /*** Block UI ***/
    slot_sound_name_activate.block(true);
    slot_sound_name_change.block(true);
    slot_selected_sound_change.block(true);
    /* algo */
    slot_algo.block(true);
    slot_feedback.block(true);
    slot_note_transpose.block(true);
    slot_octv_transpose.block(true);
    slot_oks.block(true);
    /* general lfo */
    slot_lfo_wav.block(true);
    slot_lfo_sync.block(true);
    slot_lfo_speed.block(true);
    slot_lfo_delay.block(true);
    slot_lfo_pmd.block(true);
    slot_lfo_amd.block(true);
    /* lfo modulation */
    slot_pms.block(true);
    /* pitch eg*/
    slot_pitch_rt1.block(true);
    slot_pitch_rt2.block(true);
    slot_pitch_rt3.block(true);
    slot_pitch_rt4.block(true);
    slot_pitch_lvl1.block(true);
    slot_pitch_lvl2.block(true);
    slot_pitch_lvl3.block(true);
    slot_pitch_lvl4.block(true);

    /* op1 */
    slot_ams_op1.block(true);
    slot_freq_mode_op1.block(true);
    slot_freq_coarse_op1.block(true);
    slot_freq_fine_op1.block(true);
    slot_dtun_op1.block(true);
    /* op1 EG*/
    slot_eg_rt1_op1.block(true);
    slot_eg_rt2_op1.block(true);
    slot_eg_rt3_op1.block(true);
    slot_eg_rt4_op1.block(true);
    slot_eg_lvl1_op1.block(true);
    slot_eg_lvl2_op1.block(true);
    slot_eg_lvl3_op1.block(true);
    slot_eg_lvl4_op1.block(true);
    /* op1 VOLUME */
    slot_krs_op1.block(true);
    slot_kvs_op1.block(true);
    slot_lvl_op1.block(true);
    slot_mute_op1.block(true);
    //slot_mute_hexter_op1.block(true);
    /* op1 KLS */
    slot_kls_lft_curve_op1.block(true);
    slot_kls_rght_curve_op1.block(true);
    slot_kls_lft_depth_op1.block(true);
    slot_kls_rght_depth_op1.block(true);
    slot_kls_note_brk_pt_op1.block(true);
    slot_kls_octv_brk_pt_op1.block(true);

    /* op2 */
    slot_ams_op2.block(true);
    slot_freq_mode_op2.block(true);
    slot_freq_coarse_op2.block(true);
    slot_freq_fine_op2.block(true);
    slot_dtun_op2.block(true);
    /* op2 EG*/
    slot_eg_rt1_op2.block(true);
    slot_eg_rt2_op2.block(true);
    slot_eg_rt3_op2.block(true);
    slot_eg_rt4_op2.block(true);
    slot_eg_lvl1_op2.block(true);
    slot_eg_lvl2_op2.block(true);
    slot_eg_lvl3_op2.block(true);
    slot_eg_lvl4_op2.block(true);
    /* op2 VOLUME */
    slot_krs_op2.block(true);
    slot_kvs_op2.block(true);
    slot_lvl_op2.block(true);
    slot_mute_op2.block(true);
    //slot_mute_hexter_op2.block(true);
    /* op2 KLS */
    slot_kls_lft_curve_op2.block(true);
    slot_kls_rght_curve_op2.block(true);
    slot_kls_lft_depth_op2.block(true);
    slot_kls_rght_depth_op2.block(true);
    slot_kls_note_brk_pt_op2.block(true);
    slot_kls_octv_brk_pt_op2.block(true);

    /* op3 */
    slot_ams_op3.block(true);
    slot_freq_mode_op3.block(true);
    slot_freq_coarse_op3.block(true);
    slot_freq_fine_op3.block(true);
    slot_dtun_op3.block(true);
    /* op3 EG*/
    slot_eg_rt1_op3.block(true);
    slot_eg_rt2_op3.block(true);
    slot_eg_rt3_op3.block(true);
    slot_eg_rt4_op3.block(true);
    slot_eg_lvl1_op3.block(true);
    slot_eg_lvl2_op3.block(true);
    slot_eg_lvl3_op3.block(true);
    slot_eg_lvl4_op3.block(true);
    /* op3 VOLUME */
    slot_krs_op3.block(true);
    slot_kvs_op3.block(true);
    slot_lvl_op3.block(true);
    slot_mute_op3.block(true);
    //slot_mute_hexter_op3.block(true);
    /* op3 KLS */
    slot_kls_lft_curve_op3.block(true);
    slot_kls_rght_curve_op3.block(true);
    slot_kls_lft_depth_op3.block(true);
    slot_kls_rght_depth_op3.block(true);
    slot_kls_note_brk_pt_op3.block(true);
    slot_kls_octv_brk_pt_op3.block(true);

    /* op4 */
    slot_ams_op4.block(true);
    slot_freq_mode_op4.block(true);
    slot_freq_coarse_op4.block(true);
    slot_freq_fine_op4.block(true);
    slot_dtun_op4.block(true);
    /* op4 EG*/
    slot_eg_rt1_op4.block(true);
    slot_eg_rt2_op4.block(true);
    slot_eg_rt3_op4.block(true);
    slot_eg_rt4_op4.block(true);
    slot_eg_lvl1_op4.block(true);
    slot_eg_lvl2_op4.block(true);
    slot_eg_lvl3_op4.block(true);
    slot_eg_lvl4_op4.block(true);
    /* op4 VOLUME */
    slot_krs_op4.block(true);
    slot_kvs_op4.block(true);
    slot_lvl_op4.block(true);
    slot_mute_op4.block(true);
    //slot_mute_hexter_op4.block(true);
    /* op4 KLS */
    slot_kls_lft_curve_op4.block(true);
    slot_kls_rght_curve_op4.block(true);
    slot_kls_lft_depth_op4.block(true);
    slot_kls_rght_depth_op4.block(true);
    slot_kls_note_brk_pt_op4.block(true);
    slot_kls_octv_brk_pt_op4.block(true);

    /* op5 */
    slot_ams_op5.block(true);
    slot_freq_mode_op5.block(true);
    slot_freq_coarse_op5.block(true);
    slot_freq_fine_op5.block(true);
    slot_dtun_op5.block(true);
    /* op5 EG*/
    slot_eg_rt1_op5.block(true);
    slot_eg_rt2_op5.block(true);
    slot_eg_rt3_op5.block(true);
    slot_eg_rt4_op5.block(true);
    slot_eg_lvl1_op5.block(true);
    slot_eg_lvl2_op5.block(true);
    slot_eg_lvl3_op5.block(true);
    slot_eg_lvl4_op5.block(true);
    /* op5 VOLUME */
    slot_krs_op5.block(true);
    slot_kvs_op5.block(true);
    slot_lvl_op5.block(true);
    slot_mute_op5.block(true);
    //slot_mute_hexter_op5.block(true);
    /* op5 KLS */
    slot_kls_lft_curve_op5.block(true);
    slot_kls_rght_curve_op5.block(true);
    slot_kls_lft_depth_op5.block(true);
    slot_kls_rght_depth_op5.block(true);
    slot_kls_note_brk_pt_op5.block(true);
    slot_kls_octv_brk_pt_op5.block(true);

    /* op6 */
    slot_ams_op6.block(true);
    slot_freq_mode_op6.block(true);
    slot_freq_coarse_op6.block(true);
    slot_freq_fine_op6.block(true);
    slot_dtun_op6.block(true);
    /* op6 EG*/
    slot_eg_rt1_op6.block(true);
    slot_eg_rt2_op6.block(true);
    slot_eg_rt3_op6.block(true);
    slot_eg_rt4_op6.block(true);
    slot_eg_lvl1_op6.block(true);
    slot_eg_lvl2_op6.block(true);
    slot_eg_lvl3_op6.block(true);
    slot_eg_lvl4_op6.block(true);
    /* op6 VOLUME */
    slot_krs_op6.block(true);
    slot_kvs_op6.block(true);
    slot_lvl_op6.block(true);
    slot_mute_op6.block(true);
    //slot_mute_hexter_op6.block(true);
    /* op6 KLS */
    slot_kls_lft_curve_op6.block(true);
    slot_kls_rght_curve_op6.block(true);
    slot_kls_lft_depth_op6.block(true);
    slot_kls_rght_depth_op6.block(true);
    slot_kls_note_brk_pt_op6.block(true);
    slot_kls_octv_brk_pt_op6.block(true);

};

void Dx7interface::unblock_ui(){
    /*** Unblock UI ***/
    slot_sound_name_activate.unblock();
    slot_sound_name_change.unblock();
    slot_selected_sound_change.unblock();
    /* algo */
    slot_algo.unblock();
    slot_feedback.unblock();
    slot_octv_transpose.unblock();
    slot_note_transpose.unblock();
    slot_oks.unblock();
    /* general lfo */
    slot_lfo_wav.unblock();
    slot_lfo_sync.unblock();
    slot_lfo_speed.unblock();
    slot_lfo_delay.unblock();
    slot_lfo_pmd.unblock();
    slot_lfo_amd.unblock();
    /* lfo modulation */
    slot_pms.unblock();
    /* pitch eg*/
    slot_pitch_rt1.unblock();
    slot_pitch_rt2.unblock();
    slot_pitch_rt3.unblock();
    slot_pitch_rt4.unblock();
    slot_pitch_lvl1.unblock();
    slot_pitch_lvl2.unblock();
    slot_pitch_lvl3.unblock();
    slot_pitch_lvl4.unblock();

    /* op1 */
    slot_ams_op1.unblock();
    slot_freq_mode_op1.unblock();
    slot_freq_coarse_op1.unblock();
    slot_freq_fine_op1.unblock();
    slot_dtun_op1.unblock();
    /* op1 EG*/
    slot_eg_rt1_op1.unblock();
    slot_eg_rt2_op1.unblock();
    slot_eg_rt3_op1.unblock();
    slot_eg_rt4_op1.unblock();
    slot_eg_lvl1_op1.unblock();
    slot_eg_lvl2_op1.unblock();
    slot_eg_lvl3_op1.unblock();
    slot_eg_lvl4_op1.unblock();
    /* op1 VOLUME */
    slot_krs_op1.unblock();
    slot_kvs_op1.unblock();
    slot_lvl_op1.unblock();
    slot_mute_op1.unblock();
    //slot_mute_hexter_op1.unblock();
    /* op1 KLS */
    slot_kls_lft_curve_op1.unblock();
    slot_kls_rght_curve_op1.unblock();
    slot_kls_lft_depth_op1.unblock();
    slot_kls_rght_depth_op1.unblock();
    slot_kls_note_brk_pt_op1.unblock();
    slot_kls_octv_brk_pt_op1.unblock();

    /* op2 */
    slot_ams_op2.unblock();
    slot_freq_mode_op2.unblock();
    slot_freq_coarse_op2.unblock();
    slot_freq_fine_op2.unblock();
    slot_dtun_op2.unblock();
    /* op2 EG*/
    slot_eg_rt1_op2.unblock();
    slot_eg_rt2_op2.unblock();
    slot_eg_rt3_op2.unblock();
    slot_eg_rt4_op2.unblock();
    slot_eg_lvl1_op2.unblock();
    slot_eg_lvl2_op2.unblock();
    slot_eg_lvl3_op2.unblock();
    slot_eg_lvl4_op2.unblock();
    /* op2 VOLUME */
    slot_krs_op2.unblock();
    slot_kvs_op2.unblock();
    slot_lvl_op2.unblock();
    slot_mute_op2.unblock();
    //slot_mute_hexter_op2.unblock();
    /* op2 KLS */
    slot_kls_lft_curve_op2.unblock();
    slot_kls_rght_curve_op2.unblock();
    slot_kls_lft_depth_op2.unblock();
    slot_kls_rght_depth_op2.unblock();
    slot_kls_note_brk_pt_op2.unblock();
    slot_kls_octv_brk_pt_op2.unblock();

    /* op3 */
    slot_ams_op3.unblock();
    slot_freq_mode_op3.unblock();
    slot_freq_coarse_op3.unblock();
    slot_freq_fine_op3.unblock();
    slot_dtun_op3.unblock();
    /* op3 EG*/
    slot_eg_rt1_op3.unblock();
    slot_eg_rt2_op3.unblock();
    slot_eg_rt3_op3.unblock();
    slot_eg_rt4_op3.unblock();
    slot_eg_lvl1_op3.unblock();
    slot_eg_lvl2_op3.unblock();
    slot_eg_lvl3_op3.unblock();
    slot_eg_lvl4_op3.unblock();
    /* op3 VOLUME */
    slot_krs_op3.unblock();
    slot_kvs_op3.unblock();
    slot_lvl_op3.unblock();
    slot_mute_op3.unblock();
    //slot_mute_hexter_op3.unblock();

    /* op3 KLS */
    slot_kls_lft_curve_op3.unblock();
    slot_kls_rght_curve_op3.unblock();
    slot_kls_lft_depth_op3.unblock();
    slot_kls_rght_depth_op3.unblock();
    slot_kls_note_brk_pt_op3.unblock();
    slot_kls_octv_brk_pt_op3.unblock();

    /* op4 */
    slot_ams_op4.unblock();
    slot_freq_mode_op4.unblock();
    slot_freq_coarse_op4.unblock();
    slot_freq_fine_op4.unblock();
    slot_dtun_op4.unblock();
    /* op4 EG*/
    slot_eg_rt1_op4.unblock();
    slot_eg_rt2_op4.unblock();
    slot_eg_rt3_op4.unblock();
    slot_eg_rt4_op4.unblock();
    slot_eg_lvl1_op4.unblock();
    slot_eg_lvl2_op4.unblock();
    slot_eg_lvl3_op4.unblock();
    slot_eg_lvl4_op4.unblock();
    /* op4 VOLUME */
    slot_krs_op4.unblock();
    slot_kvs_op4.unblock();
    slot_lvl_op4.unblock();
    slot_mute_op4.unblock();
    //slot_mute_hexter_op4.unblock();
    /* op4 KLS */
    slot_kls_lft_curve_op4.unblock();
    slot_kls_rght_curve_op4.unblock();
    slot_kls_lft_depth_op4.unblock();
    slot_kls_rght_depth_op4.unblock();
    slot_kls_note_brk_pt_op4.unblock();
    slot_kls_octv_brk_pt_op4.unblock();

    /* op5 */
    slot_ams_op5.unblock();
    slot_freq_mode_op5.unblock();
    slot_freq_coarse_op5.unblock();
    slot_freq_fine_op5.unblock();
    slot_dtun_op5.unblock();
    /* op5 EG*/
    slot_eg_rt1_op5.unblock();
    slot_eg_rt2_op5.unblock();
    slot_eg_rt3_op5.unblock();
    slot_eg_rt4_op5.unblock();
    slot_eg_lvl1_op5.unblock();
    slot_eg_lvl2_op5.unblock();
    slot_eg_lvl3_op5.unblock();
    slot_eg_lvl4_op5.unblock();
    /* op5 VOLUME */
    slot_krs_op5.unblock();
    slot_kvs_op5.unblock();
    slot_lvl_op5.unblock();
    slot_mute_op5.unblock();
    //slot_mute_hexter_op5.unblock();
    /* op5 KLS */
    slot_kls_lft_curve_op5.unblock();
    slot_kls_rght_curve_op5.unblock();
    slot_kls_lft_depth_op5.unblock();
    slot_kls_rght_depth_op5.unblock();
    slot_kls_note_brk_pt_op5.unblock();
    slot_kls_octv_brk_pt_op5.unblock();

    /* op6 */
    slot_ams_op6.unblock();
    slot_freq_mode_op6.unblock();
    slot_freq_coarse_op6.unblock();
    slot_freq_fine_op6.unblock();
    slot_dtun_op6.unblock();
    /* op6 EG*/
    slot_eg_rt1_op6.unblock();
    slot_eg_rt2_op6.unblock();
    slot_eg_rt3_op6.unblock();
    slot_eg_rt4_op6.unblock();
    slot_eg_lvl1_op6.unblock();
    slot_eg_lvl2_op6.unblock();
    slot_eg_lvl3_op6.unblock();
    slot_eg_lvl4_op6.unblock();
    /* op6 VOLUME */
    slot_krs_op6.unblock();
    slot_kvs_op6.unblock();
    slot_lvl_op6.unblock();
    slot_mute_op6.unblock();
    //slot_mute_hexter_op6.unblock();
    /* op6 KLS */
    slot_kls_lft_curve_op6.unblock();
    slot_kls_rght_curve_op6.unblock();
    slot_kls_lft_depth_op6.unblock();
    slot_kls_rght_depth_op6.unblock();
    slot_kls_note_brk_pt_op6.unblock();
    slot_kls_octv_brk_pt_op6.unblock();
};

void Dx7interface::dettach_signals(){
    LOG( LOG_IN() );
    slot_bank_select.disconnect();
    /* Sound Select */
    slot_selected_sound_change.disconnect();
    slot_sound_name_activate.disconnect();
    slot_sound_name_change.disconnect();

    /* Algo */
    slot_algo.disconnect();
    slot_feedback.disconnect();
    slot_note_transpose.disconnect();
    slot_octv_transpose.disconnect();
    slot_oks.disconnect();

    /* lfo */
    slot_lfo_wav.disconnect();
    slot_lfo_sync.disconnect();
    slot_lfo_speed.disconnect();
    slot_lfo_delay.disconnect();
    slot_lfo_pmd.disconnect();
    slot_lfo_amd.disconnect();
    /* lfo modulation */
    slot_pms.disconnect();

    /* pitch eg */
    slot_pitch_rt1.disconnect();
    slot_pitch_rt2.disconnect();
    slot_pitch_rt3.disconnect();
    slot_pitch_rt4.disconnect();
    slot_pitch_lvl1.disconnect();
    slot_pitch_lvl2.disconnect();
    slot_pitch_lvl3.disconnect();
    slot_pitch_lvl4.disconnect();

    /* OP1 */
    slot_ams_op1.disconnect();
    /* OP1 FREQUENCE */
    slot_freq_mode_op1.disconnect();
    slot_freq_coarse_op1.disconnect();
    slot_freq_fine_op1.disconnect();
    slot_dtun_op1.disconnect();
    /* OP1 EG */
    slot_eg_rt1_op1.disconnect();
    slot_eg_rt2_op1.disconnect();
    slot_eg_rt3_op1.disconnect();
    slot_eg_rt4_op1.disconnect();
    slot_eg_lvl1_op1.disconnect();
    slot_eg_lvl2_op1.disconnect();
    slot_eg_lvl3_op1.disconnect();
    slot_eg_lvl4_op1.disconnect();
    /* OP1 VOLUME */
    slot_krs_op1.disconnect();
    slot_kvs_op1.disconnect();
    slot_lvl_op1.disconnect();
    slot_mute_op1.disconnect();
    //slot_mute_hexter_op1.disconnect();
    /* OP1 KLS */
    slot_kls_lft_curve_op1.disconnect();
    slot_kls_rght_curve_op1.disconnect();
    slot_kls_lft_depth_op1.disconnect();
    slot_kls_rght_depth_op1.disconnect();
    slot_kls_note_brk_pt_op1.disconnect();
    slot_kls_octv_brk_pt_op1.disconnect();


    /* OP2 */
    slot_ams_op2.disconnect();
    /* OP2 FREQUENCE */
    slot_freq_mode_op2.disconnect();
    slot_freq_coarse_op2.disconnect();
    slot_freq_fine_op2.disconnect();
    slot_dtun_op2.disconnect();
    /* OP2 EG */
    slot_eg_rt1_op2.disconnect();
    slot_eg_rt2_op2.disconnect();
    slot_eg_rt3_op2.disconnect();
    slot_eg_rt4_op2.disconnect();
    slot_eg_lvl1_op2.disconnect();
    slot_eg_lvl2_op2.disconnect();
    slot_eg_lvl3_op2.disconnect();
    slot_eg_lvl4_op2.disconnect();
    /* OP2 VOLUME */
    slot_krs_op2.disconnect();
    slot_kvs_op2.disconnect();
    slot_lvl_op2.disconnect();
    slot_mute_op2.disconnect();
    //slot_mute_hexter_op2.disconnect();
    /* OP2 KLS */
    slot_kls_lft_curve_op2.disconnect();
    slot_kls_rght_curve_op2.disconnect();
    slot_kls_lft_depth_op2.disconnect();
    slot_kls_rght_depth_op2.disconnect();
    slot_kls_note_brk_pt_op2.disconnect();
    slot_kls_octv_brk_pt_op2.disconnect();


    /* OP3 */
    slot_ams_op3.disconnect();
    /* OP3 FREQUENCE */
    slot_freq_mode_op3.disconnect();
    slot_freq_coarse_op3.disconnect();
    slot_freq_fine_op3.disconnect();
    slot_dtun_op3.disconnect();
    /* OP3 EG */
    slot_eg_rt1_op3.disconnect();
    slot_eg_rt2_op3.disconnect();
    slot_eg_rt3_op3.disconnect();
    slot_eg_rt4_op3.disconnect();
    slot_eg_lvl1_op3.disconnect();
    slot_eg_lvl2_op3.disconnect();
    slot_eg_lvl3_op3.disconnect();
    slot_eg_lvl4_op3.disconnect();
    /* OP3 VOLUME */
    slot_krs_op3.disconnect();
    slot_kvs_op3.disconnect();
    slot_lvl_op3.disconnect();
    slot_mute_op3.disconnect();
    //slot_mute_hexter_op3.disconnect();
    /* OP3 KLS */
    slot_kls_lft_curve_op3.disconnect();
    slot_kls_rght_curve_op3.disconnect();
    slot_kls_lft_depth_op3.disconnect();
    slot_kls_rght_depth_op3.disconnect();
    slot_kls_note_brk_pt_op3.disconnect();
    slot_kls_octv_brk_pt_op3.disconnect();


    /* OP4 */
    slot_ams_op4.disconnect();
    /* OP4 FREQUENCE */
    slot_freq_mode_op4.disconnect();
    slot_freq_coarse_op4.disconnect();
    slot_freq_fine_op4.disconnect();
    slot_dtun_op4.disconnect();
    /* OP4 EG */
    slot_eg_rt1_op4.disconnect();
    slot_eg_rt2_op4.disconnect();
    slot_eg_rt3_op4.disconnect();
    slot_eg_rt4_op4.disconnect();
    slot_eg_lvl1_op4.disconnect();
    slot_eg_lvl2_op4.disconnect();
    slot_eg_lvl3_op4.disconnect();
    slot_eg_lvl4_op4.disconnect();
    /* OP4 VOLUME */
    slot_krs_op4.disconnect();
    slot_kvs_op4.disconnect();
    slot_lvl_op4.disconnect();
    slot_mute_op4.disconnect();
    //slot_mute_hexter_op4.disconnect();
    /* OP4 KLS */
    slot_kls_lft_curve_op4.disconnect();
    slot_kls_rght_curve_op4.disconnect();
    slot_kls_lft_depth_op4.disconnect();
    slot_kls_rght_depth_op4.disconnect();
    slot_kls_note_brk_pt_op4.disconnect();
    slot_kls_octv_brk_pt_op4.disconnect();

    /* OP5 */
    slot_ams_op5.disconnect();
    /* OP5 FREQUENCE */
    slot_freq_mode_op5.disconnect();
    slot_freq_coarse_op5.disconnect();
    slot_freq_fine_op5.disconnect();
    slot_dtun_op5.disconnect();
    /* OP5 EG */
    slot_eg_rt1_op5.disconnect();
    slot_eg_rt2_op5.disconnect();
    slot_eg_rt3_op5.disconnect();
    slot_eg_rt4_op5.disconnect();
    slot_eg_lvl1_op5.disconnect();
    slot_eg_lvl2_op5.disconnect();
    slot_eg_lvl3_op5.disconnect();
    slot_eg_lvl4_op5.disconnect();
    /* OP5 VOLUME */
    slot_krs_op5.disconnect();
    slot_kvs_op5.disconnect();
    slot_lvl_op5.disconnect();
    slot_mute_op5.disconnect();
    //slot_mute_hexter_op5.disconnect();
    /* op5 KLS */
    slot_kls_lft_curve_op5.disconnect();
    slot_kls_rght_curve_op5.disconnect();
    slot_kls_lft_depth_op5.disconnect();
    slot_kls_rght_depth_op5.disconnect();
    slot_kls_note_brk_pt_op5.disconnect();
    slot_kls_octv_brk_pt_op5.disconnect();

    /* OP6 */
    slot_ams_op6.disconnect();
    /* OP6 FREQUENCE */
    slot_freq_mode_op6.disconnect();
    slot_freq_coarse_op6.disconnect();
    slot_freq_fine_op6.disconnect();
    slot_dtun_op6.disconnect();
    /* OP6 EG */
    slot_eg_rt1_op6.disconnect();
    slot_eg_rt2_op6.disconnect();
    slot_eg_rt3_op6.disconnect();
    slot_eg_rt4_op6.disconnect();
    slot_eg_lvl1_op6.disconnect();
    slot_eg_lvl2_op6.disconnect();
    slot_eg_lvl3_op6.disconnect();
    slot_eg_lvl4_op6.disconnect();
    /* OP6 VOLUME */
    slot_krs_op6.disconnect();
    slot_kvs_op6.disconnect();
    slot_lvl_op6.disconnect();
    slot_mute_op6.disconnect();
    //slot_mute_hexter_op6.disconnect();
    /* OP6 KLS */
    slot_kls_lft_curve_op6.disconnect();
    slot_kls_rght_curve_op6.disconnect();
    slot_kls_lft_depth_op6.disconnect();
    slot_kls_rght_depth_op6.disconnect();
    slot_kls_octv_brk_pt_op6.disconnect();
    LOG( LOG_OUT() );
};
#endif /* Dx7interface_CC */
