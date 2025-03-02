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
/* constante ui_file
 * pratique pour le dev pour pas reinstaller
 * a depacer dans .h
 */
extern "C" {
    std::tuple<std::shared_ptr<void>, Gtk::Box*, Glib::ustring> LoadPlug(uint8_t index){
        auto editor = std::make_shared<Dx7interface>(UI,index);
        Gtk::Box* mbox =  editor->get_rootbox();
        return std::make_tuple(editor, mbox, CSSFILE); //CSSFILE
    };
}

Dx7interface::Dx7interface(Glib::ustring ui, uint8_t index) : Gx_module(ui,MODULE_NAME), Synth(MODULE_NAME)  {
    /*basic constructor */
    LOG_IN();
    block_midi();
    init_nls();
    /* I/O init */
    Gio::init();
    if (index <= 0){
        set_app_name(MODULE_NAME);
    }else{
        set_app_name(MODULE_NAME+index);
    };

    /* MIDI */
    /* Yamaha specific */
    Synth::id_fabricant=id_fabricant;
    /* set channel & sub_status */
    Synth::channel_send=0xF1;
    Synth::channel_receive=0xF0;
    Synth::sub_status=0x10;
    seq_handle=get_seq_handler();
    ev=get_seq_event_handler();

    /* UI */
    /* mouse gesture for drawing area*/
    init_gesture_controller();
    /* create store for voices column list view */
    create_bank_voices_list();
    /* create ist function */
    create_param_list();
    // Save dialog
    create_save_dialog();
    // Create menu
    create_popover_menu();
    /* attach GUI signals */
    attach_signals();
	init_global_fonction_parameter();
    /* start thread */
    S_Thread();
    S_Thread2();
    unblock_midi();
    LOG_OUT();
};

Dx7interface::~Dx7interface(){
    LOG_IN();
    /* basic destructor*/
    /* terminate thread */
    T_Thread();
    dettach_signals();
    LOG_OUT();
};

void Dx7interface::create_bank_voices_list(){
    /* create specific data structure model for voice bank list */
    bank_data_model = Gio::ListStore<SoundBankItem>::create();
    /* set model to GUI */
    bank_selection_model=Glib::RefPtr<Gtk::SingleSelection>(get_gwidget<Gtk::SingleSelection>("selection_bank"));
    bank_selection_model->set_autoselect(false);
    bank_selection_model->set_model(bank_data_model);
    slot_selected_sound_change = bank_selection_model->signal_selection_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_selected_sound_change));

    /* Bank load */
    slot_bank_reveal = (get_gwidget<Gtk::Button>("btn_toolbar_reveal_bank"))->signal_clicked().connect(
        sigc::mem_fun(*this, &Dx7interface::on_bank_reveal));
    slot_bank_select = (get_gwidget<Gtk::Button>("bank_select"))->signal_clicked().connect(
        sigc::mem_fun(*this, &Dx7interface::on_bank_select));
    /* Sound Select */

    //auto factory_num=Glib::RefPtr<Gtk::SignalListItemFactory>(get_gwidget<Gtk::SignalListItemFactory>("factory_num"));
    (get_gwidget<Gtk::SignalListItemFactory>("factory_num"))->signal_setup().connect(
        sigc::bind(sigc::mem_fun(*this, &Dx7interface::on_setup_label), Gtk::Align::START));
    (get_gwidget<Gtk::SignalListItemFactory>("factory_num"))->signal_bind().connect(
        sigc::mem_fun(*this, &Dx7interface::on_bind_num));
    (get_gwidget<Gtk::SignalListItemFactory>("factory_name"))->signal_setup().connect(
        sigc::bind(sigc::mem_fun(*this, &Dx7interface::on_setup_label), Gtk::Align::START));
    (get_gwidget<Gtk::SignalListItemFactory>("factory_name"))->signal_bind().connect(
        sigc::mem_fun(*this, &Dx7interface::on_bind_name));
};

void Dx7interface::create_param_list(){
    /* create specific data structure model for voice bank list */
    param_data_model = Gio::ListStore<ParamItem>::create();
    /* set model to GUI */
    param_selection_model=Glib::RefPtr<Gtk::SingleSelection>(get_gwidget<Gtk::SingleSelection>("selection_param"));
    param_selection_model->set_autoselect(false);
    param_selection_model->set_model(param_data_model);
    //auto factory_name=Glib::RefPtr<Gtk::SignalListItemFactory>(get_gwidget<Gtk::SignalListItemFactory>("factory_name"));
    (get_gwidget<Gtk::SignalListItemFactory>("factory_param"))->signal_setup().connect(
        sigc::bind(sigc::mem_fun(*this, &Dx7interface::on_setup_param_label), Gtk::Align::START));
    (get_gwidget<Gtk::SignalListItemFactory>("factory_param"))->signal_bind().connect(
        sigc::mem_fun(*this, &Dx7interface::on_bind_param_name));
    Glib::ustring param_list[169]{
    "aftrtch_assgn_event",
    "aftrtch_rng_event",
    "algo_event",
    "amd_event",
    "ams_op1_event",
    "ams_op2_event",
    "ams_op3_event",
    "ams_op4_event",
    "ams_op5_event",
    "ams_op6_event",
    "brth_assgn_event",
    "brth_rng_event",
    "compare_event",
    "delay_event",
    "dtun_op1_event",
    "dtun_op2_event",
    "dtun_op3_event",
    "dtun_op4_event",
    "dtun_op5_event",
    "dtun_op6_event",
    "eg_lvl1_op1_event",
    "eg_lvl1_op2_event",
    "eg_lvl1_op3_event",
    "eg_lvl1_op4_event",
    "eg_lvl1_op5_event",
    "eg_lvl1_op6_event",
    "eg_lvl2_op1_event",
    "eg_lvl2_op2_event",
    "eg_lvl2_op3_event",
    "eg_lvl2_op4_event",
    "eg_lvl2_op5_event",
    "eg_lvl2_op6_event",
    "eg_lvl3_op1_event",
    "eg_lvl3_op2_event",
    "eg_lvl3_op3_event",
    "eg_lvl3_op4_event",
    "eg_lvl3_op5_event",
    "eg_lvl3_op6_event",
    "eg_lvl4_op1_event",
    "eg_lvl4_op2_event",
    "eg_lvl4_op3_event",
    "eg_lvl4_op4_event",
    "eg_lvl4_op5_event",
    "eg_lvl4_op6_event",
    "eg_rt1_op1_event",
    "eg_rt1_op2_event",
    "eg_rt1_op3_event",
    "eg_rt1_op4_event",
    "eg_rt1_op5_event",
    "eg_rt1_op6_event",
    "eg_rt2_op1_event",
    "eg_rt2_op2_event",
    "eg_rt2_op3_event",
    "eg_rt2_op4_event",
    "eg_rt2_op5_event",
    "eg_rt2_op6_event",
    "eg_rt3_op1_event",
    "eg_rt3_op2_event",
    "eg_rt3_op3_event",
    "eg_rt3_op4_event",
    "eg_rt3_op5_event",
    "eg_rt3_op6_event",
    "eg_rt4_op1_event",
    "eg_rt4_op2_event",
    "eg_rt4_op3_event",
    "eg_rt4_op4_event",
    "eg_rt4_op5_event",
    "eg_rt4_op6_event",
    "feedback_event",
    "foot_assgn_event",
    "foot_rng_event",
    "freq_coarse_op1_event",
    "freq_coarse_op2_event",
    "freq_coarse_op3_event",
    "freq_coarse_op4_event",
    "freq_coarse_op5_event",
    "freq_coarse_op6_event",
    "freq_fine_op1_event",
    "freq_fine_op2_event",
    "freq_fine_op3_event",
    "freq_fine_op4_event",
    "freq_fine_op5_event",
    "freq_fine_op6_event",
    "freq_mode_op1_event",
    "freq_mode_op2_event",
    "freq_mode_op3_event",
    "freq_mode_op4_event",
    "freq_mode_op5_event",
    "freq_mode_op6_event",
    "kls_brk_pt_op1_event",
    "kls_brk_pt_op2_event",
    "kls_brk_pt_op3_event",
    "kls_brk_pt_op4_event",
    "kls_brk_pt_op5_event",
    "kls_brk_pt_op6_event",
    "kls_lft_curve_op1_event",
    "kls_lft_curve_op2_event",
    "kls_lft_curve_op3_event",
    "kls_lft_curve_op4_event",
    "kls_lft_curve_op5_event",
    "kls_lft_curve_op6_event",
    "kls_lft_dpth_op1_event",
    "kls_lft_dpth_op2_event",
    "kls_lft_dpth_op3_event",
    "kls_lft_dpth_op4_event",
    "kls_lft_dpth_op5_event",
    "kls_lft_dpth_op6_event",
    "kls_rght_curve_op1_event",
    "kls_rght_curve_op2_event",
    "kls_rght_curve_op3_event",
    "kls_rght_curve_op4_event",
    "kls_rght_curve_op5_event",
    "kls_rght_curve_op6_event",
    "kls_rght_dpth_op1_event",
    "kls_rght_dpth_op2_event",
    "kls_rght_dpth_op3_event",
    "kls_rght_dpth_op4_event",
    "kls_rght_dpth_op5_event",
    "kls_rght_dpth_op6_event",
    "krs_op1_event",
    "krs_op2_event",
    "krs_op3_event",
    "krs_op4_event",
    "krs_op5_event",
    "krs_op6_event",
    "kvs_op1_event",
    "kvs_op2_event",
    "kvs_op3_event",
    "kvs_op4_event",
    "kvs_op5_event",
    "kvs_op6_event",
    "lfo_sync_event",
    "lfo_wav_event",
    "lvl_op1_event",
    "lvl_op2_event",
    "lvl_op3_event",
    "lvl_op4_event",
    "lvl_op5_event",
    "lvl_op6_event",
    "md_whl_assgn_event",
    "md_whl_rng_event",
    "mono_poly_event",
    "mute_op1_event",
    "mute_op2_event",
    "mute_op3_event",
    "mute_op4_event",
    "mute_op5_event",
    "mute_op6_event",
    "oks_event",
    "panic_event",
    "pitch_lvl1_event",
    "pitch_lvl2_event",
    "pitch_lvl3_event",
    "pitch_lvl4_event",
    "pitch_rt1_event",
    "pitch_rt2_event",
    "pitch_rt3_event",
    "pitch_rt4_event",
    "pmd_event",
    "pms_event",
    "portamento_glss_event",
    "portamento_md_event",
    "portamento_tm_event",
    "ptch_bnd_rng_event",
    "ptch_bnd_stp_event",
    "send_extra_parameters_event",
    "speed_event",
    "transpose_event"
    };
    midi_learned.resize(169);
    for(int i=0; i<169; i++){
        param_data_model->append(ParamItem::create(param_list[i]));
    };
};

bool Dx7interface::error(){
    std::cout << "erreur fichier syx " << std::endl;
    return true;
};

bool Dx7interface::Run(){
    /* Thread looped function */
    listen_midi();
    return true;
};

void Dx7interface::add_midi_learned(int index, int value){
        if( index >= static_cast<int>(midi_learned.size()) ){
            midi_learned.resize(index + 1);
        };
        midi_learned[index].push_back(value);
};

void Dx7interface::rem_midi_learned(int index, int value){
    if( index < static_cast<int>(midi_learned.size()) ){
        auto& vec = midi_learned[index];
        vec.erase(std::remove(vec.begin(), vec.end(), value), vec.end());
    };
};

void Dx7interface::listen_midi(){
    /* TODO : use all seq event */
    snd_seq_event_input(seq_handle, &ev);
    Synth::print_event_info(ev);
    if( (int)ev->dest.client == Synth::get_client_id() && (int)ev->data.control.channel == (int)(Synth::channel_receive & 0x0F) ){
        switch (ev->type) {
            case SND_SEQ_EVENT_NOTEON:
                //Synth::print_event_info(ev);
                std::cout << "Channel: "  << (int(ev->data.control.channel) +1) << " " << '\t'
                << "value: " << int(ev->data.note.note) << std::endl;;
                break;
            case SND_SEQ_EVENT_NOTEOFF:
                std::cout << "Note OFF" << std::endl;
                //Synth::print_event_info(ev);
                std::cout << "Channel: "  << (int(ev->data.control.channel) +1) << " " << '\t'
                << "value: " <<  int(ev->data.note.note) << std::endl;
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
                std::cout << "Channel: " << ( (int)(ev->data.control.channel) +1) << " " << '\t'
                << "param: "  << ev->data.control.param << " "
                << "value: " << int(ev->data.control.value) << std::endl;
                if(midi_learn){
                    (get_gwidget<Gtk::Entry>("entry_affect_param"))->set_text(tostr<int>(ev->data.control.param));
                }else{
                    if ( ev->data.control.param < midi_learned.size() && !midi_learned[ev->data.control.param].empty()) {
                        //while(lock && lock2){};
                        /*lock=true;
                        midi_param[ev->data.control.param]=ev->data.control.value;
                        lock=false;*/
                        //std::cout
                        (this->*list_ui_parameters_functions[midi_learned[ev->data.control.param][0]])(ev->data.control.value);
                    };
                };
                break;
            case SND_SEQ_EVENT_PITCHBEND:
                //Synth::print_event_info(ev);
                std::cout << "Channel: " << (int(ev->data.control.channel) +1)<< " " << '\t'
                << "value: " << int(ev->data.control.value) << std::endl;
                break;
            case SND_SEQ_EVENT_PGMCHANGE:
                //Synth::print_event_info(ev);
                /*event data type = snd_seq_ev_ctrl_t */
                std::cout <<  "Channel : "  << (int(ev->data.control.channel) +1) << '\t'
                << "param : "  << ev->data.control.param << " "
                << "value : " << int(ev->data.control.value)
                << std::endl;
                if ( ev->data.control.param == 0 && ( (uint)ev->data.control.value < bank_nb_sound ) ){
                    #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 12)
                        get_gwidget<Gtk::ColumnView>("columnview_bank")->scroll_to((uint)ev->data.control.value,{},Gtk::ListScrollFlags::SELECT,NULL);
                    #else
                        auto adjustment = get_gwidget<Gtk::ColumnView>("columnview_bank")->get_vadjustment();
                        adjustment->set_value((double)ev->data.control.value);
                        bank_selection_model->set_selected((uint)ev->data.control.value);
                    #endif
                }
                break;
            case SND_SEQ_EVENT_SYSEX:
                //Synth::print_event_info(ev);
                //SND_SEQ_EVENT_SYSEX 	system exclusive data (variable length);
                // event data type = snd_seq_ev_ext_t
                /*std::cout <<  "Channel : "  << (int(ev->data.control.channel) +1) << '\t'
                << "length : "  << int(ev->data.ext.len) << " "
                << "ptr : " << ev->data.ext.ptr
                << std::endl;*/
                break;
            case SND_SEQ_EVENT_SENSING:
                // CLOCK REQUEST
                break;
        };
    };
    snd_seq_free_event(ev);
};

bool Dx7interface::Run2(){
    //sleep_for(std::chrono::milliseconds(5000));
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000000;
    nanosleep(&ts, NULL);
    //std::cout << "coucou" << std::endl;
    while(lock){};
    lock=true;
    for(ulong i=0; i < midi_param.size(); i++){
        if ( midi_param[i] != -1) {
            int val = midi_param[i];
            (this->*list_ui_parameters_functions[midi_learned[i][0]])(val);
            std::cout << " Param: " << i << " Value: " << midi_param[i] << std::endl;
            midi_param[i] = -1;
        };
    };
    lock=false;
    return true;
};

