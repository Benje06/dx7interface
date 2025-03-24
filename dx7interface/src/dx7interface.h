/* ----------------------------------------------------------------------------
 * Dx7interface.h -- DX7 Graphic interface
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
/*
 * TODO :
 * timer
 * receive sysex
*/
#pragma once
#define MODULE_NAME "Dx7interface"
/* define for export type */
#define DX7_1 1
#define DX7_32 2
#define DX7_128 3
#define DX7_RAW 4
#define DX7_SYX 5
#define BANK 0
#define SOUND 1
/* sys */
//#include <memory>
/*** APP ***/
#include <gxinterface/0.0.1/common.h>
#include <gxinterface/0.0.1/gxmodule.h>
#include <filesystem>
#include "GtkClass.h"
/* Synth */
#include <synth.h>
/* sysex */
#include "dx7sysex.h"

/** CONSTANTS **/
#define DATA_DIR PROGRAMNAME_DATA_DIR
#if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
        #define UI MOD_UI_DIRECTORY"dx7interface-0.0.1-simplify.ui"
#else
         #define UI MOD_UI_DIRECTORY"dx7interface-0.0.1-simplify_4.8.ui"
#endif
//#define UI MOD_UI_DIRECTORY"dx7interface-0.0.1-gtk4_simplify.ui"
#define CSSFILE MOD_UI_DIRECTORY"theme.css"

extern "C" {
    std::tuple<std::shared_ptr<void>, Gtk::Box*, Glib::ustring> LoadPlug(uint8_t);
};

class Dx7interface : public Gx_module, public Synth {
    public:
        Dx7interface(Glib::ustring,uint8_t);
        virtual ~Dx7interface();
        void add_action();

    private:
        /**** Generic ****/
        bool error();
        using FunctionPtr = void (Dx7interface::*)();  /* abstract for function as array */
        FunctionPtr mute_hexter_functions[6] = {
            &Dx7interface::on_mute_hexter_op1_event,
            &Dx7interface::on_mute_hexter_op2_event,
            &Dx7interface::on_mute_hexter_op3_event,
            &Dx7interface::on_mute_hexter_op4_event,
            &Dx7interface::on_mute_hexter_op5_event,
            &Dx7interface::on_mute_hexter_op6_event
        };

        /*** ALSA MIDI ***/
        snd_seq_t* seq_handle = nullptr;                /* handler */
        snd_seq_system_info_t* info = nullptr;          /* info */
        snd_seq_event_t* ev = nullptr;                  /* evenement */
        size_t in_buff_size, out_buff_size;               /* buffer d'entré et de sortie */

        /* */
        bool lock = false;
        bool compare = false;                   /* set if compare button is activate */
        bool send_extra_params = false;         /* set if send_extra paraameter is activate */
        bool write_extra_params = false;         /* set if send_extra paraameter is activate */
        bool mode_tf1 = false;                   /* mode tf1 = fonction parameter by sound */

        /*** Dx7 specific ***/
        static const uint8_t id_fabricant=0x43; /* static fix yamaha id */

        /* SySeX format (bank/sound/message) */
        St_dx7sysex<1> bank_1_origin;           /* bank d'origine 1 son */
        St_dx7sysex<1> bank_1_modif;            /* bank modifié 1 son */
        St_dx7sysex<32> bank_32_origin;         /* bank d'origine 32 sons */
        St_dx7sysex<32> bank_32_modif;          /* ... */
        St_dx7sysex<128> bank_128_origin;       /* ... */
        St_dx7sysex<128> bank_128_modif;        /* ... */

        /* default write format */
        uint export_config = DX7_32;
        uint save_type = BANK;

        /* Bank */
        uint bank_nb_sound = 0;                                /* number of sound in the current loaded bank 1/32/128 */
        uint old_snum = 0;                                     /* old selected sound number memo for set_original_sound */
        Glib::RefPtr<Gio::File> bank_file=nullptr;              /* pointeur de lecture de fichier */
        Glib::RefPtr<Gio::File> initial_folder_open=nullptr;
        Glib::RefPtr<Gio::File> initial_folder_save=nullptr;
        Glib::RefPtr<Gio::File> initial_folder_open_param=nullptr;
        Glib::RefPtr<Gio::File> initial_folder_save_param=nullptr;

        Glib::RefPtr<Gio::DataInputStream> data_stream=nullptr;           /* pointeur de flux du fichier de données */
        Glib::RefPtr<Gio::DataInputStream> data_stream_param=nullptr;     /* pointer de flux du fichier de parametres */
        bool isStreamClosed(Glib::RefPtr<Gio::DataInputStream>&);
        /* bank list view */
        void create_bank_voices_list();
        Glib::RefPtr<Gio::ListStore<SoundBankItem>> bank_data_model=nullptr; /* liste des nom des sons de la banque chargé */
        Glib::RefPtr<Gtk::SingleSelection> bank_selection_model=nullptr;
        Glib::RefPtr<Gtk::SignalListItemFactory> bank_factory=nullptr;

        /* midi learn */
        /* param list view */
        bool midi_learn=false;
        static const int max_param_nb = 168;
        std::vector<int> midi_param{std::vector<int>(max_param_nb, -1)};
        std::vector<std::vector<int>> midi_learned;

        /* read and write midi learn config file */
        void read_midi_learned_param(Glib::RefPtr<Gio::File>);
        void save_midi_learned_param(Glib::RefPtr<Gio::File>);
        void clean_midi_learn();
        sigc::connection slot_midi_learn_load;
        void on_midi_learn_param_select();
        sigc::connection slot_midi_learn_save;
        void on_midi_learn_param_save();

        /* manage midi learn event */
        void on_midi_learn_event();
        void on_add_midi_learn_event();
        void add_midi_learned(int, int);
        void add_midi_learn_param_widget(Glib::ustring, Glib::ustring,int);
        void rem_midi_learned(int, int);

        Glib::ustring function_list[max_param_nb]{
            "aftrtch_assgn_event",
            "aftrtch_rng_event",
            "algo_event",
            "ams_op1_event",
            "ams_op2_event",
            "ams_op3_event",
            "ams_op4_event",
            "ams_op5_event",
            "ams_op6_event",
            "brth_assgn_event",
            "brth_rng_event",
            "compare_event",
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
            "lfo_amd_event",
            "lfo_delay_event",
            "lfo_pmd_event",
            "lfo_speed_event",
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
            "pms_event",
            "portamento_glss_event",
            "portamento_md_event",
            "portamento_tm_event",
            "ptch_bnd_rng_event",
            "ptch_bnd_stp_event",
            "send_extra_parameters_event",
            "transpose_event"
        };

