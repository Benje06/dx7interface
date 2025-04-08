/*
 * Class object for GTK:: specific structure
 */
#pragma once
#include <gxinterface/0.0.1/gxmodule.h>
#include <gtkmm/eventcontrollerscroll.h>

/* Class to manager Items in ListStore Sound_bank*/
class SoundBankItem : public Glib::Object {
    private:
        unsigned int i_number;         /*internal number*/
        Glib::ustring i_name;   /*internal name*/
        SoundBankItem(unsigned int, const Glib::ustring&);
    public:
        static Glib::RefPtr<SoundBankItem> create(unsigned int number, const Glib::ustring& name){
            return Glib::make_refptr_for_instance<SoundBankItem>(new SoundBankItem(number, name));
        };
        void set_number(unsigned int num) { i_number = num; };
        void set_name(Glib::ustring name) { i_name = name; };
        unsigned int get_number() { return i_number; };
        Glib::ustring get_name() { return i_name; };
};

/*class Factory {
    public:
        void on_bind_num(void*, const Glib::RefPtr<Gtk::ListItem>&);
        void on_bind_name(void*, const Glib::RefPtr<Gtk::ListItem>&);
};*/

/* Class to manager Items in ListStore Sound_bank*/
class ParamItem : public Glib::Object {
private:
    Glib::ustring i_name;   /*internal name*/
    ParamItem(const Glib::ustring&);
public:
    static Glib::RefPtr<ParamItem> create(const Glib::ustring& name){
        return Glib::make_refptr_for_instance<ParamItem>(new ParamItem(name));
    };
    Glib::ustring get_name() { return i_name; };
};

/*class FactoryParam {
public:
    void on_bind_name(void*, const Glib::RefPtr<Gtk::ListItem>&);
};*/

class DropDownScrollController{
    private:
        //Glib::RefPtr<Gtk::EventControllerScroll>  scroll_controller;
        bool on_scroll(double, double,Glib::RefPtr<Gtk::EventControllerScroll>);
        bool on_scroll_kls(double, double,Glib::RefPtr<Gtk::EventControllerScroll>,Gtk::SpinButton*,sigc::connection);
        //Gtk::DropDown* widget;
    public:
        DropDownScrollController(Gtk::DropDown*);
        DropDownScrollController(Gtk::DropDown*,Gtk::SpinButton*,sigc::connection);
};
