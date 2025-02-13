/*
*
*
*/
#include "GtkClass.h"

SoundBankItem::SoundBankItem(uint number,const Glib::ustring& name) : i_number(number),  i_name(name) {};

void Factory::on_bind_num(void*, const Glib::RefPtr<Gtk::ListItem>& list_item){
    auto col = std::dynamic_pointer_cast<SoundBankItem>(list_item->get_item());
    if (!col){ return; };
    auto label = dynamic_cast<Gtk::Label*>(list_item->get_child());
    if (!label){ return; };
    label->set_text(Glib::ustring::format(col->get_number()));
};

void Factory::on_bind_name(void*, const Glib::RefPtr<Gtk::ListItem>& list_item){
    auto col = std::dynamic_pointer_cast<SoundBankItem>(list_item->get_item());
    if (!col){ return; };
    auto label = dynamic_cast<Gtk::Label*>(list_item->get_child());
    if (!label){ return; };
    label->set_text(col->get_name());
};

DropDownScrollController::DropDownScrollController(Gtk::DropDown* dropdown)  {
    auto scroll_controller = Gtk::EventControllerScroll::create();
    scroll_controller->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
    scroll_controller->signal_scroll().connect(
            sigc::bind(
                sigc::mem_fun(*this, &DropDownScrollController::on_scroll)
                , scroll_controller
            )
        ,false
    );
    dropdown->add_controller(scroll_controller);
};

bool DropDownScrollController::on_scroll(double dx, double dy, Glib::RefPtr<Gtk::EventControllerScroll> scroll_controller){
    Gtk::DropDown* widget = dynamic_cast<Gtk::DropDown*>(scroll_controller->get_widget());
    if (widget) {
        auto model = widget->get_model();
        if (!model) return false;
        int current = widget->get_selected();
        int n_items = model->get_n_items();
        if (dy < 0) {
            // Scroll up
            widget->set_selected(std::max(0, current - 1));
        } else if (dy > 0) {
            // Scroll down
            widget->set_selected(std::min(n_items - 1, current + 1));
        };
        // Handle the scroll event
    };
    return true;
};

DropDownScrollController::DropDownScrollController(Gtk::DropDown* dropdown, Gtk::SpinButton* spinbutton)  {
    auto scroll_controller = Gtk::EventControllerScroll::create();
    scroll_controller->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
    scroll_controller->signal_scroll().connect(
        sigc::bind(
            sigc::mem_fun(*this, &DropDownScrollController::on_scroll_kls)
            , scroll_controller, spinbutton
        )
        ,false
    );
    dropdown->add_controller(scroll_controller);
};

bool DropDownScrollController::on_scroll_kls(double dx, double dy, Glib::RefPtr<Gtk::EventControllerScroll> scroll_controller, Gtk::SpinButton* spinbutton){
    Gtk::DropDown* widget = dynamic_cast<Gtk::DropDown*>(scroll_controller->get_widget());
    if (widget) {
        auto model = widget->get_model();
        if (!model) return false;
        int current = widget->get_selected();
        int n_items = model->get_n_items();
        if (dy < 0) {
            // Scroll up
            if(current == 0){ //A
                widget->set_selected(n_items-1);
            }else if(current == 3){ //C
                widget->set_selected(std::max(0, current - 1));
                spinbutton->set_value(spinbutton->get_value()-1);
            }else{
                widget->set_selected(std::max(0, current - 1));
            }
        } else if (dy > 0) {
            if(current == n_items-1){ //G#
                widget->set_selected(0);
            }else if(current == 2){
                widget->set_selected(std::max(0, current + 1));
                spinbutton->set_value(spinbutton->get_value()+1);
            }else{
            // Scroll down
                widget->set_selected(std::min(n_items - 1, current + 1));
            };
        };
        // Handle the scroll event
    };
    return true;
};