        void create_param_list();
        Glib::RefPtr<Gio::ListStore<ParamItem>> param_data_model=nullptr; /* liste des nom des sons de la banque chargé */
        Glib::RefPtr<Gtk::SingleSelection> param_selection_model=nullptr;
        Glib::RefPtr<Gtk::SignalListItemFactory> param_factory=nullptr;
        void on_bind_param_name(const Glib::RefPtr<Gtk::ListItem>&);
        void on_setup_param_label(const Glib::RefPtr<Gtk::ListItem>&, Gtk::Align);

        using FunctionIntPtr = void (Dx7interface::*)(int);  /* abstract for function as array */
        FunctionIntPtr list_ui_parameters_functions[max_param_nb] = {
            &Dx7interface::set_aftrtch_assgn_event,
            &Dx7interface::set_aftrtch_rng_event,
            &Dx7interface::set_algo_event,
            &Dx7interface::set_ams_op1_event,
            &Dx7interface::set_ams_op2_event,
            &Dx7interface::set_ams_op3_event,
            &Dx7interface::set_ams_op4_event,
            &Dx7interface::set_ams_op5_event,
            &Dx7interface::set_ams_op6_event,
            &Dx7interface::set_brth_assgn_event,
            &Dx7interface::set_brth_rng_event,
            &Dx7interface::set_compare_event,
            &Dx7interface::set_dtun_op1_event,
            &Dx7interface::set_dtun_op2_event,
            &Dx7interface::set_dtun_op3_event,
            &Dx7interface::set_dtun_op4_event,
            &Dx7interface::set_dtun_op5_event,
            &Dx7interface::set_dtun_op6_event,
            &Dx7interface::set_eg_lvl1_op1_event,
            &Dx7interface::set_eg_lvl1_op2_event,
            &Dx7interface::set_eg_lvl1_op3_event,
            &Dx7interface::set_eg_lvl1_op4_event,
            &Dx7interface::set_eg_lvl1_op5_event,
            &Dx7interface::set_eg_lvl1_op6_event,
            &Dx7interface::set_eg_lvl2_op1_event,
            &Dx7interface::set_eg_lvl2_op2_event,
            &Dx7interface::set_eg_lvl2_op3_event,
            &Dx7interface::set_eg_lvl2_op4_event,
            &Dx7interface::set_eg_lvl2_op5_event,
            &Dx7interface::set_eg_lvl2_op6_event,
            &Dx7interface::set_eg_lvl3_op1_event,
            &Dx7interface::set_eg_lvl3_op2_event,
            &Dx7interface::set_eg_lvl3_op3_event,
            &Dx7interface::set_eg_lvl3_op4_event,
            &Dx7interface::set_eg_lvl3_op5_event,
            &Dx7interface::set_eg_lvl3_op6_event,
            &Dx7interface::set_eg_lvl4_op1_event,
            &Dx7interface::set_eg_lvl4_op2_event,
            &Dx7interface::set_eg_lvl4_op3_event,
            &Dx7interface::set_eg_lvl4_op4_event,
            &Dx7interface::set_eg_lvl4_op5_event,
            &Dx7interface::set_eg_lvl4_op6_event,
            &Dx7interface::set_eg_rt1_op1_event,
            &Dx7interface::set_eg_rt1_op2_event,
            &Dx7interface::set_eg_rt1_op3_event,
            &Dx7interface::set_eg_rt1_op4_event,
            &Dx7interface::set_eg_rt1_op5_event,
            &Dx7interface::set_eg_rt1_op6_event,
            &Dx7interface::set_eg_rt2_op1_event,
            &Dx7interface::set_eg_rt2_op2_event,
            &Dx7interface::set_eg_rt2_op3_event,
            &Dx7interface::set_eg_rt2_op4_event,
            &Dx7interface::set_eg_rt2_op5_event,
            &Dx7interface::set_eg_rt2_op6_event,
            &Dx7interface::set_eg_rt3_op1_event,
            &Dx7interface::set_eg_rt3_op2_event,
            &Dx7interface::set_eg_rt3_op3_event,
            &Dx7interface::set_eg_rt3_op4_event,
            &Dx7interface::set_eg_rt3_op5_event,
            &Dx7interface::set_eg_rt3_op6_event,
            &Dx7interface::set_eg_rt4_op1_event,
            &Dx7interface::set_eg_rt4_op2_event,
            &Dx7interface::set_eg_rt4_op3_event,
            &Dx7interface::set_eg_rt4_op4_event,
            &Dx7interface::set_eg_rt4_op5_event,
            &Dx7interface::set_eg_rt4_op6_event,
            &Dx7interface::set_feedback_event,
            &Dx7interface::set_foot_assgn_event,
            &Dx7interface::set_foot_rng_event,
            &Dx7interface::set_freq_coarse_op1_event,
            &Dx7interface::set_freq_coarse_op2_event,
            &Dx7interface::set_freq_coarse_op3_event,
            &Dx7interface::set_freq_coarse_op4_event,
            &Dx7interface::set_freq_coarse_op5_event,
            &Dx7interface::set_freq_coarse_op6_event,
            &Dx7interface::set_freq_fine_op1_event,
            &Dx7interface::set_freq_fine_op2_event,
            &Dx7interface::set_freq_fine_op3_event,
            &Dx7interface::set_freq_fine_op4_event,
            &Dx7interface::set_freq_fine_op5_event,
            &Dx7interface::set_freq_fine_op6_event,
            &Dx7interface::set_freq_mode_op1_event,
            &Dx7interface::set_freq_mode_op2_event,
            &Dx7interface::set_freq_mode_op3_event,
            &Dx7interface::set_freq_mode_op4_event,
            &Dx7interface::set_freq_mode_op5_event,
            &Dx7interface::set_freq_mode_op6_event,
            &Dx7interface::set_kls_brk_pt_op1_event,
            &Dx7interface::set_kls_brk_pt_op2_event,
            &Dx7interface::set_kls_brk_pt_op3_event,
            &Dx7interface::set_kls_brk_pt_op4_event,
            &Dx7interface::set_kls_brk_pt_op5_event,
            &Dx7interface::set_kls_brk_pt_op6_event,
            &Dx7interface::set_kls_lft_curve_op1_event,
            &Dx7interface::set_kls_lft_curve_op2_event,
            &Dx7interface::set_kls_lft_curve_op3_event,
            &Dx7interface::set_kls_lft_curve_op4_event,
            &Dx7interface::set_kls_lft_curve_op5_event,
            &Dx7interface::set_kls_lft_curve_op6_event,
            &Dx7interface::set_kls_lft_dpth_op1_event,
            &Dx7interface::set_kls_lft_dpth_op2_event,
            &Dx7interface::set_kls_lft_dpth_op3_event,
            &Dx7interface::set_kls_lft_dpth_op4_event,
            &Dx7interface::set_kls_lft_dpth_op5_event,
            &Dx7interface::set_kls_lft_dpth_op6_event,
            &Dx7interface::set_kls_rght_curve_op1_event,
            &Dx7interface::set_kls_rght_curve_op2_event,
            &Dx7interface::set_kls_rght_curve_op3_event,
            &Dx7interface::set_kls_rght_curve_op4_event,
            &Dx7interface::set_kls_rght_curve_op5_event,
            &Dx7interface::set_kls_rght_curve_op6_event,
            &Dx7interface::set_kls_rght_dpth_op1_event,
            &Dx7interface::set_kls_rght_dpth_op2_event,
            &Dx7interface::set_kls_rght_dpth_op3_event,
            &Dx7interface::set_kls_rght_dpth_op4_event,
            &Dx7interface::set_kls_rght_dpth_op5_event,
            &Dx7interface::set_kls_rght_dpth_op6_event,
            &Dx7interface::set_krs_op1_event,
            &Dx7interface::set_krs_op2_event,
            &Dx7interface::set_krs_op3_event,
            &Dx7interface::set_krs_op4_event,
            &Dx7interface::set_krs_op5_event,
            &Dx7interface::set_krs_op6_event,
            &Dx7interface::set_kvs_op1_event,
            &Dx7interface::set_kvs_op2_event,
            &Dx7interface::set_kvs_op3_event,
            &Dx7interface::set_kvs_op4_event,
            &Dx7interface::set_kvs_op5_event,
            &Dx7interface::set_kvs_op6_event,
            &Dx7interface::set_lfo_amd_event,
            &Dx7interface::set_lfo_delay_event,
            &Dx7interface::set_lfo_pmd_event,
            &Dx7interface::set_lfo_speed_event,
            &Dx7interface::set_lfo_sync_event,
            &Dx7interface::set_lfo_wav_event,
            &Dx7interface::set_lvl_op1_event,
            &Dx7interface::set_lvl_op2_event,
            &Dx7interface::set_lvl_op3_event,
            &Dx7interface::set_lvl_op4_event,
            &Dx7interface::set_lvl_op5_event,
            &Dx7interface::set_lvl_op6_event,
            &Dx7interface::set_md_whl_assgn_event,
            &Dx7interface::set_md_whl_rng_event,
            &Dx7interface::set_mono_poly_event,
            &Dx7interface::set_mute_op1_event,
            &Dx7interface::set_mute_op2_event,
            &Dx7interface::set_mute_op3_event,
            &Dx7interface::set_mute_op4_event,
            &Dx7interface::set_mute_op5_event,
            &Dx7interface::set_mute_op6_event,
            &Dx7interface::set_oks_event,
            &Dx7interface::set_panic_event,
            &Dx7interface::set_pitch_lvl1_event,
            &Dx7interface::set_pitch_lvl2_event,
            &Dx7interface::set_pitch_lvl3_event,
            &Dx7interface::set_pitch_lvl4_event,
            &Dx7interface::set_pitch_rt1_event,
            &Dx7interface::set_pitch_rt2_event,
            &Dx7interface::set_pitch_rt3_event,
            &Dx7interface::set_pitch_rt4_event,
            &Dx7interface::set_pms_event,
            &Dx7interface::set_portamento_glss_event,
            &Dx7interface::set_portamento_md_event,
            &Dx7interface::set_portamento_tm_event,
            &Dx7interface::set_ptch_bnd_rng_event,
            &Dx7interface::set_ptch_bnd_stp_event,
            &Dx7interface::set_send_extra_parameters_event,
            &Dx7interface::set_transpose_event
        };

