/*
 * Class object for GTK:: specific structure
 */
#pragma once
#include <gxinterface/0.0.1/gxmodule.h>
#include <gtkmm/eventcontrollerscroll.h>

/* Class to manager Items in ListStore Sound_bank*/
class SoundBankItem : public Glib::Object {
    private:
        // Protected constructor to enforce the use of the factory method
        SoundBankItem(const Glib::ustring& number, const Glib::ustring& name)
        : Glib::ObjectBase("SoundBankItem"),
        i_number(*this, "number", number),  // Initialize properties
        i_name(*this, "name", name) {}
        // Properties
        Glib::Property<Glib::ustring> i_number;  // Internal number property
        Glib::Property<Glib::ustring> i_name;   // Internal name property

    public:
        // Factory method for creating instances
        static Glib::RefPtr<SoundBankItem> create(const Glib::ustring& number, const Glib::ustring& name) {
            return Glib::make_refptr_for_instance<SoundBankItem>(new SoundBankItem(number, name));
        }

        // Setters
        void set_number(const Glib::ustring& num) {
            i_number.set_value(num);
        }
        void set_name(const Glib::ustring& name) {
            i_name.set_value(name);
        }

        // Getters (marked as const)
        Glib::ustring get_number() const { return i_number.get_value(); }
        Glib::ustring get_name() const { return i_name.get_value(); }
};
/* Class to manager Items in ListStore midi learn*/
class ParamItem : public Glib::Object {
    private:
        ParamItem(const Glib::ustring& name)
        : Glib::ObjectBase("ParamItem"),
        i_name(*this, "name", name) {}  // Initialize properties

        Glib::Property<Glib::ustring> i_name;   /*internal name*/
    public:
        static Glib::RefPtr<ParamItem> create(const Glib::ustring& name){
            return Glib::make_refptr_for_instance<ParamItem>(new ParamItem(name));
        }

        void set_name(const Glib::ustring& name) {
            i_name.set_value(name);
        }
        Glib::ustring get_name() const { return i_name.get_value(); }
};

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