void Dx7interface::create_popover_menu(){
    auto menu = Gio::Menu::create();
    menu->append("_Save sound", "menu.save_sound");
    menu->append("_Save bank", "menu.save_bank");
    menu->append("_Restore sound", "menu.restore_sound");
    menu->append("_Restore bank", "menu.restore_bank");
    menu->append("_Insert_After", "menu.insert_after");
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

void Dx7interface::create_save_dialog() {
    #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
        file_dialog = get_gwidget<Gtk::FileDialog>("FileDialog_bank_select");
        file_dialog_save = get_gwidget<Gtk::FileDialog>("FileDialog_bank_save");
    #else
        file_dialog = get_gwidget<Gtk::FileChooserDialog>("FileDialog_bank_save");
        file_dialog->add_button("_Open", Gtk::ResponseType::ACCEPT);
        file_dialog->add_button("_Cancel", Gtk::ResponseType::CANCEL);
        file_dialog->set_action(Gtk::FileChooser::Action::OPEN);
        file_dialog_save = get_gwidget<Gtk::FileChooserDialog>("FileDialog_bank_select");
        file_dialog_save->add_button("_Save", Gtk::ResponseType::ACCEPT);
        file_dialog_save->add_button("_Cancel", Gtk::ResponseType::CANCEL);
        file_dialog_save->set_action(Gtk::FileChooser::Action::SAVE);
    #endif
    button_save = get_gwidget<Gtk::Button>("button_save");
    dialog_save = get_gwidget<Gtk::Window>("dialog_save");
    dialog_save->set_default_size(20, 10);
    dialog_save->set_hide_on_close(true);
    dialog_save->set_modal(true);
    file_dialog->set_modal(true);
    file_dialog_save->set_modal(true);
    get_gwidget<Gtk::CheckButton>("checkbutton_128")->set_group(*(get_gwidget<Gtk::CheckButton>("checkbutton_32")));
    get_gwidget<Gtk::CheckButton>("checkbutton_extra_parameters_by_bank")->set_group(*(get_gwidget<Gtk::CheckButton>("checkbutton_extra_parameters_by_sound")));
};

void Dx7interface::OpenFileDialog(){
    file_dialog_save->set_title(dialog_save->get_title());
    Glib::ustring filename;
    uint index = 0;
    bool as_raw = get_gwidget<Gtk::CheckButton>("checkbutton_as_raw")->get_active();
    bool is_32 = get_gwidget<Gtk::CheckButton>("checkbutton_32")->get_active();
    bool is_128 = get_gwidget<Gtk::CheckButton>("checkbutton_128")->get_active();
    write_extra_params = get_gwidget<Gtk::CheckButton>("checkbutton_add_extra_parameters")->get_active();
    if(save_type == SOUND){
        export_config = DX7_1;
        filename = bank_1_modif.sound->name;
    }else if(save_type == BANK){
        filename = bank_1_modif.name;
        if(is_32){
            export_config = DX7_32;
            index = get_gwidget<Gtk::SpinButton>("spinbutton_save_start")->get_value();
        }else if(is_128){
            export_config = DX7_128;
        }else{
            export_config = DX7_1;
        };
    };
    #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
        if(as_raw){
            file_dialog_save->set_initial_name(filename+".dx7");
            export_config = DX7_RAW;
        }else{
            file_dialog_save->set_initial_name(filename+".syx");
        };
        std::cout << "base filename: " << filename << std::endl;
        std::cout << "with format: " << (as_raw ? "Raw" : "Bulk" ) << std::endl;
        if(initial_folder_save==nullptr){
            initial_folder_save=initial_folder_open;
        }
        file_dialog_save->set_initial_folder(initial_folder_save);
        file_dialog_save->save( *(get_window()), [this,index](const Glib::RefPtr<Gio::AsyncResult>& result) {
            try {
                Glib::RefPtr<Gio::File> file = file_dialog_save->save_finish(result);
                if (file) {
                    std::cout << "writing file: " << file->get_path() << std::endl;
                    initial_folder_save = Gio::File::create_for_path(file->get_parent()->get_path());
                    //Glib::shell_quote(filename+".dx7");
                    if(save_type == SOUND){
                        if(get_gwidget<Gtk::CheckButton>("checkbutton_as_raw")->get_active()){
                            write_voice_as_raw(file);
                        }else{
                            write_voice_as_sysex(file);
                        };
                    }else if(save_type == BANK){
                        write_bank(file,index);
                    };
                };
            } catch (const std::exception & ex) {
                std::string err_msg = "From: " + std::string(__PRETTY_FUNCTION__) +
                " Reason: " + ex.what();
                std::cerr << err_msg << std::endl;
            }
        });
    #else
        file_dialog_save->set_transient_for(*(get_window()));
        if(as_raw){
            file_dialog_save->set_current_name(filename+".dx7");
            export_config = DX7_RAW;
        }else{
            file_dialog_save->set_current_name(filename+".syx");
        };
        std::cout << "base filename: " << filename << std::endl;
        std::cout << "with format: " << (as_raw ? "Raw" : "Bulk" ) << std::endl;
        if(initial_folder_save==nullptr){
            initial_folder_save=initial_folder_open;
        }
        file_dialog_save->set_current_folder(initial_folder_save);
        file_dialog_save->signal_response().connect([this,index](int response) {
            try {
                if (response == Gtk::ResponseType::ACCEPT) {
                    auto file = file_dialog_save->get_file();
                    if (file) {
                        if(save_type == SOUND){
                            if(get_gwidget<Gtk::CheckButton>("checkbutton_as_raw")->get_active()){
                                write_voice_as_raw(file);
                            }else{
                                write_voice_as_sysex(file);
                            };
                        }else if(save_type == BANK){
                            write_bank(file,index);
                        };
                    };
                }
                file_dialog_save->hide();
            } catch (const std::exception & ex) {
                std::string err_msg = "From: " + std::string(__PRETTY_FUNCTION__)\
                + "Reason: " + ex.what();
                std::cerr << err_msg << std::endl;
            };
        });
        file_dialog_save->show();
    #endif
};

void Dx7interface::OpenDialog(Glib::ustring title,Glib::ustring filename){
    try{
        dialog_save->set_transient_for(*(get_window()));
        dialog_save->set_title(title);
        Glib::ustring save_label=title+": "+filename;
        get_gwidget<Gtk::Label>("label_save_name")->set_label(save_label);
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
        dialog_save->present();
    }catch (const std::exception & ex) {
        std::string err_msg = "from: " + std::string(__PRETTY_FUNCTION__)\
        + "Reason: " + ex.what();
        //throw std::runtime_error(err_msg);
    };
};

/*** BANK ***/
void Dx7interface::on_bank_select(){
    LOG_IN();
    try{
        file_dialog->set_title("Select bank");
        #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
            file_dialog->set_initial_folder(initial_folder_open);
            file_dialog->open( *(get_window()), [this](const Glib::RefPtr<Gio::AsyncResult>& result ) {
                try {
                    Glib::RefPtr<Gio::File> bank_file = file_dialog->open_finish(result);
                    if (bank_file) {
                        set_bank(bank_file);
                        initial_folder_open = Gio::File::create_for_path(bank_file->get_parent()->get_path());
                    };
                } catch (const std::exception & ex) {
                    std::string err_msg = "From: " + std::string(__PRETTY_FUNCTION__)\
                    + "Reason: " + ex.what();
                    std::cerr << err_msg << std::endl;
                };
            }
            ); /* end dialog open function */
        #else
            file_dialog->set_transient_for(*(get_window()));
            file_dialog->signal_response().connect([this](int response) {
                try {
                    if (response == Gtk::ResponseType::ACCEPT) {
                        auto bank_file = file_dialog->get_file();
                        if (bank_file) {
                            set_bank(bank_file);
                            initial_folder_open= Gio::File::create_for_path(bank_file->get_path());;
                        };
                    };
                    file_dialog->hide();
                } catch (const std::exception & ex) {
                    std::string err_msg = "From: " + std::string(__PRETTY_FUNCTION__)\
                    + "Reason: " + ex.what();
                    std::cerr << err_msg << std::endl;
                };
            });
            file_dialog->show();
        #endif
    }catch (const std::exception & ex) {
        std::string err_msg = "from: " + std::string(__PRETTY_FUNCTION__)\
        + "Reason: " + ex.what();
        //throw std::runtime_error(err_msg);
    };
    LOG_OUT();
};
void Dx7interface::on_columnview_right_click(int n_press, double x, double y){
    LOG_IN();
        m_popover_menu->set_pointing_to(Gdk::Rectangle(x, y, 1, 1));
        m_popover_menu->popup();
    LOG_OUT();
};
/* set/load */
void Dx7interface::set_bank(Glib::RefPtr<Gio::File> bank_file){
    old_snum=0;
    //for (auto i : bank_file->query_info()->list_attributes())
    ////    std::cout << i << std::endl;
    //std::cout << bank_file->get_parent()->get_path() << std::endl;
    clean_bank();
    load_bank(bank_file);
    bank_selection_model->set_selected(0);
    redraw_all_curve();
    Glib::ustring filename = (bank_file->query_info(G_FILE_ATTRIBUTE_STANDARD_NAME))->get_name();
    Glib::ustring name = filename.substr(0,filename.find_last_of("."));
    get_gwidget<Gtk::Button>("bank_select")->set_label(name);
};
void Dx7interface::clean_bank(){
    LOG_IN();
    int i;
    clear_sound(&bank_1_modif.sound[0],0,false);
    clear_sound(&bank_1_origin.sound[0],0,false);
    for (i = 31; i >= 0; i-- ){
        clear_sound(&bank_32_modif.sound[i],i,false);
        clear_sound(&bank_32_origin.sound[i],i,false);
    }
    for (i = 127; i >= 0; i-- ){
        clear_sound(&bank_128_modif.sound[i],i,false);
        clear_sound(&bank_128_origin.sound[i],i,false);
    }
    /* by file */
    /*load_bank(Gio::File::create_for_path(DATA_DIR"/reset1.syx"));
    load_bank(Gio::File::create_for_path(DATA_DIR"/reset32.syx"));
    load_bank(Gio::File::create_for_path(DATA_DIR"/reset128.syx"));*/
    uint n_items = bank_data_model->get_n_items();
    if (n_items != 0) {
        bank_data_model->remove_all();
    };
    LOG_OUT();
};
void Dx7interface::load_bank(Glib::RefPtr<Gio::File> bank_file){
    LOG_IN();
    /* open file */
    try {
        uint8_t i;
        data_stream = Gio::DataInputStream::create(bank_file->read());
        uint file_size = (bank_file->query_info(G_FILE_ATTRIBUTE_STANDARD_SIZE))->get_size();
        Glib::ustring filename = bank_file->get_path();
        Glib::ustring bank_file_base = filename.substr(0,filename.find_last_of("."));
        Glib::ustring bank_name = bank_file_base.substr( bank_file_base.find_last_of("/")+1, bank_file_base.length() );
        std::cout << "Bank name: " << bank_name << std::endl;
        u_char data = data_stream->read_byte();
        if (data == 0xF0 ){
            for ( i=0; i < 5; i++){
                data_stream->read_byte();
            };
            file_size -= 8;
        }else{
            data_stream->close();
            data_stream = Gio::DataInputStream::create(bank_file->read());
            file_size = (bank_file->query_info(G_FILE_ATTRIBUTE_STANDARD_SIZE))->get_size();
        };
        if( std::filesystem::exists( (bank_file_base+"_fct.syx").c_str() ) ){
            Glib::RefPtr<Gio::File> file=Gio::File::create_for_path( (bank_file_base+"_fct.syx").c_str() );
            data_stream_param = Gio::DataInputStream::create(file->read());
        }
        switch ( file_size ){
            case 128: /* one voice */
                i = 0;
                seek_voice(i, &bank_1_origin.sound[i]);
                seek_parameters(bank_file_base, &bank_1_origin.sound[i]);
                bank_1_origin.name = bank_name;
                bank_1_modif=bank_1_origin;
                bank_nb_sound = 1;
                break;
            case 155: /* one voice Dx7 bulk 1 */
                i = 0;
                seek_voice_by_byte(i, &bank_1_origin.sound[i]);
                seek_parameters(bank_file_base, &bank_1_origin.sound[i]);
                bank_1_origin.name = bank_name;
                bank_1_modif=bank_1_origin;
                bank_nb_sound = 1;
                break;
            case 4096: /* 32 voices Dx7 bulk 32 */
                for( i = 0; i < 32; i++ ){
                    seek_voice(i,&bank_32_origin.sound[i]);
                    seek_parameters(bank_file_base, &bank_32_origin.sound[i]);
                };
                bank_32_origin.name = bank_name;
                bank_32_modif=bank_32_origin;
                bank_1_origin.sound[0]=bank_32_origin.sound[0];
                bank_1_origin.name = bank_name;
                bank_1_modif=bank_1_origin;
                bank_nb_sound = 32;
                break;
            case 16384: /* 128 voices */
                for( i = 0; i < 128; i++ ){
                    seek_voice(i, &bank_128_origin.sound[i]);
                    seek_parameters(bank_file_base, &bank_128_origin.sound[i]);
                };
                bank_128_origin.name = bank_name;
                bank_128_modif=bank_128_origin;
                bank_1_origin.sound[0]=bank_128_origin.sound[0];
                bank_1_origin.name = bank_name;
                bank_1_modif=bank_1_origin;
                bank_nb_sound = 128;
                break;
        };
        data_stream->close();
        if(!isStreamClosed(data_stream_param)){
            data_stream_param->close();
        };
    }catch(const std::exception& ex){
        Glib::ustring filename = (bank_file->query_info(G_FILE_ATTRIBUTE_STANDARD_NAME))->get_name();
        std::string err_msg = "from: " + std::string(__PRETTY_FUNCTION__)\
                            + "Cannot load: " + filename + "\n"
                            + "Reason: " + ex.what();
        throw std::runtime_error(err_msg);
    };
    LOG_OUT();
};
/* restore */
void Dx7interface::on_restore_bank(){
    LOG_IN();
    restore_origin_bank();
    set_voice(&bank_1_modif.sound[0]);
    redraw_all_curve();
    LOG_OUT();
};
void Dx7interface::restore_origin_bank(){
    // set bank_X_modif.sound[snum] in bank_X_origin.sound[snum]
    switch ( bank_nb_sound ){
        case 1:
            std::cout << "Restore bank: " << bank_1_modif.name << " from origin bank." << std::endl;
            bank_1_modif = bank_1_origin;
            break;
        case 32:
            std::cout << "Restore bank: " << bank_32_modif.name << " from origin bank." << std::endl;
            bank_32_modif = bank_32_origin;
            bank_1_origin.sound[0] = bank_32_origin.sound[old_snum];
            break;
        case 128:
            std::cout << "Restore bank: " << bank_128_modif.name << " from origin bank." << std::endl;
            bank_128_modif = bank_128_origin;
            bank_1_origin.sound[0] = bank_128_origin.sound[old_snum];
            break;
    };
    bank_1_modif = bank_1_origin;
};
void Dx7interface::on_restore_sound(){
    LOG_IN();
    // TODO: get index pointed by cursor
    restore_origin_sound();
    set_voice(&bank_1_modif.sound[0]);
    redraw_all_curve();
    LOG_OUT();
};

void Dx7interface::restore_origin_sound(){
    // set bank_X_origin.sound[snum] in bank_X_modif.sound[snum] and use it
    std::cout << "Restore voice: " << bank_1_modif.sound->name << " from origin bank." << std::endl;

    switch ( bank_nb_sound ){
        case 32:
            bank_32_modif.sound[old_snum] = bank_32_origin.sound[old_snum];
            bank_1_origin.sound[0] = bank_32_origin.sound[old_snum];
            bank_1_modif.sound[0] = bank_32_origin.sound[old_snum];
            break;
        case 128:
            bank_128_modif.sound[old_snum] = bank_128_origin.sound[old_snum];
            bank_1_origin.sound[0] = bank_128_origin.sound[old_snum];
            bank_1_modif.sound[0] = bank_128_origin.sound[old_snum];
            break;
    };
    bank_1_modif = bank_1_origin;
};
/* insert/replace/delete */
void Dx7interface::on_insert_after(){
};
void Dx7interface::on_replace_sound(){
};
void Dx7interface::on_delete_sound(){
};
/*** save/write ***/
void Dx7interface::write_file(Glib::RefPtr<Gio::File> file, u_char* msg, uint msg_size){
    auto output_stream = file->replace();
    auto data_stream = Gio::DataOutputStream::create(output_stream);
    for( uint i=0; i<msg_size; i++ ){
        data_stream->put_byte(msg[i]);
    };
    data_stream->flush();
    data_stream->close();
    output_stream->close();
};
/** BANK **/
void Dx7interface::on_save_bank(){
    LOG_IN();
    try{
        Glib::ustring title = "Saving Bank";
        save_type = BANK;
        save_modif_sound();
        OpenDialog(title, bank_1_modif.name);
    }catch (const std::exception & ex) {
        std::string err_msg = "from: " + std::string(__PRETTY_FUNCTION__)\
        + "Reason: " + ex.what();
        //throw std::runtime_error(err_msg);
    };
    LOG_OUT();
};
void Dx7interface::write_bank(Glib::RefPtr<Gio::File> file, uint index){
    LOG_IN();
    switch(export_config){
        case DX7_1:        /* write bulk1 bank */
        case DX7_32:       /* write bulk32 bank */
            write_bank_as_sysex(file,index);
            break;
        case DX7_128: {    /* write 4x bulk32 bank */
            Glib::ustring filename = file->get_path();
            Glib::ustring bank_file_base = filename.substr(0,filename.find_last_of("."));
            for(uint i = 0; i < 127; i=i+32){
                file = Gio::File::create_for_path( (bank_file_base+"_"+tostr<int>(i)+"_"+tostr<int>(i+31)+".syx").c_str() );
                write_bank_as_sysex(file,i);
            }
            break;
        }
        case DX7_RAW: {     /* write raw bank */
            /* sound without sysex headers (HEXTER 128) */
            write_bank_as_raw(file,index);
            break;
        }
    };
    LOG_OUT();

};
/* Dx7 format bulk sysex */
void Dx7interface::write_bank_as_sysex(Glib::RefPtr<Gio::File> file,uint index){
    LOG_IN();
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
        uint l=6, msg_size;
        Bank_ptr bank_ptr;
        u_char msb,lsb,format_nb;
        switch ( bank_nb_sound ){
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
        u_char msg[msg_size];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=0x00 & channel_send;
        msg[3]=format_nb;
        msg[4]=msb;
        msg[5]=lsb;
        if(bank_nb_sound == 1){
            write_voice_bulk1(&l, msg, &bank_ptr->sound[0], &voice_checksum );
        }else{
            for (uint i=index; i<(32+index); i++){
                write_voice_bulk32(&l, msg, &bank_ptr->sound[i], &voice_checksum );
            };
        };
        msg[l++]=u_char(voice_checksum & 0x7F) ;
        msg[l]=0xF7;
        write_file(file,msg,msg_size);
    } catch (const std::exception& ex) {
        std::cerr << "Error writing to file: " << ex.what() << std::endl;
    }
    /*std::cout << "varaiable l: " << (int)l << std::endl ;
    for (uint i = 0 ; i < size ; i++){
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)(msg[i] & 0xFF)<< " " ;
    };*/
    std::cout << std::dec << std::endl;
    LOG_OUT();
};

void Dx7interface::write_bank_as_raw(Glib::RefPtr<Gio::File> file, uint index){
    /* index to export from a sound number */
    LOG_IN();
    try{
        uint8_t voice_checksum = 0;
        uint l=0, msg_size, msg_extra_size, msg_extra_index=0;
        Bank_ptr bank_ptr;
        switch ( bank_nb_sound ){
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
        u_char msg[msg_size];
        u_char msg_extra[msg_extra_size];
        if(bank_nb_sound == 1){
            write_voice_bulk1(&l, msg, &bank_ptr->sound[0], &voice_checksum );
            write_voice_extra_parameters(&bank_ptr->sound[0],msg_extra,&msg_extra_index);
        }else{
            for (uint i=index, msg_extra_index=0; i<((msg_size/128)+index); i++){
                write_voice_bulk32(&l, msg, &bank_ptr->sound[i], &voice_checksum );
                write_voice_extra_parameters(&bank_ptr->sound[i],msg_extra,&msg_extra_index);
            };
        };
        write_file(file,msg,msg_size);
        if(write_extra_params){
            Glib::ustring file_full = file->get_path();
            Glib::ustring file_base = file_full.substr(0,file_full.find_last_of("."));
            Glib::RefPtr<Gio::File> file_extra=Gio::File::create_for_path( (file_base+"_fct.syx").c_str() );
            write_file(file_extra,msg_extra,msg_extra_size);
        };
    } catch (const std::exception& ex) {
        std::cerr << "Error writing to file: " << ex.what() << std::endl;
    }
    /*for (uint i = 0 ; i < size ; i++){
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)(msg[i] & 0xFF)<< " " ;
    };
    std::cout << std::dec << std::endl;
    std::cout << "varaiable l: " << (int)l << std::endl ;
    std::cout << "DX_: " << (int)export_config << std::endl ;
    std::cout << "index: " << (int)index << std::endl ;
    std::cout << "file: " << file->get_path() << std::endl ;
    std::cout << "size: " << size << std::endl ;*/
    LOG_OUT();
};
/** VOICE **/
void Dx7interface::write_voice_bulk1(uint* l, u_char* msg, St_dx7sysex_1* sound, uint8_t* voice_checksum){
    /* operator j */
    uint j, k;
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
};
void Dx7interface::write_voice_bulk32(uint* l, u_char* msg, St_dx7sysex_1* sound, uint8_t* voice_checksum){
    /* TODO : check original sound format and other kind */
    uint j, k;
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
        msg[(*l)++] = ( (sound->op[j].kls.rght_curve.val << 2) + (sound->op[j].kls.lft_curve.val & 0x03) ) & 0x0F ;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = ( (sound->op[j].dtun.val << 3) + (sound->op[j].krs.val & 0x07) ) & 0x7F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = ( (sound->op[j].kvs.val << 2) + (sound->op[j].ams.val & 0x03) ) & 0x1F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = sound->op[j].lvl.val & 0x7F;
        *voice_checksum -= msg[(*l)-1];
        msg[(*l)++] = ( ( sound->op[j].freq_coarse.val << 1) + (sound->op[j].freq_mode.val & 0x01)  ) & 0x3F;
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
    msg[(*l)++] = ( (sound->algo.oks.val << 3) + (sound->algo.feedback.val & 0x07) ) & 0x0F;
    *voice_checksum -= msg[(*l)-1];

    msg[(*l)++] = sound->lfo.speed.val & 0x7F;
    *voice_checksum -= msg[(*l)-1];
    msg[(*l)++] = sound->lfo.delay.val & 0x7F;
    *voice_checksum -= msg[(*l)-1];
    msg[(*l)++] = sound->lfo.pmd.val & 0x7F;
    *voice_checksum -= msg[(*l)-1];
    msg[(*l)++] = sound->lfo.amd.val & 0x7F;
    *voice_checksum -= msg[(*l)-1];
    msg[(*l)++] = ( ( sound->lfo.pms.val << 4) + ( (sound->lfo.wave.val & 0x07) <<1 ) + (sound->lfo.sync.val & 0x01) ) & 0x7F;
    *voice_checksum -= msg[(*l)-1];
    msg[(*l)++] = sound->algo.transpose.val & 0x7F;
    *voice_checksum -= msg[(*l)-1];

    for (uint8_t carac = 0 ; carac <= 9 ; carac++ ){
        msg[(*l)++] = sound->name.data()[carac];
    };
    *voice_checksum -= msg[(*l)-1];
};
void Dx7interface::write_voice_as_sysex(Glib::RefPtr<Gio::File> file){
    try {
        uint8_t voice_checksum=0;
        uint l = 6;
        uint msg_size=163;
        u_char msg[msg_size];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=0x00 & channel_send;
        msg[3]=0x00;
        msg[4]=0x01;
        msg[5]=0x1B;
        write_voice_bulk1(&l, msg, &bank_1_modif.sound[0], &voice_checksum );
        msg[161]=u_char(voice_checksum & 0x7F);
        msg[162]=0xF7;
        write_file(file,msg,msg_size);
    } catch (const std::exception& ex) {
        std::cerr << "Error writing to file: " << ex.what() << std::endl;
    }
};
void Dx7interface::write_voice_as_raw(Glib::RefPtr<Gio::File> file){
try {
    uint8_t voice_checksum=0;
    uint l = 0;
    uint msg_size = 155;
    u_char msg[msg_size];
    write_voice_bulk1(&l, msg, &bank_1_modif.sound[0], &voice_checksum );
    write_file(file,msg,msg_size);
} catch (const std::exception& ex) {
    std::cerr << "Error writing to file: " << ex.what() << std::endl;
}
};

void Dx7interface::write_voice_extra_parameters(st_dx7sysex_1* sound,u_char* msg, uint* index){
    LOG_IN();
    if(write_extra_params){
        try{
            // size = 98
            uint i, j;
            uint nb_elem = 14;
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
                 *              std::cout << "char j: " << std::hex << (int)j << std::dec << std::endl;
                 *              std::cout << "i: " << (int)i << std::endl;
                 *              std::cout << "sound val: " << std::hex << (int)val[i] << std::dec << std::endl;
                 */
                msg[(*index)++]=0xF0;
                msg[(*index)++]=id_fabricant;
                msg[(*index)++]=sub_status & channel_send;
                msg[(*index)++]=0x08;
                msg[(*index)++]=j;
                msg[(*index)++]=val[i];
                msg[(*index)++]=0xF7;
            };
        } catch (const std::exception& ex) {
            std::cerr << "Error writing to file: " << ex.what() << std::endl;
        };
    };
    LOG_OUT();
};
void Dx7interface::save_modif_sound(){
    std::cout << "Save voice: " << bank_1_modif.sound->name << " in modif bank and export file." << std::endl;
    switch ( bank_nb_sound ){
        case 32:
            bank_32_modif.sound[old_snum] = bank_1_modif.sound[0];
            //bank_32_origin.sound[old_snum] = bank_32_modif.sound[old_snum];
            bank_1_modif.sound[0] = bank_32_modif.sound[old_snum];
            break;
        case 128:
            bank_128_modif.sound[old_snum] = bank_1_modif.sound[0];
            //bank_128_origin.sound[old_snum] = bank_128_modif.sound[old_snum];
            bank_1_modif.sound[0] = bank_128_modif.sound[old_snum];
            break;
    };
    bank_1_origin.sound[0] = bank_1_modif.sound[0];
};
void Dx7interface::on_save_sound(){
    LOG_IN();
    try{
        save_modif_sound();
        Glib::ustring title = "Saving sound";
        save_type = SOUND;
        OpenDialog(title, bank_1_modif.sound->name);
    }catch (const std::exception & ex) {
        std::string err_msg = "from: " + std::string(__PRETTY_FUNCTION__)\
        + "Reason: " + ex.what();
        //throw std::runtime_error(err_msg);
    };
    LOG_OUT();
};
/* extended sysex */
void Dx7interface::write_voices_as_n_sysex(St_dx7sysex_1* sound){
    /* TODO : check original sound format and other kind */
    uint8_t l=0, j, k;
    u_char msg[128];
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
        msg[l++] = ( (sound->op[j].kls.lft_curve.val << 2) + (sound->op[j].kls.rght_curve.val & 0x03) ) & 0x0F ;
        msg[l++] = ( (sound->op[j].dtun.val << 3) + (sound->op[j].krs.val & 0x07) ) & 0x7F;
        msg[l++] = ( (sound->op[j].kvs.val << 2) + (sound->op[j].ams.val & 0x03) ) & 0x1F;
        msg[l++] = sound->op[j].lvl.val & 0x7F;
        msg[l++] = ( ( sound->op[j].freq_coarse.val << 1) + (sound->op[j].freq_mode.val & 0x01)  ) & 0x3F;
        msg[l++] = sound->op[j].freq_fine.val & 0x7F;
    };
    for(j=0 ; j < 4; j++ ){
        msg[l++] = sound->pitch.eg_rt[j].val & 0x7F;
    };
    for(j=0 ; j < 4; j++ ){
        msg[l++] = sound->pitch.eg_lvl[j].val & 0x7F;
    };
    msg[l++] = sound->algo.algo.val & 0x1F;
    msg[l++] = ( (sound->algo.oks.val << 3) + (sound->algo.feedback.val & 0x07) ) & 0x0F;

    msg[l++] = sound->lfo.speed.val & 0x7F;
    msg[l++] = sound->lfo.delay.val & 0x7F;
    msg[l++] = sound->lfo.pmd.val & 0x7F;
    msg[l++] = sound->lfo.amd.val & 0x7F;
    msg[l++] = ( ( sound->lfo.pms.val << 4) + ( (sound->lfo.wave.val & 0x07) <<1 ) + (sound->lfo.sync.val & 0x01) ) & 0x7F;
    msg[l++] = sound->algo.transpose.val & 0x7F;

    for (uint8_t carac = 0 ; carac <= 9 ; carac++ ){
        msg[l++] = sound->name.data()[carac];
    };
    std::cout << "Voice Output: ";
    for (int i = 0 ; i < 128 ; i++){
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)(msg[i] & 0x7F)<< " " ;
    };
    std::cout << std::dec << std::endl;
};