        /* pop hover menu */
        void create_popover_menu();
        Glib::RefPtr<Gio::SimpleActionGroup> action_group=nullptr;
        Gtk::PopoverMenu* m_popover_menu = nullptr;
        /* save dialog */
        void create_save_dialog();
        Gtk::Window* dialog_bank_save = nullptr;
        Gtk::Button* button_bank_save = nullptr;

        Gtk::CheckButton* checkbutton_bulk = nullptr;
        #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
            Gtk::FileDialog* file_dialog_bank_select = nullptr;
            Gtk::FileDialog* file_dialog_bank_save = nullptr;
            Gtk::FileDialog* file_dialog_param_select = nullptr;
            Gtk::FileDialog* file_dialog_param_save = nullptr;
        #else
            Gtk::FileChooserDialog* file_dialog_bank_select = nullptr;
            Gtk::FileChooserDialog* file_dialog_bank_save = nullptr;
            Gtk::FileChooserDialog* file_dialog_param_select = nullptr;
            Gtk::FileChooserDialog* file_dialog_param_save = nullptr;
            Gtk::Button* button_accept = nullptr;
        #endif

        void OpenFileSaveDialog();
        void OpenFileSelectDialog(Glib::ustring,Glib::ustring);

        /*** THREAD ***/
        bool Run();    /* Thread function  */
        bool Run2();    /* Thread function  */
        /*** MIDI ***/
        void listen_midi() override;

        /** SOUND BANK **/
        /* set/load */
        void set_default_values();
        void set_bank(Glib::RefPtr<Gio::File>);
        void clean_bank();  // read reset1.syx reset32.syx reset128.syx (empty file 0x00 of specified number of voice)
        void load_bank(Glib::RefPtr<Gio::File>);
        /* restore */
        void on_restore_bank();
        void restore_origin_bank();
        void on_restore_sound();
        void restore_origin_sound();
        /* repalce/delete */
        void on_insert_after();
        void on_replace_sound();
        void on_delete_sound();
        /* save/write */
        void write_file(Glib::RefPtr<Gio::File>, u_char*, uint);
        void write_voice_extra_parameters(st_dx7sysex_1*, u_char*, uint*);
        /* BANK */
        void on_save_bank();
        void write_bank(Glib::RefPtr<Gio::File>, uint);
        void write_bank_as_sysex(Glib::RefPtr<Gio::File>, uint);
        void write_bank_as_raw(Glib::RefPtr<Gio::File> file, uint);
        void on_as_raw_event();
        void on_extra_param_event();

