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
    #ifdef ENABLE_NLS
        setlocale (LC_ALL, "");
        bindtextdomain (GETTEXT_PACKAGE, PROGRAMNAME_LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);
    #endif
    /* I/O init */
    Gio::init();
    if (index <= 0){
        set_app_name(MODULE_NAME);
    }else{
        set_app_name(MODULE_NAME+index);
    };
    // done in gxmodule
    //set_style_file(CSSFILE);
    //apply_style();

    /* MIDI */
    /* Yamaha specific */
    Synth::id_fabricant=id_fabricant;
    /* set channel & sub_status */
    Synth::channel=0xF0;
    Synth::sub_status=0x10;
    seq_handle=get_seq_handler();
    ev=get_seq_event_handler();
    /* UI */
    m_refListStore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(get_gobject("liststore_bank"));
    uint val = (get_gwidget<Gtk::SpinButton>("algo_number"))->get_value();
    (get_gwidget<Gtk::Image>("image_algo"))->set(MOD_IMG_DIRECTORY"/algo"+tostr<uint>(val)+".png");
    Glib::ustring name;
    ((get_gwidget<Gtk::ComboBox>("lfo_wav"))->get_active())->get_value(0,name) ;
    (get_gwidget<Gtk::Image>("image_lfo"))->set(MOD_IMG_DIRECTORY"/"+name+".png");
    attach_signals();
    /* start thread */
    S_Thread();

    LOG_OUT();
};

Dx7interface::~Dx7interface(){
    LOG_IN();
    /* basic destructor*/
    /* terminate thread */
    //block_all();
    T_Thread();
    dettach_signals();
    LOG_OUT();
};

bool Dx7interface::Run(){
    /* Thread looped function */
    listen_midi();
    return true;
};

void Dx7interface::listen_midi(){
    /* TODO : use all seq event */
    /* TODO : use all seq event */
    snd_seq_event_input(seq_handle, &ev);

    std::cout << std::endl;
    std::cout << "event: " << get_event_name(int(ev->type)) << " "
    << "type: " << int(ev->type)<< std::endl;
    std::cout << "flags: " << int(ev->flags) << " "
    << "tag: " << int( ev->tag) << '\t'
    << "queue: " << int(ev->queue) << std::endl;
    std::cout << "ticks: " << int(ev->time.tick) << " "
    << "time: " << int(ev->time.time.tv_sec) << std::endl;
    std::cout << "source: " << int( ev->source.client) << " " << '\t'
    << "dest: " << int(ev->dest.client) << std::endl;

    switch (ev->type) {
        case SND_SEQ_EVENT_NOTEON:
            std::cout << "Channel: "  << (int(ev->data.control.channel) +1) << " " << '\t'
            << "value: " << int(ev->data.note.note) << std::endl;;
            break;
        case SND_SEQ_EVENT_NOTEOFF:
            std::cout << "Channel: "  << (int(ev->data.control.channel) +1) << " " << '\t'
            << "value: " <<  int(ev->data.note.note) << std::endl;
            break;
        case SND_SEQ_EVENT_CONTROLLER:
            std::cout << "Channel: " << (int(ev->data.control.channel) +1) << " " << '\t'
            << "param: "  << ev->data.control.param << " "
            << "value: " << int(ev->data.control.value) << std::endl;
            break;
        case SND_SEQ_EVENT_PITCHBEND:
            std::cout << "Channel: " << (int(ev->data.control.channel) +1)<< " " << '\t'
            << "value: " << int(ev->data.control.value) << std::endl;
            break;
        case SND_SEQ_EVENT_PGMCHANGE:
            /*event data type = snd_seq_ev_ctrl_t */
            std::cout <<  "Channel : "  << (int(ev->data.control.channel) +1) << '\t'
            << "param : "  << ev->data.control.param << " "
            << "value : " << int(ev->data.control.value)
            << std::endl;
            break;
    };
    std::cout << std::endl;
    snd_seq_free_event(ev);
};

/* load raw bank file */
void Dx7interface::load_bank(Glib::RefPtr<Gio::File> bank_file){
    /* open file */
    try {
        uint8_t i;
        data_stream = Gio::DataInputStream::create(bank_file->read());
        uint file_size = (bank_file->query_info(G_FILE_ATTRIBUTE_STANDARD_SIZE))->get_size();
        Glib::ustring filename = (bank_file->query_info(G_FILE_ATTRIBUTE_STANDARD_NAME))->get_name();
        Glib::ustring ext = filename.substr(  filename.find_last_of(".")+1, filename.length() );
        if ( ext == "syx" && data_stream->read_byte() == 0xF0) {
            for ( i=0; i < 5; i++){
                data_stream->read_byte();
            };
            file_size -= 8;
        };
        switch ( file_size ){
            case 128: /* one voice */
                seek_voice(0, &bank_1_origin.sound[0]);
                bank_1_modif=bank_1_origin;
                break;
            case 4096: /* 32 voices */
                for( i = 0; i < 32; i++ ){
                    seek_voice(i,&bank_32_origin.sound[i]);
                };
                bank_32_modif=bank_32_origin;
                break;
            case 16384: /* 128 voices */
                for( i = 0; i < 128; i++ ){
                    seek_voice(i, &bank_128_origin.sound[i]);
                };
                bank_128_modif=bank_128_origin;
                break;
        };
        data_stream->close();
    }catch(const Glib::Exception& ex){
        std::cerr << "Exception caught: " << ex.what() << std::endl;
    };
};

bool Dx7interface::error(){
    std::cout << "erreur fichier syx " << std::endl;
    return true;
};

void Dx7interface::clean_bank(){
    // TODO clean bank_modif
    m_refListStore->clear();
};

void Dx7interface::seek_voice(uint8_t i, St_dx7sysex_1* sound){
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
        sound->op[j].ams.val=val &  0x03;
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
    /* add voice name to treeview */
    /* create row */
    Gtk::TreeIter iter = m_refListStore->append();
    Gtk::TreeModel::Row row = *(iter);
    /* C++ version*/
    /*
    row->set_value(1,sound->name);
    row->set_value(0, i+1);
    */

    /* C version working*/
    gtk_list_store_set(m_refListStore->gobj(), iter.gobj(), 0, i+1, 1,sound->name.c_str(), -1);
};


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
    msg[2]=0x00 & channel;
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
    LOG_OUT();
};


