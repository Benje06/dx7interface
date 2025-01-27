/*
 * Class object for GTK:: specific structure
 */
#pragma once
#include <gxinterface/0.0.1/gxmodule.h>

/* Class to manager Items in ListStore Sound_bank*/
class SoundBankItem : public Glib::Object {
    private:
        uint i_number;         /*internal number*/
        Glib::ustring i_name;   /*internal name*/
        SoundBankItem(uint, const Glib::ustring&);
    public:

        static Glib::RefPtr<SoundBankItem> create(uint number, const Glib::ustring& name){
            return Glib::make_refptr_for_instance<SoundBankItem>(new SoundBankItem(number, name));
        };
        uint get_number() { return i_number; };
        Glib::ustring get_name() { return i_name; };

};

class Factory {
    public:
        void on_bind_num(void*, const Glib::RefPtr<Gtk::ListItem>&);
        void on_bind_name(void*, const Glib::RefPtr<Gtk::ListItem>&);
};