        /* VOICE */
        void write_voice_bulk1(uint*, u_char*, St_dx7sysex_1*, uint8_t*);
        void write_voice_bulk32(uint*, u_char*, St_dx7sysex_1*, uint8_t*);
        void write_voice_as_sysex(Glib::RefPtr<Gio::File>);
        void write_voice_as_raw(Glib::RefPtr<Gio::File>);
        void save_modif_sound();    /* save internally on origin bank */
        void on_save_sound();       /* save internally and write file */
        void write_voices_as_n_sysex(St_dx7sysex_1*);
        /* */
        void save_bank_as(Glib::RefPtr<Gio::File>);
        void clear_sound(St_dx7sysex_1*,uint8_t,bool);     // set 0x00 to all param to voice struct "aka clear struct"
        void set_as_origin_sound(uint);                // set bank_X_modif.sound as bank_X_origin.sound

        /** VOICE  **/
        /* seek voice value from bank file and write it to sound */
        void seek_voice(uint8_t, st_dx7sysex_1*);     // get voice param from file to fill sound struct
        void seek_voice_by_byte(uint8_t, st_dx7sysex_1*);     // get voice param from file to fill sound struct
        void seek_parameters(Glib::ustring, St_dx7sysex_1*); // get sound parameter from file to fill sound extra param struct
        void seek_voice_parameters(St_dx7sysex_1*);
        /* send voice over midi */
        void send_voice(st_dx7sysex_1*);              // send voice to midi
        void send_extra_parameters(st_dx7sysex_1*);         // send voice to midi
        /* set voice to interface */
        void set_voice(st_dx7sysex_1*);               // set voice in GUI
        void set_voice_parameters(St_dx7sysex_1*);    // set sound parameter


        /*** DRAWING ***/
        /* lines/curves */
        double line_width = 3.0;                                      // epaisseur
        /* point */
        double r_point = 4.5;
        double r_point_shadow = 9.0;
        std::array<double, 4> line_color = {0.4,0.8,0.6,1.0};       // couleur des courbes ( vert Dx7 ) format rgbax
        /* dashes */
        double dash_width = 0.5;                                      // epaisseur
        std::array<double, 4> dash_color = {0.4,0.8,0.6,0.8};    // couleur format rgba
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
        void redraw_all_curve();
        void draw_background(const Cairo::RefPtr<Cairo::Context>&);                                 /* dessine le fond */
        void draw_grid(const Cairo::RefPtr<Cairo::Context>&, double, double);                       /* dessisne la grille */
        void draw_adsr(const Cairo::RefPtr<Cairo::Context>&, double, double, Glib::ustring);        /* dessine la courbe */
        void draw_point(const Cairo::RefPtr<Cairo::Context>&, double, double,double,bool);          /* dessine un point */
        void draw_note_off(const Cairo::RefPtr<Cairo::Context>&, double, double);
        /* Level Scaling */
        void draw_kls(const Cairo::RefPtr<Cairo::Context>&, double, double, Glib::ustring);         /* dessine la courbe */
        void draw_keyboard(const Cairo::RefPtr<Cairo::Context>&, double, double, Glib::ustring);    /*dessine le clavier */
        void draw_axis(const Cairo::RefPtr<Cairo::Context>&, double, double);                       /*dessine le clavier */
        void draw_kls_curve(const Cairo::RefPtr<Cairo::Context>&,Glib::ustring, double, double, double, Glib::ustring); /* draw kls curve type */

        /* Mouse Gesture */
        void init_gesture_controller();
        std::map< Glib::ustring, std::pair<double,double>[5]> drawarea;                             /* array of points coordinates */
        int p_drag = -1;                                                                            /* current index in points coordinates array */
        /* Mouse click */
        Glib::RefPtr<Gtk::GestureClick> controller_mouse_button_op1;
        Glib::RefPtr<Gtk::GestureClick> controller_mouse_button_op2;
        Glib::RefPtr<Gtk::GestureClick> controller_mouse_button_op3;
        Glib::RefPtr<Gtk::GestureClick> controller_mouse_button_op4;
        Glib::RefPtr<Gtk::GestureClick> controller_mouse_button_op5;
        Glib::RefPtr<Gtk::GestureClick> controller_mouse_button_op6;
        Glib::RefPtr<Gtk::GestureClick> controller_mouse_button_pitch;
        void mouse_click(int, double, double, Glib::ustring);
        void mouse_click_release(int, double, double, Glib::ustring);
        /* Mouse mooves */
        Glib::RefPtr<Gtk::EventControllerMotion> controller_mouse_moove_op1;
        Glib::RefPtr<Gtk::EventControllerMotion> controller_mouse_moove_op2;
        Glib::RefPtr<Gtk::EventControllerMotion> controller_mouse_moove_op3;
        Glib::RefPtr<Gtk::EventControllerMotion> controller_mouse_moove_op4;
        Glib::RefPtr<Gtk::EventControllerMotion> controller_mouse_moove_op5;
        Glib::RefPtr<Gtk::EventControllerMotion> controller_mouse_moove_op6;
        Glib::RefPtr<Gtk::EventControllerMotion> controller_mouse_moove_pitch;
        void mouse_mooves(double, double, Glib::ustring);

        /*** UI ***/
        /** EVENTS / SIGNAL **/
        void block_ui();                       /* block all interface events */
        void unblock_ui();                     /* ... */
        void attach_signals() override;
        void dettach_signals() override;
        void attach_action_group_signals();

        /** Drawing **/
        void attach_drawarea_signals();
        void on_draw_algo(const Cairo::RefPtr<Cairo::Context>&, double, double);
        void on_draw_lfo(const Cairo::RefPtr<Cairo::Context>&, double, double);
        void on_draw_pitch_event(const Cairo::RefPtr<Cairo::Context>&, int, int);
        void on_draw_op_event(const Cairo::RefPtr<Cairo::Context>&, int, int, Glib::ustring);
        void on_draw_kls_event(const Cairo::RefPtr<Cairo::Context>&, int, int, Glib::ustring);

