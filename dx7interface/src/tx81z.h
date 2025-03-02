/* ----------------------------------------------------------------------------
 * Tx81z.h -- DX7 Graphic interface
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
/*TODO :
 * timer
 *delete midi
 *write bank
 *receive sysex
 *charger/decharger synth
 *attache/detache midi one by synth
 */
#pragma once
#define MODULE_NAME "Tx81zSyx"
/* sys */
#include <memory>
#include <gxinterface/0.0.1/gxmodule.h>
#include "GtkClass.h"
/* Synth */
#include <synth.h>
/* sysex */
#include "tx81zsysex.h"

/** CONSTANTS **/
//#define UI MOD_DIRECTORY"libdx7interface-0.0.1.ui"
//#define CSSFILE MOD_DIRECTORY"theme.css"
#define UI "data/ui/libdx7interface-0.0.1.ui"
#define CSSFILE "data/ui/theme.css"

extern "C" {
	std::tuple<std::shared_ptr<void>, Gtk::Box*, Glib::ustring> LoadPlug(uint8_t);
};

class Tx81z : public Gx_module, public Synth {
    public:
        Tx81z(Glib::ustring,uint8_t);
        virtual ~Tx81z();
    private:
        /*** Tx81z sysex ***/
        static const uint8_t id_fabricant=0x43;  /* static fix yamaha id */
        /* SySeX format (bank/sound/message) */
        St_tx81zsysex<1> bank_1_origin;           /* bank d'origine 1 son */
        St_tx81zsysex<1> bank_1_modif;            /* bank modifié 1 son */
        St_tx81zsysex<32> bank_32_origin;         /* bank d'origine 32 sons */
        St_tx81zsysex<32> bank_32_modif;          /* ... */
        St_tx81zsysex<128> bank_128_origin;       /* ... */
        St_tx81zsysex<128> bank_128_modif;        /* ... */
        Glib::RefPtr<Gio::File> bank_file;      /* pointeur de lecture de fichier */
        Glib::RefPtr<Gio::DataInputStream> data_stream; /* pointeur de flux */
        /* MIDI */
        snd_seq_t* seq_handle = nullptr;	            /* handler */
        snd_seq_system_info_t* seq_info = nullptr;      /* info */
        snd_seq_event_t* ev = nullptr;                  /* evenement */
        size_t in_buff_size, out_buff_size;   /* buffer d'entré et de sorti */
        /* Generic error */
        bool error();
        /*** THREAD ***/
        bool Run() ;    /* Thread function  */
        bool Run2();    /* Thread function  */
        /*** MIDI ***/
        void listen_midi() override;
        /* sound bank */
        void load_bank(Glib::RefPtr<Gio::File>);
        void save_bank(Glib::RefPtr<Gio::File>);
        void save_bank_as(Glib::RefPtr<Gio::File>);
        void clean_bank();
        /* voice  */
        void seek_voice(uint, st_tx81zsysex_1*);
        void send_voice(st_tx81zsysex_1*);
        void set_voice(st_tx81zsysex_1*);

        /**** UI ****/
        //Glib::RefPtr<Gio::ListStore> m_refListStore ; /* liste des nom des sons de la banque chargé */
        Glib::RefPtr<Gio::ListStore<SoundBankItem>> m_refListStore;
        /*** DRAWING ***/
        /* lines/curves */
        double line_width=1.0;                                      // epaisseur
        std::array<double, 4> line_color = {0.4,0.8,0.6,1.0};       // couleur des courbes ( vert Dx7 ) format rgbax
        /* dashes */
        double dash_width=0.5;                                      // epaisseur
        std::array<double, 4> dash_color = {0.4,0.8,0.6, 0.8};    // couleur format rgba
        const std::vector< double > dash_pattern = {3.0, 5.0, 3.0, 5.0};  // pattern des pointillé ( lgt couleur, lgt espace , lgt couleur , lgt espace)
        double dash_offset = 0.0;                                   // offset du dash pattern
        /* grid */
        double grid_step_y=5.0, grid_step_x=20.0;                  // nombre de pas voulu en x y
        /* text */
        std::array<double, 4> text_color = {1.0, 0.5, 0.2, 0.8};
        std::array<double, 4> bg_color = {0.0, 0.0, 0.0, 0.0};
        /* Cairomm context helpers */
        int* get_cr_visible_size(const Cairo::RefPtr<Cairo::Context>&, Glib::ustring);  /* retourne la taille de sla zone visible */
        /* ADSR */
        void draw_background(const Cairo::RefPtr<Cairo::Context>&);                     /* dessine le fond */
        void draw_grid(const Cairo::RefPtr<Cairo::Context>&, int*, double*);            /* dessisne la grille */
        double* draw_adsr(const Cairo::RefPtr<Cairo::Context>&, int*, Glib::ustring);   /* dessine la courbe */
        void draw_point(const Cairo::RefPtr<Cairo::Context>& cr,double,double);                        /* dessine un point */

        /** EVENTS / SIGNAL **/
        void block_all();                       /* blocage des evenements de l'interface */
        void unblock_all();                     /* ... */
        void attach_signals() override;
        void dettach_signals() override;
        void on_bank_select();
        sigc::connection slot_bank_select;
        void on_bank_reveal();
        sigc::connection slot_bank_reveal;

};