/** Save current sound on bank_X_modif and load set_selected sound from bank_X_modif**/
void Dx7interface::on_selected_sound_change(uint num, uint nb_elmnt){
    LOG_IN();
    auto snum = bank_selection_model->get_selected();
    switch ( bank_nb_sound ){
        case 32:
            bank_32_modif.sound[old_snum]= bank_1_modif.sound[0];
            bank_1_modif.sound[0]= bank_32_modif.sound[snum];
            bank_1_origin.sound[0]= bank_32_origin.sound[snum];
            break;
        case 128:
            bank_128_modif.sound[old_snum]= bank_1_modif.sound[0];
            bank_1_modif.sound[0]= bank_128_modif.sound[snum];
            bank_1_origin.sound[0]= bank_128_origin.sound[snum];
            break;
    };
    old_snum = snum;
    (get_gwidget<Gtk::ToggleButton>("btn_compare"))->set_active(false);
    set_voice(&bank_1_modif.sound[0]);
    send_voice(&bank_1_modif.sound[0]);
    redraw_all_curve();
    LOG_OUT();
};

/*** VOICE ***/
/* read: seek (in bank file)*/
void Dx7interface::seek_voice(uint8_t i, St_dx7sysex_1* sound){ /* BULK 32 */
    //LOG_IN();
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
    bank_data_model->append(SoundBankItem::create(i,sound->name));
    //LOG_OUT();
};
void Dx7interface::seek_voice_by_byte(uint8_t i, St_dx7sysex_1* sound){ /* BULK 1 */
    //LOG_IN();
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
    bank_data_model->append(SoundBankItem::create(i,sound->name));
    //LOG_OUT();
};
void Dx7interface::seek_parameters(Glib::ustring bank_file_base, St_dx7sysex_1* sound){
    /*Glib::RefPtr<Gio::File> file = nullptr;
    Glib::ustring bank_file_full = basefile->get_path();
    Glib::ustring bank_file_path = bank_file_full.substr(0,bank_file_full.find_last_of("/")+1);*/
    //std::cout << bank_file_base + "_fct.syx" << std::endl;
    //std::cout << bank_file_path + sound->name.c_str() + "_fct.syx" << std::endl;
    //
    /*if( std::filesystem::exists( (bank_file_base+"_fct.syx").c_str() ) ){
        file=Gio::File::create_for_path( (bank_file_base+"_fct.syx").c_str() );
        Glib::RefPtr<Gio::DataInputStream> data_stream_param = Gio::DataInputStream::create(file->read());
    }else*/
    if( isStreamClosed(data_stream_param) && std::filesystem::exists( (bank_file_base + sound->name.c_str() + "_fct.syx").c_str() ) ){
    //if( std::filesystem::exists( (bank_file_base + sound->name.c_str() + "_fct.syx").c_str() ) ){
        Glib::RefPtr<Gio::File> file=Gio::File::create_for_path( (bank_file_base + sound->name.c_str() + "_fct.syx").c_str() );
        std::cout << "For voice: " << sound->name.c_str() << "\tParameter Function read from file: " << file->get_path() << std::endl;
        data_stream_param = Gio::DataInputStream::create(file->read());
        seek_voice_parameters(sound);
        data_stream_param->close();
    }else{
        seek_voice_parameters(sound);
    };
};

bool Dx7interface::isStreamClosed(Glib::RefPtr<Gio::DataInputStream>& stream) {
    try {
        if(stream){
            return false;
        }else{
            return true;
        };
    } catch (const Gio::Error& e) {
        if (e.code() == Gio::Error::CLOSED) {
            return true; // Stream is closed
        }
        return true;
    }
}