        /*** EVENTS and SIGC ::connection slot for blocking ***/
        /** BANK **/
        void on_bank_reveal();
        sigc::connection slot_bank_reveal;
        void on_columnview_right_click(int, double, double);
        sigc::connection slot_columnview_right_click;
        void on_selected_sound_change(uint,uint);
        sigc::connection slot_selected_sound_change;
        void on_bank_select();
        sigc::connection slot_bank_select;
        /* populate columnview */
        void on_bind_num(const Glib::RefPtr<Gtk::ListItem>&);
        void on_bind_name(const Glib::RefPtr<Gtk::ListItem>&);
        void on_setup_label(const Glib::RefPtr<Gtk::ListItem>&, Gtk::Align);

        /** Functions parameters **/
        void init_global_fonction_parameter();

        void on_midi_channel_send_event();
        sigc::connection slot_midi_channel_send;
        void on_midi_channel_receive_event();
        sigc::connection slot_midi_channel_receive;

        void on_mono_poly_event();
        sigc::connection slot_poly;
        void on_portamento_md_event();
        sigc::connection slot_portamento_md;
        void on_portamento_glss_event();
        sigc::connection slot_portamento_glss;
        void on_portamento_tm_event();
        sigc::connection slot_portamento_tm;
        void on_ptch_bnd_rng_event();
        sigc::connection slot_ptch_bnd_rng;
        void on_ptch_bnd_stp_event();
        sigc::connection slot_ptch_bnd_stp;
        /* tableau des controleurs */
        void on_md_whl_rng_event();
        sigc::connection slot_md_whl_rng;
        void on_md_whl_assgn_event();
        sigc::connection slot_md_whl_ptch;
        sigc::connection slot_md_whl_mp;
        sigc::connection slot_md_whl_gbs;
        void on_foot_rng_event();
        sigc::connection slot_foot_rng;
        void on_foot_assgn_event();
        sigc::connection slot_foot_ptch;
        sigc::connection slot_foot_mp;
        sigc::connection slot_foot_gbs;
        void on_brth_rng_event();
        sigc::connection slot_brth_rng;
        void on_brth_assgn_event();
        sigc::connection slot_brth_ptch;
        sigc::connection slot_brth_mp;
        sigc::connection slot_brth_gbs;
        void on_aftrtch_rng_event();
        sigc::connection slot_aftrtch_rng;
        void on_aftrtch_assgn_event();
        sigc::connection slot_aftrtch_ptch;
        sigc::connection slot_aftrtch_mp;
        sigc::connection slot_aftrtch_gbs;

        /* compare */
        sigc::connection slot_btn_compare;
        void on_compare_event();
        /* send parameters */
        sigc::connection slot_btn_send_extra_parameters;
        void on_send_extra_parameters_event();
        sigc::connection slot_btn_mode_tf1;
        void on_mode_tf1_event();

        /* Panic */
        sigc::connection slot_btn_panic;
        void on_panic_event();

        /** general algo **/
        void on_algo_event();
        sigc::connection slot_algo;
        void on_feedback_event();
        sigc::connection slot_feedback;
        void on_transpose_event();
        sigc::connection slot_note_transpose;
        sigc::connection slot_octv_transpose;
        void on_oks_event();
        sigc::connection slot_oks;

        /* general lfo */
        void on_lfo_wav_event();
        sigc::connection slot_lfo_wav;
        void on_lfo_sync_event();
        sigc::connection slot_lfo_sync;
        void on_lfo_speed_event();
        sigc::connection slot_lfo_speed;
        void on_lfo_delay_event();
        sigc::connection slot_lfo_delay;
        void on_lfo_pmd_event();
        sigc::connection slot_lfo_pmd;
        void on_lfo_amd_event();
        sigc::connection slot_lfo_amd;
        void on_pms_event();
        sigc::connection slot_pms;

        /* pitch eg*/
        void on_pitch_rt1_event();
        sigc::connection slot_pitch_rt1;
        void on_pitch_rt2_event();
        sigc::connection slot_pitch_rt2;
        void on_pitch_rt3_event();
        sigc::connection slot_pitch_rt3;
        void on_pitch_rt4_event();
        sigc::connection slot_pitch_rt4;
        void on_pitch_lvl1_event();
        sigc::connection slot_pitch_lvl1;
        void on_pitch_lvl2_event();
        sigc::connection slot_pitch_lvl2;
        void on_pitch_lvl3_event();
        sigc::connection slot_pitch_lvl3;
        void on_pitch_lvl4_event();
        sigc::connection slot_pitch_lvl4;

        /* mute operator for dx7*/
        void on_mute_op_event();
        sigc::connection slot_mute_op1;
        sigc::connection slot_mute_op2;
        sigc::connection slot_mute_op3;
        sigc::connection slot_mute_op4;
        sigc::connection slot_mute_op5;
        sigc::connection slot_mute_op6;
        /* mute operator FOR HEXTER DX7 modeling DSSI plugin */
        void on_mute_hexter_op1_event();
        void on_mute_hexter_op2_event();
        void on_mute_hexter_op3_event();
        void on_mute_hexter_op4_event();
        void on_mute_hexter_op5_event();
        void on_mute_hexter_op6_event();

        /* update show freq label value */
        void on_txt_freq_op_event();

        /* OP1 */
        void on_ams_op1_event(); //frame lfo
        sigc::connection slot_ams_op1;
        void on_freq_mode_op1_event();
        sigc::connection slot_freq_mode_op1;
        void on_freq_coarse_op1_event();
        sigc::connection slot_freq_coarse_op1;
        void on_freq_fine_op1_event();
        sigc::connection slot_freq_fine_op1;
        void on_dtun_op1_event();
        sigc::connection slot_dtun_op1;
        /* op1 EG*/
        void on_eg_rt1_op1_event();
        sigc::connection slot_eg_rt1_op1;
        void on_eg_rt2_op1_event();
        sigc::connection slot_eg_rt2_op1;
        void on_eg_rt3_op1_event();
        sigc::connection slot_eg_rt3_op1;
        void on_eg_rt4_op1_event();
        sigc::connection slot_eg_rt4_op1;
        void on_eg_lvl1_op1_event();
        sigc::connection slot_eg_lvl1_op1;
        void on_eg_lvl2_op1_event();
        sigc::connection slot_eg_lvl2_op1;
        void on_eg_lvl3_op1_event();
        sigc::connection slot_eg_lvl3_op1;
        void on_eg_lvl4_op1_event();
        sigc::connection slot_eg_lvl4_op1;
        /* op1 VOLUME*/
        void on_krs_op1_event();
        sigc::connection slot_krs_op1;
        void on_kvs_op1_event();
        sigc::connection slot_kvs_op1;
        void on_lvl_op1_event();
        sigc::connection slot_lvl_op1;
        /* op1 KLS*/
        void on_kls_lft_curve_op1_event();
        sigc::connection slot_kls_lft_curve_op1;
        void on_kls_rght_curve_op1_event();
        sigc::connection slot_kls_rght_curve_op1;
        void on_kls_lft_dpth_op1_event();
        sigc::connection slot_kls_lft_depth_op1;
        void on_kls_rght_dpth_op1_event();
        sigc::connection slot_kls_rght_depth_op1;
        void on_kls_brk_pt_op1_event();
        sigc::connection slot_kls_note_brk_pt_op1;
        sigc::connection slot_kls_octv_brk_pt_op1;