void Dx7interface::set_voice(St_dx7sysex_1* sound){ LOG_IN();
    /* set value in each widget from modif */
    uint8_t j,k;
    /* general */
    /* ALGO */
    (get_gwidget<Gtk::SpinButton>("algo_number"))->set_value(sound->algo.algo.val+1);
    (get_gwidget<Gtk::Image>("image_algo"))->set(MOD_IMG_DIRECTORY"/algo"+tostr<uint>(sound->algo.algo.val+1)+".png");
    (get_gwidget<Gtk::SpinButton>("feedback"))->set_value(sound->algo.feedback.val);
        /*	[0-11] + (([1-5]-1)*12)	*/
    (get_gwidget<Gtk::ComboBox>("note_transpose"))->set_active(sound->algo.transpose.val % 12);
    (get_gwidget<Gtk::SpinButton>("octv_transpose"))->set_value( (sound->algo.transpose.val / 12)+1 );
    (get_gwidget<Gtk::CheckButton>("oks"))->set_active(sound->algo.oks.val);
    /* LFO */
    (get_gwidget<Gtk::ComboBox>("lfo_wav"))->set_active(sound->lfo.wave.val);
    Glib::ustring name;
    ((get_gwidget<Gtk::ComboBox>("lfo_wav"))->get_active())->get_value(0,name) ;
    (get_gwidget<Gtk::Image>("image_lfo"))->set(MOD_IMG_DIRECTORY"/"+name+".png");
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
    for ( j=0;j<6;j++){
    /* AMS */
        (get_gwidget<Gtk::Scale>("ams_op"+tostr<uint>(j+1)))->set_value(sound->op[j].ams.val);
    /* FREQUENCE */
        (get_gwidget<Gtk::ComboBox>("freq_mode_op"+tostr<uint>(j+1)))->set_active(sound->op[j].freq_mode.val);
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
    /* VOLUME */
        /* KRS */
        (get_gwidget<Gtk::Scale>("krs_op"+tostr<uint>(j+1)))->set_value(sound->op[j].krs.val);
        /* KVS */
        (get_gwidget<Gtk::Scale>("kvs_op"+tostr<uint>(j+1)))->set_value(sound->op[j].kvs.val);
        /* LVL */
        (get_gwidget<Gtk::SpinButton>("lvl_op"+tostr<uint>(j+1)))->set_value(sound->op[j].lvl.val);
        /* MUTE */
            /* NO MUTE VALUE IN STD SYSEX CAN BE ADD IN LEFT SPACE */
        /* KLS */
        (get_gwidget<Gtk::ComboBox>("kls_lft_curve_op"+tostr<uint>(j+1)))->set_active(sound->op[j].kls.lft_curve.val);
        (get_gwidget<Gtk::ComboBox>("kls_rght_curve_op"+tostr<uint>(j+1)))->set_active(sound->op[j].kls.rght_curve.val);
        (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op"+tostr<uint>(j+1)))->set_value(sound->op[j].kls.lft_dpth.val);
        (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op"+tostr<uint>(j+1)))->set_value(sound->op[j].kls.rght_curve.val);
        (get_gwidget<Gtk::ComboBox>("note_brk_pt_op"+tostr<uint>(j+1)))->set_active(sound->op[j].kls.brk_pt.val % 12);
        (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op"+tostr<uint>(j+1)))->set_value( (sound->op[j].kls.brk_pt.val / 12) );
    };
    (get_window())->set_title(sound->name.c_str());
    LOG_OUT();
};

/* UI SIGNALS CONNECTION */

bool Dx7interface::on_bank_reveal(GdkEventButton* event){
    LOG_IN();
    (get_gwidget<Gtk::Revealer>("revealer_bank"))->set_reveal_child(!(get_gwidget<Gtk::Revealer>("revealer_bank"))->get_reveal_child());
    return true;
    LOG_OUT();
};

void Dx7interface::attach_signals(){
    LOG_IN();
    //slot_OBJECT_NAME = (je recupere l'object)->sur le signal de l'evenement.je connecte( le signal de ( la fonction ));
    // (i get the ui object)->on_signal_of_event.I_connect(the_signal_of(the_function));

    /*  prototype bind button
    m_button1.signal_clicked().connect(
        sigc::bind<Glib::ustring>( sigc::mem_fun(*this, &HelloWorld::on_button_clicked), "button 1") );
    void on_button_clicked(Glib::ustring data){

    };
    */

    /*** generale  ***/
    /* Bank load */
    (get_gwidget<Gtk::Toolbar>("toolbar_reveal_bank"))->add_events(Gdk::BUTTON_PRESS_MASK);
    slot_bank_reveal = (get_gwidget<Gtk::Toolbar>("toolbar_reveal_bank"))->signal_button_press_event().connect(
        sigc::mem_fun(*this, &Dx7interface::on_bank_reveal));

    slot_bank_select = (get_gwidget<Gtk::FileChooserButton>("bank_select"))->signal_selection_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_bank_select));
    /* Sound Select */
    slot_treeview_row_clicked = (get_gwidget<Gtk::TreeView>("treeview_bank"))->signal_cursor_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_treeview_row_clicked));

    /* Algo */
    (get_gwidget<Gtk::SpinButton>("algo_number"))->set_size_request(0, 0);
    (get_gwidget<Gtk::SpinButton>("feedback"))->set_size_request(0, 0);
    (get_gwidget<Gtk::SpinButton>("octv_transpose"))->set_size_request(0, 0);

    slot_algo = (get_gwidget<Gtk::SpinButton>("algo_number"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_algo_event));
    slot_feedback = (get_gwidget<Gtk::SpinButton>("feedback"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_feedback_event));
    slot_note_transpose = (get_gwidget<Gtk::ComboBox>("note_transpose"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_transpose_event));
    slot_octv_transpose = (get_gwidget<Gtk::SpinButton>("octv_transpose"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_transpose_event));
    slot_oks = (get_gwidget<Gtk::CheckButton>("oks"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_oks_event));

    /* lfo */
    slot_lfo_wav = (get_gwidget<Gtk::ComboBox>("lfo_wav"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lfo_wav_event));
    slot_lfo_sync = (get_gwidget<Gtk::CheckButton>("lfo_sync"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lfo_sync_event));
    slot_speed = (get_gwidget<Gtk::SpinButton>("speed"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_speed_event));
    slot_delay = (get_gwidget<Gtk::SpinButton>("delay"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_delay_event));
    slot_pmd = (get_gwidget<Gtk::SpinButton>("pmd"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pmd_event));
    slot_amd = (get_gwidget<Gtk::SpinButton>("amd"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_amd_event));
    /* lfo modulation */
    slot_pms = (get_gwidget<Gtk::Scale>("pms"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pms_event));

    /* pitch eg */
    slot_pitch_rt1 = (get_gwidget<Gtk::SpinButton>("eg_rt1_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_rt1_event));
    slot_pitch_rt2 = (get_gwidget<Gtk::SpinButton>("eg_rt2_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_rt2_event));
    slot_pitch_rt3 = (get_gwidget<Gtk::SpinButton>("eg_rt3_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_rt3_event));
    slot_pitch_rt4 = (get_gwidget<Gtk::SpinButton>("eg_rt4_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_rt4_event));
    slot_pitch_lvl1 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_lvl1_event));
    slot_pitch_lvl2 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_lvl2_event));
    slot_pitch_lvl3 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_lvl3_event));
    slot_pitch_lvl4 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_pitch"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_pitch_lvl4_event));

    /* Drawing area for pitch */
    slot_pitch_draw_eg = (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->signal_draw().connect(
        sigc::mem_fun(*this, &Dx7interface::on_draw_pitch_event));

    /* Drawing area for each operator */
    //for (uint i=1;i<=6;i++) {	    // for operator
    slot_op1_draw = (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op1"))->signal_draw().connect(
            sigc::bind<Glib::ustring>( sigc::mem_fun(*this, &Dx7interface::on_draw_op_event), "1" ) );
    slot_op2_draw = (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op2"))->signal_draw().connect(
            sigc::bind<Glib::ustring>( sigc::mem_fun(*this, &Dx7interface::on_draw_op_event), "2" ) );
    slot_op3_draw = (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op3"))->signal_draw().connect(
            sigc::bind<Glib::ustring>( sigc::mem_fun(*this, &Dx7interface::on_draw_op_event), "3" ) );
    slot_op4_draw = (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op4"))->signal_draw().connect(
            sigc::bind<Glib::ustring>( sigc::mem_fun(*this, &Dx7interface::on_draw_op_event), "4" ) );
    slot_op5_draw = (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op5"))->signal_draw().connect(
            sigc::bind<Glib::ustring>( sigc::mem_fun(*this, &Dx7interface::on_draw_op_event), "5" ) );
    slot_op6_draw = (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op6"))->signal_draw().connect(
            sigc::bind<Glib::ustring>( sigc::mem_fun(*this, &Dx7interface::on_draw_op_event), "6" ) );
    //};

    /* OP1 */
    slot_ams_op1 = (get_gwidget<Gtk::Scale>("ams_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op1_event)); //frame lfo
    /* OP1 FREQUENCE */
    slot_freq_mode_op1 = (get_gwidget<Gtk::ComboBox>("freq_mode_op1"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op1_event));
    slot_freq_coarse_op1 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op1_event));
    slot_freq_fine_op1 = (get_gwidget<Gtk::SpinButton>("freq_fine_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op1_event));
    slot_dtun_op1 = (get_gwidget<Gtk::Scale>("dtun_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op1_event));
    /* OP1 EG */
    slot_eg_rt1_op1 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op1_event));
    slot_eg_rt2_op1 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op1_event));
    slot_eg_rt3_op1 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op1_event));
    slot_eg_rt4_op1 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op1_event));
    slot_eg_lvl1_op1 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op1_event));
    slot_eg_lvl2_op1 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op1_event));
    slot_eg_lvl3_op1 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op1_event));
    slot_eg_lvl4_op1 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op1_event));
    /* OP1 VOLUME */
    slot_krs_op1 = (get_gwidget<Gtk::Scale>("krs_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op1_event));
    slot_kvs_op1 = (get_gwidget<Gtk::Scale>("kvs_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op1_event));
    slot_lvl_op1 = (get_gwidget<Gtk::SpinButton>("lvl_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op1_event));
    slot_mute_op1 = (get_gwidget<Gtk::ToggleButton>("mute_op1"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    slot_mute_hexter_op1 = (get_gwidget<Gtk::ToggleButton>("mute_op1"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_hexter_op1_event));
    /* OP1 KLS */
    slot_kls_lft_curve_op1 = (get_gwidget<Gtk::ComboBox>("kls_lft_curve_op1"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op1_event));
    slot_kls_rght_curve_op1 = (get_gwidget<Gtk::ComboBox>("kls_rght_curve_op1"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op1_event));
    slot_kls_lft_depth_op1 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op1_event));
    slot_kls_rght_depth_op1 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op1_event));
    slot_kls_note_brk_pt_op1 = (get_gwidget<Gtk::ComboBox>("note_brk_pt_op1"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op1_event));
    slot_kls_octv_brk_pt_op1 = (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op1_event));


    /* OP2 */
    slot_ams_op2 = (get_gwidget<Gtk::Scale>("ams_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op2_event)); //frame lfo
    /* OP2 FREQUENCE */
    slot_freq_mode_op2 = (get_gwidget<Gtk::ComboBox>("freq_mode_op2"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op2_event));
    slot_freq_coarse_op2 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op2_event));
    slot_freq_fine_op2 = (get_gwidget<Gtk::SpinButton>("freq_fine_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op2_event));
    slot_dtun_op2 = (get_gwidget<Gtk::Scale>("dtun_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op2_event));
    /* OP2 EG */
    slot_eg_rt1_op2 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op2_event));
    slot_eg_rt2_op2 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op2_event));
    slot_eg_rt3_op2 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op2_event));
    slot_eg_rt4_op2 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op2_event));
    slot_eg_lvl1_op2 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op2_event));
    slot_eg_lvl2_op2 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op2_event));
    slot_eg_lvl3_op2 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op2_event));
    slot_eg_lvl4_op2 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op2_event));
    /* OP2 VOLUME */
    slot_krs_op2 = (get_gwidget<Gtk::Scale>("krs_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op2_event));
    slot_kvs_op2 = (get_gwidget<Gtk::Scale>("kvs_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op2_event));
    slot_lvl_op2 = (get_gwidget<Gtk::SpinButton>("lvl_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op2_event));
    slot_mute_op2 = (get_gwidget<Gtk::ToggleButton>("mute_op2"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    slot_mute_hexter_op2 = (get_gwidget<Gtk::ToggleButton>("mute_op2"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_hexter_op2_event));
    /* OP2 KLS */
    slot_kls_lft_curve_op2 = (get_gwidget<Gtk::ComboBox>("kls_lft_curve_op2"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op2_event));
    slot_kls_rght_curve_op2 = (get_gwidget<Gtk::ComboBox>("kls_rght_curve_op2"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op2_event));
    slot_kls_lft_depth_op2 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op2_event));
    slot_kls_rght_depth_op2 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op2_event));
    slot_kls_note_brk_pt_op2 = (get_gwidget<Gtk::ComboBox>("note_brk_pt_op2"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op2_event));
    slot_kls_octv_brk_pt_op2 = (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op2"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op2_event));


    /* OP3 */
    slot_ams_op3 = (get_gwidget<Gtk::Scale>("ams_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op3_event)); //frame lfo
    /* OP3 FREQUENCE */
    slot_freq_mode_op3 = (get_gwidget<Gtk::ComboBox>("freq_mode_op3"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op3_event));
    slot_freq_coarse_op3 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op3_event));
    slot_freq_fine_op3 = (get_gwidget<Gtk::SpinButton>("freq_fine_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op3_event));
    slot_dtun_op3 = (get_gwidget<Gtk::Scale>("dtun_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op3_event));
    /* OP3 EG */
    slot_eg_rt1_op3 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op3_event));
    slot_eg_rt2_op3 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op3_event));
    slot_eg_rt3_op3 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op3_event));
    slot_eg_rt4_op3 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op3_event));
    slot_eg_lvl1_op3 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op3_event));
    slot_eg_lvl2_op3 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op3_event));
    slot_eg_lvl3_op3 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op3_event));
    slot_eg_lvl4_op3 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op3_event));
    /* OP3 VOLUME */
    slot_krs_op3 = (get_gwidget<Gtk::Scale>("krs_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op3_event));
    slot_kvs_op3 = (get_gwidget<Gtk::Scale>("kvs_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op3_event));
    slot_lvl_op3 = (get_gwidget<Gtk::SpinButton>("lvl_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op3_event));
    slot_mute_op3 = (get_gwidget<Gtk::ToggleButton>("mute_op3"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    slot_mute_hexter_op3 = (get_gwidget<Gtk::ToggleButton>("mute_op3"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_hexter_op3_event));
    /* OP3 KLS */
    slot_kls_lft_curve_op3 = (get_gwidget<Gtk::ComboBox>("kls_lft_curve_op3"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op3_event));
    slot_kls_rght_curve_op3 = (get_gwidget<Gtk::ComboBox>("kls_rght_curve_op3"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op3_event));
    slot_kls_lft_depth_op3 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op3_event));
    slot_kls_rght_depth_op3 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op3_event));
    slot_kls_note_brk_pt_op3 = (get_gwidget<Gtk::ComboBox>("note_brk_pt_op3"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op3_event));
    slot_kls_octv_brk_pt_op3 = (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op3"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op3_event));


    /* OP4 */
    slot_ams_op4 = (get_gwidget<Gtk::Scale>("ams_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op4_event)); //frame lfo
    /* OP4 FREQUENCE */
    slot_freq_mode_op4 = (get_gwidget<Gtk::ComboBox>("freq_mode_op4"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op4_event));
    slot_freq_coarse_op4 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op4_event));
    slot_freq_fine_op4 = (get_gwidget<Gtk::SpinButton>("freq_fine_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op4_event));
    slot_dtun_op4 = (get_gwidget<Gtk::Scale>("dtun_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op4_event));
    /* OP4 EG */
    slot_eg_rt1_op4 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op4_event));
    slot_eg_rt2_op4 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op4_event));
    slot_eg_rt3_op4 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op4_event));
    slot_eg_rt4_op4 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op4_event));
    slot_eg_lvl1_op4 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op4_event));
    slot_eg_lvl2_op4 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op4_event));
    slot_eg_lvl3_op4 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op4_event));
    slot_eg_lvl4_op4 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op4_event));
    /* OP4 VOLUME */
    slot_krs_op4 = (get_gwidget<Gtk::Scale>("krs_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op4_event));
    slot_kvs_op4 = (get_gwidget<Gtk::Scale>("kvs_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op4_event));
    slot_lvl_op4 = (get_gwidget<Gtk::SpinButton>("lvl_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op4_event));
    slot_mute_op4 = (get_gwidget<Gtk::ToggleButton>("mute_op4"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    slot_mute_hexter_op4 = (get_gwidget<Gtk::ToggleButton>("mute_op4"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_hexter_op4_event));
    /* OP4 KLS */
    slot_kls_lft_curve_op4 = (get_gwidget<Gtk::ComboBox>("kls_lft_curve_op4"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op4_event));
    slot_kls_rght_curve_op4 = (get_gwidget<Gtk::ComboBox>("kls_rght_curve_op4"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op4_event));
    slot_kls_lft_depth_op4 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op4_event));
    slot_kls_rght_depth_op4 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op4_event));
    slot_kls_note_brk_pt_op4 = (get_gwidget<Gtk::ComboBox>("note_brk_pt_op4"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op4_event));
    slot_kls_octv_brk_pt_op4 = (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op4"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op4_event));

    /* OP5 */
    slot_ams_op5 = (get_gwidget<Gtk::Scale>("ams_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op5_event)); //frame lfo
    /* OP5 FREQUENCE */
    slot_freq_mode_op5 = (get_gwidget<Gtk::ComboBox>("freq_mode_op5"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op5_event));
    slot_freq_coarse_op5 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op5_event));
    slot_freq_fine_op5 = (get_gwidget<Gtk::SpinButton>("freq_fine_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op5_event));
    slot_dtun_op5 = (get_gwidget<Gtk::Scale>("dtun_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op5_event));
    /* OP5 EG */
    slot_eg_rt1_op5 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op5_event));
    slot_eg_rt2_op5 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op5_event));
    slot_eg_rt3_op5 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op5_event));
    slot_eg_rt4_op5 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op5_event));
    slot_eg_lvl1_op5 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op5_event));
    slot_eg_lvl2_op5 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op5_event));
    slot_eg_lvl3_op5 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op5_event));
    slot_eg_lvl4_op5 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op5_event));
    /* OP5 VOLUME */
    slot_krs_op5 = (get_gwidget<Gtk::Scale>("krs_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op5_event));
    slot_kvs_op5 = (get_gwidget<Gtk::Scale>("kvs_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op5_event));
    slot_lvl_op5 = (get_gwidget<Gtk::SpinButton>("lvl_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op5_event));
    slot_mute_op5 = (get_gwidget<Gtk::ToggleButton>("mute_op5"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    slot_mute_hexter_op5 = (get_gwidget<Gtk::ToggleButton>("mute_op5"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_hexter_op5_event));
    /* op5 KLS */
    slot_kls_lft_curve_op5 = (get_gwidget<Gtk::ComboBox>("kls_lft_curve_op5"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op5_event));
    slot_kls_rght_curve_op5 = (get_gwidget<Gtk::ComboBox>("kls_rght_curve_op5"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op5_event));
    slot_kls_lft_depth_op5 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op5_event));
    slot_kls_rght_depth_op5 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op5_event));
    slot_kls_note_brk_pt_op5 = (get_gwidget<Gtk::ComboBox>("note_brk_pt_op5"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op5_event));
    slot_kls_octv_brk_pt_op5 = (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op5"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op5_event));

    /* OP6 */
    slot_ams_op6 = (get_gwidget<Gtk::Scale>("ams_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_ams_op6_event)); //frame lfo
    /* OP6 FREQUENCE */
    slot_freq_mode_op6 = (get_gwidget<Gtk::ComboBox>("freq_mode_op6"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_mode_op6_event));
    slot_freq_coarse_op6 = (get_gwidget<Gtk::SpinButton>("freq_coarse_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_coarse_op6_event));
    slot_freq_fine_op6 = (get_gwidget<Gtk::SpinButton>("freq_fine_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_freq_fine_op6_event));
    slot_dtun_op6 = (get_gwidget<Gtk::Scale>("dtun_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_dtun_op6_event));
    /* OP6 EG */
    slot_eg_rt1_op6 = (get_gwidget<Gtk::SpinButton>("eg_rt1_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt1_op6_event));
    slot_eg_rt2_op6 = (get_gwidget<Gtk::SpinButton>("eg_rt2_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt2_op6_event));
    slot_eg_rt3_op6 = (get_gwidget<Gtk::SpinButton>("eg_rt3_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt3_op6_event));
    slot_eg_rt4_op6 = (get_gwidget<Gtk::SpinButton>("eg_rt4_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_rt4_op6_event));
    slot_eg_lvl1_op6 = (get_gwidget<Gtk::SpinButton>("eg_lvl1_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl1_op6_event));
    slot_eg_lvl2_op6 = (get_gwidget<Gtk::SpinButton>("eg_lvl2_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl2_op6_event));
    slot_eg_lvl3_op6 = (get_gwidget<Gtk::SpinButton>("eg_lvl3_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl3_op6_event));
    slot_eg_lvl4_op6 = (get_gwidget<Gtk::SpinButton>("eg_lvl4_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_eg_lvl4_op6_event));
    /* OP6 VOLUME */
    slot_krs_op6 = (get_gwidget<Gtk::Scale>("krs_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_krs_op6_event));
    slot_kvs_op6 = (get_gwidget<Gtk::Scale>("kvs_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kvs_op6_event));
    slot_lvl_op6 = (get_gwidget<Gtk::SpinButton>("lvl_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_lvl_op6_event));
    slot_mute_op6 = (get_gwidget<Gtk::ToggleButton>("mute_op6"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_op_event));
    slot_mute_hexter_op6 = (get_gwidget<Gtk::ToggleButton>("mute_op6"))->signal_toggled().connect(
        sigc::mem_fun(*this, &Dx7interface::on_mute_hexter_op6_event));
    /* OP6 KLS */
    slot_kls_lft_curve_op6 = (get_gwidget<Gtk::ComboBox>("kls_lft_curve_op6"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_curve_op6_event));
    slot_kls_rght_curve_op6 = (get_gwidget<Gtk::ComboBox>("kls_rght_curve_op6"))->signal_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_curve_op6_event));
    slot_kls_lft_depth_op6 = (get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_lft_dpth_op6_event));
    slot_kls_rght_depth_op6 = (get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_rght_dpth_op6_event));


    slot_kls_octv_brk_pt_op6 = (get_gwidget<Gtk::SpinButton>("octv_brk_pt_op6"))->signal_value_changed().connect(
        sigc::mem_fun(*this, &Dx7interface::on_kls_brk_pt_op6_event));
    //get_gwidget<gtk::algo>("algo_select")->click_on().connect(sigc::mrmçfunc(*this,&Dx7interface::on_change_algo_event))
    LOG_OUT();
};


int* Dx7interface::get_cr_visible_size(const Cairo::RefPtr<Cairo::Context>& cr, Glib::ustring name){
    //LOG_IN();
    // get current size of the cr
    int *size = nullptr;
    size = new int[2];
    Glib::RefPtr<Gdk::Window> window = (get_gwidget<Gtk::DrawingArea>("drawingarea_"+name))->get_window();
    Cairo::RefPtr<Cairo::Region> visible_region = window->get_visible_region();
    Cairo::RectangleInt rect = visible_region->get_extents();
    size[0] = rect.width;
    size[1] = rect.height;
    //LOG_OUT();
    return size;
}

void Dx7interface::draw_background(const Cairo::RefPtr<Cairo::Context>& cr){
    // TODO set value in a var to be changed by interface
    //LOG_IN();
    cr->save();
    cr->set_source_rgba(bg_color[0], bg_color[1], bg_color[2], bg_color[3]);
    cr->paint();	// fill with color
    cr->restore();
    //LOG_OUT();
};

void Dx7interface::draw_grid(const Cairo::RefPtr<Cairo::Context>& cr, int *vsize, double *d_length){
    //LOG_IN();
    double x=0.0, y=0.0;   // coordonee du point
    double width=(double)vsize[0];
    double height=(double)vsize[1];
    // representation d'un pas en fonction de la zone d'affichage
    double x_step=(width/grid_step_x);
    double y_step=(height/grid_step_y);
    cr->translate(0, height);
    cr->scale(1, -1);
    y=0.0;
    //draw verticals lines
    for(double x=0.0;x<=(width - dash_width); x+=x_step) {
        cr->save();
        cr->set_source_rgba(dash_color[0], dash_color[1], dash_color[2], dash_color[3]);
        cr->set_dash(dash_pattern,dash_offset);
        cr->set_line_width(dash_width);
        cr->move_to(x, 0.0);           // deplace le curseur
        cr->line_to(x, height);    // trace une ligne
        cr->stroke();
        cr->restore();
    };
    cr->save();
    cr->set_source_rgba(dash_color[0], dash_color[1], dash_color[2], dash_color[3]);
    cr->set_dash(dash_pattern,dash_offset);
    cr->set_line_width(dash_width);
    cr->move_to( (width - dash_width), 0.0);           // deplace le curseur
    cr->line_to( (width - dash_width), height);    // trace une ligne
    cr->stroke();
    cr->restore();

    //draw horizontals lines
    x=0.0;
    for(double y=0.0;y<=(height - dash_width); y+=y_step) {
        cr->save();
        cr->set_source_rgba(dash_color[0], dash_color[1], dash_color[2], dash_color[3]);
        cr->set_dash(dash_pattern,dash_offset);
        cr->set_line_width(dash_width);
        cr->move_to(0.0, y);           // deplace le curseur
        cr->line_to(width, y);    // trace une ligne
        cr->stroke();
        cr->restore();
    };
    cr->save();
    cr->set_source_rgba(dash_color[0], dash_color[1], dash_color[2], dash_color[3]);
    cr->set_dash(dash_pattern,dash_offset);
    cr->set_line_width(dash_width);
    cr->move_to(0, (height - dash_width) );           // deplace le curseur
    cr->line_to(width, (height - dash_width));    // trace une ligne
    cr->stroke();
    cr->restore();
    /* DEBUG prints
    std::cout << " Total step : x " << x_wanted_step \
            << " de " <<  x_step << std::endl;
    std::cout << " Total step : y " << y_wanted_step \
            << " de " << y_step << std::endl;

    std::cout << "x: "<< x << " d_x: " <<  d_length[0] << std::endl;
    std::cout << "y: "<< y << " d_y: " <<  d_length[1] << std::endl;
    */
    //LOG_OUT();
};

void Dx7interface::draw_point(const Cairo::RefPtr<Cairo::Context>& cr, double x, double y){
    // TODO set value in a var to be changed by interface
    //LOG_IN();
    double radius;
    cr->save();
    // interior circle
    cr->set_source_rgba(line_color[0],line_color[1],line_color[2],1.0);
    radius = r_point/2.0;       // Radius of the point
    cr->arc(x, y, radius, 0.0, 2.0 * M_PI);
    cr->fill();
    cr->stroke();
    cr->restore();
    cr->save();
    // interior circle
    cr->set_source_rgba(line_color[0],line_color[1],line_color[2],0.8);
    radius = r_point/1.33;       // Radius of the point
    cr->arc(x, y, radius, 0.0, 2.0 * M_PI);
    cr->fill();
    cr->stroke();
    cr->restore();
    // interior circle
    cr->save();
    cr->set_source_rgba(line_color[0],line_color[1],line_color[2],0.8);
    radius = r_point;       // Radius of the point
    cr->arc(x, y, radius, 0.0, 2.0 * M_PI);
    //cr->fill();
    cr->stroke();
    cr->restore();
    // exterior circle
    cr->save();
    cr->set_source_rgba(line_color[0],line_color[1],line_color[2],0.1);
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
double* Dx7interface::old_draw_adsr(const Cairo::RefPtr<Cairo::Context>& cr, int *vsize, Glib::ustring type){
    // TODO : get sound from bank_1_modif.sound
    //LOG_IN();

    // draw length
    double *d_length = new double[2]{0.0, 0.0};
    // visible size
    double width=(double)vsize[0];
    double height=(double)vsize[1];
    //set origin to bottom left
    cr->translate(0, height);
    cr->scale(1, -1);
    double x_noteoff=(width)*3/4;
    draw_note_off(cr, x_noteoff, height);

    // Draw ADSR
    // scale factor, to compute x
    height-=2*r_point;
    width-=2*r_point;
    double x_ratio=( (width ) /400);
    double y_ratio=( (height) /100);
    // TODO: draw key off line l3 at r4 rate to l4
    // global output lvl scale the distance
    // put key on key off mark
    //height = height - 6.0;
    /* Draw curve */
    double x=0.0, y=0.0;     // coordonee du point
    for(uint8_t i=1;i<=4;i++) {
        cr->save();
        cr->set_source_rgba(line_color[0],line_color[1],line_color[2],1.0);
        cr->set_line_width(line_width);
        if (i==1) {
            x += r_point;
            y += (r_point/2.0);
            cr->move_to( x,
                       ( y + ( (double)(get_gwidget<Gtk::SpinButton>("eg_lvl4_"+type))->get_value()+1.0) * y_ratio )
            );
        }else{
            cr->move_to(x, y);    // deplace le curseur
        };
        if(i == 4){
            x = x_noteoff;
            cr->line_to(x, y);      // trace une ligne
            cr->move_to(x, y);
        }
        x += ( ( x_ratio * (
            std::abs( 100.0 - (double)( (get_gwidget<Gtk::SpinButton>("eg_rt"+tostr<uint>(i)+"_"+type))->get_value() + 1.0 ) )
        ) ) ) ;
        y = (r_point/2.0) + ( ( (double)(get_gwidget<Gtk::SpinButton>("eg_lvl"+tostr<uint>(i)+"_"+type))->get_value() + 1.0 ) * y_ratio ) /*+ line_width*/;
        cr->line_to(x, y);      // trace une ligne
        cr->move_to(x, y);

        cr->stroke();
        cr->restore();
        draw_point(cr,x,y);
        if(x > d_length[0]){
            d_length[0]=x;
        };
        if(y > d_length[1]){
            d_length[1]=y;
        };
    };
    return d_length;
    //LOG_OUT();
};

double* Dx7interface::draw_adsr(const Cairo::RefPtr<Cairo::Context>& cr, int *vsize, Glib::ustring type){
    // TODO : get sound from bank_1_modif.sound
    //LOG_IN();
    // TODO: draw key off line l3 at r4 rate to l4
    // global output lvl scale the distance
    // put key on key off mark
    double x1=(double) ((get_gwidget<Gtk::SpinButton>("eg_rt1_"+type))->get_value() + 1.0);
    double y1=(double) ((get_gwidget<Gtk::SpinButton>("eg_lvl1_"+type))->get_value() + 1.0);
    double x2=(double) ((get_gwidget<Gtk::SpinButton>("eg_rt2_"+type))->get_value()  + 1.0);
    double y2=(double) ((get_gwidget<Gtk::SpinButton>("eg_lvl2_"+type))->get_value() + 1.0);
    double x3=(double) ((get_gwidget<Gtk::SpinButton>("eg_rt3_"+type))->get_value() + 1.0);
    double y3=(double) ((get_gwidget<Gtk::SpinButton>("eg_lvl3_"+type))->get_value() + 1.0);
    double x4=(double) ((get_gwidget<Gtk::SpinButton>("eg_rt4_"+type))->get_value() + 1.0);
    double y4=(double) ((get_gwidget<Gtk::SpinButton>("eg_lvl4_"+type))->get_value() + 1.0);

    // draw length
    double *d_length = new double[2]{0.0, 0.0};
    // visible size
    double width=(double)vsize[0];
    double height=(double)vsize[1];
    cr->translate(0, height);
    cr->scale(1, -1);
    double x_noteoff=(width)*3/4;
    draw_note_off(cr, x_noteoff, height);

    // Draw ADSR
    // scale factor, to compute x
    height-=line_width * 3;
    width-=line_width * 2;
    double x_ratio=( (width ) /400);
    double y_ratio=( (height) /100);
    // TODO: draw key off line l3 at r4 rate to l4
    // global output lvl scale the distance
    // put key on key off mark
    //height = height - 6.0;
    /* Draw curve */
    double x=2* line_width, y= 3 * line_width;     // coordonee du point
    for(uint8_t i=1;i<=4;i++) {
        cr->save();
        cr->set_source_rgba(line_color[0],line_color[1],line_color[2],1.0);
        cr->set_line_width(line_width);
        if (i==1) {
            cr->move_to(0.0 + ( line_width ),
               (( (double)(get_gwidget<Gtk::SpinButton>("eg_lvl4_"+type))->get_value()+1.0) * y_ratio ) + line_width
            );
        }else{
            cr->move_to(x, y);    // deplace le curseur
        };
        if(i == 4){
            x = x_noteoff;
            cr->line_to(x, y);      // trace une ligne
            cr->move_to(x, y);
        }
        x += ( ( (std::abs(100.0 - (double)
                ((get_gwidget<Gtk::SpinButton>("eg_rt"+tostr<uint>(i)+"_"+type))->get_value() + 1.0)
                ) ) * x_ratio ) /* + line_width*/
            ) ;
        y = ( ( (double)(get_gwidget<Gtk::SpinButton>("eg_lvl"+tostr<uint>(i)+"_"+type))->get_value() + 1.0 ) * y_ratio ) /*+ line_width*/;
        cr->line_to(x, y);      // trace une ligne
        cr->move_to(x, y);

        cr->stroke();
        cr->restore();
        draw_point(cr,x,y);
        if(x > d_length[0]){
            d_length[0]=x;
        };
        if(y > d_length[1]){
            d_length[1]=y;
        };
    };
    return d_length;
    //LOG_OUT();
};

/* EVENTS */

bool Dx7interface::on_draw_op_event(const Cairo::RefPtr<Cairo::Context>& cr, Glib::ustring num_op){
    //LOG_IN();
    if (cr){
        auto vsize = get_cr_visible_size(cr, "eg_op"+num_op);
        if( !vsize || ( vsize[0] == 0 || vsize[1] == 0 ) ){
            return true;
        }
        //std::cout << "Area size: " << vsize[0] << "x" << vsize[1] << std::endl;
        draw_background(cr);
        //auto d_length = draw_adsr( cr, vsize, "op"+num_op );
        auto d_length = old_draw_adsr( cr, vsize, "op"+num_op );
        draw_grid( cr, vsize, d_length );
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_op"+num_op))->queue_draw();
    };
    //LOG_OUT();
    return true;
};

bool Dx7interface::on_draw_pitch_event(const Cairo::RefPtr<Cairo::Context>& cr){
    //LOG_IN();
    if (cr){
        auto vsize = get_cr_visible_size(cr, "eg_pitch" );
        if( !vsize ||( vsize[0] == 0 && vsize[1] == 0) ){
            return true;
        }
        //std::cout << "Area size: " << vsize[0] << "x" << vsize[1] << std::endl;
        draw_background(cr);
        auto d_length = old_draw_adsr( cr, vsize, "pitch" );
        draw_grid( cr, vsize, d_length );
        (get_gwidget<Gtk::DrawingArea>("drawingarea_eg_pitch"))->queue_draw();
    };
    //LOG_OUT();
    return true;
};

void Dx7interface::on_treeview_row_clicked(){
    LOG_IN();
    Gtk::TreeModel::Path path;
    Gtk::TreeViewColumn* col;
    (get_gwidget<Gtk::TreeView>("treeview_bank"))->get_cursor(path,col);
    Gtk::TreeModel::iterator iter = m_refListStore->get_iter(path);
    if(iter){
        Gtk::TreeModel::Row row = *iter;
        uint num, nb_elmnt=0;
        row->get_value(0,num);
        Gtk::TreeModel::Children::iterator iter_child = m_refListStore->children().begin();
        for( ; iter_child != m_refListStore->children().end(); ++iter_child,nb_elmnt++);
        /* block signals */
        block_all();
        switch ( nb_elmnt ){
            case 1:
                set_voice(&bank_1_modif.sound[num-1]);
                send_voice(&bank_1_modif.sound[num-1]);
                break;
            case 32:
                set_voice(&bank_32_modif.sound[num-1]);
                send_voice(&bank_32_modif.sound[num-1]);
                break;
            case 128:
                set_voice(&bank_128_modif.sound[num-1]);
                send_voice(&bank_128_modif.sound[num-1]);
                break;
        };
        on_txt_freq_op_event();
        unblock_all();
    };
    LOG_OUT();
};

void Dx7interface::on_bank_select(){
    /* if (modif done)
        ask save
    */
    clean_bank();
    load_bank((get_gwidget<Gtk::FileChooserButton>("bank_select"))->get_file());
    (get_gwidget<Gtk::TreeView>("treeview_bank"))->set_cursor(
            m_refListStore->get_path(m_refListStore->get_iter("0:0")) );
};


/* ALGO */
void Dx7interface::on_algo_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x06;
    msg[5]=(get_gwidget<Gtk::SpinButton>("algo_number"))->get_value()-1;
    msg[6]=0xF7;
    get_gwidget<Gtk::Image>("image_algo")->set(MOD_IMG_DIRECTORY"/algo"+tostr<uint>(msg[5]+1)+".png");
    send_midi(SND_SEQ_EVENT_SYSEX, 7, msg);
    LOG_OUT();
};

void Dx7interface::on_feedback_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x07;
    msg[5]=(get_gwidget<Gtk::SpinButton>("feedback"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
};

void Dx7interface::on_transpose_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x10;
    msg[5]=(get_gwidget<Gtk::ComboBox>("note_transpose"))->get_active_row_number()
        +((get_gwidget<Gtk::SpinButton>("octv_transpose"))->get_value()-1)*12;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
};

void Dx7interface::on_oks_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x08;
    if ( (get_gwidget<Gtk::CheckButton>("oks"))->get_active() ) {
        msg[5]=0x01;
    }else{
        msg[5]=0x00;
    }
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
};

/* LFO */
void Dx7interface::on_lfo_wav_event() {
    /* lfo wave form image */
    Glib::ustring name;
    ((get_gwidget<Gtk::ComboBox>("lfo_wav"))->get_active())->get_value(0,name) ;
    (get_gwidget<Gtk::Image>("image_lfo"))->set(MOD_IMG_DIRECTORY"/"+name+".png");
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x0E;
    msg[5]=(get_gwidget<Gtk::ComboBox>("lfo_wav"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
};

void Dx7interface::on_lfo_sync_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x0D;
    if ( (get_gwidget<Gtk::CheckButton>("lfo_sync"))->get_active() ) {
        msg[5]=0x01;
    }else{
        msg[5]=0x00;
    }
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
};

void Dx7interface::on_speed_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x09;
    msg[5]=(get_gwidget<Gtk::SpinButton>("speed"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
};

void Dx7interface::on_delay_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x0A;
    msg[5]=(get_gwidget<Gtk::SpinButton>("delay"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
};

void Dx7interface::on_pmd_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x0B;
    msg[5]=(get_gwidget<Gtk::SpinButton>("pmd"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_amd_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x0C;
    msg[5]=(get_gwidget<Gtk::SpinButton>("amd"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/*LFO MODULATION */
void Dx7interface::on_pms_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x0F;
    msg[5]=(get_gwidget<Gtk::Scale>("pms"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/* PITCH EG */
void Dx7interface::on_pitch_rt1_event() {	LOG_IN();

    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x7E;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_pitch"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->pitch.eg_rt[0].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_pitch_rt2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x7F;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_pitch"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->pitch.eg_rt[1].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_pitch_rt3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x00;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_pitch"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->pitch.eg_rt[2].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_pitch_rt4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x01;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_pitch"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->pitch.eg_rt[3].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_pitch_lvl1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x02;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_pitch"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->pitch.eg_lvl[0].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_pitch_lvl2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x03;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_pitch"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->pitch.eg_lvl[1].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_pitch_lvl3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x04;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_pitch"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->pitch.eg_lvl[2].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_pitch_lvl4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x05;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_pitch"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->pitch.eg_lvl[3].val=msg[5];
    LOG_OUT();
};

/* MUTE FOR EACH OPERATOR IN DX7 */
void Dx7interface::on_mute_op_event() {
    u_char msg[7];
    uint8_t i,mute_val=0x00;
    for(i=1; i<=6;i++){
        mute_val=mute_val | !((get_gwidget<Gtk::ToggleButton>("mute_op"+tostr<uint>(i)))->get_active());
        if (i!=6){
            mute_val=mute_val << 1;
        };
    };
    msg[5]=mute_val;
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x01;
    msg[4]=0x1B;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
};

/* view frequence for each operator  */
void Dx7interface::on_txt_freq_op_event()   {
    gdouble freq_val,ff,fc;
    uint i ;
    for ( i=1;i<=6;i++){
        fc=(get_gwidget<Gtk::SpinButton>("freq_coarse_op"+tostr<uint>(i)))->get_value();
        ff=(get_gwidget<Gtk::SpinButton>("freq_fine_op"+tostr<uint>(i)))->get_value();
        if ( (get_gwidget<Gtk::ComboBox>("freq_mode_op"+tostr<uint>(i)))->get_active_row_number() ) {
            (get_gwidget<Gtk::Label>("label_view_freq_op"+tostr<uint>(i)))->set_label("Hz");
            // calcul termitor
            gdouble  A = exp(log(9.772)/99);
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
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x77;
    msg[5]=(get_gwidget<Gtk::Scale>("ams_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/* OP1 FREQ */
void Dx7interface::on_freq_mode_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x7A;
    msg[5]=(get_gwidget<Gtk::ComboBox>("freq_mode_op1"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_coarse_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x7B;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_fine_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x7C;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_dtun_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x7D;
    msg[5]=(get_gwidget<Gtk::Scale>("dtun_op1"))->get_value()+7;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/* OP1 EG */
void Dx7interface::on_eg_rt1_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x69;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->op[0].eg_rt[0].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_eg_rt2_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x6A;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->op[1].eg_rt[0].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_eg_rt3_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x6B;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->op[2].eg_rt[0].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_eg_rt4_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x6C;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->op[3].eg_rt[0].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_eg_lvl1_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x6D;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->op[0].eg_lvl[1].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_eg_lvl2_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x6E;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->op[0].eg_lvl[1].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_eg_lvl3_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x6F;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->op[0].eg_lvl[2].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_eg_lvl4_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x70;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->op[0].eg_lvl[3].val=msg[5];
    LOG_OUT();
};

/* OP1 FRAME VOLUME */
void Dx7interface::on_krs_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x76;
    msg[5]=(get_gwidget<Gtk::Scale>("krs_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kvs_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x78;
    msg[5]=(get_gwidget<Gtk::Scale>("kvs_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_lvl_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x79;
    msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    /* logout fonction mute */
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op1"))->get_active() ) {
        (get_gwidget<Gtk::ToggleButton>("mute_op1"))->set_active(false);
    };
    LOG_OUT();
};

/* OP1 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op1_event(){
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op1"))->get_active() ) {
        (get_gwidget<Gtk::Label>("label_general_op1"))->set_label(_("(OP1)"));
    u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel;
        msg[3]=0x00;
        msg[4]=0x79;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op1"))->set_label(_(" OP1 "));
        on_lvl_op1_event();
    };
};

/* OP1 KLS */
void Dx7interface::on_kls_lft_curve_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x74;
    msg[5]=(get_gwidget<Gtk::ComboBox>("kls_lft_curve_op1"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_rght_curve_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x75;
    msg[5]=(get_gwidget<Gtk::ComboBox>("kls_rght_curve_op1"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_lft_dpth_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x72;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_rght_dpth_op1_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x73;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op1"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_brk_pt_op1_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x71;
    msg[5]=(get_gwidget<Gtk::ComboBox>("note_brk_pt_op1"))->get_active_row_number()
        +((get_gwidget<Gtk::SpinButton>("octv_brk_pt_op1"))->get_value())*12;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
};


/* OP2 */
void Dx7interface::on_ams_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x62;
    msg[5]=(get_gwidget<Gtk::Scale>("ams_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/* OP2 FREQ */
void Dx7interface::on_freq_mode_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x65;
    msg[5]=(get_gwidget<Gtk::ComboBox>("freq_mode_op2"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);

    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_coarse_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x66;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_fine_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x67;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_dtun_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x68;
    msg[5]=(get_gwidget<Gtk::Scale>("dtun_op2"))->get_value()+7;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/* OP2 EG */
void Dx7interface::on_eg_rt1_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x54;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->op[1].eg_rt[0].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_eg_rt2_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x55;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->op[1].eg_rt[1].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_eg_rt3_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x56;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->op[1].eg_rt[2].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_eg_rt4_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x57;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->op[1].eg_rt[3].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_eg_lvl1_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x58;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->op[1].eg_lvl[0].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_eg_lvl2_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x59;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->op[1].eg_lvl[1].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_eg_lvl3_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x5A;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->op[1].eg_lvl[2].val=msg[5];
    LOG_OUT();
};

void Dx7interface::on_eg_lvl4_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x5B;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    bank_1_modif.sound->op[1].eg_lvl[3].val=msg[5];
    LOG_OUT();
};

/* OP2 FRAME VOLUME */
void Dx7interface::on_krs_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x61;
    msg[5]=(get_gwidget<Gtk::Scale>("krs_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kvs_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x63;
    msg[5]=(get_gwidget<Gtk::Scale>("kvs_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_lvl_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x64;
    msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    // logout fonction mute
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op2"))->get_active() ) {
        (get_gwidget<Gtk::ToggleButton>("mute_op2"))->set_active(false);
    };
    LOG_OUT();
};

/* OP2 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op2_event() {
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op2"))->get_active() ) {
        (get_gwidget<Gtk::Label>("label_general_op2"))->set_label(_("(OP2)"));
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel;
        msg[3]=0x00;
        msg[4]=0x64;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op2"))->set_label(_(" OP2 "));
        on_lvl_op2_event();
    };

};

/* OP2 KLS */
void Dx7interface::on_kls_lft_curve_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x5F;
    msg[5]=(get_gwidget<Gtk::ComboBox>("kls_lft_curve_op2"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_rght_curve_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x60;
    msg[5]=(get_gwidget<Gtk::ComboBox>("kls_rght_curve_op2"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_lft_dpth_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x5D;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_rght_dpth_op2_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x5E;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op2"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_brk_pt_op2_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x5C;
    msg[5]=(get_gwidget<Gtk::ComboBox>("note_brk_pt_op2"))->get_active_row_number()
        +((get_gwidget<Gtk::SpinButton>("octv_brk_pt_op2"))->get_value())*12;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
};


/* OP3 */
void Dx7interface::on_ams_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x4D;
    msg[5]=(get_gwidget<Gtk::Scale>("ams_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/* OP3 FREQ */
void Dx7interface::on_freq_mode_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x50;
    msg[5]=(get_gwidget<Gtk::ComboBox>("freq_mode_op3"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_coarse_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x51;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_fine_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x52;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_dtun_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x53;
    msg[5]=(get_gwidget<Gtk::Scale>("dtun_op3"))->get_value()+7;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/* OP3 EG */
void Dx7interface::on_eg_rt1_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x3F;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_rt2_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x40;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_rt3_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x41;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_rt4_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x42;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_lvl1_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x43;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_lvl2_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x44;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_lvl3_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x45;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_lvl4_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x46;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/* OP3 FRAME VOLUME */
void Dx7interface::on_krs_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x4C;
    msg[5]=(get_gwidget<Gtk::Scale>("krs_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kvs_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x4E;
    msg[5]=(get_gwidget<Gtk::Scale>("kvs_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_lvl_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x4F;
    msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    // logout fonction mute
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op3"))->get_active() ) {
        (get_gwidget<Gtk::ToggleButton>("mute_op3"))->set_active(false);
    };
    LOG_OUT();
};
/* OP3 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op3_event() {
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op3"))->get_active() ) {
        (get_gwidget<Gtk::Label>("label_general_op3"))->set_label(_("(OP3)"));
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel;
        msg[3]=0x00;
        msg[4]=0x4F;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op3"))->set_label(_(" OP3 "));
        on_lvl_op3_event();
    };

};


/* OP3 KLS */
void Dx7interface::on_kls_lft_curve_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x4A;
    msg[5]=(get_gwidget<Gtk::ComboBox>("kls_lft_curve_op3"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_rght_curve_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x4B;
    msg[5]=(get_gwidget<Gtk::ComboBox>("kls_rght_curve_op3"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_lft_dpth_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x48;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_rght_dpth_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x49;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op3"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_brk_pt_op3_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x47;
    msg[5]=(get_gwidget<Gtk::ComboBox>("note_brk_pt_op3"))->get_active_row_number()
        +((get_gwidget<Gtk::SpinButton>("octv_brk_pt_op3"))->get_value())*12;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
};


/* OP4 */
void Dx7interface::on_ams_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x38;
    msg[5]=(get_gwidget<Gtk::Scale>("ams_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/* OP4 FREQ*/
void Dx7interface::on_freq_mode_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x3B;
    msg[5]=(get_gwidget<Gtk::ComboBox>("freq_mode_op4"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_coarse_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x3C;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_fine_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x3D;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_dtun_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x3E;
    msg[5]=(get_gwidget<Gtk::Scale>("dtun_op4"))->get_value()+7;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/* OP4 EG*/
void Dx7interface::on_eg_rt1_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x2A;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_rt2_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x2B;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_rt3_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x2C;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_rt4_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x2D;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_lvl1_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x2E;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_lvl2_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x2F;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_lvl3_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x30;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_lvl4_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x31;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};


/* OP4 FRAME VOLUME */
void Dx7interface::on_krs_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x37;
    msg[5]=(get_gwidget<Gtk::Scale>("krs_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kvs_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x39;
    msg[5]=(get_gwidget<Gtk::Scale>("kvs_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_lvl_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x3A;
    msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    // logout fonction mute
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op4"))->get_active() ) {
        (get_gwidget<Gtk::ToggleButton>("mute_op4"))->set_active(false);
    };
    LOG_OUT();
};

/* OP4 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op4_event() {
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op4"))->get_active() ) {
        (get_gwidget<Gtk::Label>("label_general_op4"))->set_label(_("(OP4)"));
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel;
        msg[3]=0x00;
        msg[4]=0x3A;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op4"))->set_label(_(" OP4 "));
        on_lvl_op4_event();
    };
};

/* OP4 KLS*/
void Dx7interface::on_kls_lft_curve_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x35;
    msg[5]=(get_gwidget<Gtk::ComboBox>("kls_lft_curve_op4"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_rght_curve_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x36;
    msg[5]=(get_gwidget<Gtk::ComboBox>("kls_rght_curve_op4"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_lft_dpth_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x33;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_rght_dpth_op4_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x34;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op4"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_brk_pt_op4_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x32;
    msg[5]=(get_gwidget<Gtk::ComboBox>("note_brk_pt_op4"))->get_active_row_number()
        +((get_gwidget<Gtk::SpinButton>("octv_brk_pt_op4"))->get_value())*12;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
};

/* OP5 */
void Dx7interface::on_ams_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x23;
    msg[5]=(get_gwidget<Gtk::Scale>("ams_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/* OP5 FREQ */
void Dx7interface::on_freq_mode_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x26;
    msg[5]=(get_gwidget<Gtk::ComboBox>("freq_mode_op5"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_coarse_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x27;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_fine_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x28;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_dtun_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x29;
    msg[5]=(get_gwidget<Gtk::Scale>("dtun_op5"))->get_value()+7;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/* OP5 EG */
void Dx7interface::on_eg_rt1_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x15;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_rt2_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x16;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_rt3_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x17;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_rt4_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x18;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_lvl1_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x19;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_lvl2_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x1A;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_lvl3_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x1B;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_lvl4_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x1C;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/* OP5 FRAME VOLUME */
void Dx7interface::on_krs_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x22;
    msg[5]=(get_gwidget<Gtk::Scale>("krs_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kvs_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x24;
    msg[5]=(get_gwidget<Gtk::Scale>("kvs_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_lvl_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x25;
    msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    // logout fonction mute
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op5"))->get_active() ) {
        (get_gwidget<Gtk::ToggleButton>("mute_op5"))->set_active(false);
    };
    LOG_OUT();
};

/* OP5 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op5_event() {
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op5"))->get_active() ) {
        (get_gwidget<Gtk::Label>("label_general_op5"))->set_label(_("(OP5)"));
        u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel;
        msg[3]=0x00;
        msg[4]=0x25;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op5"))->set_label(_(" OP5 "));
        on_lvl_op5_event();
    };
};

/* OP5 KLS */
void Dx7interface::on_kls_lft_curve_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x20;
    msg[5]=(get_gwidget<Gtk::ComboBox>("kls_lft_curve_op5"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_rght_curve_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x21;
    msg[5]=(get_gwidget<Gtk::ComboBox>("kls_rght_curve_op5"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_lft_dpth_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x1E;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_rght_dpth_op5_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x1F;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op5"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_brk_pt_op5_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x1D;
    msg[5]=(get_gwidget<Gtk::ComboBox>("note_brk_pt_op5"))->get_active_row_number()
        +((get_gwidget<Gtk::SpinButton>("octv_brk_pt_op5"))->get_value())*12;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
};

/* OP6 */
void Dx7interface::on_ams_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x0E;
    msg[5]=(get_gwidget<Gtk::Scale>("ams_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/* OP6 FREQ */
void Dx7interface::on_freq_mode_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x11;
    msg[5]=(get_gwidget<Gtk::ComboBox>("freq_mode_op6"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_coarse_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x12;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_coarse_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_freq_fine_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x13;
    msg[5]=(get_gwidget<Gtk::SpinButton>("freq_fine_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    on_txt_freq_op_event();
    LOG_OUT();
};

void Dx7interface::on_dtun_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x14;
    msg[5]=(get_gwidget<Gtk::Scale>("dtun_op6"))->get_value()+7;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/* OP6 EG */
void Dx7interface::on_eg_rt1_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x00;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt1_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_rt2_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x01;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt2_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_rt3_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x02;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt3_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_rt4_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x03;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_rt4_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_lvl1_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x04;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl1_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_lvl2_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x05;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl2_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_lvl3_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x06;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl3_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_eg_lvl4_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x07;
    msg[5]=(get_gwidget<Gtk::SpinButton>("eg_lvl4_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

/* OP6 FRAME VOLUME */
void Dx7interface::on_krs_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x0D;
    msg[5]=(get_gwidget<Gtk::Scale>("krs_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kvs_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x0F;
    msg[5]=(get_gwidget<Gtk::Scale>("kvs_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_lvl_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x10;
    msg[5]=(get_gwidget<Gtk::SpinButton>("lvl_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    // logout fonction mute
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op6"))->get_active() ) {
        (get_gwidget<Gtk::ToggleButton>("mute_op6"))->set_active(false);
    };
    LOG_OUT();
};

/* OP6 mute for UI & Hexter */
void Dx7interface::on_mute_hexter_op6_event() {
    if ( (get_gwidget<Gtk::ToggleButton>("mute_op6"))->get_active() ) {
        (get_gwidget<Gtk::Label>("label_general_op6"))->set_label(_("/* OP6 */"));
    u_char msg[7];
        msg[0]=0xF0;
        msg[1]=id_fabricant;
        msg[2]=sub_status & channel;
        msg[3]=0x00;
        msg[4]=0x10;
        msg[5]=0x00;
        msg[6]=0xF7;
        send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    }else{
        (get_gwidget<Gtk::Label>("label_general_op6"))->set_label(_(" OP6 "));
        on_lvl_op6_event();
    };

};

/* OP6 KLS */
void Dx7interface::on_kls_lft_curve_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x0B;
    msg[5]=(get_gwidget<Gtk::ComboBox>("kls_lft_curve_op6"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_rght_curve_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x0C;
    msg[5]=(get_gwidget<Gtk::ComboBox>("kls_rght_curve_op6"))->get_active_row_number();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_lft_dpth_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x09;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_lft_dpth_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_rght_dpth_op6_event() {	LOG_IN();
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x0A;
    msg[5]=(get_gwidget<Gtk::SpinButton>("kls_rght_dpth_op6"))->get_value();
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
    LOG_OUT();
};

void Dx7interface::on_kls_brk_pt_op6_event() {
    u_char msg[7];
    msg[0]=0xF0;
    msg[1]=id_fabricant;
    msg[2]=sub_status & channel;
    msg[3]=0x00;
    msg[4]=0x08;
    msg[5]=(get_gwidget<Gtk::ComboBox>("note_brk_pt_op6"))->get_active_row_number()
        +((get_gwidget<Gtk::SpinButton>("octv_brk_pt_op6"))->get_value())*12;
    msg[6]=0xF7;
    send_midi(SND_SEQ_EVENT_SYSEX ,7,msg);
};

void Dx7interface::block_all(){
    /*** Block UI ***/
    /* Draw */
    slot_op1_draw.block(true);
    slot_op2_draw.block(true);
    slot_op3_draw.block(true);
    slot_op4_draw.block(true);
    slot_op5_draw.block(true);
    slot_op6_draw.block(true);
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
    slot_mute_hexter_op1.block(true);
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
    slot_mute_hexter_op2.block(true);
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
    slot_mute_hexter_op3.block(true);
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
    slot_mute_hexter_op4.block(true);
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
    slot_mute_hexter_op5.block(true);
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
    slot_mute_hexter_op6.block(true);
    /* op6 KLS */
    slot_kls_lft_curve_op6.block(true);
    slot_kls_rght_curve_op6.block(true);
    slot_kls_lft_depth_op6.block(true);
    slot_kls_rght_depth_op6.block(true);
    slot_kls_note_brk_pt_op6.block(true);
    slot_kls_octv_brk_pt_op6.block(true);

};

void Dx7interface::unblock_all(){
    /*** Unblock UI ***/
    /* Draw */
    slot_pitch_draw_eg.unblock();
    slot_op1_draw.unblock();
    slot_op2_draw.unblock();
    slot_op3_draw.unblock();
    slot_op4_draw.unblock();
    slot_op5_draw.unblock();
    slot_op6_draw.unblock();
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
    slot_mute_hexter_op1.unblock();
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
    slot_mute_hexter_op2.unblock();
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
    slot_mute_hexter_op3.unblock();

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
    slot_mute_hexter_op4.unblock();
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
    slot_mute_hexter_op5.unblock();
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
    slot_mute_hexter_op6.unblock();
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
    slot_treeview_row_clicked.disconnect();

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

    /* Drawing area for pitch */
    slot_pitch_draw_eg.disconnect();

    /* Drawing area for each operator */
    slot_op1_draw.disconnect();
    slot_op2_draw.disconnect();
    slot_op3_draw.disconnect();
    slot_op4_draw.disconnect();
    slot_op5_draw.disconnect();
    slot_op6_draw.disconnect();
    //};

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
    slot_mute_hexter_op1.disconnect();
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
    slot_mute_hexter_op2.disconnect();
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
    slot_mute_hexter_op3.disconnect();
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
    slot_mute_hexter_op4.disconnect();
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
    slot_mute_hexter_op5.disconnect();
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
    slot_mute_hexter_op6.disconnect();
    /* OP6 KLS */
    slot_kls_lft_curve_op6.disconnect();
    slot_kls_rght_curve_op6.disconnect();
    slot_kls_lft_depth_op6.disconnect();
    slot_kls_rght_depth_op6.disconnect();
    slot_kls_octv_brk_pt_op6.disconnect();
    LOG_OUT();
};
#endif /* Dx7interface_CC */
