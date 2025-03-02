/* ----------------------------------------------------------------------------
 * Tx81z.c -- DX7 Graphic interface
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
#include "tx81z.h"

extern "C" {
    std::tuple<std::shared_ptr<void>, Gtk::Box*, Glib::ustring> LoadPlug(uint8_t index){
        auto editor = std::make_shared<Tx81z>(UI,index);
        Gtk::Box* mbox =  editor->get_rootbox();
        return std::make_tuple(editor, mbox, CSSFILE); //CSSFILE
    };
}

Tx81z::Tx81z(Glib::ustring ui, uint8_t index) : Gx_module(ui,MODULE_NAME), Synth(MODULE_NAME) {
    /*basic constructor */
    LOG_IN();
    /* I/O init */
    Gio::init();
    if (index <= 0){
        set_app_name(MODULE_NAME);
    }else{
        set_app_name(MODULE_NAME+index);
    };
    //get_gwidget()->set_title("Tx81z");
    /* MIDI */
    Synth::id_fabricant=id_fabricant;
    Synth::channel_send=0x00;
    Synth::channel_receive=0x00;
    Synth::sub_status=0x10;
    seq_handle=get_seq_handler();
    ev=get_seq_event_handler();
    /* UI */
    attach_signals();
    S_Thread();
    LOG_OUT();
};

Tx81z::~Tx81z(){
    LOG_IN();
    /* basic destructor*/
    /* terminate thread */
    //block_all();
    T_Thread();
    dettach_signals();
    LOG_OUT();
};

bool Tx81z::Run(){
    listen_midi();
    return true;
}
bool Tx81z::Run2(){
    return true;
}
void Tx81z::listen_midi(){
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
}

void Tx81z::load_bank(Glib::RefPtr<Gio::File> file) {
    LOG_IN();
    LOG_OUT();
};
void Tx81z::save_bank(Glib::RefPtr<Gio::File> file){
    LOG_IN();
    LOG_OUT();
};
void Tx81z::save_bank_as(Glib::RefPtr<Gio::File> file){
    LOG_IN();
    LOG_OUT();
};
void Tx81z::clean_bank() {
    LOG_IN();
    LOG_OUT();
};

void Tx81z::seek_voice(uint, st_tx81zsysex_1* voice){
    LOG_IN();
    LOG_OUT();
};
void Tx81z::send_voice(st_tx81zsysex_1* voice){
    LOG_IN();
    LOG_OUT();
};
void Tx81z::set_voice(st_tx81zsysex_1* voice){
        LOG_IN();
        LOG_OUT();
};

/* UI SIGNALS CONNECTION */
void Tx81z::attach_signals(){
    LOG_IN();
    /*slot_bank_select = (get_gwidget<Gtk::FileChooserButton>("bank_select"))->signal_selection_changed().connect(
     *   sigc::mem_fun(*this, &Dx7interface::on_bank_select));
     */
    slot_bank_reveal = (get_gwidget<Gtk::Button>("btn_toolbar_reveal_bank"))->signal_clicked().connect(
        sigc::mem_fun(*this, &Tx81z::on_bank_reveal));


    LOG_OUT();
};

void Tx81z::on_bank_reveal(){
    LOG_IN();
    (get_gwidget<Gtk::Revealer>("revealer_bank"))->set_reveal_child(!(get_gwidget<Gtk::Revealer>("revealer_bank"))->get_reveal_child());
    LOG_OUT();
};

void Tx81z::dettach_signals(){
    LOG_IN();
    slot_bank_reveal.disconnect();
    LOG_OUT();
};

void Tx81z::block_all(){
    LOG_IN();
    /*** Block UI ***/
    /* Draw */
    //slot_bank_select.block(true);
    LOG_OUT();
};

void Tx81z::unblock_all(){
    LOG_IN();
    /*** Block UI ***/
    /* Draw */
    //slot_bank_select.unblock();
    LOG_OUT();
};

void Tx81z::on_bank_select(){
    LOG_IN();
    LOG_OUT();
};
