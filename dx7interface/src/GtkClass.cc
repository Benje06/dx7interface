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