        /*----------------------LES AUTRES OPERATEURS------------------------*/

        /* OP2 */
        void on_ams_op2_event(); //frame lfo
        sigc::connection slot_ams_op2;
        void on_freq_mode_op2_event();
        sigc::connection slot_freq_mode_op2;
        void on_freq_coarse_op2_event();
        sigc::connection slot_freq_coarse_op2;
        void on_freq_fine_op2_event();
        sigc::connection slot_freq_fine_op2;
        void on_dtun_op2_event();
        sigc::connection slot_dtun_op2;
        /* op2 EG*/
        void on_eg_rt1_op2_event();
        sigc::connection slot_eg_rt1_op2;
        void on_eg_rt2_op2_event();
        sigc::connection slot_eg_rt2_op2;
        void on_eg_rt3_op2_event();
        sigc::connection slot_eg_rt3_op2;
        void on_eg_rt4_op2_event();
        sigc::connection slot_eg_rt4_op2;
        void on_eg_lvl1_op2_event();
        sigc::connection slot_eg_lvl1_op2;
        void on_eg_lvl2_op2_event();
        sigc::connection slot_eg_lvl2_op2;
        void on_eg_lvl3_op2_event();
        sigc::connection slot_eg_lvl3_op2;
        void on_eg_lvl4_op2_event();
        sigc::connection slot_eg_lvl4_op2;
        /* op2 VOLUME*/
        void on_krs_op2_event();
        sigc::connection slot_krs_op2;
        void on_kvs_op2_event();
        sigc::connection slot_kvs_op2;
        void on_lvl_op2_event();
        sigc::connection slot_lvl_op2;
        /* op2 KLS*/
        void on_kls_lft_curve_op2_event();
        sigc::connection slot_kls_lft_curve_op2;
        void on_kls_rght_curve_op2_event();
        sigc::connection slot_kls_rght_curve_op2;
        void on_kls_lft_dpth_op2_event();
        sigc::connection slot_kls_lft_depth_op2;
        void on_kls_rght_dpth_op2_event();
        sigc::connection slot_kls_rght_depth_op2;
        void on_kls_brk_pt_op2_event();
        sigc::connection slot_kls_note_brk_pt_op2;
        sigc::connection slot_kls_octv_brk_pt_op2;

        /* OP3 */
        void on_ams_op3_event(); //frame lfo
        sigc::connection slot_ams_op3;
        void on_freq_mode_op3_event();
        sigc::connection slot_freq_mode_op3;
        void on_freq_coarse_op3_event();
        sigc::connection slot_freq_coarse_op3;
        void on_freq_fine_op3_event();
        sigc::connection slot_freq_fine_op3;
        void on_dtun_op3_event();
        sigc::connection slot_dtun_op3;
        /* op3 EG*/
        void on_eg_rt1_op3_event();
        sigc::connection slot_eg_rt1_op3;
        void on_eg_rt2_op3_event();
        sigc::connection slot_eg_rt2_op3;
        void on_eg_rt3_op3_event();
        sigc::connection slot_eg_rt3_op3;
        void on_eg_rt4_op3_event();
        sigc::connection slot_eg_rt4_op3;
        void on_eg_lvl1_op3_event();
        sigc::connection slot_eg_lvl1_op3;
        void on_eg_lvl2_op3_event();
        sigc::connection slot_eg_lvl2_op3;
        void on_eg_lvl3_op3_event();
        sigc::connection slot_eg_lvl3_op3;
        void on_eg_lvl4_op3_event();
        sigc::connection slot_eg_lvl4_op3;
        /* op3 VOLUME*/
        void on_krs_op3_event();
        sigc::connection slot_krs_op3;
        void on_kvs_op3_event();
        sigc::connection slot_kvs_op3;
        void on_lvl_op3_event();
        sigc::connection slot_lvl_op3;
        /* op3 KLS*/
        void on_kls_lft_curve_op3_event();
        sigc::connection slot_kls_lft_curve_op3;
        void on_kls_rght_curve_op3_event();
        sigc::connection slot_kls_rght_curve_op3;
        void on_kls_lft_dpth_op3_event();
        sigc::connection slot_kls_lft_depth_op3;
        void on_kls_rght_dpth_op3_event();
        sigc::connection slot_kls_rght_depth_op3;
        void on_kls_brk_pt_op3_event();
        sigc::connection slot_kls_note_brk_pt_op3;
        sigc::connection slot_kls_octv_brk_pt_op3;

        /* OP4 */
        void on_ams_op4_event(); //frame lfo
        sigc::connection slot_ams_op4;
        void on_freq_mode_op4_event();
        sigc::connection slot_freq_mode_op4;
        void on_freq_coarse_op4_event();
        sigc::connection slot_freq_coarse_op4;
        void on_freq_fine_op4_event();
        sigc::connection slot_freq_fine_op4;
        void on_dtun_op4_event();
        sigc::connection slot_dtun_op4;
        /* op4 EG*/
        void on_eg_rt1_op4_event();
        sigc::connection slot_eg_rt1_op4;
        void on_eg_rt2_op4_event();
        sigc::connection slot_eg_rt2_op4;
        void on_eg_rt3_op4_event();
        sigc::connection slot_eg_rt3_op4;
        void on_eg_rt4_op4_event();
        sigc::connection slot_eg_rt4_op4;
        void on_eg_lvl1_op4_event();
        sigc::connection slot_eg_lvl1_op4;
        void on_eg_lvl2_op4_event();
        sigc::connection slot_eg_lvl2_op4;
        void on_eg_lvl3_op4_event();
        sigc::connection slot_eg_lvl3_op4;
        void on_eg_lvl4_op4_event();
        sigc::connection slot_eg_lvl4_op4;
        /* op4 VOLUME*/
        void on_krs_op4_event();
        sigc::connection slot_krs_op4;
        void on_kvs_op4_event();
        sigc::connection slot_kvs_op4;
        void on_lvl_op4_event();
        sigc::connection slot_lvl_op4;
        /* op4 KLS*/
        void on_kls_lft_curve_op4_event();
        sigc::connection slot_kls_lft_curve_op4;
        void on_kls_rght_curve_op4_event();
        sigc::connection slot_kls_rght_curve_op4;
        void on_kls_lft_dpth_op4_event();
        sigc::connection slot_kls_lft_depth_op4;
        void on_kls_rght_dpth_op4_event();
        sigc::connection slot_kls_rght_depth_op4;
        void on_kls_brk_pt_op4_event();
        sigc::connection slot_kls_note_brk_pt_op4;
        sigc::connection slot_kls_octv_brk_pt_op4;