void Dx7interface::seek_voice_parameters(St_dx7sysex_1* sound){
    if(!isStreamClosed(data_stream_param)){
        /* skip */
        //for (uint i=0; i<(pos*98); data_stream_param->read_byte(),i++);
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

/* read: set (in ui from struct) */
void Dx7interface::set_voice(St_dx7sysex_1* sound){ LOG_IN();
    /* set value in each widget from modif */
    uint8_t j,k;
    /* general */
    block_ui();
    block_midi();
    (get_window())->set_title(Glib::ustring(MODULE_NAME) + ": "+sound->name.c_str());
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
    (get_gwidget<Gtk::SpinButton>("speed"))->set_value(sound->lfo.speed.val);
    (get_gwidget<Gtk::SpinButton>("delay"))->set_value(sound->lfo.delay.val);
    (get_gwidget<Gtk::SpinButton>("pmd"))->set_value(sound->lfo.pmd.val);
    (get_gwidget<Gtk::SpinButton>("amd"))->set_value(sound->lfo.amd.val);
    /* LFO modulation */
    (get_gwidget<Gtk::Scale>("pms"))->set_value(sound->lfo.pms.val);
    /* PITCH EG */
    for ( k = 0; k < 4 ; k++ ){
        get_gwidget<Gtk::SpinButton>("eg_rt" + tostr<uint>(k+1) + "_pitch")->set_value(sound->pitch.eg_rt[k].val);
    };
    for ( k = 0; k < 4 ; k++ ){
        get_gwidget<Gtk::SpinButton>("eg_lvl"+tostr<uint>(k+1)+"_pitch")->set_value(sound->pitch.eg_lvl[k].val);
    };
    /* OPERATEUR j+1 */
    uint8_t mute_val=sound->extra.mute.val & 0x7F; // get mute status from extra struct;
    for ( j=0;j<6;j++){
        /* AMS */
        (get_gwidget<Gtk::Scale>("ams_op"+tostr<uint>(j+1)))->set_value(sound->op[j].ams.val);
        /* FREQUENCE */
        (get_gwidget<Gtk::DropDown>("freq_mode_op"+tostr<uint>(j+1)))->set_selected(sound->op[j].freq_mode.val);
        (get_gwidget<Gtk::SpinButton>("freq_coarse_op"+tostr<uint>(j+1)))->set_value(sound->op[j].freq_coarse.val);
        (get_gwidget<Gtk::SpinButton>("freq_fine_op"+tostr<uint>(j+1)))->set_value(sound->op[j].freq_fine.val);
        (get_gwidget<Gtk::Scale>("dtun_op"+tostr<uint>(j+1)))->set_value(sound->op[j].dtun.val-7);
        /* DRAWING AREA */

        /* OP[J] EG RT[k] */
        for ( k = 0; k < 4 ; k++ ){
            (get_gwidget<Gtk::SpinButton>("eg_rt"+tostr<uint>(k+1)+"_op"+tostr<uint>(j+1)))->set_value(sound->op[j].eg_rt[k].val);
        };
        /* OP[J] EG LVL[k] */
        for ( k = 0; k < 4 ; k++ ){
            (get_gwidget<Gtk::SpinButton>("eg_lvl"+tostr<uint>(k+1)+"_op"+tostr<uint>(j+1)))->set_value(sound->op[j].eg_lvl[k].val);
        };
        /* KRS */
        (get_gwidget<Gtk::Scale>("krs_op"+tostr<uint>(j+1)))->set_value(sound->op[j].krs.val);
        /* KVS */
        (get_gwidget<Gtk::Scale>("kvs_op"+tostr<uint>(j+1)))->set_value(sound->op[j].kvs.val);
        /* LVL */
        (get_gwidget<Gtk::SpinButton>("lvl_op"+tostr<uint>(j+1)))->set_value(sound->op[j].lvl.val);
        /* MUTE */
        /* NO MUTE VALUE IN STD SYSEX CAN BE ADD IN LEFT SPACE */
        u_int8_t muted = mute_val;
        muted = ( muted >> ( 5 - j ) );
        (get_gwidget<Gtk::ToggleButton>("mute_op"+tostr<uint>(j+1)))->set_active(!(muted & 0x01));
        /* KLS */
        (get_gwidget<Gtk::DropDown>("kls_lft_curve_op"+tostr<uint>(j+1)))->set_selected(sound->op[j].kls.lft_curve.val);
        (get_gwidget<Gtk::DropDown>("kls_rght_curve_op"+tostr<uint>(j+1)))->set_selected(sound->op[j].kls.rght_curve.val);
        (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op"+tostr<uint>(j+1)))->set_value(sound->op[j].kls.lft_dpth.val);
        (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op"+tostr<uint>(j+1)))->set_value(sound->op[j].kls.rght_dpth.val);
        (get_gwidget<Gtk::DropDown>("note_brk_pt_op"+tostr<uint>(j+1)))->set_selected(sound->op[j].kls.brk_pt.val % 12);
        int val = ((sound->op[j].kls.brk_pt.val) -3);
        if(val < 0){
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op"+tostr<uint>(j+1)))->set_value( -1 );
        }else{
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op"+tostr<uint>(j+1)))->set_value( val / 12 );
        };
    };
    set_voice_parameters(sound);
    unblock_midi();
    unblock_ui();
    on_txt_freq_op_event();
    LOG_OUT();
};
void Dx7interface::set_voice_parameters(St_dx7sysex_1* sound){
    (get_gwidget<Gtk::ToggleButton>("btn_poly_mono"))->set_active(sound->extra.functions.poly_mono.val);
    (get_gwidget<Gtk::Scale>("ptch_bnd_rng"))->set_value(sound->extra.functions.ptch_bnd_rng.val);
    (get_gwidget<Gtk::Scale>("ptch_bnd_stp"))->set_value(sound->extra.functions.ptch_bnd_stp.val);
    (get_gwidget<Gtk::ToggleButton>("btn_portamento_md"))->set_active(sound->extra.functions.portamento_md.val);
    (get_gwidget<Gtk::ToggleButton>("btn_portamento_glss"))->set_active(sound->extra.functions.portamento_glss.val);
    (get_gwidget<Gtk::SpinButton>("portamento_tm"))->set_value(sound->extra.functions.portamento_tm.val);

    (get_gwidget<Gtk::SpinButton>("md_whl_rng"))->set_value(sound->extra.functions.md_whl_rng.val);
    u_char val = sound->extra.functions.md_whl_assgn.val;
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
/* clear (struct) */
void Dx7interface::clear_sound(St_dx7sysex_1* sound,uint8_t pos,bool remove){
    //LOG_IN();
    uint8_t j,k;
    /* operator j */
    for ( j = 6; j-- != 0 ; ){
        /* OP[J] EG RATE[k] */
        for ( k = 0; k < 4 ; k++ ){
            sound->op[j].eg_rt[k].val=0x00 & 0x7F;
        };
        /* OP[J] EG LVL[k] */
        for ( k = 0; k < 4 ; k++  ){
            sound->op[j].eg_lvl[k].val=0x00 & 0x7F;
        };
        sound->op[j].kls.brk_pt.val=0x00 & 0x7F;
        sound->op[j].kls.lft_dpth.val=0x00 & 0x7F;
        sound->op[j].kls.rght_dpth.val=0x00 & 0x7F;
        sound->op[j].kls.lft_curve.val=0x00 & 0x0F ;
        sound->op[j].kls.rght_curve.val=0x00 & 0x0F;
        sound->op[j].krs.val=0x00 & 0x0F;
        sound->op[j].dtun.val=0x00 & 0x0F ;
        sound->op[j].ams.val=0x00 & 0x0F;
        sound->op[j].kvs.val=0x00 & 0x0F;
        sound->op[j].lvl.val=0x00 & 0x7F;
        sound->op[j].freq_mode.val=0x00 & 0x0F;
        sound->op[j].freq_coarse.val=0x00 & 0x0F;
        sound->op[j].freq_fine.val=0x00 & 0x7F;
    };
    for( j=0 ; j < 4; j++ ){
        sound->pitch.eg_rt[j].val=0x00 & 0x7F;
    };
    for( j=0 ; j < 4; j++ ){
        sound->pitch.eg_lvl[j].val=0x00 & 0x7F;
    };
    sound->algo.algo.val=0x00 & 0x1F ;
    sound->algo.feedback.val=0x00 & 0x0F;
    sound->algo.oks.val=0x00 & 0x0F;
    sound->lfo.speed.val=0x00 & 0x7F;
    sound->lfo.delay.val=0x00 & 0x7F;
    sound->lfo.pmd.val=0x00 & 0x7F;
    sound->lfo.amd.val=0x00 & 0x7F;
    sound->lfo.sync.val=0x00 & 0x0F;
    sound->lfo.wave.val=0x00 & 0x0F;
    sound->lfo.pms.val=0x00 & 0x0F;
    sound->algo.transpose.val=0x00 & 0x7F;
    std::ostringstream strm;
    for( j=0; j <= 9; j++ ){
        strm << (0x00);
    };
    sound->name=strm.str();
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
    /* remove voice name from liststore */
    if(remove){
        bank_data_model->remove(pos);
    };
    //LOG_OUT();
};
/* write: send (to midi) */
void Dx7interface::send_voice(st_dx7sysex_1* sound){
    LOG_IN();
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
    u_char msg[163];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=0x00 & channel_send;
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
    sound->sum = u_char(voice_checksum & 0x7F) ;
    msg[161]=sound->sum;
    msg[162]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 163, msg);
    /* extra parameters */
    send_extra_parameters(sound);
    /* send mute for dx and hexter */
    on_mute_op_event();
    LOG_OUT();
};
void Dx7interface::send_extra_parameters(st_dx7sysex_1* sound){
    LOG_IN();
    if(send_extra_params){
        uint8_t i, j;
        uint nb_elem = 14;
        u_char msg[7];
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
                std::cout << "char j: " << std::hex << (int)j << std::dec << std::endl;
                std::cout << "i: " << (int)i << std::endl;
                std::cout << "sound val: " << std::hex << (int)val[i] << std::dec << std::endl;
            */
            msg[0]=0xF0;
            msg[1]=id_fabricant;
            msg[2]=sub_status & channel_send;
            msg[3]=0x08;
            msg[4]=j;
            msg[5]=val[i];
            msg[6]=0xF7;
            send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
        };
    };
    LOG_OUT();
};

/**** UI SIGNALS CONNECTION ****/
/* fold/unfold bank's sounds list */
void Dx7interface::on_bank_reveal(){
    LOG_IN();
    (get_gwidget<Gtk::Revealer>("revealer_bank"))->set_reveal_child(!(get_gwidget<Gtk::Revealer>("revealer_bank"))->get_reveal_child());
    LOG_OUT();
};

/* bank view / columnview population functions */
void Dx7interface::on_bind_num(const Glib::RefPtr<Gtk::ListItem>& list_item){
    auto col = std::dynamic_pointer_cast<SoundBankItem>(list_item->get_item());
    if (!col){ return; };
    auto label = dynamic_cast<Gtk::Label*>(list_item->get_child());
    if (!label){ return; };
    label->set_text(Glib::ustring::format(col->get_number()));
};
void Dx7interface::on_bind_name(const Glib::RefPtr<Gtk::ListItem>& list_item){
    auto col = std::dynamic_pointer_cast<SoundBankItem>(list_item->get_item());
    if (!col){ return; };
    auto label = dynamic_cast<Gtk::Label*>(list_item->get_child());
    if (!label){ return; };
    label->set_text(col->get_name());
};
void Dx7interface::on_setup_label(const Glib::RefPtr<Gtk::ListItem>& list_item, Gtk::Align halign){
    list_item->set_child(*Gtk::make_managed<Gtk::Label>("", halign));
};

void Dx7interface::on_bind_param_name(const Glib::RefPtr<Gtk::ListItem>& list_item){
    auto col = std::dynamic_pointer_cast<ParamItem>(list_item->get_item());
    if (!col){ return; };
    auto label = dynamic_cast<Gtk::Label*>(list_item->get_child());
    if (!label){ return; };
    label->set_text(col->get_name());
};
void Dx7interface::on_setup_param_label(const Glib::RefPtr<Gtk::ListItem>& list_item, Gtk::Align halign){
    list_item->set_child(*Gtk::make_managed<Gtk::Label>("", halign));
};

void Dx7interface::on_midi_learn_event(){ // set the bool for midi learn
    if( (get_gwidget<Gtk::ToggleButton>("toggle_midi_learn"))->get_active() ){
        midi_learn=true;
    }else{
        midi_learn=false;
    };
};

void Dx7interface::add_midi_learn_param_widget(Glib::ustring param_name, Glib::ustring param_number, int selected_item){
    /* affected param name widget */
    auto text_fct = Gtk::make_managed<Gtk::Text>();
    text_fct->set_name(param_name);
    text_fct->set_name(param_name);
    text_fct->set_text(param_name);
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
    box_funct->append(*text_fct);
    box_funct->append(*text_param);
    box_funct->append(*btn_delete);
    /* ui box to attach result */
    auto box = get_gwidget<Gtk::Box>("box_listen_affect_param");
    box->append(*box_funct);
    /* scoped slot to be autoremove */
    auto slot_btn_delete_params = std::make_shared<sigc::scoped_connection>();
    *slot_btn_delete_params = btn_delete->signal_clicked().connect([this, box_funct, text_fct, text_param, btn_delete, slot_btn_delete_params, selected_item, param_number]() {
        auto box = get_gwidget<Gtk::Box>("box_listen_affect_param");
        box_funct->remove(*text_param);
        box_funct->remove(*text_fct);
        box_funct->remove(*btn_delete);
        box->remove(*box_funct);
        int p_number = std::stoi(param_number.raw());
        rem_midi_learned(p_number, selected_item);
    });
};

void Dx7interface::on_add_midi_learn_event(){
    auto selected_item = get_gwidget<Gtk::DropDown>("dropdown_affect_param")->get_selected_item();
    auto selected_number = get_gwidget<Gtk::DropDown>("dropdown_affect_param")->get_selected();
    if (selected_item) {
        Glib::ustring param_name = (std::dynamic_pointer_cast<ParamItem>(selected_item))->get_name();
        Glib::ustring param_number = get_gwidget<Gtk::Entry>("entry_affect_param")->get_text();
        int p_number = std::stoi(param_number.raw());
        add_midi_learned(p_number, selected_number);
        add_midi_learn_param_widget(param_name, param_number, selected_number);
    };
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
    LOG_IN();
    on_mono_poly_event();
    on_portamento_md_event();
    LOG_OUT();
};

/* Attach all signals */
void Dx7interface::attach_action_group_signals(){
    action_group->add_action("save_sound", sigc::mem_fun(*this, &Dx7interface::on_save_sound));
    action_group->add_action("save_bank", sigc::mem_fun(*this, &Dx7interface::on_save_bank));
    action_group->add_action("restore_sound", sigc::mem_fun(*this, &Dx7interface::on_restore_sound));
    action_group->add_action("restore_bank", sigc::mem_fun(*this, &Dx7interface::on_restore_bank));
    action_group->add_action("insert_after", sigc::mem_fun(*this, &Dx7interface::on_insert_after));
    action_group->add_action("replace_sound", sigc::mem_fun(*this, &Dx7interface::on_replace_sound));
    action_group->add_action("delete_sound", sigc::mem_fun(*this, &Dx7interface::on_delete_sound));
};
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
    LOG_IN();
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
    /* save dialog */
    button_save->signal_clicked().connect([this]() {
        OpenFileDialog();
        dialog_save->close();
    });
    (get_gwidget<Gtk::CheckButton>("checkbutton_add_extra_parameters"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_extra_param_event));

    /* Midi learn */
    (get_gwidget<Gtk::ToggleButton>("toggle_midi_learn"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_midi_learn_event));
    (get_gwidget<Gtk::Button>("button_add_param"))->signal_clicked().connect(
        sigc::mem_fun(*this, &Dx7interface::on_add_midi_learn_event));

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
    /* panic */
    slot_btn_panic = (get_gwidget<Gtk::Button>("btn_panic"))->signal_clicked().connect(
        sigc::mem_fun(*this, &Dx7interface::on_panic_event));

    /* Algo */
    (get_gwidget<Gtk::DrawingArea>("drawingarea_algo"))->set_draw_func(
        sigc::mem_fun(*this, &Dx7interface::on_draw_algo) );

    slot_algo = (get_gwidget<Gtk::SpinButton>("algo_number"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_algo_event));
    (get_gwidget<Gtk::SpinButton>("algo_number"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.algo.val;
        (get_gwidget<Gtk::SpinButton>("algo_number"))->set_value(value + 1);
        return true; // Return false to remove the callback after one executio
    });

    slot_feedback = (get_gwidget<Gtk::SpinButton>("feedback"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_feedback_event));
    (get_gwidget<Gtk::SpinButton>("feedback"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.feedback.val;
        (get_gwidget<Gtk::SpinButton>("feedback"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });

    auto note_transpose_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("note_transpose"));

    slot_note_transpose = (get_gwidget<Gtk::DropDown>("note_transpose"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_transpose_event));
    (get_gwidget<Gtk::DropDown>("note_transpose"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.transpose.val;
        (get_gwidget<Gtk::SpinButton>("octv_transpose"))->set_value( (value / 12)+1 );
        (get_gwidget<Gtk::DropDown>("note_transpose"))->set_selected(value % 12);
        return true; // Return false to remove the callback after one executio
    });

    slot_octv_transpose = (get_gwidget<Gtk::SpinButton>("octv_transpose"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_transpose_event));
    (get_gwidget<Gtk::SpinButton>("octv_transpose"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.transpose.val;
        (get_gwidget<Gtk::SpinButton>("octv_transpose"))->set_value( (value / 12)+1 );
        (get_gwidget<Gtk::DropDown>("note_transpose"))->set_selected(value % 12);
        return true; // Return false to remove the callback after one executio
    });

    slot_oks = (get_gwidget<Gtk::CheckButton>("oks"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_oks_event));
    (get_gwidget<Gtk::CheckButton>("oks"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.oks.val;
        (get_gwidget<Gtk::CheckButton>("oks"))->set_active(value);
        return true; // Return false to remove the callback after one executio
    });

    /* lfo */
    (get_gwidget<Gtk::DrawingArea>("drawingarea_lfo_wav"))->set_draw_func(
        sigc::mem_fun(*this, &Dx7interface::on_draw_lfo) );

    slot_lfo_wav = (get_gwidget<Gtk::DropDown>("lfo_wav"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lfo_wav_event));
    (get_gwidget<Gtk::DropDown>("lfo_wav"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->lfo.wave.val;
        (get_gwidget<Gtk::DropDown>("lfo_wav"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto lfo_wav_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("lfo_wav"));

    slot_lfo_sync = (get_gwidget<Gtk::CheckButton>("lfo_sync"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lfo_sync_event));
    (get_gwidget<Gtk::CheckButton>("lfo_sync"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->lfo.sync.val;
        (get_gwidget<Gtk::CheckButton>("lfo_sync"))->set_active(value);
        return true; // Return false to remove the callback after one executio
    });

    slot_speed = (get_gwidget<Gtk::SpinButton>("speed"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_speed_event));
    (get_gwidget<Gtk::SpinButton>("speed"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->lfo.speed.val;
        (get_gwidget<Gtk::SpinButton>("speed"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_delay = (get_gwidget<Gtk::SpinButton>("delay"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_delay_event));
    (get_gwidget<Gtk::SpinButton>("delay"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->lfo.delay.val;
        (get_gwidget<Gtk::SpinButton>("delay"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_pmd = (get_gwidget<Gtk::SpinButton>("pmd"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pmd_event));
    (get_gwidget<Gtk::SpinButton>("pmd"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->lfo.pmd.val;
        (get_gwidget<Gtk::SpinButton>("pmd"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });

    slot_amd = (get_gwidget<Gtk::SpinButton>("amd"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_amd_event));
    (get_gwidget<Gtk::SpinButton>("amd"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->lfo.amd.val;
        (get_gwidget<Gtk::SpinButton>("amd"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });

    /* lfo modulation */
    slot_pms = (get_gwidget<Gtk::Scale>("pms"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pms_event));
    (get_gwidget<Gtk::Scale>("pms"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->lfo.pms.val;
        (get_gwidget<Gtk::Scale>("pms"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });

    /* pitch eg */
    slot_pitch_rt1 = (get_gwidget<Gtk::SpinButton>("eg_rt1_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_rt1_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt1_pitch"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->pitch.eg_rt[0].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt1_pitch"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_pitch_rt2 = (get_gwidget<Gtk::SpinButton>("eg_rt2_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_rt2_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt2_pitch"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->pitch.eg_rt[1].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt2_pitch"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_pitch_rt3 = (get_gwidget<Gtk::SpinButton>("eg_rt3_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_rt3_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt3_pitch"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->pitch.eg_rt[2].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt3_pitch"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_pitch_rt4 = (get_gwidget<Gtk::SpinButton>("eg_rt4_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_rt4_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt4_pitch"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->pitch.eg_rt[3].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt4_pitch"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_pitch_lvl1 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_lvl1_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_pitch"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->pitch.eg_lvl[0].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl1_pitch"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_pitch_lvl2 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_lvl2_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_pitch"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->pitch.eg_lvl[1].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl2_pitch"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_pitch_lvl3 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_lvl3_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_pitch"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->pitch.eg_lvl[2].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl3_pitch"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_pitch_lvl4 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_lvl4_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_pitch"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->pitch.eg_lvl[3].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl4_pitch"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });

    attach_drawarea_signals();

    /* OPERATEUR 1 */
    slot_ams_op1 = (get_gwidget<Gtk::Scale>("ams_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op1_event)); //frame lfo
    (get_gwidget<Gtk::Scale>("ams_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].ams.val;
        (get_gwidget<Gtk::Scale>("ams_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP1 FREQUENCE */
    slot_freq_mode_op1 = (get_gwidget<Gtk::DropDown>("freq_mode_op1"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op1_event));
    (get_gwidget<Gtk::DropDown>("freq_mode_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].freq_mode.val;
        (get_gwidget<Gtk::DropDown>("freq_mode_op1"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto freq_mode_op1_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("freq_mode_op1"));

    slot_freq_coarse_op1 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op1_event));
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].freq_coarse.val;
        (get_gwidget<Gtk::SpinButton>("freq_coarse_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_freq_fine_op1 = (get_gwidget<Gtk::SpinButton>("freq_fine_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op1_event));
    (get_gwidget<Gtk::SpinButton>("freq_fine_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].freq_fine.val;
        (get_gwidget<Gtk::SpinButton>("freq_fine_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_dtun_op1 = (get_gwidget<Gtk::Scale>("dtun_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op1_event));
    (get_gwidget<Gtk::Scale>("dtun_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].dtun.val-7;
        (get_gwidget<Gtk::Scale>("dtun_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP1 EG */
    slot_eg_rt1_op1 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op1_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].eg_rt[0].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt1_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt2_op1 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op1_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].eg_rt[1].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt2_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt3_op1 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op1_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].eg_rt[2].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt3_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt4_op1 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op1_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].eg_rt[3].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt4_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl1_op1 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op1_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].eg_lvl[0].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl1_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl2_op1 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op1_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].eg_lvl[1].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl2_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl3_op1 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op1_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].eg_lvl[2].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl3_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl4_op1 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op1_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].eg_lvl[3].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl4_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP1 VOLUME */
    slot_krs_op1 = (get_gwidget<Gtk::Scale>("krs_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op1_event));
    (get_gwidget<Gtk::Scale>("krs_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].krs.val;
        (get_gwidget<Gtk::Scale>("krs_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kvs_op1 = (get_gwidget<Gtk::Scale>("kvs_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op1_event));
    (get_gwidget<Gtk::Scale>("kvs_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].kvs.val;
        (get_gwidget<Gtk::Scale>("kvs_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_lvl_op1 = (get_gwidget<Gtk::SpinButton>("lvl_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op1_event));
    (get_gwidget<Gtk::SpinButton>("lvl_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].lvl.val;
        (get_gwidget<Gtk::SpinButton>("lvl_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_mute_op1 = (get_gwidget<Gtk::ToggleButton>("mute_op1"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    (get_gwidget<Gtk::ToggleButton>("mute_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->extra.mute.val >>5;
        (get_gwidget<Gtk::ToggleButton>("mute_op1"))->set_active(!(value & 0x01));
        return true; // Return false to remove the callback after one executio
    });

    /* OP1 KLS */
    slot_kls_lft_curve_op1 = (get_gwidget<Gtk::DropDown>("kls_lft_curve_op1"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op1_event));
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].kls.lft_curve.val;
        (get_gwidget<Gtk::DropDown>("kls_lft_curve_op1"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_lft_curve_op1_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_lft_curve_op1"));

    slot_kls_rght_curve_op1 = (get_gwidget<Gtk::DropDown>("kls_rght_curve_op1"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op1_event));
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].kls.rght_curve.val;
        (get_gwidget<Gtk::DropDown>("kls_rght_curve_op1"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_rght_curve_op1_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_rght_curve_op1"));

    slot_kls_lft_depth_op1 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op1_event));
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].kls.lft_dpth.val;
        (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_rght_depth_op1 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op1_event));
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[0].kls.rght_dpth.val;
        (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op1"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_note_brk_pt_op1 = (get_gwidget<Gtk::DropDown>("note_brk_pt_op1"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op1_event));
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.transpose.val;
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
    (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.transpose.val;
        (get_gwidget<Gtk::DropDown>("note_brk_pt_op1"))->set_selected(value % 12);
        int val = (value -3);
        if(val < 0){
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->set_value( -1 );
        }else{
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->set_value( val / 12 );
        };
        return true; // Return false to remove the callback after one executio
    });

    /* OP2 */
    slot_ams_op2 = (get_gwidget<Gtk::Scale>("ams_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op2_event)); //frame lfo
    (get_gwidget<Gtk::Scale>("ams_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].ams.val;
        (get_gwidget<Gtk::Scale>("ams_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP2 FREQUENCE */
    slot_freq_mode_op2 = (get_gwidget<Gtk::DropDown>("freq_mode_op2"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op2_event));
    (get_gwidget<Gtk::DropDown>("freq_mode_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].freq_mode.val;
        (get_gwidget<Gtk::DropDown>("freq_mode_op2"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto freq_mode_op2_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("freq_mode_op2"));

    slot_freq_coarse_op2 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op2_event));
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].freq_coarse.val;
        (get_gwidget<Gtk::SpinButton>("freq_coarse_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_freq_fine_op2 = (get_gwidget<Gtk::SpinButton>("freq_fine_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op2_event));
    (get_gwidget<Gtk::SpinButton>("freq_fine_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].freq_fine.val;
        (get_gwidget<Gtk::SpinButton>("freq_fine_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_dtun_op2 = (get_gwidget<Gtk::Scale>("dtun_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op2_event));
    (get_gwidget<Gtk::Scale>("dtun_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].dtun.val-7;
        (get_gwidget<Gtk::Scale>("dtun_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP2 EG */
    slot_eg_rt1_op2 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op2_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].eg_rt[0].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt1_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt2_op2 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op2_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].eg_rt[1].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt2_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt3_op2 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op2_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].eg_rt[2].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt3_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt4_op2 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op2_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].eg_rt[3].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt4_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl1_op2 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op2_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].eg_lvl[0].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl1_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl2_op2 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op2_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].eg_lvl[1].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl2_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl3_op2 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op2_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].eg_lvl[2].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl3_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl4_op2 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op2_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].eg_lvl[3].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl4_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP2 VOLUME */
    slot_krs_op2 = (get_gwidget<Gtk::Scale>("krs_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op2_event));
    (get_gwidget<Gtk::Scale>("krs_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].krs.val;
        (get_gwidget<Gtk::Scale>("krs_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kvs_op2 = (get_gwidget<Gtk::Scale>("kvs_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op2_event));
    (get_gwidget<Gtk::Scale>("kvs_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].kvs.val;
        (get_gwidget<Gtk::Scale>("kvs_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_lvl_op2 = (get_gwidget<Gtk::SpinButton>("lvl_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op2_event));
    (get_gwidget<Gtk::SpinButton>("lvl_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].lvl.val;
        (get_gwidget<Gtk::SpinButton>("lvl_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_mute_op2 = (get_gwidget<Gtk::ToggleButton>("mute_op2"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    (get_gwidget<Gtk::ToggleButton>("mute_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->extra.mute.val >>4;
        (get_gwidget<Gtk::ToggleButton>("mute_op2"))->set_active(!(value & 0x01));
        return true; // Return false to remove the callback after one executio
    });

    /* OP2 KLS */
    slot_kls_lft_curve_op2 = (get_gwidget<Gtk::DropDown>("kls_lft_curve_op2"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op2_event));
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].kls.lft_curve.val;
        (get_gwidget<Gtk::DropDown>("kls_lft_curve_op2"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_lft_curve_op2_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_lft_curve_op2"));

    slot_kls_rght_curve_op2 = (get_gwidget<Gtk::DropDown>("kls_rght_curve_op2"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op2_event));
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].kls.rght_curve.val;
        (get_gwidget<Gtk::DropDown>("kls_rght_curve_op2"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_rght_curve_op2_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_rght_curve_op2"));

    slot_kls_lft_depth_op2 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op2_event));
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].kls.lft_dpth.val;
        (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_rght_depth_op2 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op2_event));
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[1].kls.rght_dpth.val;
        (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op2"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_note_brk_pt_op2 = (get_gwidget<Gtk::DropDown>("note_brk_pt_op2"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op2_event));
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.transpose.val;
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
    (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op2"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.transpose.val;
        (get_gwidget<Gtk::DropDown>("note_brk_pt_op2"))->set_selected(value % 12);
        int val = (value -3);
        if(val < 0){
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op2"))->set_value( -1 );
        }else{
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op2"))->set_value( val / 12 );
        };
        return true; // Return false to remove the callback after one executio
    });
    /* OP3 */
    slot_ams_op3 = (get_gwidget<Gtk::Scale>("ams_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op3_event)); //frame lfo
    (get_gwidget<Gtk::Scale>("ams_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].ams.val;
        (get_gwidget<Gtk::Scale>("ams_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP3 FREQUENCE */
    slot_freq_mode_op3 = (get_gwidget<Gtk::DropDown>("freq_mode_op3"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op3_event));
    (get_gwidget<Gtk::DropDown>("freq_mode_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].freq_mode.val;
        (get_gwidget<Gtk::DropDown>("freq_mode_op3"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto freq_mode_op3_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("freq_mode_op3"));

    slot_freq_coarse_op3 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op3_event));
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].freq_coarse.val;
        (get_gwidget<Gtk::SpinButton>("freq_coarse_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_freq_fine_op3 = (get_gwidget<Gtk::SpinButton>("freq_fine_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op3_event));
    (get_gwidget<Gtk::SpinButton>("freq_fine_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].freq_fine.val;
        (get_gwidget<Gtk::SpinButton>("freq_fine_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_dtun_op3 = (get_gwidget<Gtk::Scale>("dtun_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op3_event));
    (get_gwidget<Gtk::Scale>("dtun_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].dtun.val-7;
        (get_gwidget<Gtk::Scale>("dtun_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP3 EG */
    slot_eg_rt1_op3 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op3_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].eg_rt[0].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt1_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt2_op3 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op3_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].eg_rt[1].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt2_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt3_op3 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op3_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].eg_rt[2].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt3_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt4_op3 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op3_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].eg_rt[3].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt4_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl1_op3 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op3_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].eg_lvl[0].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl1_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl2_op3 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op3_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].eg_lvl[1].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl2_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl3_op3 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op3_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].eg_lvl[2].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl3_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl4_op3 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op3_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].eg_lvl[3].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl4_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP3 VOLUME */
    slot_krs_op3 = (get_gwidget<Gtk::Scale>("krs_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op3_event));
    (get_gwidget<Gtk::Scale>("krs_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].krs.val;
        (get_gwidget<Gtk::Scale>("krs_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kvs_op3 = (get_gwidget<Gtk::Scale>("kvs_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op3_event));
    (get_gwidget<Gtk::Scale>("kvs_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].kvs.val;
        (get_gwidget<Gtk::Scale>("kvs_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_lvl_op3 = (get_gwidget<Gtk::SpinButton>("lvl_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op3_event));
    (get_gwidget<Gtk::SpinButton>("lvl_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].lvl.val;
        (get_gwidget<Gtk::SpinButton>("lvl_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_mute_op3 = (get_gwidget<Gtk::ToggleButton>("mute_op3"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    (get_gwidget<Gtk::ToggleButton>("mute_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->extra.mute.val >>3;
        (get_gwidget<Gtk::ToggleButton>("mute_op3"))->set_active(!(value & 0x01));
        return true; // Return false to remove the callback after one executio
    });

    /* OP3 KLS */
    slot_kls_lft_curve_op3 = (get_gwidget<Gtk::DropDown>("kls_lft_curve_op3"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op3_event));
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].kls.lft_curve.val;
        (get_gwidget<Gtk::DropDown>("kls_lft_curve_op3"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_lft_curve_op3_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_lft_curve_op3"));

    slot_kls_rght_curve_op3 = (get_gwidget<Gtk::DropDown>("kls_rght_curve_op3"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op3_event));
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].kls.rght_curve.val;
        (get_gwidget<Gtk::DropDown>("kls_rght_curve_op3"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_rght_curve_op3_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_rght_curve_op3"));

    slot_kls_lft_depth_op3 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op3_event));
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].kls.lft_dpth.val;
        (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_rght_depth_op3 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op3_event));
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[2].kls.rght_dpth.val;
        (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op3"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_note_brk_pt_op3 = (get_gwidget<Gtk::DropDown>("note_brk_pt_op3"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op3_event));
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.transpose.val;
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
    (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op3"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.transpose.val;
        (get_gwidget<Gtk::DropDown>("note_brk_pt_op3"))->set_selected(value % 12);
        int val = (value -3);
        if(val < 0){
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op3"))->set_value( -1 );
        }else{
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op3"))->set_value( val / 12 );
        };
        return true; // Return false to remove the callback after one executio
    });
    /* OP4 */
    slot_ams_op4 = (get_gwidget<Gtk::Scale>("ams_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op4_event)); //frame lfo
    (get_gwidget<Gtk::Scale>("ams_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].ams.val;
        (get_gwidget<Gtk::Scale>("ams_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP4 FREQUENCE */
    slot_freq_mode_op4 = (get_gwidget<Gtk::DropDown>("freq_mode_op4"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op4_event));
    (get_gwidget<Gtk::DropDown>("freq_mode_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].freq_mode.val;
        (get_gwidget<Gtk::DropDown>("freq_mode_op4"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto freq_mode_op4_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("freq_mode_op4"));

    slot_freq_coarse_op4 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op4_event));
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].freq_coarse.val;
        (get_gwidget<Gtk::SpinButton>("freq_coarse_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_freq_fine_op4 = (get_gwidget<Gtk::SpinButton>("freq_fine_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op4_event));
    (get_gwidget<Gtk::SpinButton>("freq_fine_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].freq_fine.val;
        (get_gwidget<Gtk::SpinButton>("freq_fine_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_dtun_op4 = (get_gwidget<Gtk::Scale>("dtun_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op4_event));
    (get_gwidget<Gtk::Scale>("dtun_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].dtun.val;
        (get_gwidget<Gtk::Scale>("dtun_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP4 EG */
    slot_eg_rt1_op4 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op4_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].eg_rt[0].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt1_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt2_op4 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op4_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].eg_rt[1].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt2_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt3_op4 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op4_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].eg_rt[2].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt3_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt4_op4 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op4_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].eg_rt[3].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt4_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl1_op4 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op4_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].eg_lvl[0].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl1_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl2_op4 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op4_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].eg_lvl[1].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl2_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl3_op4 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op4_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].eg_lvl[2].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl3_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl4_op4 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op4_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].eg_lvl[3].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl4_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP4 VOLUME */
    slot_krs_op4 = (get_gwidget<Gtk::Scale>("krs_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op4_event));
    (get_gwidget<Gtk::Scale>("krs_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].krs.val;
        (get_gwidget<Gtk::Scale>("krs_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kvs_op4 = (get_gwidget<Gtk::Scale>("kvs_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op4_event));
    (get_gwidget<Gtk::Scale>("kvs_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].kvs.val;
        (get_gwidget<Gtk::Scale>("kvs_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_lvl_op4 = (get_gwidget<Gtk::SpinButton>("lvl_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op4_event));
    (get_gwidget<Gtk::SpinButton>("lvl_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].lvl.val;
        (get_gwidget<Gtk::SpinButton>("lvl_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_mute_op4 = (get_gwidget<Gtk::ToggleButton>("mute_op4"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    (get_gwidget<Gtk::ToggleButton>("mute_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->extra.mute.val >>2;
        (get_gwidget<Gtk::ToggleButton>("mute_op4"))->set_active(!(value & 0x01));
        return true; // Return false to remove the callback after one executio
    });

    /* OP4 KLS */
    slot_kls_lft_curve_op4 = (get_gwidget<Gtk::DropDown>("kls_lft_curve_op4"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op4_event));
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].kls.lft_curve.val;
        (get_gwidget<Gtk::DropDown>("kls_lft_curve_op4"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_lft_curve_op4_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_lft_curve_op4"));

    slot_kls_rght_curve_op4 = (get_gwidget<Gtk::DropDown>("kls_rght_curve_op4"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op4_event));
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].kls.rght_curve.val;
        (get_gwidget<Gtk::DropDown>("kls_rght_curve_op4"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_rght_curve_op4_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_rght_curve_op4"));

    slot_kls_lft_depth_op4 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op4_event));
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].kls.lft_dpth.val;
        (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_rght_depth_op4 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op4_event));
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[3].kls.rght_dpth.val;
        (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op4"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_note_brk_pt_op4 = (get_gwidget<Gtk::DropDown>("note_brk_pt_op4"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op4_event));
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.transpose.val;
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
    (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op4"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.transpose.val;
        (get_gwidget<Gtk::DropDown>("note_brk_pt_op4"))->set_selected(value % 12);
        int val = (value -3);
        if(val < 0){
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op4"))->set_value( -1 );
        }else{
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op4"))->set_value( val / 12 );
        };
        return true; // Return false to remove the callback after one executio
    });

    /* OP5 */
    slot_ams_op5 = (get_gwidget<Gtk::Scale>("ams_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op5_event)); //frame lfo
    (get_gwidget<Gtk::Scale>("ams_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].ams.val;
        (get_gwidget<Gtk::Scale>("ams_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP5 FREQUENCE */
    slot_freq_mode_op5 = (get_gwidget<Gtk::DropDown>("freq_mode_op5"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op5_event));
    (get_gwidget<Gtk::DropDown>("freq_mode_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].freq_mode.val;
        (get_gwidget<Gtk::DropDown>("freq_mode_op5"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto freq_mode_op5_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("freq_mode_op5"));

    slot_freq_coarse_op5 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op5_event));
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].freq_coarse.val;
        (get_gwidget<Gtk::SpinButton>("freq_coarse_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_freq_fine_op5 = (get_gwidget<Gtk::SpinButton>("freq_fine_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op5_event));
    (get_gwidget<Gtk::SpinButton>("freq_fine_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].freq_fine.val;
        (get_gwidget<Gtk::SpinButton>("freq_fine_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_dtun_op5 = (get_gwidget<Gtk::Scale>("dtun_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op5_event));
    (get_gwidget<Gtk::Scale>("dtun_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].dtun.val-7;
        (get_gwidget<Gtk::Scale>("dtun_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP5 EG */
    slot_eg_rt1_op5 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op5_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].eg_rt[0].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt1_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt2_op5 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op5_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].eg_rt[1].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt2_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt3_op5 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op5_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].eg_rt[2].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt3_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt4_op5 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op5_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].eg_rt[3].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt4_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl1_op5 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op5_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].eg_lvl[0].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl1_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl2_op5 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op5_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].eg_lvl[1].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl2_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl3_op5 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op5_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].eg_lvl[2].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl3_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl4_op5 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op5_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].eg_lvl[3].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl4_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP5 VOLUME */
    slot_krs_op5 = (get_gwidget<Gtk::Scale>("krs_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op5_event));
    (get_gwidget<Gtk::Scale>("krs_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].krs.val;
        (get_gwidget<Gtk::Scale>("krs_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kvs_op5 = (get_gwidget<Gtk::Scale>("kvs_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op5_event));
    (get_gwidget<Gtk::Scale>("kvs_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].kvs.val;
        (get_gwidget<Gtk::Scale>("kvs_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_lvl_op5 = (get_gwidget<Gtk::SpinButton>("lvl_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op5_event));
    (get_gwidget<Gtk::SpinButton>("lvl_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].lvl.val;
        (get_gwidget<Gtk::SpinButton>("lvl_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_mute_op5 = (get_gwidget<Gtk::ToggleButton>("mute_op5"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    (get_gwidget<Gtk::ToggleButton>("mute_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->extra.mute.val >>1;
        (get_gwidget<Gtk::ToggleButton>("mute_op5"))->set_active(!(value & 0x01));
        return true; // Return false to remove the callback after one executio
    });

    /* OP5 KLS */
    slot_kls_lft_curve_op5 = (get_gwidget<Gtk::DropDown>("kls_lft_curve_op5"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op5_event));
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].kls.lft_curve.val;
        (get_gwidget<Gtk::DropDown>("kls_lft_curve_op5"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_lft_curve_op5_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_lft_curve_op5"));

    slot_kls_rght_curve_op5 = (get_gwidget<Gtk::DropDown>("kls_rght_curve_op5"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op5_event));
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].kls.rght_curve.val;
        (get_gwidget<Gtk::DropDown>("kls_rght_curve_op5"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_rght_curve_op5_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_rght_curve_op5"));

    slot_kls_lft_depth_op5 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op5_event));
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].kls.lft_dpth.val;
        (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_rght_depth_op5 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op5_event));
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[4].kls.rght_dpth.val;
        (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op5"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_note_brk_pt_op5 = (get_gwidget<Gtk::DropDown>("note_brk_pt_op5"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op5_event));
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.transpose.val;
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
    (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op5"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.transpose.val;
        (get_gwidget<Gtk::DropDown>("note_brk_pt_op5"))->set_selected(value % 12);
        int val = (value -3);
        if(val < 0){
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op5"))->set_value( -1 );
        }else{
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op5"))->set_value( val / 12 );
        };
        return true; // Return false to remove the callback after one executio
    });

    /* OP6 */
    slot_ams_op6 = (get_gwidget<Gtk::Scale>("ams_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op6_event)); //frame lfo
    (get_gwidget<Gtk::Scale>("ams_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].ams.val;
        (get_gwidget<Gtk::Scale>("ams_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP6 FREQUENCE */
    slot_freq_mode_op6 = (get_gwidget<Gtk::DropDown>("freq_mode_op6"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op6_event));
    (get_gwidget<Gtk::DropDown>("freq_mode_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].freq_mode.val;
        (get_gwidget<Gtk::DropDown>("freq_mode_op6"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto freq_mode_op6_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("freq_mode_op6"));

    slot_freq_coarse_op6 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op6_event));
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].freq_coarse.val;
        (get_gwidget<Gtk::SpinButton>("freq_coarse_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_freq_fine_op6 = (get_gwidget<Gtk::SpinButton>("freq_fine_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op6_event));
    (get_gwidget<Gtk::SpinButton>("freq_fine_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].freq_fine.val;
        (get_gwidget<Gtk::SpinButton>("freq_fine_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_dtun_op6 = (get_gwidget<Gtk::Scale>("dtun_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op6_event));
    (get_gwidget<Gtk::Scale>("dtun_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].dtun.val-7;
        (get_gwidget<Gtk::Scale>("dtun_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP6 EG */
    slot_eg_rt1_op6 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op6_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].eg_rt[0].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt1_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt2_op6 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op6_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].eg_rt[1].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt2_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt3_op6 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op6_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].eg_rt[2].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt3_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_rt4_op6 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op6_event));
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].eg_rt[3].val;
        (get_gwidget<Gtk::SpinButton>("eg_rt4_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl1_op6 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op6_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].eg_lvl[0].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl1_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl2_op6 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op6_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].eg_lvl[1].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl2_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl3_op6 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op6_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].eg_lvl[2].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl3_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_eg_lvl4_op6 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op6_event));
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].eg_lvl[3].val;
        (get_gwidget<Gtk::SpinButton>("eg_lvl4_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    /* OP6 VOLUME */
    slot_krs_op6 = (get_gwidget<Gtk::Scale>("krs_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op6_event));
    (get_gwidget<Gtk::Scale>("krs_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].krs.val;
        (get_gwidget<Gtk::Scale>("krs_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kvs_op6 = (get_gwidget<Gtk::Scale>("kvs_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op6_event));
    (get_gwidget<Gtk::Scale>("kvs_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].kvs.val;
        (get_gwidget<Gtk::Scale>("kvs_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_lvl_op6 = (get_gwidget<Gtk::SpinButton>("lvl_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op6_event));
    (get_gwidget<Gtk::SpinButton>("lvl_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].lvl.val;
        (get_gwidget<Gtk::SpinButton>("lvl_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_mute_op6 = (get_gwidget<Gtk::ToggleButton>("mute_op6"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    (get_gwidget<Gtk::ToggleButton>("mute_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->extra.mute.val;
        (get_gwidget<Gtk::ToggleButton>("mute_op6"))->set_active(!(value & 0x01));
        return true; // Return false to remove the callback after one executio
    });

    /* OP6 KLS */
    slot_kls_lft_curve_op6 = (get_gwidget<Gtk::DropDown>("kls_lft_curve_op6"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op6_event));
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].kls.lft_curve.val;
        (get_gwidget<Gtk::DropDown>("kls_lft_curve_op6"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_lft_curve_op6_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_lft_curve_op6"));

    slot_kls_rght_curve_op6 = (get_gwidget<Gtk::DropDown>("kls_rght_curve_op6"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op6_event));
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].kls.rght_curve.val;
        (get_gwidget<Gtk::DropDown>("kls_rght_curve_op6"))->set_selected(value);
        return true; // Return false to remove the callback after one executio
    });
    auto kls_rght_curve_op6_scroller = DropDownScrollController(get_gwidget<Gtk::DropDown>("kls_rght_curve_op6"));

    slot_kls_lft_depth_op6 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op6_event));
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].kls.lft_dpth.val;
        (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_rght_depth_op6 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op6_event));
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->op[5].kls.rght_dpth.val;
        (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op6"))->set_value(value);
        return true; // Return false to remove the callback after one executio
    });
    slot_kls_note_brk_pt_op6 = (get_gwidget<Gtk::DropDown>("note_brk_pt_op6"))->property_selected().signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op6_event));
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.transpose.val;
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
    (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op6"))->add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) {
        int value = bank_1_modif.sound->algo.transpose.val;
        (get_gwidget<Gtk::DropDown>("note_brk_pt_op6"))->set_selected(value % 12);
        int val = (value -3);
        if(val < 0){
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op6"))->set_value( -1 );
        }else{
            (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op6"))->set_value( val / 12 );
        };
        return true; // Return false to remove the callback after one executio
    });

    LOG_OUT();
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
    // TODO set value in a var to be changed by interface
    //LOG_IN();
    cr->save();
    cr->set_source_rgba(bg_color[0], bg_color[1], bg_color[2], bg_color[3]);
    cr->paint();    // fill with color
    cr->restore();
    //LOG_OUT();
};

void Dx7interface::draw_grid(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height){
    //LOG_IN();
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

void Dx7interface::draw_point(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y,double width,bool orange){
    // TODO set value in a var to be changed by interface
    //LOG_IN();
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
    //LOG_OUT();
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
    // TODO : get sound from bank_1_modif.sound
    //LOG_IN();
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
    // TODO: global output lvl scale the distance
    // put key on key off mark

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
            std::abs( 100.0 - (double)( (get_gwidget<Gtk::SpinButton>("eg_rt"+tostr<uint>(i)+"_"+name))->get_value() + 1.0 ) )
        ) ) ) ;
        y = (r_point) + ( ( (double)(get_gwidget<Gtk::SpinButton>("eg_lvl"+tostr<uint>(i)+"_"+name))->get_value() ) * y_ratio );
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
    //LOG_OUT();
};
/* KLS */
void Dx7interface::draw_keyboard(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height, Glib::ustring num_op){
    // TODO : get sound from bank_1_modif.sound
    //LOG_IN();
    /* key touch */
    if (std::filesystem::exists(std::string(MOD_IMG_DIRECTORY"/touche_b.png"))
        && std::filesystem::exists(std::string(MOD_IMG_DIRECTORY"/touche_w.png"))
        && std::filesystem::exists(std::string(MOD_IMG_DIRECTORY"/keyboard_background.png"))
        && std::filesystem::exists(std::string(MOD_IMG_DIRECTORY"/keyboard.png"))
    ){
        Cairo::RefPtr<Cairo::ImageSurface> touch;
        Cairo::RefPtr<Cairo::ImageSurface> touch_b = Cairo::ImageSurface::create_from_png(MOD_IMG_DIRECTORY"/touche_b.png");
        Cairo::RefPtr<Cairo::ImageSurface> touch_w = Cairo::ImageSurface::create_from_png(MOD_IMG_DIRECTORY"/touche_w.png");
        double width_w = (double)touch_w->get_width();
        /* keyboard */
        Cairo::RefPtr<Cairo::ImageSurface> keyboard_bg_image_surface = Cairo::ImageSurface::create_from_png(MOD_IMG_DIRECTORY"/keyboard_background.png");
        Cairo::RefPtr<Cairo::ImageSurface> keyboard_image_surface = Cairo::ImageSurface::create_from_png(MOD_IMG_DIRECTORY"/keyboard.png");
        double keyboard_width = (double)keyboard_image_surface->get_width();
        double keyboard_heigth = (double)keyboard_image_surface->get_height();
        /* UI values */
        Glib::ustring note = (std::dynamic_pointer_cast<Gtk::StringObject>((get_gwidget<Gtk::DropDown>("kls_rght_curve_op"+num_op))->get_selected_item()))->get_string();
        uint num_note = get_gwidget<Gtk::DropDown>("note_brk_pt_op"+num_op)->get_selected();
        uint octv = get_gwidget<Gtk::SpinButton>("octv_brk_pt_op"+num_op)->get_value();
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
        std::cerr << "File not found: "<<std::endl;
        std::cerr<<MOD_IMG_DIRECTORY"/ keyboard or touch .png"<<std::endl;
    };
    //LOG_OUT();
};

void Dx7interface::draw_axis(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height){
    // TODO : get sound from bank_1_modif.sound
    //LOG_IN();
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
    //LOG_OUT();
};

void Dx7interface::draw_kls_curve(const Cairo::RefPtr<Cairo::Context>& cr,Glib::ustring type_curve, double width, double height, double dpth, Glib::ustring dir){
    double half_width  = width/2.0;
    double half_height = height/2.0;
    double scale_factor = (100.0 - dpth) +25 ; // +25, to get 85 at max ( 85==100 depth)
    switch (str_const_hash(type_curve.c_str())) {
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
    // TODO : get sound from bank_1_modif.sound
    //LOG_IN();
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

    cr->save();
    cr->translate(0, height);
    cr->scale(1, -1);
    cr->set_source_rgba(line_color[0],line_color[1],line_color[2],1.0);
    cr->set_line_width(line_width);
    draw_kls_curve(cr,rght_curve,width,height,rght_dpth,"rght");
    cr->stroke();
    draw_kls_curve(cr,lft_curve,width,height,lft_dpth,"lft");
    cr->stroke();
    cr->restore();
    //LOG_OUT();
};

/** EVENTS **/
/* Draw */
void Dx7interface::on_draw_kls_event(const Cairo::RefPtr<Cairo::Context>& cr,int width, int height, Glib::ustring num_op){
    // LOG_IN();
    if( cr && (width != 0) && (height != 0) ){
        double wdth=(double)width, hght=(double)height;
        draw_background(cr);
        draw_axis(cr,wdth, hght);
        draw_kls( cr, wdth, hght, num_op);
        draw_keyboard( cr, wdth, hght, num_op );
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op"+num_op))->queue_draw();
    };
    // LOG_OUT();
};

void Dx7interface::on_draw_op_event(const Cairo::RefPtr<Cairo::Context>& cr,int width, int height, Glib::ustring num_op){
    // LOG_IN();
    if( cr && (width != 0) && (height != 0) ){
        double wdth=(double)width, hght=(double)height;
        draw_background(cr);
        draw_grid( cr, wdth, hght);
        draw_adsr( cr, wdth, hght, "op"+num_op );
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op"+num_op))->queue_draw();
    };
    // LOG_OUT();
};

void Dx7interface::on_draw_pitch_event(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height){
    // LOG_IN();
    if (cr && (width != 0) && (height != 0) ){
        double wdth=(double)width, hght=(double)height;
        draw_background(cr);
        draw_grid( cr, wdth, hght );
        draw_adsr( cr, wdth, hght, "pitch" );
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    };
    // LOG_OUT();
};

void Dx7interface::on_draw_algo(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height){
    // LOG_IN();
    std::string img;
    if(compare){
        img = std::string(MOD_IMG_DIRECTORY"/algo"+tostr<uint>(bank_1_origin.sound->algo.algo.val+1)+".png");
    }else{
        img = std::string(MOD_IMG_DIRECTORY"/algo"+tostr<uint>(bank_1_modif.sound->algo.algo.val+1)+".png");
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
        std::cerr << "File not found: "<<std::endl;
        std::cerr<<img<<std::endl;
    };
    // LOG_OUT();
};

void Dx7interface::on_draw_lfo(const Cairo::RefPtr<Cairo::Context>& cr, double width, double height){
    // LOG_IN();
    Glib::ustring img = ( std::dynamic_pointer_cast<Gtk::StringObject>((get_gwidget<Gtk::DropDown>("lfo_wav"))->get_selected_item()) )->get_string();
    if (img == "S/HOLD"){
        img="S_HOLD";
    };
    if (std::filesystem::exists(std::string(MOD_IMG_DIRECTORY"/"+img+".png"))){
        Cairo::RefPtr<Cairo::ImageSurface> lfo_image_surface = Cairo::ImageSurface::create_from_png(MOD_IMG_DIRECTORY"/"+img+".png");
        double scale_factor = width / lfo_image_surface->get_width();
        cr->save();
        cr->scale(scale_factor,scale_factor);
        cr->set_source(lfo_image_surface, 0,0);
        cr->paint();
        cr->restore();
    }else{
        std::cerr << "File not found: "<<std::endl;
        std::cerr<<MOD_IMG_DIRECTORY"/"+img+".png"<<std::endl;
    };
    // LOG_OUT();
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

/* Extra Functions */
void Dx7interface::on_mono_poly_event(){
	LOG_IN();
    u_char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status & channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x40;                        // 40
                                        // bit0 0=poly/bit0 1=mono
    msg[5]=(get_gwidget<Gtk::ToggleButton>("btn_poly_mono"))->get_active();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    if(!compare){
        bank_1_modif.sound->extra.functions.poly_mono.val = msg[5];
    };
    if ( (get_gwidget<Gtk::ToggleButton>("btn_poly_mono"))->get_active() ) {
        (get_gwidget<Gtk::ToggleButton>("btn_poly_mono"))->set_label(_("Monophonic"));
    }else{
        (get_gwidget<Gtk::ToggleButton>("btn_poly_mono"))->set_label(_("Polyphonic"));
    };
	LOG_OUT();
};

void Dx7interface::on_ptch_bnd_rng_event(){
    LOG_IN();
    u_char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status & channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x41;                        // 41
                                        // 0111 1000
    msg[5]=(get_gwidget<Gtk::Scale>("ptch_bnd_rng"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    if(!compare){
        bank_1_modif.sound->extra.functions.ptch_bnd_rng.val = msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_ptch_bnd_stp_event(){
    LOG_IN();
    u_char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status & channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x42;                        // 42
    msg[5]=(get_gwidget<Gtk::Scale>("ptch_bnd_stp"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    if(!compare){
        bank_1_modif.sound->extra.functions.ptch_bnd_stp.val = msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_portamento_md_event(){
    LOG_IN();
    u_char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status & channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x43;                        // 43
                                        // 0-3 bit0=retain; bit1=follow
    msg[5]=(get_gwidget<Gtk::ToggleButton>("btn_portamento_md"))->get_active();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    if(!compare){
        bank_1_modif.sound->extra.functions.portamento_md.val = msg[5];
    };
    if ( (get_gwidget<Gtk::ToggleButton>("btn_portamento_md"))->get_active() ) {
        (get_gwidget<Gtk::ToggleButton>("btn_portamento_md"))->set_label(_("Follow"));
    }else{
        (get_gwidget<Gtk::ToggleButton>("btn_portamento_md"))->set_label(_("Retain"));
    };
    LOG_OUT();
};

void Dx7interface::on_portamento_glss_event(){
    LOG_IN();
    u_char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status & channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x44;                        // 44
                                        // 0-3 bit 0= 1=gliss ???
    msg[5]=(get_gwidget<Gtk::ToggleButton>("btn_portamento_glss"))->get_active();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    if(!compare){
        bank_1_modif.sound->extra.functions.portamento_glss.val = msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_portamento_tm_event(){
    LOG_IN();
    u_char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status & channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x45;                        // 45
    msg[5]=(get_gwidget<Gtk::SpinButton>("portamento_tm"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    if(!compare){
        bank_1_modif.sound->extra.functions.portamento_tm.val = msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_md_whl_rng_event(){
    LOG_IN();
    u_char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status & channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x46;                        // 46
    msg[5]=(get_gwidget<Gtk::SpinButton>("md_whl_rng"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    if(!compare){
        bank_1_modif.sound->extra.functions.md_whl_rng.val = msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_md_whl_assgn_event(){
    LOG_IN();
    u_char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status & channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x47;                        // 47
    msg[5]= (get_gwidget<Gtk::CheckButton>("md_whl_ptch"))->get_active()
          +((get_gwidget<Gtk::CheckButton>("md_whl_mp"))->get_active()*2)
          +((get_gwidget<Gtk::CheckButton>("md_whl_gbs"))->get_active()*4);
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    if(!compare){
        bank_1_modif.sound->extra.functions.md_whl_assgn.val = msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_foot_rng_event(){
    LOG_IN();
    u_char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status & channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x48;                        // 48
    msg[5]=(get_gwidget<Gtk::SpinButton>("foot_rng"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    if(!compare){
        bank_1_modif.sound->extra.functions.foot_rng.val = msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_foot_assgn_event(){
    LOG_IN();
    u_char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status & channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x49;                        // 49
    msg[5]= (get_gwidget<Gtk::CheckButton>("foot_ptch"))->get_active()
          +((get_gwidget<Gtk::CheckButton>("foot_mp"))->get_active()*2)
          +((get_gwidget<Gtk::CheckButton>("foot_gbs"))->get_active()*4);
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    if(!compare){
        bank_1_modif.sound->extra.functions.foot_assgn.val = msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_brth_rng_event(){
    LOG_IN();
    u_char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status & channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x4A;                        // 4A
    msg[5]=(get_gwidget<Gtk::SpinButton>("brth_rng"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    if(!compare){
        bank_1_modif.sound->extra.functions.brth_rng.val = msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_brth_assgn_event(){
    LOG_IN();
    u_char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status & channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x4B;                        // 4B
    msg[5]= (get_gwidget<Gtk::CheckButton>("brth_ptch"))->get_active()
          +((get_gwidget<Gtk::CheckButton>("brth_mp"))->get_active()*2)
          +((get_gwidget<Gtk::CheckButton>("brth_gbs"))->get_active()*4);
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    if(!compare){
        bank_1_modif.sound->extra.functions.brth_assgn.val = msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_aftrtch_rng_event(){
    LOG_IN();
    u_char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status & channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x4C;                        // 4C
    msg[5]=(get_gwidget<Gtk::SpinButton>("aftrtch_rng"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    if(!compare){
        bank_1_modif.sound->extra.functions.aftrtch_rng.val = msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_aftrtch_assgn_event(){
    LOG_IN();
    u_char msg[7];                      // paremter change
    msg[0]=0xF0;                        // F0
    msg[1]=id_fabricant;                // 43
    msg[2]=sub_status & channel_send;   // 10
    msg[3]=0x08;                        // 08
    msg[4]=0x4D;                        // 4D
    msg[5]= (get_gwidget<Gtk::CheckButton>("aftrtch_ptch"))->get_active()
    +((get_gwidget<Gtk::CheckButton>("aftrtch_mp"))->get_active()*2)
    +((get_gwidget<Gtk::CheckButton>("aftrtch_gbs"))->get_active()*4);
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    if(!compare){
        bank_1_modif.sound->extra.functions.aftrtch_assgn.val = msg[5];
    };
    LOG_OUT();
};

/* Compare */
void Dx7interface::on_compare_event(){
    // TODO: add set mute/unmute from struct
    // BUG: compare on empty object with load
    if ( (get_gwidget<Gtk::ToggleButton>("btn_compare"))->get_active() ) {
        std::cout<< "compare on"<< std::endl;
        compare=true;
        get_gwidget<Gtk::ToggleButton>("btn_compare")->add_css_class("blink");
        set_voice(&bank_1_origin.sound[0]);
        send_voice(&bank_1_origin.sound[0]);
        redraw_all_curve();
        block_ui();
        block_midi();
    }else{
        std::cout<< "compare off"<< std::endl;
        compare=false;
        get_gwidget<Gtk::ToggleButton>("btn_compare")->remove_css_class("blink");
        set_voice(&bank_1_modif.sound[0]);
        send_voice(&bank_1_modif.sound[0]);
        redraw_all_curve();
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

/* Panic */
void Dx7interface::on_panic_event(){
    u_char msg[3];
    msg[0]=0xB0;
    msg[1]=0x7B;
    msg[2]=0x00;
    send_midi(SND_SEQ_EVENT_CONTROLLER, 3, msg);
};

/* ALGO */
void Dx7interface::on_algo_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x06;
    msg[5]=(get_gwidget<Gtk::SpinButton>("algo_number"))->get_value()-1;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    if (!compare){
        bank_1_modif.sound->algo.algo.val = msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_algo"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_feedback_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x07;
    msg[5]=(get_gwidget<Gtk::SpinButton>("feedback"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->algo.feedback.val = msg[5];
    };
};

void Dx7interface::on_transpose_event() {
    char val =  (get_gwidget<Gtk::DropDown>("note_transpose"))->get_selected()
             +( ((get_gwidget<Gtk::SpinButton>("octv_transpose"))->get_value()-1)*12 );
    if( val >= 0 && val <= 48){
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel_send;
        msg[3]=0x01;
        msg[4]=0x10;
        msg[5]=val;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
        if (!compare){
            bank_1_modif.sound->algo.transpose.val = msg[5];
        };
    };
};

void Dx7interface::on_oks_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x08;
    if ( (get_gwidget<Gtk::CheckButton>("oks"))->get_active() ) {
        msg[5]=0x01;
    }else{
        msg[5]=0x00;
    }
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->algo.oks.val = msg[5];
    };
};

/* LFO */
void Dx7interface::on_lfo_wav_event() {
    LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x0E;
    msg[5]=(get_gwidget<Gtk::DropDown>("lfo_wav"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->lfo.wave.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_lfo_wav"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_lfo_sync_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x0D;
    if ( (get_gwidget<Gtk::CheckButton>("lfo_sync"))->get_active() ) {
        msg[5]=0x01;
    }else{
        msg[5]=0x00;
    }
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->lfo.sync.val=msg[5];
    };
};

void Dx7interface::on_speed_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x09;
    msg[5]=(get_gwidget<Gtk::SpinButton>("speed"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->lfo.speed.val = msg[5];
    };
};

void Dx7interface::on_delay_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x0A;
    msg[5]=(get_gwidget<Gtk::SpinButton>("delay"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->lfo.delay.val = msg[5];
    };
};

void Dx7interface::on_pmd_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x0B;
    msg[5]=(get_gwidget<Gtk::SpinButton>("pmd"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->lfo.pmd.val = msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_amd_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x0C;
    msg[5]=(get_gwidget<Gtk::SpinButton>("amd"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->lfo.amd.val = msg[5];
    };
    LOG_OUT();
};

/*LFO MODULATION */
void Dx7interface::on_pms_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x0F;
    msg[5]=(get_gwidget<Gtk::Scale>("pms"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->lfo.pms.val = msg[5];
    };
    LOG_OUT();
};

/* PITCH EG */
void Dx7interface::on_pitch_rt1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x7E;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_pitch"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->pitch.eg_rt[0].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_pitch_rt2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x7F;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_pitch"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->pitch.eg_rt[1].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_pitch_rt3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x00;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_pitch"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->pitch.eg_rt[2].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_pitch_rt4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x01;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_pitch"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->pitch.eg_rt[3].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_pitch_lvl1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x02;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_pitch"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->pitch.eg_lvl[0].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_pitch_lvl2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x03;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_pitch"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->pitch.eg_lvl[1].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_pitch_lvl3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x04;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_pitch"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->pitch.eg_lvl[2].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_pitch_lvl4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x05;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_pitch"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->pitch.eg_lvl[3].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    LOG_OUT();
};

/* MUTE FOR EACH OPERATOR IN DX7 */
void Dx7interface::on_mute_op_event() {
    u_char msg[7];
    uint8_t i,mute_val=0x00;

    for(i=1; i<=6;i++){
        bool widget_active = (bool)((get_gwidget<Gtk::ToggleButton>("mute_op"+tostr<uint>(i)))->get_active());
        mute_val=( mute_val | (!widget_active) ) ;
        if (i!=6){
            mute_val=mute_val << 1;
        };
        //std::cout << "on_mute_op mute_val : " << std::bitset<8>(mute_val) <<std::endl;
    };
    if(!compare){
        bank_1_modif.sound->extra.mute.val=mute_val;
    };
    for(i=1; i<=6;i++){
        (this->*mute_hexter_functions[i-1])();
    };
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x01;
    msg[4]=0x1B;
    msg[5]=mute_val;
    msg[6]=0xF7;
    if (!compare){
        bank_1_modif.sound->extra.mute.val=msg[5];
    };
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
};

/* view frequence for each operator  */
void Dx7interface::on_txt_freq_op_event()   {
    gdouble freq_val,ff,fc;
    uint i ;
    for ( i=1;i<=6;i++){
        fc=(get_gwidget<Gtk::SpinButton>("freq_coarse_op"+tostr<uint>(i)))->get_value();
        ff=(get_gwidget<Gtk::SpinButton>("freq_fine_op"+tostr<uint>(i)))->get_value();
        if ( (get_gwidget<Gtk::DropDown>("freq_mode_op"+tostr<uint>(i)))->get_selected() ) {
            (get_gwidget<Gtk::Label>("label_view_freq_op"+tostr<uint>(i)))->set_label("Hz");
            // calcul termitor thanks ^^
            gdouble A = exp(log(9.772)/99);
            freq_val = pow(A,ff);
            switch (uint(fc) & 3) {
                case 1: freq_val *=  10;
                    break;
                case 2: freq_val *=  100;
                    break;
                case 3: freq_val *=  1000;
                    break;
            };

        }else{
            (get_gwidget<Gtk::Label>("label_view_freq_op"+tostr<uint>(i)))->set_label("Rate");
            if (fc==0) {
                freq_val = 0.500+(0.005*ff);
            }else{
                freq_val = fc+((fc/100)*ff);
            };
        };
        (get_gwidget<Gtk::Entry>("entry_freq_op"+tostr<uint>(i)))->set_text(tostr(freq_val));
    };
};

/* OP1 */
void Dx7interface::on_ams_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x77;
    msg[5]=(get_gwidget<Gtk::Scale>("ams_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].ams.val=msg[5];
    };
    LOG_OUT();
};

/* OP1 FREQ */
void Dx7interface::on_freq_mode_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x7A;
    msg[5]=(get_gwidget<Gtk::DropDown>("freq_mode_op1"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].freq_mode.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_coarse_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x7B;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].freq_coarse.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_fine_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x7C;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].freq_fine.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_dtun_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x7D;
    msg[5]=(get_gwidget<Gtk::Scale>("dtun_op1"))->get_value()+7;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].dtun.val=msg[5];
    };
    LOG_OUT();
};

/* OP1 EG */
void Dx7interface::on_eg_rt1_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x69;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].eg_rt[0].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt2_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x6A;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].eg_rt[1].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt3_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x6B;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].eg_rt[2].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt4_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x6C;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].eg_rt[3].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl1_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x6D;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].eg_lvl[0].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl2_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x6E;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].eg_lvl[1].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl3_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x6F;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].eg_lvl[2].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl4_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x70;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].eg_lvl[3].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->queue_draw();
    LOG_OUT();
};

/* OP1 FRAME VOLUME */
void Dx7interface::on_krs_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x76;
    msg[5]=(get_gwidget<Gtk::Scale>("krs_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].krs.val=msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_kvs_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x78;
    msg[5]=(get_gwidget<Gtk::Scale>("kvs_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].kvs.val=msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_lvl_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x79;
    msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].lvl.val=msg[5];
    };
    /* logout fonction mute */
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op1"))->get_active() ) {
        (get_gwidget<Gtk::ToggleButton>("mute_op1"))->set_active(false);
    };
    LOG_OUT();
};


/* OP1 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op1_event(){
    LOG_IN();
    u_int8_t mute_val;
    if(!compare){
        mute_val = bank_1_modif.sound->extra.mute.val;
    }else{
        mute_val = bank_1_origin.sound->extra.mute.val;
    };
    mute_val = mute_val >>5;
    if ( !(mute_val & 0x01) ) {
        (get_gwidget<Gtk::Label>("label_general_op1"))->set_label(_("/* OP1 */"));
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel_send;
        msg[3]=0x00;
        msg[4]=0x79;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op1"))->set_label(_(" OP1 "));
        on_lvl_op1_event();
    };
    LOG_OUT();
};

/* OP1 KLS */
void Dx7interface::on_kls_lft_curve_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x74;
    msg[5]=(get_gwidget<Gtk::DropDown>("kls_lft_curve_op1"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].kls.lft_curve.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op1"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_rght_curve_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x75;
    msg[5]=(get_gwidget<Gtk::DropDown>("kls_rght_curve_op1"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].kls.rght_curve.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op1"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_lft_dpth_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x72;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].kls.lft_dpth.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op1"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_rght_dpth_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x73;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[0].kls.rght_dpth.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op1"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_brk_pt_op1_event() {
    char val_note =(get_gwidget<Gtk::DropDown>("note_brk_pt_op1"))->get_selected();
    char val_octv =(get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->get_value();
    if(val_note < 3){
        val_note = val_note+12;
    };
    char val = ((val_note) + (val_octv * 12));
    if( val >= 0 && val <= 99 ){
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel_send;
        msg[3]=0x00;
        msg[4]=0x71;
        msg[5]=val;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
        if (!compare){
            bank_1_modif.sound->op[0].kls.brk_pt.val=msg[5];
        };
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op1"))->queue_draw();
    };
};

/* OP2 */
void Dx7interface::on_ams_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x62;
    msg[5]=(get_gwidget<Gtk::Scale>("ams_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].ams.val=msg[5];
    };
    LOG_OUT();
};

/* OP2 FREQ */
void Dx7interface::on_freq_mode_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x65;
    msg[5]=(get_gwidget<Gtk::DropDown>("freq_mode_op2"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].freq_mode.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_coarse_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x66;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].freq_coarse.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_fine_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x67;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].freq_fine.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_dtun_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x68;
    msg[5]=(get_gwidget<Gtk::Scale>("dtun_op2"))->get_value()+7;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].dtun.val=msg[5];
    };
    LOG_OUT();
};

/* OP2 EG */
void Dx7interface::on_eg_rt1_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x54;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].eg_rt[0].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt2_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x55;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].eg_rt[1].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt3_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x56;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].eg_rt[2].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt4_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x57;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].eg_rt[3].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl1_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x58;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].eg_lvl[0].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl2_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x59;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].eg_lvl[1].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl3_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x5A;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].eg_lvl[2].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl4_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x5B;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].eg_lvl[3].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->queue_draw();
    LOG_OUT();
};

/* OP2 FRAME VOLUME */
void Dx7interface::on_krs_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x61;
    msg[5]=(get_gwidget<Gtk::Scale>("krs_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].krs.val=msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_kvs_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x63;
    msg[5]=(get_gwidget<Gtk::Scale>("kvs_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].kvs.val=msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_lvl_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x64;
    msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].lvl.val=msg[5];
    };
    // logout fonction mute
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op2"))->get_active() ) {
        (get_gwidget<Gtk::ToggleButton>("mute_op2"))->set_active(false);
    };
    LOG_OUT();
};

/* OP2 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op2_event() {
    LOG_IN();
    u_int8_t mute_val;
    if(!compare){
        mute_val = bank_1_modif.sound->extra.mute.val;
    }else{
        mute_val = bank_1_origin.sound->extra.mute.val;
    };
    mute_val = mute_val >>4;
    if ( !(mute_val & 0x01) ) {
        (get_gwidget<Gtk::Label>("label_general_op2"))->set_label(_("/* OP2 */"));
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel_send;
        msg[3]=0x00;
        msg[4]=0x64;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op2"))->set_label(_(" OP2 "));
        on_lvl_op2_event();
    };
    LOG_OUT();
};

/* OP2 KLS */
void Dx7interface::on_kls_lft_curve_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x5F;
    msg[5]=(get_gwidget<Gtk::DropDown>("kls_lft_curve_op2"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].kls.lft_curve.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op2"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_rght_curve_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x60;
    msg[5]=(get_gwidget<Gtk::DropDown>("kls_rght_curve_op2"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].kls.rght_curve.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op2"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_lft_dpth_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x5D;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].kls.lft_dpth.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op2"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_rght_dpth_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x5E;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[1].kls.rght_dpth.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op2"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_brk_pt_op2_event() {
    char val_note =(get_gwidget<Gtk::DropDown>("note_brk_pt_op2"))->get_selected();
    char val_octv =(get_gwidget<Gtk::SpinButton>("octv_brk_pt_op2"))->get_value();
    if(val_note < 3){
        val_note = val_note+12;
    };
    char val = ((val_note) + (val_octv * 12));
    if( val >= 0 && val <= 99 ){
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel_send;
        msg[3]=0x00;
        msg[4]=0x5C;
        msg[5]=val;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
        if (!compare){
            bank_1_modif.sound->op[1].kls.brk_pt.val=msg[5];
        };
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op2"))->queue_draw();
    };
};


/* OP3 */
void Dx7interface::on_ams_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x4D;
    msg[5]=(get_gwidget<Gtk::Scale>("ams_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].ams.val=msg[5];
    };
    LOG_OUT();
};

/* OP3 FREQ */
void Dx7interface::on_freq_mode_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x50;
    msg[5]=(get_gwidget<Gtk::DropDown>("freq_mode_op3"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].freq_mode.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_coarse_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x51;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].freq_coarse.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_fine_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x52;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].freq_fine.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_dtun_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x53;
    msg[5]=(get_gwidget<Gtk::Scale>("dtun_op3"))->get_value()+7;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].dtun.val=msg[5];
    };
    LOG_OUT();
};

/* OP3 EG */
void Dx7interface::on_eg_rt1_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x3F;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].eg_rt[0].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt2_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x40;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].eg_rt[1].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt3_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x41;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].eg_rt[2].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt4_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x42;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].eg_rt[3].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl1_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x43;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].eg_lvl[0].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl2_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x44;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].eg_lvl[1].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl3_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x45;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].eg_lvl[2].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl4_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x46;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].eg_lvl[3].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->queue_draw();
    LOG_OUT();
};

/* OP3 FRAME VOLUME */
void Dx7interface::on_krs_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x4C;
    msg[5]=(get_gwidget<Gtk::Scale>("krs_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].krs.val=msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_kvs_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x4E;
    msg[5]=(get_gwidget<Gtk::Scale>("kvs_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].kvs.val=msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_lvl_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x4F;
    msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].lvl.val=msg[5];
    };
    // logout fonction mute
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op3"))->get_active() ) {
        (get_gwidget<Gtk::ToggleButton>("mute_op3"))->set_active(false);
    };
    LOG_OUT();
};
/* OP3 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op3_event() {
    LOG_IN();
    u_int8_t mute_val;
    if(!compare){
        mute_val = bank_1_modif.sound->extra.mute.val;
    }else{
        mute_val = bank_1_origin.sound->extra.mute.val;
    };
    mute_val = mute_val >>3;
    if ( !(mute_val & 0x01) ) {
        (get_gwidget<Gtk::Label>("label_general_op3"))->set_label(_("/* OP3 */"));
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel_send;
        msg[3]=0x00;
        msg[4]=0x4F;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op3"))->set_label(_(" OP3 "));
        on_lvl_op3_event();
    };
    LOG_OUT();
};

/* OP3 KLS */
void Dx7interface::on_kls_lft_curve_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x4A;
    msg[5]=(get_gwidget<Gtk::DropDown>("kls_lft_curve_op3"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].kls.lft_curve.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op3"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_rght_curve_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x4B;
    msg[5]=(get_gwidget<Gtk::DropDown>("kls_rght_curve_op3"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].kls.rght_curve.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op3"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_lft_dpth_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x48;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].kls.lft_dpth.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op3"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_rght_dpth_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x49;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[2].kls.rght_dpth.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op3"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_brk_pt_op3_event() {	LOG_IN();
    char val_note =(get_gwidget<Gtk::DropDown>("note_brk_pt_op3"))->get_selected();
    char val_octv =(get_gwidget<Gtk::SpinButton>("octv_brk_pt_op3"))->get_value();
    if(val_note < 3){
        val_note = val_note+12;
    };
    char val = ((val_note) + (val_octv * 12));
    if( val >= 0 && val <= 99 ){
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel_send;
        msg[3]=0x00;
        msg[4]=0x47;
        msg[5]=val;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
        if (!compare){
            bank_1_modif.sound->op[2].kls.brk_pt.val=msg[5];
        };
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op3"))->queue_draw();
    };
};


/* OP4 */
void Dx7interface::on_ams_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x38;
    msg[5]=(get_gwidget<Gtk::Scale>("ams_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].ams.val=msg[5];
    };
    LOG_OUT();
};

/* OP4 FREQ*/
void Dx7interface::on_freq_mode_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x3B;
    msg[5]=(get_gwidget<Gtk::DropDown>("freq_mode_op4"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].freq_mode.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_coarse_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x3C;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].freq_coarse.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_fine_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x3D;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].freq_fine.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_dtun_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x3E;
    msg[5]=(get_gwidget<Gtk::Scale>("dtun_op4"))->get_value()+7;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].dtun.val=msg[5];
    };
    LOG_OUT();
};

/* OP4 EG*/
void Dx7interface::on_eg_rt1_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x2A;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].eg_rt[0].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt2_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x2B;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].eg_rt[1].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt3_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x2C;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].eg_rt[2].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt4_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x2D;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].eg_rt[3].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl1_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x2E;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].eg_lvl[0].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl2_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x2F;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].eg_lvl[1].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl3_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x30;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].eg_lvl[2].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl4_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x31;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].eg_lvl[3].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->queue_draw();
    LOG_OUT();
};


/* OP4 FRAME VOLUME */
void Dx7interface::on_krs_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x37;
    msg[5]=(get_gwidget<Gtk::Scale>("krs_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].krs.val=msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_kvs_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x39;
    msg[5]=(get_gwidget<Gtk::Scale>("kvs_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].kvs.val=msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_lvl_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x3A;
    msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].lvl.val=msg[5];
    };
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op4"))->get_active() ) {
        (get_gwidget<Gtk::ToggleButton>("mute_op4"))->set_active(false);
    }
    LOG_OUT();
};

/* OP4 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op4_event() {
    LOG_IN();
    u_int8_t mute_val;
    if(!compare){
        mute_val = bank_1_modif.sound->extra.mute.val;
    }else{
        mute_val = bank_1_origin.sound->extra.mute.val;
    };
    mute_val = mute_val >>2;
    if ( !(mute_val & 0x01) ) {
        (get_gwidget<Gtk::Label>("label_general_op4"))->set_label(_("/* OP4 */"));
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel_send;
        msg[3]=0x00;
        msg[4]=0x3A;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op4"))->set_label(_(" OP4 "));
        on_lvl_op4_event();
    };
    LOG_OUT();
};

/* OP4 KLS*/
void Dx7interface::on_kls_lft_curve_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x35;
    msg[5]=(get_gwidget<Gtk::DropDown>("kls_lft_curve_op4"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].kls.lft_curve.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op4"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_rght_curve_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x36;
    msg[5]=(get_gwidget<Gtk::DropDown>("kls_rght_curve_op4"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].kls.rght_curve.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op4"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_lft_dpth_op4_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x33;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].kls.lft_dpth.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op4"))->queue_draw();
};

void Dx7interface::on_kls_rght_dpth_op4_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x34;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[3].kls.rght_dpth.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op4"))->queue_draw();
};

void Dx7interface::on_kls_brk_pt_op4_event(){
    char val_note =(get_gwidget<Gtk::DropDown>("note_brk_pt_op4"))->get_selected();
    char val_octv =(get_gwidget<Gtk::SpinButton>("octv_brk_pt_op4"))->get_value();
    if(val_note < 3){
        val_note = val_note+12;
    };
    char val = ((val_note) + (val_octv * 12));
    if( val >= 0 && val <= 99 ){
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel_send;
        msg[3]=0x00;
        msg[4]=0x32;
        msg[5]=val;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
        if (!compare){
            bank_1_modif.sound->op[3].kls.brk_pt.val=msg[5];
        };
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op4"))->queue_draw();
    };
};

/* OP5 */
void Dx7interface::on_ams_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x23;
    msg[5]=(get_gwidget<Gtk::Scale>("ams_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].ams.val=msg[5];
    };
    LOG_OUT();
};

/* OP5 FREQ */
void Dx7interface::on_freq_mode_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x26;
    msg[5]=(get_gwidget<Gtk::DropDown>("freq_mode_op5"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].freq_mode.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_coarse_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x27;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].freq_coarse.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_fine_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x28;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].freq_fine.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_dtun_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x29;
    msg[5]=(get_gwidget<Gtk::Scale>("dtun_op5"))->get_value()+7;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].dtun.val=msg[5];
    };
    LOG_OUT();
};

/* OP5 EG */
void Dx7interface::on_eg_rt1_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x15;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].eg_rt[0].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt2_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x16;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].eg_rt[1].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt3_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x17;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].eg_rt[2].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt4_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x18;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].eg_rt[3].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl1_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x19;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].eg_lvl[0].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl2_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x1A;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].eg_lvl[1].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl3_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x1B;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].eg_lvl[2].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl4_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x1C;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].eg_lvl[3].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->queue_draw();
    LOG_OUT();
};

/* OP5 FRAME VOLUME */
void Dx7interface::on_krs_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x22;
    msg[5]=(get_gwidget<Gtk::Scale>("krs_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].krs.val=msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_kvs_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x24;
    msg[5]=(get_gwidget<Gtk::Scale>("kvs_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].kvs.val=msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_lvl_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x25;
    msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].lvl.val=msg[5];
    };
    // logout fonction mute
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op5"))->get_active() ) {
        (get_gwidget<Gtk::ToggleButton>("mute_op5"))->set_active(false);
    };
    LOG_OUT();
};

/* OP5 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op5_event() {
    LOG_IN();
    u_int8_t mute_val;
    if(!compare){
        mute_val = bank_1_modif.sound->extra.mute.val;
    }else{
        mute_val = bank_1_origin.sound->extra.mute.val;
    };
    mute_val = mute_val >>1;
    if ( !(mute_val & 0x01) ) {
        (get_gwidget<Gtk::Label>("label_general_op5"))->set_label(_("/* OP5 */"));
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel_send;
        msg[3]=0x00;
        msg[4]=0x25;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op5"))->set_label(_(" OP5 "));
        on_lvl_op5_event();
    };
    LOG_OUT();
};

/* OP5 KLS */
void Dx7interface::on_kls_lft_curve_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x20;
    msg[5]=(get_gwidget<Gtk::DropDown>("kls_lft_curve_op5"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].kls.lft_curve.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op5"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_rght_curve_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x21;
    msg[5]=(get_gwidget<Gtk::DropDown>("kls_rght_curve_op5"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].kls.rght_curve.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op5"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_lft_dpth_op5_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x1E;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].kls.lft_dpth.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op5"))->queue_draw();
};

void Dx7interface::on_kls_rght_dpth_op5_event(){
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x1F;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[4].kls.rght_dpth.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op5"))->queue_draw();
};

void Dx7interface::on_kls_brk_pt_op5_event() {
    char val_note =(get_gwidget<Gtk::DropDown>("note_brk_pt_op5"))->get_selected();
    char val_octv =(get_gwidget<Gtk::SpinButton>("octv_brk_pt_op5"))->get_value();
    if(val_note < 3){
        val_note = val_note+12;
    };
    char val = ((val_note) + (val_octv * 12));
    if( val >= 0 && val <= 99 ){
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel_send;
        msg[3]=0x00;
        msg[4]=0x1D;
        msg[5]=val;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
        if (!compare){
            bank_1_modif.sound->op[4].kls.brk_pt.val=msg[5];
        };
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op5"))->queue_draw();
    };
};

/* OP6 */
void Dx7interface::on_ams_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x0E;
    msg[5]=(get_gwidget<Gtk::Scale>("ams_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].ams.val=msg[5];
    };
    LOG_OUT();
};

/* OP6 FREQ */
void Dx7interface::on_freq_mode_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x11;
    msg[5]=(get_gwidget<Gtk::DropDown>("freq_mode_op6"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].freq_mode.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_coarse_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x12;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].freq_coarse.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_fine_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x13;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].freq_fine.val=msg[5];
    };
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_dtun_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x14;
    msg[5]=(get_gwidget<Gtk::Scale>("dtun_op6"))->get_value()+7;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].dtun.val=msg[5];
    };
    LOG_OUT();
};

/* OP6 EG */
void Dx7interface::on_eg_rt1_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x00;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].eg_rt[0].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt2_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x01;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].eg_rt[1].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt3_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x02;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].eg_rt[2].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_rt4_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x03;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].eg_rt[3].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl1_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x04;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].eg_lvl[0].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl2_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x05;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].eg_lvl[1].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl3_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x06;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].eg_lvl[2].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_eg_lvl4_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x07;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].eg_lvl[3].val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->queue_draw();
    LOG_OUT();
};

/* OP6 FRAME VOLUME */
void Dx7interface::on_krs_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x0D;
    msg[5]=(get_gwidget<Gtk::Scale>("krs_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].krs.val=msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_kvs_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x0F;
    msg[5]=(get_gwidget<Gtk::Scale>("kvs_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].kvs.val=msg[5];
    };
    LOG_OUT();
};

void Dx7interface::on_lvl_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x10;
    msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].lvl.val=msg[5];
    };
    // logout fonction mute
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op6"))->get_active() ) {
        (get_gwidget<Gtk::ToggleButton>("mute_op6"))->set_active(false);
    };
    LOG_OUT();
};

/* OP6 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op6_event() {
    LOG_IN();
    u_int8_t mute_val;
    if(!compare){
        mute_val = bank_1_modif.sound->extra.mute.val;
    }else{
        mute_val = bank_1_origin.sound->extra.mute.val;
    };
    if ( !(mute_val & 0x01) ) {
        (get_gwidget<Gtk::Label>("label_general_op6"))->set_label(_("/* OP6 */"));
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel_send;
        msg[3]=0x00;
        msg[4]=0x10;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op6"))->set_label(_(" OP6 "));
        on_lvl_op6_event();
    };
    LOG_OUT();
};

/* OP6 KLS */
void Dx7interface::on_kls_lft_curve_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x0B;
    msg[5]=(get_gwidget<Gtk::DropDown>("kls_lft_curve_op6"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].kls.lft_curve.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op6"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_rght_curve_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x0C;
    msg[5]=(get_gwidget<Gtk::DropDown>("kls_rght_curve_op6"))->get_selected();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].kls.rght_curve.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op6"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_lft_dpth_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x09;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].kls.lft_dpth.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op6"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_rght_dpth_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel_send;
    msg[3]=0x00;
    msg[4]=0x0A;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    if (!compare){
        bank_1_modif.sound->op[5].kls.rght_dpth.val=msg[5];
    };
    (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op6"))->queue_draw();
    LOG_OUT();
};

void Dx7interface::on_kls_brk_pt_op6_event() {
    char val_note =(get_gwidget<Gtk::DropDown>("note_brk_pt_op6"))->get_selected();
    char val_octv =(get_gwidget<Gtk::SpinButton>("octv_brk_pt_op6"))->get_value();
    if(val_note < 3){
        val_note = val_note+12;
    };
    char val = ((val_note) + (val_octv * 12));
    if( val >= 0 && val <= 99 ){
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel_send;
        msg[3]=0x00;
        msg[4]=0x08;
        msg[5]=val;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
        if (!compare){
            bank_1_modif.sound->op[5].kls.brk_pt.val=msg[5];
        };
        (get_gwidget<Gtk::DrawingArea>("drawingarea_kls_op6"))->queue_draw();
    };
};

void Dx7interface::set_aftrtch_assgn_event(int value){
    value = value/(127/bank_1_modif.sound->extra.functions.aftrtch_assgn.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    int val = value;
    (get_gwidget<Gtk::CheckButton>("aftrtch_ptch"))->set_active(val & 0x01);
    (get_gwidget<Gtk::CheckButton>("aftrtch_mp"))->set_active((val & 0x02)>>1);
    (get_gwidget<Gtk::CheckButton>("aftrtch_gbs"))->set_active((val & 0x04)>>2);
};
void Dx7interface::set_aftrtch_rng_event(int value){
    value = value/(127/bank_1_modif.sound->extra.functions.aftrtch_rng.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("aftrtch_rng"))->set_value(value);
};
void Dx7interface::set_algo_event(int value){
    LOG_IN();
    value = value/(127/bank_1_modif.sound->algo.algo.max);
    if(value <=0){
        value=0;
    }else if(value > bank_1_modif.sound->algo.algo.max){
        value=bank_1_modif.sound->algo.algo.max;
    };
    bank_1_modif.sound->algo.algo.val=(int)value;
    //(get_gwidget<Gtk::SpinButton>("algo_number"))->set_value((int)value+1);
    LOG_OUT();
};
void Dx7interface::set_amd_event(int value){
    value = value/(127/bank_1_modif.sound->lfo.amd.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->lfo.amd.max){
        value=bank_1_modif.sound->lfo.amd.max;
    };
    (get_gwidget<Gtk::SpinButton>("amd"))->set_value(value);
};
void Dx7interface::set_ams_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].ams.max);
    if(value <=0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].ams.max){
        value=bank_1_modif.sound->op[0].ams.max;
    };
    bank_1_modif.sound->op[0].ams.val = (int)value;
};

void Dx7interface::set_ams_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].ams.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::Scale>("ams_op2"))->set_value(value);
};
void Dx7interface::set_ams_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].ams.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::Scale>("ams_op3"))->set_value(value);
};
void Dx7interface::set_ams_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].ams.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::Scale>("ams_op4"))->set_value(value);
};
void Dx7interface::set_ams_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].ams.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::Scale>("ams_op5"))->set_value(value);
};
void Dx7interface::set_ams_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].ams.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::Scale>("ams_op6"))->set_value(value);
};
void Dx7interface::set_brth_assgn_event(int value){
    value = value/(127/bank_1_modif.sound->extra.functions.brth_assgn.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    int val = value;
    (get_gwidget<Gtk::CheckButton>("brth_ptch"))->set_active(val & 0x01);
    (get_gwidget<Gtk::CheckButton>("brth_mp"))->set_active((val & 0x02)>>1);
    (get_gwidget<Gtk::CheckButton>("brth_gbs"))->set_active((val & 0x04)>>2);
};
void Dx7interface::set_brth_rng_event(int value){
    value = value/(127/bank_1_modif.sound->extra.functions.brth_rng.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("brth_rng"))->set_value(value);
};
void Dx7interface::set_compare_event(int value){
    //value = value/(127/bank_1_modif.sound->;
};
void Dx7interface::set_delay_event(int value){
    value = value/(127/bank_1_modif.sound->lfo.delay.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("delay"))->set_value(value);
};
void Dx7interface::set_dtun_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].dtun.max);
    if( value < 0 ){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].dtun.max){
        value=bank_1_modif.sound->op[0].dtun.max;
    };
    (get_gwidget<Gtk::Scale>("dtun_op1"))->set_value(value-7);
};
void Dx7interface::set_dtun_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].dtun.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::Scale>("dtun_op2"))->set_value(value-7);
};
void Dx7interface::set_dtun_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].dtun.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::Scale>("dtun_op3"))->set_value(value-7);
};
void Dx7interface::set_dtun_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].dtun.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::Scale>("dtun_op4"))->set_value(value-7);
};
void Dx7interface::set_dtun_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].dtun.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::Scale>("dtun_op5"))->set_value(value-7);
};
void Dx7interface::set_dtun_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].dtun.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::Scale>("dtun_op6"))->set_value(value-7);
};
void Dx7interface::set_eg_lvl1_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].eg_lvl[0].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op1"))->set_value(value);
};
void Dx7interface::set_eg_lvl1_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].eg_lvl[0].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op2"))->set_value(value);
};
void Dx7interface::set_eg_lvl1_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].eg_lvl[0].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op3"))->set_value(value);
};
void Dx7interface::set_eg_lvl1_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].eg_lvl[0].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op4"))->set_value(value);
};
void Dx7interface::set_eg_lvl1_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].eg_lvl[0].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op5"))->set_value(value);
};
void Dx7interface::set_eg_lvl1_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].eg_lvl[0].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl1_op6"))->set_value(value);
};
void Dx7interface::set_eg_lvl2_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].eg_lvl[1].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op1"))->set_value(value);
};
void Dx7interface::set_eg_lvl2_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].eg_lvl[1].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op2"))->set_value(value);
};
void Dx7interface::set_eg_lvl2_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].eg_lvl[1].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op3"))->set_value(value);
};
void Dx7interface::set_eg_lvl2_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].eg_lvl[1].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op4"))->set_value(value);
};
void Dx7interface::set_eg_lvl2_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].eg_lvl[1].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op5"))->set_value(value);
};
void Dx7interface::set_eg_lvl2_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].eg_lvl[1].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl2_op6"))->set_value(value);
};
void Dx7interface::set_eg_lvl3_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].eg_lvl[2].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op1"))->set_value(value);
};
void Dx7interface::set_eg_lvl3_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].eg_lvl[2].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op2"))->set_value(value);
};
void Dx7interface::set_eg_lvl3_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].eg_lvl[2].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op3"))->set_value(value);
};
void Dx7interface::set_eg_lvl3_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].eg_lvl[2].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op4"))->set_value(value);
};
void Dx7interface::set_eg_lvl3_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].eg_lvl[2].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op5"))->set_value(value);
};
void Dx7interface::set_eg_lvl3_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].eg_lvl[2].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl3_op6"))->set_value(value);
};
void Dx7interface::set_eg_lvl4_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].eg_lvl[3].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op1"))->set_value(value);
};
void Dx7interface::set_eg_lvl4_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].eg_lvl[3].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op2"))->set_value(value);
};
void Dx7interface::set_eg_lvl4_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].eg_lvl[3].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op3"))->set_value(value);
};
void Dx7interface::set_eg_lvl4_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].eg_lvl[3].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op4"))->set_value(value);
};
void Dx7interface::set_eg_lvl4_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].eg_lvl[3].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op5"))->set_value(value);
};
void Dx7interface::set_eg_lvl4_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].eg_lvl[3].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_lvl4_op6"))->set_value(value);
};
void Dx7interface::set_eg_rt1_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].eg_rt[0].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op1"))->set_value(value);
};
void Dx7interface::set_eg_rt1_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].eg_rt[0].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op2"))->set_value(value);
};
void Dx7interface::set_eg_rt1_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].eg_rt[0].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op3"))->set_value(value);
};
void Dx7interface::set_eg_rt1_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].eg_rt[0].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op4"))->set_value(value);
};
void Dx7interface::set_eg_rt1_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].eg_rt[0].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op5"))->set_value(value);
};
void Dx7interface::set_eg_rt1_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].eg_rt[0].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt1_op6"))->set_value(value);
};
void Dx7interface::set_eg_rt2_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].eg_rt[1].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op1"))->set_value(value);
};
void Dx7interface::set_eg_rt2_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].eg_rt[1].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op2"))->set_value(value);
};
void Dx7interface::set_eg_rt2_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].eg_rt[1].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op2"))->set_value(value);
};
void Dx7interface::set_eg_rt2_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].eg_rt[1].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op3"))->set_value(value);
};
void Dx7interface::set_eg_rt2_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].eg_rt[1].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op5"))->set_value(value);
};
void Dx7interface::set_eg_rt2_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].eg_rt[1].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt2_op6"))->set_value(value);
};
void Dx7interface::set_eg_rt3_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].eg_rt[2].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op1"))->set_value(value);
};
void Dx7interface::set_eg_rt3_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].eg_rt[2].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op2"))->set_value(value);
};
void Dx7interface::set_eg_rt3_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].eg_rt[2].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op3"))->set_value(value);
};
void Dx7interface::set_eg_rt3_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].eg_rt[2].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op4"))->set_value(value);
};
void Dx7interface::set_eg_rt3_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].eg_rt[2].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op5"))->set_value(value);
};
void Dx7interface::set_eg_rt3_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].eg_rt[2].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt3_op6"))->set_value(value);
};
void Dx7interface::set_eg_rt4_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].eg_rt[3].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op1"))->set_value(value);
};
void Dx7interface::set_eg_rt4_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].eg_rt[3].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op2"))->set_value(value);
};
void Dx7interface::set_eg_rt4_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].eg_rt[3].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op3"))->set_value(value);
};
void Dx7interface::set_eg_rt4_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].eg_rt[3].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op4"))->set_value(value);
};
void Dx7interface::set_eg_rt4_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].eg_rt[3].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op5"))->set_value(value);
};
void Dx7interface::set_eg_rt4_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].eg_rt[3].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("eg_rt4_op6"))->set_value(value);
};
void Dx7interface::set_feedback_event(int value){
    value = value/(127/bank_1_modif.sound->algo.feedback.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("feedback"))->set_value(value);
};
void Dx7interface::set_foot_assgn_event(int value){
    value = value/(127/bank_1_modif.sound->extra.functions.foot_assgn.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    int val = value;
    (get_gwidget<Gtk::CheckButton>("foot_ptch"))->set_active(val & 0x01);
    (get_gwidget<Gtk::CheckButton>("foot_mp"))->set_active((val & 0x02)>>1);
    (get_gwidget<Gtk::CheckButton>("foot_gbs"))->set_active((val & 0x04)>>2);
};
void Dx7interface::set_foot_rng_event(int value){
    value = value/(127/bank_1_modif.sound->extra.functions.foot_rng.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("foot_rng"))->set_value(value);
};
void Dx7interface::set_freq_coarse_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].freq_coarse.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op1"))->set_value(value);
};
void Dx7interface::set_freq_coarse_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].freq_coarse.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op2"))->set_value(value);
};
void Dx7interface::set_freq_coarse_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].freq_coarse.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op3"))->set_value(value);
};
void Dx7interface::set_freq_coarse_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].freq_coarse.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op4"))->set_value(value);
};
void Dx7interface::set_freq_coarse_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].freq_coarse.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op5"))->set_value(value);
};
void Dx7interface::set_freq_coarse_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].freq_coarse.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("freq_coarse_op6"))->set_value(value);
};
void Dx7interface::set_freq_fine_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].freq_fine.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("freq_fine_op1"))->set_value(value);
};
void Dx7interface::set_freq_fine_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].freq_fine.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("freq_fine_op2"))->set_value(value);
};
void Dx7interface::set_freq_fine_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].freq_fine.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("freq_fine_op3"))->set_value(value);
};
void Dx7interface::set_freq_fine_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].freq_fine.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("freq_fine_op4"))->set_value(value);
};
void Dx7interface::set_freq_fine_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].freq_fine.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("freq_fine_op5"))->set_value(value);
};
void Dx7interface::set_freq_fine_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].freq_fine.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("freq_fine_op6"))->set_value(value);
};
void Dx7interface::set_freq_mode_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].freq_mode.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].freq_mode.max){
        value=bank_1_modif.sound->op[0].freq_mode.max;
    };
    (get_gwidget<Gtk::DropDown>("freq_mode_op1"))->set_selected(value);
};
void Dx7interface::set_freq_mode_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].freq_mode.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::DropDown>("freq_mode_op2"))->set_selected(value);
};
void Dx7interface::set_freq_mode_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].freq_mode.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::DropDown>("freq_mode_op3"))->set_selected(value);
};
void Dx7interface::set_freq_mode_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].freq_mode.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::DropDown>("freq_mode_op4"))->set_selected(value);
};
void Dx7interface::set_freq_mode_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].freq_mode.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::DropDown>("freq_mode_op5"))->set_selected(value);
};
void Dx7interface::set_freq_mode_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].freq_mode.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::DropDown>("freq_mode_op6"))->set_selected(value);
};
void Dx7interface::set_kls_brk_pt_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].kls.brk_pt.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op1"))->set_selected(value % 12);
    int val = (value -3);
    if(val < 0){
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->set_value( -1 );
    }else{
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->set_value( val / 12 );
    };
};
void Dx7interface::set_kls_brk_pt_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].kls.brk_pt.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op2"))->set_selected(value % 12);
    int val = (value -3);
    if(val < 0){
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op2"))->set_value( -1 );
    }else{
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op2"))->set_value( val / 12 );
    };
};
void Dx7interface::set_kls_brk_pt_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].kls.brk_pt.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op3"))->set_selected(value % 12);
    int val = (value -3);
    if(val < 0){
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op3"))->set_value( -1 );
    }else{
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op3"))->set_value( val / 12 );
    };
};
void Dx7interface::set_kls_brk_pt_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].kls.brk_pt.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op4"))->set_selected(value % 12);
    int val = (value -3);
    if(val < 0){
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op4"))->set_value( -1 );
    }else{
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op4"))->set_value( val / 12 );
    };
};
void Dx7interface::set_kls_brk_pt_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].kls.brk_pt.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op5"))->set_selected(value % 12);
    int val = (value -3);
    if(val < 0){
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op5"))->set_value( -1 );
    }else{
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op5"))->set_value( val / 12 );
    };
};
void Dx7interface::set_kls_brk_pt_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].kls.brk_pt.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::DropDown>("note_brk_pt_op6"))->set_selected(value % 12);
    int val = (value -3);
    if(val < 0){
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op6"))->set_value( -1 );
    }else{
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op6"))->set_value( val / 12 );
    };
};
void Dx7interface::set_kls_lft_curve_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].kls.lft_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].kls.lft_curve.max){
        value=bank_1_modif.sound->op[0].kls.lft_curve.max;
    };
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op1"))->set_selected(value);
};
void Dx7interface::set_kls_lft_curve_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].kls.lft_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].kls.lft_curve.max){
        value=bank_1_modif.sound->op[1].kls.lft_curve.max;
    };
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op2"))->set_selected(value);
};
void Dx7interface::set_kls_lft_curve_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].kls.lft_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].kls.lft_curve.max){
        value=bank_1_modif.sound->op[2].kls.lft_curve.max;
    };
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op3"))->set_selected(value);
};
void Dx7interface::set_kls_lft_curve_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].kls.lft_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].kls.lft_curve.max){
        value=bank_1_modif.sound->op[3].kls.lft_curve.max;
    };
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op4"))->set_selected(value);
};
void Dx7interface::set_kls_lft_curve_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].kls.lft_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].kls.lft_curve.max){
        value=bank_1_modif.sound->op[4].kls.lft_curve.max;
    };
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op5"))->set_selected(value);
};
void Dx7interface::set_kls_lft_curve_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].kls.lft_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].kls.lft_curve.max){
        value=bank_1_modif.sound->op[5].kls.lft_curve.max;
    };
    (get_gwidget<Gtk::DropDown>("kls_lft_curve_op6"))->set_selected(value);
};
void Dx7interface::set_kls_lft_dpth_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].kls.lft_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].kls.lft_dpth.max){
        value=bank_1_modif.sound->op[0].kls.lft_dpth.max;
    };
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op1"))->set_value(value);
};
void Dx7interface::set_kls_lft_dpth_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].kls.lft_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].kls.lft_dpth.max){
        value=bank_1_modif.sound->op[1].kls.lft_dpth.max;
    };
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op2"))->set_value(value);
};
void Dx7interface::set_kls_lft_dpth_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].kls.lft_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].kls.lft_dpth.max){
        value=bank_1_modif.sound->op[2].kls.lft_dpth.max;
    };
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op3"))->set_value(value);
};
void Dx7interface::set_kls_lft_dpth_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].kls.lft_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].kls.lft_dpth.max){
        value=bank_1_modif.sound->op[3].kls.lft_dpth.max;
    };
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op4"))->set_value(value);
};
void Dx7interface::set_kls_lft_dpth_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].kls.lft_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].kls.lft_dpth.max){
        value=bank_1_modif.sound->op[4].kls.lft_dpth.max;
    };
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op5"))->set_value(value);
};
void Dx7interface::set_kls_lft_dpth_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].kls.lft_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].kls.lft_dpth.max){
        value=bank_1_modif.sound->op[5].kls.lft_dpth.max;
    };
    (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op6"))->set_value(value);
};
void Dx7interface::set_kls_rght_curve_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].kls.rght_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].kls.rght_curve.max){
        value=bank_1_modif.sound->op[0].kls.rght_curve.max;
    };
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op1"))->set_selected(value);
};
void Dx7interface::set_kls_rght_curve_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].kls.rght_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].kls.rght_curve.max){
        value=bank_1_modif.sound->op[1].kls.rght_curve.max;
    };
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op2"))->set_selected(value);
};
void Dx7interface::set_kls_rght_curve_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].kls.rght_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].kls.rght_curve.max){
        value=bank_1_modif.sound->op[2].kls.rght_curve.max;
    };
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op3"))->set_selected(value);
};
void Dx7interface::set_kls_rght_curve_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].kls.rght_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].kls.rght_curve.max){
        value=bank_1_modif.sound->op[3].kls.rght_curve.max;
    };
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op4"))->set_selected(value);
};
void Dx7interface::set_kls_rght_curve_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].kls.rght_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].kls.rght_curve.max){
        value=bank_1_modif.sound->op[4].kls.rght_curve.max;
    };
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op5"))->set_selected(value);
};
void Dx7interface::set_kls_rght_curve_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].kls.rght_curve.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].kls.rght_curve.max){
        value=bank_1_modif.sound->op[5].kls.rght_curve.max;
    };
    (get_gwidget<Gtk::DropDown>("kls_rght_curve_op6"))->set_selected(value);
};
void Dx7interface::set_kls_rght_dpth_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].kls.rght_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].kls.rght_dpth.max){
        value=bank_1_modif.sound->op[0].kls.rght_dpth.max;
    };
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op1"))->set_value(value);
};
void Dx7interface::set_kls_rght_dpth_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].kls.rght_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].kls.rght_dpth.max){
        value=bank_1_modif.sound->op[1].kls.rght_dpth.max;
    };
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op2"))->set_value(value);
};
void Dx7interface::set_kls_rght_dpth_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].kls.rght_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].kls.rght_dpth.max){
        value=bank_1_modif.sound->op[2].kls.rght_dpth.max;
    };
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op3"))->set_value(value);
};
void Dx7interface::set_kls_rght_dpth_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].kls.rght_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].kls.rght_dpth.max){
        value=bank_1_modif.sound->op[3].kls.rght_dpth.max;
    };
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op4"))->set_value(value);
};
void Dx7interface::set_kls_rght_dpth_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].kls.rght_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].kls.rght_dpth.max){
        value=bank_1_modif.sound->op[4].kls.rght_dpth.max;
    };
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op5"))->set_value(value);
};
void Dx7interface::set_kls_rght_dpth_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].kls.rght_dpth.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].kls.rght_dpth.max){
        value=bank_1_modif.sound->op[5].kls.rght_dpth.max;
    };
    (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op6"))->set_value(value);
};
void Dx7interface::set_krs_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].krs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].krs.max){
        value=bank_1_modif.sound->op[0].krs.max;
    };
    (get_gwidget<Gtk::Scale>("krs_op1"))->set_value(value);
};
void Dx7interface::set_krs_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].krs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].krs.max){
        value=bank_1_modif.sound->op[1].krs.max;
    };
    (get_gwidget<Gtk::Scale>("krs_op2"))->set_value(value);
};
void Dx7interface::set_krs_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].krs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].krs.max){
        value=bank_1_modif.sound->op[2].krs.max;
    };
    (get_gwidget<Gtk::Scale>("krs_op3"))->set_value(value);
};
void Dx7interface::set_krs_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].krs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].krs.max){
        value=bank_1_modif.sound->op[3].krs.max;
    };
    (get_gwidget<Gtk::Scale>("krs_op4"))->set_value(value);
};
void Dx7interface::set_krs_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].krs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].krs.max){
        value=bank_1_modif.sound->op[4].krs.max;
    };
    (get_gwidget<Gtk::Scale>("krs_op5"))->set_value(value);
};
void Dx7interface::set_krs_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].krs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].krs.max){
        value=bank_1_modif.sound->op[5].krs.max;
    };
    bank_1_modif.sound->op[5].krs.val = value;
};
void Dx7interface::set_kvs_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].kvs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[0].kvs.max){
        value=bank_1_modif.sound->op[0].kvs.max;
    };
    (get_gwidget<Gtk::Scale>("kvs_op1"))->set_value(value);
};
void Dx7interface::set_kvs_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].kvs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[1].kvs.max){
        value=bank_1_modif.sound->op[1].kvs.max;
    };
    (get_gwidget<Gtk::Scale>("kvs_op2"))->set_value(value);
};
void Dx7interface::set_kvs_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].kvs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[2].kvs.max){
        value=bank_1_modif.sound->op[2].kvs.max;
    };
    (get_gwidget<Gtk::Scale>("kvs_op3"))->set_value(value);
};
void Dx7interface::set_kvs_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].kvs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[3].kvs.max){
        value=bank_1_modif.sound->op[3].kvs.max;
    };
    (get_gwidget<Gtk::Scale>("kvs_op4"))->set_value(value);
};
void Dx7interface::set_kvs_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].kvs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[4].kvs.max){
        value=bank_1_modif.sound->op[4].kvs.max;
    };
    (get_gwidget<Gtk::Scale>("kvs_op5"))->set_value(value);
};
void Dx7interface::set_kvs_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].kvs.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->op[5].kvs.max){
        value=bank_1_modif.sound->op[5].kvs.max;
    };
    (get_gwidget<Gtk::Scale>("kvs_op6"))->set_value(value);
};
void Dx7interface::set_lfo_sync_event(int value){
    value = value/(127/bank_1_modif.sound->lfo.sync.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->lfo.sync.max){
        value=bank_1_modif.sound->lfo.sync.max;
    };
    (get_gwidget<Gtk::CheckButton>("lfo_sync"))->set_active(value);
};
void Dx7interface::set_lfo_wav_event(int value){
    value = value/(127/bank_1_modif.sound->lfo.wave.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->lfo.wave.max){
        value=bank_1_modif.sound->lfo.wave.max;
    };
    (get_gwidget<Gtk::DropDown>("lfo_wav"))->set_selected(value);
};
void Dx7interface::set_lvl_op1_event(int value){
    value = value/(127/bank_1_modif.sound->op[0].lvl.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("lvl_op1"))->set_value(value);
};
void Dx7interface::set_lvl_op2_event(int value){
    value = value/(127/bank_1_modif.sound->op[1].lvl.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("lvl_op2"))->set_value(value);
};
void Dx7interface::set_lvl_op3_event(int value){
    value = value/(127/bank_1_modif.sound->op[2].lvl.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("lvl_op3"))->set_value(value);
};
void Dx7interface::set_lvl_op4_event(int value){
    value = value/(127/bank_1_modif.sound->op[3].lvl.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("lvl_op4"))->set_value(value);
};
void Dx7interface::set_lvl_op5_event(int value){
    value = value/(127/bank_1_modif.sound->op[4].lvl.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("lvl_op5"))->set_value(value);
};
void Dx7interface::set_lvl_op6_event(int value){
    value = value/(127/bank_1_modif.sound->op[5].lvl.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("lvl_op6"))->set_value(value);
};
void Dx7interface::set_md_whl_assgn_event(int value){
    value = value/(127/bank_1_modif.sound->extra.functions.md_whl_assgn.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    u_char val = value;
    (get_gwidget<Gtk::CheckButton>("md_whl_ptch"))->set_active( (val & 0x01) );
    (get_gwidget<Gtk::CheckButton>("md_whl_mp"))->set_active( (val & 0x02)>>1 );
    (get_gwidget<Gtk::CheckButton>("md_whl_gbs"))->set_active( (val & 0x04)>>2 );
};
void Dx7interface::set_md_whl_rng_event(int value){
    value = value/(127/bank_1_modif.sound->extra.functions.md_whl_rng.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("md_whl_rng"))->set_value(value);
};
void Dx7interface::set_mono_poly_event(int value){
    value = value/(127/bank_1_modif.sound->extra.functions.poly_mono.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::ToggleButton>("btn_poly_mono"))->set_active(value);
};

void Dx7interface::set_mute_op1_event(int value){
    if( (value >= 63 && ( (bank_1_modif.sound->extra.mute.val >> 5) & 0x01 )) || (value < 63  && !( (bank_1_modif.sound->extra.mute.val >> 5) & 0x01 ) ) ){
        value=0x20;
    }else{
        value=0x00;
    };
    bank_1_modif.sound->extra.mute.val=bank_1_modif.sound->extra.mute.val ^ value;
};
void Dx7interface::set_mute_op2_event(int value){
    if( (value >= 63 && ( (bank_1_modif.sound->extra.mute.val >> 4) & 0x01 )) || (value < 63  && !( (bank_1_modif.sound->extra.mute.val >> 4) & 0x01 ) ) ){
        value=0x10;
    }else{
        value=0x00;
    };
    bank_1_modif.sound->extra.mute.val=bank_1_modif.sound->extra.mute.val ^ value;
};
void Dx7interface::set_mute_op3_event(int value){
    if( (value >= 63 && ( (bank_1_modif.sound->extra.mute.val >> 3) & 0x01 )) || (value < 63  && !( (bank_1_modif.sound->extra.mute.val >> 3) & 0x01 ) ) ){
        value=0x08;
    }else{
        value=0x00;
    };
    bank_1_modif.sound->extra.mute.val=bank_1_modif.sound->extra.mute.val ^ value;
};
void Dx7interface::set_mute_op4_event(int value){
    if( (value >= 63 && ( (bank_1_modif.sound->extra.mute.val >> 2) & 0x01 )) || (value < 63  && !( (bank_1_modif.sound->extra.mute.val >> 2) & 0x01 ) ) ){
        value=0x04;
    }else{
        value=0x00;
    };
    bank_1_modif.sound->extra.mute.val=bank_1_modif.sound->extra.mute.val ^(value << 2);
};
void Dx7interface::set_mute_op5_event(int value){
    if( (value >= 63 && ( (bank_1_modif.sound->extra.mute.val >> 1) & 0x01 )) || (value < 63  && !( (bank_1_modif.sound->extra.mute.val >> 1) & 0x01 ) ) ){
        value=0x02;
    }else{
        value=0x00;
    };
    bank_1_modif.sound->extra.mute.val=bank_1_modif.sound->extra.mute.val ^ (value << 1);
};
void Dx7interface::set_mute_op6_event(int value){
    if( (value >= 63 && ( (bank_1_modif.sound->extra.mute.val) & 0x01 )) || (value < 63  && !( (bank_1_modif.sound->extra.mute.val) & 0x01 ) ) ){
        value=0x01;
    }else{
        value=0x00;
    };
    bank_1_modif.sound->extra.mute.val = bank_1_modif.sound->extra.mute.val ^ value;
};

void Dx7interface::set_oks_event(int value){
    value = value/(127/bank_1_modif.sound->algo.oks.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::CheckButton>("oks"))->set_active(value);
};
void Dx7interface::set_panic_event(int value){
    //value = value/(127/bank_1_modif.sound->max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
};
void Dx7interface::set_pitch_lvl1_event(int value){
    value = value/(127/bank_1_modif.sound->pitch.eg_lvl[0].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    get_gwidget<Gtk::SpinButton>("eg_lvl1_pitch")->set_value(value);
};
void Dx7interface::set_pitch_lvl2_event(int value){
    value = value/(127/bank_1_modif.sound->pitch.eg_lvl[1].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    get_gwidget<Gtk::SpinButton>("eg_lvl2_pitch")->set_value(value);
};
void Dx7interface::set_pitch_lvl3_event(int value){
    value = value/(127/bank_1_modif.sound->pitch.eg_lvl[2].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    get_gwidget<Gtk::SpinButton>("eg_lvl3_pitch")->set_value(value);
};
void Dx7interface::set_pitch_lvl4_event(int value){
    value = value/(127/bank_1_modif.sound->pitch.eg_lvl[3].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    get_gwidget<Gtk::SpinButton>("eg_lvl4_pitch")->set_value(value);
};
void Dx7interface::set_pitch_rt1_event(int value){
    value = value/(127/bank_1_modif.sound->pitch.eg_rt[0].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    get_gwidget<Gtk::SpinButton>("eg_rt1_pitch")->set_value(value);
};
void Dx7interface::set_pitch_rt2_event(int value){
    value = value/(127/bank_1_modif.sound->pitch.eg_rt[1].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    get_gwidget<Gtk::SpinButton>("eg_rt2_pitch")->set_value(value);
};
void Dx7interface::set_pitch_rt3_event(int value){
    value = value/(127/bank_1_modif.sound->pitch.eg_rt[2].max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    get_gwidget<Gtk::SpinButton>("eg_rt3_pitch")->set_value(value);
};
void Dx7interface::set_pitch_rt4_event(int value){
    value = value/(127/bank_1_modif.sound->pitch.eg_rt[3].max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->pitch.eg_rt[3].max){
        value=bank_1_modif.sound->pitch.eg_rt[3].max;
    };
    get_gwidget<Gtk::SpinButton>("eg_rt4_pitch")->set_value(value);
};
void Dx7interface::set_pmd_event(int value){
    value = value/(127/bank_1_modif.sound->lfo.pmd.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->lfo.pmd.max){
        value=bank_1_modif.sound->lfo.pmd.max;
    };
    (get_gwidget<Gtk::SpinButton>("pmd"))->set_value(value);
};
void Dx7interface::set_pms_event(int value){
    value = value/(127/bank_1_modif.sound->lfo.pms.max);
    if(value <0){
        value=0;
    }else if(value > bank_1_modif.sound->lfo.pms.max){
        value=bank_1_modif.sound->lfo.pms.max;
    };
    (get_gwidget<Gtk::Scale>("pms"))->set_value(value);
};
void Dx7interface::set_portamento_glss_event(int value){
    value = value/(127/bank_1_modif.sound->extra.functions.portamento_glss.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::ToggleButton>("btn_portamento_glss"))->set_active(value);
};
void Dx7interface::set_portamento_md_event(int value){
    value = value/(127/bank_1_modif.sound->extra.functions.portamento_md.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::ToggleButton>("btn_portamento_md"))->set_active(value);
};
void Dx7interface::set_portamento_tm_event(int value){
    value = value/(127/bank_1_modif.sound->extra.functions.portamento_tm.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("portamento_tm"))->set_value(value);
};
void Dx7interface::set_ptch_bnd_rng_event(int value){
    value = value/(127/bank_1_modif.sound->extra.functions.ptch_bnd_rng.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::Scale>("ptch_bnd_rng"))->set_value(value);
};
void Dx7interface::set_ptch_bnd_stp_event(int value){
    value = value/(127/bank_1_modif.sound->extra.functions.ptch_bnd_stp.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::Scale>("ptch_bnd_stp"))->set_value(value);
};
void Dx7interface::set_send_extra_parameters_event(int value){

};
void Dx7interface::set_speed_event(int value){
    value = value/(127/bank_1_modif.sound->lfo.speed.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("speed"))->set_value(value);
};
void Dx7interface::set_transpose_event(int value){
    value = value/(127/bank_1_modif.sound->algo.transpose.max);
    if(value <0){
        value=0;
    }else if(value > 127){
        value=127;
    };
    (get_gwidget<Gtk::SpinButton>("octv_transpose"))->set_value( (value / 12)+1 );
    (get_gwidget<Gtk::DropDown>("note_transpose"))->set_selected(value % 12);
};


void Dx7interface::block_ui(){
    /*** Block UI ***/
    /* algo */
    slot_algo.block(true);
    slot_feedback.block(true);
    slot_note_transpose.block(true);
    slot_octv_transpose.block(true);
    slot_oks.block(true);
    /* general lfo */
    slot_lfo_wav.block(true);
    slot_lfo_sync.block(true);
    slot_speed.block(true);
    slot_delay.block(true);
    slot_pmd.block(true);
    slot_amd.block(true);
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
    /* algo */
    slot_algo.unblock();
    slot_feedback.unblock();
    slot_octv_transpose.unblock();
    slot_note_transpose.unblock();
    slot_oks.unblock();
    /* general lfo */
    slot_lfo_wav.unblock();
    slot_lfo_sync.unblock();
    slot_speed.unblock();
    slot_delay.unblock();
    slot_pmd.unblock();
    slot_amd.unblock();
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
    LOG_IN();
    slot_bank_select.disconnect();
    /* Sound Select */
    slot_selected_sound_change.disconnect();

    /* Algo */
    slot_algo.disconnect();
    slot_feedback.disconnect();
    slot_note_transpose.disconnect();
    slot_octv_transpose.disconnect();
    slot_oks.disconnect();

    /* lfo */
    slot_lfo_wav.disconnect();
    slot_lfo_sync.disconnect();
    slot_speed.disconnect();
    slot_delay.disconnect();
    slot_pmd.disconnect();
    slot_amd.disconnect();
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
    LOG_OUT();
};
#endif /* Dx7interface_CC */