        /* OP5 */
        void on_ams_op5_event(); //frame lfo
        sigc::connection slot_ams_op5;
        void on_freq_mode_op5_event();
        sigc::connection slot_freq_mode_op5;
        void on_freq_coarse_op5_event();
        sigc::connection slot_freq_coarse_op5;
        void on_freq_fine_op5_event();
        sigc::connection slot_freq_fine_op5;
        void on_dtun_op5_event();
        sigc::connection slot_dtun_op5;
        /* op5 EG*/
        void on_eg_rt1_op5_event();
        sigc::connection slot_eg_rt1_op5;
        void on_eg_rt2_op5_event();
        sigc::connection slot_eg_rt2_op5;
        void on_eg_rt3_op5_event();
        sigc::connection slot_eg_rt3_op5;
        void on_eg_rt4_op5_event();
        sigc::connection slot_eg_rt4_op5;
        void on_eg_lvl1_op5_event();
        sigc::connection slot_eg_lvl1_op5;
        void on_eg_lvl2_op5_event();
        sigc::connection slot_eg_lvl2_op5;
        void on_eg_lvl3_op5_event();
        sigc::connection slot_eg_lvl3_op5;
        void on_eg_lvl4_op5_event();
        sigc::connection slot_eg_lvl4_op5;
        /* op5 VOLUME*/
        void on_krs_op5_event();
        sigc::connection slot_krs_op5;
        void on_kvs_op5_event();
        sigc::connection slot_kvs_op5;
        void on_lvl_op5_event();
        sigc::connection slot_lvl_op5;
        /* op5 KLS*/
        void on_kls_lft_curve_op5_event();
        sigc::connection slot_kls_lft_curve_op5;
        void on_kls_rght_curve_op5_event();
        sigc::connection slot_kls_rght_curve_op5;
        void on_kls_lft_dpth_op5_event();
        sigc::connection slot_kls_lft_depth_op5;
        void on_kls_rght_dpth_op5_event();
        sigc::connection slot_kls_rght_depth_op5;
        void on_kls_brk_pt_op5_event();
        sigc::connection slot_kls_note_brk_pt_op5;
        sigc::connection slot_kls_octv_brk_pt_op5;

        /* OP6 */
        void on_ams_op6_event(); //frame lfo
        sigc::connection slot_ams_op6;
        void on_freq_mode_op6_event();
        sigc::connection slot_freq_mode_op6;
        void on_freq_coarse_op6_event();
        sigc::connection slot_freq_coarse_op6;
        void on_freq_fine_op6_event();
        sigc::connection slot_freq_fine_op6;
        void on_dtun_op6_event();
        sigc::connection slot_dtun_op6;
        /* op6 EG*/
        void on_eg_rt1_op6_event();
        sigc::connection slot_eg_rt1_op6;
        void on_eg_rt2_op6_event();
        sigc::connection slot_eg_rt2_op6;
        void on_eg_rt3_op6_event();
        sigc::connection slot_eg_rt3_op6;
        void on_eg_rt4_op6_event();
        sigc::connection slot_eg_rt4_op6;
        void on_eg_lvl1_op6_event();
        sigc::connection slot_eg_lvl1_op6;
        void on_eg_lvl2_op6_event();
        sigc::connection slot_eg_lvl2_op6;
        void on_eg_lvl3_op6_event();
        sigc::connection slot_eg_lvl3_op6;
        void on_eg_lvl4_op6_event();
        sigc::connection slot_eg_lvl4_op6;
        /* op6 VOLUME*/
        void on_krs_op6_event();
        sigc::connection slot_krs_op6;
        void on_kvs_op6_event();
        sigc::connection slot_kvs_op6;
        void on_lvl_op6_event();
        sigc::connection slot_lvl_op6;
        /* op6 KLS*/
        void on_kls_lft_curve_op6_event();
        sigc::connection slot_kls_lft_curve_op6;
        void on_kls_rght_curve_op6_event();
        sigc::connection slot_kls_rght_curve_op6;
        void on_kls_lft_dpth_op6_event();
        sigc::connection slot_kls_lft_depth_op6;
        void on_kls_rght_dpth_op6_event();
        sigc::connection slot_kls_rght_depth_op6;
        void on_kls_brk_pt_op6_event();
        sigc::connection slot_kls_note_brk_pt_op6;
        sigc::connection slot_kls_octv_brk_pt_op6;

        void set_aftrtch_assgn_event(int);
        void set_aftrtch_rng_event(int);
        void set_algo_event(int);
        void set_ams_op1_event(int);
        void set_ams_op2_event(int);
        void set_ams_op3_event(int);
        void set_ams_op4_event(int);
        void set_ams_op5_event(int);
        void set_ams_op6_event(int);
        void set_brth_assgn_event(int);
        void set_brth_rng_event(int);
        void set_compare_event(int);
        void set_dtun_op1_event(int);
        void set_dtun_op2_event(int);
        void set_dtun_op3_event(int);
        void set_dtun_op4_event(int);
        void set_dtun_op5_event(int);
        void set_dtun_op6_event(int);
        void set_eg_lvl1_op1_event(int);
        void set_eg_lvl1_op2_event(int);
        void set_eg_lvl1_op3_event(int);
        void set_eg_lvl1_op4_event(int);
        void set_eg_lvl1_op5_event(int);
        void set_eg_lvl1_op6_event(int);
        void set_eg_lvl2_op1_event(int);
        void set_eg_lvl2_op2_event(int);
        void set_eg_lvl2_op3_event(int);
        void set_eg_lvl2_op4_event(int);
        void set_eg_lvl2_op5_event(int);
        void set_eg_lvl2_op6_event(int);
        void set_eg_lvl3_op1_event(int);
        void set_eg_lvl3_op2_event(int);
        void set_eg_lvl3_op3_event(int);
        void set_eg_lvl3_op4_event(int);
        void set_eg_lvl3_op5_event(int);
        void set_eg_lvl3_op6_event(int);
        void set_eg_lvl4_op1_event(int);
        void set_eg_lvl4_op2_event(int);
        void set_eg_lvl4_op3_event(int);
        void set_eg_lvl4_op4_event(int);
        void set_eg_lvl4_op5_event(int);
        void set_eg_lvl4_op6_event(int);
        void set_eg_rt1_op1_event(int);
        void set_eg_rt1_op2_event(int);
        void set_eg_rt1_op3_event(int);
        void set_eg_rt1_op4_event(int);
        void set_eg_rt1_op5_event(int);
        void set_eg_rt1_op6_event(int);
        void set_eg_rt2_op1_event(int);
        void set_eg_rt2_op2_event(int);
        void set_eg_rt2_op3_event(int);
        void set_eg_rt2_op4_event(int);
        void set_eg_rt2_op5_event(int);
        void set_eg_rt2_op6_event(int);
        void set_eg_rt3_op1_event(int);
        void set_eg_rt3_op2_event(int);
        void set_eg_rt3_op3_event(int);
        void set_eg_rt3_op4_event(int);
        void set_eg_rt3_op5_event(int);
        void set_eg_rt3_op6_event(int);
        void set_eg_rt4_op1_event(int);
        void set_eg_rt4_op2_event(int);
        void set_eg_rt4_op3_event(int);
        void set_eg_rt4_op4_event(int);
        void set_eg_rt4_op5_event(int);
        void set_eg_rt4_op6_event(int);
        void set_feedback_event(int);
        void set_foot_assgn_event(int);
        void set_foot_rng_event(int);
        void set_freq_coarse_op1_event(int);
        void set_freq_coarse_op2_event(int);
        void set_freq_coarse_op3_event(int);
        void set_freq_coarse_op4_event(int);
        void set_freq_coarse_op5_event(int);
        void set_freq_coarse_op6_event(int);
        void set_freq_fine_op1_event(int);
        void set_freq_fine_op2_event(int);
        void set_freq_fine_op3_event(int);
        void set_freq_fine_op4_event(int);
        void set_freq_fine_op5_event(int);
        void set_freq_fine_op6_event(int);
        void set_freq_mode_op1_event(int);
        void set_freq_mode_op2_event(int);
        void set_freq_mode_op3_event(int);
        void set_freq_mode_op4_event(int);
        void set_freq_mode_op5_event(int);
        void set_freq_mode_op6_event(int);
        void set_kls_brk_pt_op1_event(int);
        void set_kls_brk_pt_op2_event(int);
        void set_kls_brk_pt_op3_event(int);
        void set_kls_brk_pt_op4_event(int);
        void set_kls_brk_pt_op5_event(int);
        void set_kls_brk_pt_op6_event(int);
        void set_kls_lft_curve_op1_event(int);
        void set_kls_lft_curve_op2_event(int);
        void set_kls_lft_curve_op3_event(int);
        void set_kls_lft_curve_op4_event(int);
        void set_kls_lft_curve_op5_event(int);
        void set_kls_lft_curve_op6_event(int);
        void set_kls_lft_dpth_op1_event(int);
        void set_kls_lft_dpth_op2_event(int);
        void set_kls_lft_dpth_op3_event(int);
        void set_kls_lft_dpth_op4_event(int);
        void set_kls_lft_dpth_op5_event(int);
        void set_kls_lft_dpth_op6_event(int);
        void set_kls_rght_curve_op1_event(int);
        void set_kls_rght_curve_op2_event(int);
        void set_kls_rght_curve_op3_event(int);
        void set_kls_rght_curve_op4_event(int);
        void set_kls_rght_curve_op5_event(int);
        void set_kls_rght_curve_op6_event(int);
        void set_kls_rght_dpth_op1_event(int);
        void set_kls_rght_dpth_op2_event(int);
        void set_kls_rght_dpth_op3_event(int);
        void set_kls_rght_dpth_op4_event(int);
        void set_kls_rght_dpth_op5_event(int);
        void set_kls_rght_dpth_op6_event(int);
        void set_krs_op1_event(int);
        void set_krs_op2_event(int);
        void set_krs_op3_event(int);
        void set_krs_op4_event(int);
        void set_krs_op5_event(int);
        void set_krs_op6_event(int);
        void set_kvs_op1_event(int);
        void set_kvs_op2_event(int);
        void set_kvs_op3_event(int);
        void set_kvs_op4_event(int);
        void set_kvs_op5_event(int);
        void set_kvs_op6_event(int);
        void set_lfo_amd_event(int);
        void set_lfo_delay_event(int);
        void set_lfo_pmd_event(int);
        void set_lfo_speed_event(int);
        void set_lfo_sync_event(int);
        void set_lfo_wav_event(int);
        void set_lvl_op1_event(int);
        void set_lvl_op2_event(int);
        void set_lvl_op3_event(int);
        void set_lvl_op4_event(int);
        void set_lvl_op5_event(int);
        void set_lvl_op6_event(int);
        void set_md_whl_assgn_event(int);
        void set_md_whl_rng_event(int);
        void set_mono_poly_event(int);
        void set_mute_op1_event(int);
        void set_mute_op2_event(int);
        void set_mute_op3_event(int);
        void set_mute_op4_event(int);
        void set_mute_op5_event(int);
        void set_mute_op6_event(int);
        void set_mute_op_event(int);
        void set_oks_event(int);
        void set_panic_event(int);
        void set_pitch_lvl1_event(int);
        void set_pitch_lvl2_event(int);
        void set_pitch_lvl3_event(int);
        void set_pitch_lvl4_event(int);
        void set_pitch_rt1_event(int);
        void set_pitch_rt2_event(int);
        void set_pitch_rt3_event(int);
        void set_pitch_rt4_event(int);
        void set_pms_event(int);
        void set_portamento_glss_event(int);
        void set_portamento_md_event(int);
        void set_portamento_tm_event(int);
        void set_ptch_bnd_rng_event(int);
        void set_ptch_bnd_stp_event(int);
        void set_send_extra_parameters_event(int);
        void set_transpose_event(int);
};
