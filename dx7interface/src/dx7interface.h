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
#include <variant>
#include <gxinterface/0.0.1/common.h>
#include <gxinterface/0.0.1/gxmodule.h>
//#include "../gxinterface/src/common.h"
//#include "../gxinterface/src/gxmodule.h"
#include <filesystem>
#include "GtkClass.h"
/* Synth */
#include "synth.h"
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
    std::tuple<std::shared_ptr<void>, St_mod_options> LoadPlug(uint8_t);
};

class Dx7interface : public Gx_module, public Synth {
    public:
        Dx7interface(Glib::ustring,uint8_t);
        virtual ~Dx7interface();
        void add_action();

    private:
        /**** Generic ****/
        bool error();
        std::shared_ptr<int> pending_remove = std::make_shared<int>(0); // counter storing box remove pending task

        using FunctionPtr = void (Dx7interface::*)();                             /* abstract for function without parameters */
        using FunctionPtrInt = void (Dx7interface::*)(int);                       /* abstract for function with int parameter */
        using FunctionPtrFile = void (Dx7interface::*)(Glib::RefPtr<Gio::File>);  /* abstract for function with file parameter */
        using FunctionPtr3str = void (Dx7interface::*)(Glib::ustring,Glib::ustring,unsigned int);  /* abstract for function with 3 glib::ustring parameters */

        #ifdef __linux__ 
            /*** ALSA MIDI ***/
            snd_seq_t* seq_handle = nullptr;                /* handler */
            snd_seq_system_info_t* info = nullptr;          /* info */
            snd_seq_event_t* ev = nullptr;                  /* evenement */
            size_t in_buff_size, out_buff_size;             /* buffer d'entré et de sortie */
            std::vector<uint8_t> sysex_buffer;              // Buffer to store SysEx fragments
        #endif
        /* Boolean */
        bool lock = false;
        bool compare = false;                   // set if compare button is activate
        bool send_extra_params = false;         // set if send_extra paraameter is activate
        bool write_extra_params = false;        // set if send_extra paraameter is activate
        bool mode_tf1 = false;                  // mode tf1 = fonction parameter by sound
        bool receive = false;                   // set if receive mode is activate
        bool uncomplete = false;                // bool for uncomplete sysex message

        /*** Dx7 specific ***/
        static const uint8_t id_fabricant=0x43; /* static fix yamaha id */

        /* SySeX format (bank/sound/message) */
        St_dx7sysex<1> bank_1_origin;           /* bank d'origine 1 son */
        St_dx7sysex<1> bank_1_modif;            /* bank modifié 1 son */
        St_dx7sysex<32> bank_32_origin;         /* bank d'origine 32 sons */
        St_dx7sysex<32> bank_32_modif;          /* ... */
        St_dx7sysex<128> bank_128_origin;       /* ... */
        St_dx7sysex<128> bank_128_modif;        /* ... */

        using BankVariant = std::variant<
        std::reference_wrapper<St_dx7sysex<1>>,
        std::reference_wrapper<St_dx7sysex<32>>,
        std::reference_wrapper<St_dx7sysex<128>>>;

        /* default write format */
        unsigned int export_config = DX7_32;     // DX7_1 DX7_32 DX7_128 DX7_RAW DX7_SYX
        unsigned int save_type = BANK;          // BANK or SOUND

        /* Bank */
        unsigned int bank_nb_sound = 0;                            // number of sound in the current loaded bank 1/32/128
        unsigned int snum = 0;                                 // selected sound number memo for set_original_sound
        unsigned int old_snum = 0;                                 // Previous selected sound number memo for set_original_sound
        Glib::RefPtr<Gio::File> bank_file=nullptr;                  // pointeur de lecture de fichier
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

        template<class ListStoreType>
        void update_data_model(Glib::RefPtr<Gio::ListStore<ListStoreType>> data_model, Glib::ustring sound_name);
        template<class ListStoreType>
        void update_data_model_full(Glib::RefPtr<Gio::ListStore<ListStoreType>> list_data_model, BankVariant& bank_modif_dest);
        template<class ListStoreType>
        void update_param_data_model(Glib::RefPtr<Gio::ListStore<ListStoreType>> data_model, unsigned int i, Glib::ustring name);

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
            "Aftertouch Assign",
            "Aftertouch Range",
            "Algorithms",
            "Amp Modulation Sens OP1",
            "Amp Modulation Sens OP2",
            "Amp Modulation Sens OP3",
            "Amp Modulation Sens OP4",
            "Amp Modulation Sens OP5",
            "Amp Modulation Sens OP6",
            "Breath Controller Assign",
            "Breath Controller Range",
            "Compare",
            "Detune OP1",
            "Detune OP2",
            "Detune OP3",
            "Detune OP4",
            "Detune OP5",
            "Detune OP6",
            "EG Level1 OP1",
            "EG Level1 OP2",
            "EG Level1 OP3",
            "EG Level1 OP4",
            "EG Level1 OP5",
            "EG Level1 OP6",
            "EG Level2 OP1",
            "EG Level2 OP2",
            "EG Level2 OP3",
            "EG Level2 OP4",
            "EG Level2 OP5",
            "EG Level2 OP6",
            "EG Level3 OP1",
            "EG Level3 OP2",
            "EG Level3 OP3",
            "EG Level3 OP4",
            "EG Level3 OP5",
            "EG Level3 OP6",
            "EG Level4 OP1",
            "EG Level4 OP2",
            "EG Level4 OP3",
            "EG Level4 OP4",
            "EG Level4 OP5",
            "EG Level4 OP6",
            "EG Rate1 OP1",
            "EG Rate1 OP2",
            "EG Rate1 OP3",
            "EG Rate1 OP4",
            "EG Rate1 OP5",
            "EG Rate1 OP6",
            "EG Rate2 OP1",
            "EG Rate2 OP2",
            "EG Rate2 OP3",
            "EG Rate2 OP4",
            "EG Rate2 OP5",
            "EG Rate2 OP6",
            "EG Rate3 OP1",
            "EG Rate3 OP2",
            "EG Rate3 OP3",
            "EG Rate3 OP4",
            "EG Rate3 OP5",
            "EG Rate3 OP6",
            "EG Rate4 OP1",
            "EG Rate4 OP2",
            "EG Rate4 OP3",
            "EG Rate4 OP4",
            "EG Rate4 OP5",
            "EG Rate4 OP6",
            "Feedback",
            "Foot Controller Assign",
            "Foot Controller Range",
            "Frequency Coarse OP1",
            "Frequency Coarse OP2",
            "Frequency Coarse OP3",
            "Frequency Coarse OP4",
            "Frequency Coarse OP5",
            "Frequency Coarse OP6",
            "Frequency Fine OP1",
            "Frequency Fine OP2",
            "Frequency Fine OP3",
            "Frequency Fine OP4",
            "Frequency Fine OP5",
            "Frequency Fine OP6",
            "Frequency Mode OP1",
            "Frequency Mode OP2",
            "Frequency Mode OP3",
            "Frequency Mode OP4",
            "Frequency Mode OP5",
            "Frequency Mode OP6",
            "KLS Break Point OP1",
            "KLS Break Point OP2",
            "KLS Break Point OP3",
            "KLS Break Point OP4",
            "KLS Break Point OP5",
            "KLS Break Point OP6",
            "KLS Left Curve OP1",
            "KLS Left Curve OP2",
            "KLS Left Curve OP3",
            "KLS Left Curve OP4",
            "KLS Left Curve OP5",
            "KLS Left Curve OP6",
            "KLS Left Depht OP1",
            "KLS Left Depht OP2",
            "KLS Left Depht OP3",
            "KLS Left Depht OP4",
            "KLS Left Depht OP5",
            "KLS Left Depht OP6",
            "KLS Right Curve OP1",
            "KLS Right Curve OP2",
            "KLS Right Curve OP3",
            "KLS Right Curve OP4",
            "KLS Right Curve OP5",
            "KLS Right Curve OP6",
            "KLS Right Depht OP1",
            "KLS Right Depht OP2",
            "KLS Right Depht OP3",
            "KLS Right Depht OP4",
            "KLS Right Depht OP5",
            "KLS Right Depht OP6",
            "Keyboard Rate Scaling OP1",
            "Keyboard Rate Scaling OP2",
            "Keyboard Rate Scaling OP3",
            "Keyboard Rate Scaling OP4",
            "Keyboard Rate Scaling OP5",
            "Keyboard Rate Scaling OP6",
            "Key Velocity Sens OP1",
            "Key Velocity Sens OP2",
            "Key Velocity Sens OP3",
            "Key Velocity Sens OP4",
            "Key Velocity Sens OP5",
            "Key Velocity Sens OP6",
            "LFO Amp Modulation Depht",
            "LFO Delay",
            "LFO Pitch Modulation Depht",
            "LFO Speed",
            "LFO Synchronize",
            "LFO Wave",
            "Level OP1",
            "Level OP2",
            "Level OP3",
            "Level OP4",
            "Level OP5",
            "Level OP6",
            "Modulation Wheel Assign",
            "Modulation Wheel Range",
            "Poly/Mono",
            "Mute OP1",
            "Mute OP2",
            "Mute OP3",
            "Mute OP4",
            "Mute OP5",
            "Mute OP6",
            "Oscillator Key Sync",
            "Panic",
            "Pitch EG Level1",
            "Pitch EG Level2",
            "Pitch EG Level3",
            "Pitch EG Level4",
            "Pitch EG Rate1",
            "Pitch EG Rate2",
            "Pitch EG Rate3",
            "Pitch EG Rate4",
            "Pitch Modulation Sens",
            "Portamento Gliss",
            "Portamento Mode",
            "Portamento Time",
            "Pitch Bend Range",
            "Pitch Bend Step",
            "Send Extra Parameters",
            "Transpose"
        };

        void create_param_list();
        Glib::RefPtr<Gio::ListStore<ParamItem>> param_data_model=nullptr; /* liste des nom des sons de la banque chargé */
        Glib::RefPtr<Gtk::SingleSelection> param_selection_model=nullptr;
        Glib::RefPtr<Gtk::SignalListItemFactory> param_factory=nullptr;
        void on_bind_param_name(const Glib::RefPtr<Gtk::ListItem>&);
        void on_setup_param_label(const Glib::RefPtr<Gtk::ListItem>&, Gtk::Align);

        FunctionPtrInt list_ui_parameters_functions[max_param_nb] = {
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
        void create_dialogs();
        Gtk::Window* dialog_insert = nullptr;
        Gtk::Button* button_insert = nullptr;
        Gtk::Window* dialog_save = nullptr;
        Gtk::Button* button_save = nullptr;

        Gtk::CheckButton* checkbutton_bulk = nullptr;
        #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
            Gtk::FileDialog* file_dialog_select = nullptr;
            Gtk::FileDialog* file_dialog_save = nullptr;
            Gtk::FileDialog* file_dialog_param_select = nullptr;
            Gtk::FileDialog* file_dialog_param_save = nullptr;
        #else
            Gtk::FileChooserDialog* file_dialog_select = nullptr;
            Gtk::FileChooserDialog* file_dialog_save = nullptr;
            Gtk::FileChooserDialog* file_dialog_param_select = nullptr;
            Gtk::FileChooserDialog* file_dialog_param_save = nullptr;
            Gtk::Button* button_accept = nullptr;
        #endif

        void OpenFileSaveDialog();
        void OpenDialogSave(Glib::ustring,Glib::ustring);
        void OpenFileInsertDialog(Glib::ustring,Glib::ustring);

        /*** THREAD ***/
        bool Run();    /* Thread function  */
        bool Run2();    /* Thread function  */
        /*** MIDI ***/
        #ifdef __linux__
                void listen_midi() override;
        #endif
        #if (defined(__WIN32) || defined(__MINGW32__) && defined(__RtMidi__))
                void listen_midi(double timestamp, std::vector<unsigned char>* _message, void* userData) override;
        #endif
        /** SOUND BANK **/
        /* set/load */
        void set_default_values();
        void select_voice(unsigned int);
        void clean_bank();  // read reset1.syx reset32.syx reset128.syx (empty file 0x00 of specified number of voice)
        void set_bank(Glib::RefPtr<Gio::File>);
        void set_bank_sounds(Glib::ustring, Glib::ustring, unsigned int);
        void load_file(Glib::RefPtr<Gio::File>,FunctionPtr3str);
        void receive_bank(std::vector<uint8_t>);
        void receive_voice(St_dx7sysex_1*, std::vector<uint8_t>);     // get voice param from midi message to fill sound struct
        void receive_voice_by_byte(St_dx7sysex_1*, std::vector<uint8_t>);     // get voice param from midi message to fill sound struct
        void receive_paramters(St_dx7sysex_1*, std::vector<uint8_t>);
        /* update */
        void update_bank_modif();
        /* restore */
        void restore_origin(unsigned int);
        void on_restore_bank();
        void on_restore_sound();
        /* repalce/delete */
        void on_insert_at();                                            // Open dialog insert at position
        void on_insert_sound(Glib::RefPtr<Gio::File>);                  // call load_file->insert_at
        void insert_at(Glib::ustring, Glib::ustring, unsigned int);     // do the insert

        /* Prepare bank
         * to populate it on "insert at" call: get_bank_... , copy, moove, read_voice
         */
        void prepare_bank(unsigned int, unsigned int, bool);

        std::pair<
            Dx7interface::BankVariant,
            Dx7interface::BankVariant> get_banks_source();
        std::tuple<unsigned int,std::pair<Dx7interface::BankVariant,
            Dx7interface::BankVariant>> get_banks_dest(unsigned int);
        void copy_bank(BankVariant&, BankVariant&, BankVariant&, BankVariant&, unsigned int, unsigned int);
        void moove_sound(BankVariant&, BankVariant&, unsigned int, unsigned int);
        void read_voice(BankVariant&, BankVariant&, unsigned int, bool); // call seek_voice or seek_voice_by_byte

        /* REPLACE */
        void replace_sound(Glib::ustring, Glib::ustring, unsigned int);
        void on_replace_sound(Glib::RefPtr<Gio::File> file);
        void on_delete_sound();
        /* save/write */
        void write_file(Glib::RefPtr<Gio::File>, unsigned char*, unsigned int);
        void write_voice_extra_parameters(st_dx7sysex_1*, unsigned char*, unsigned int*);
        /* BANK */
        void on_save_bank();
        void write_bank(Glib::RefPtr<Gio::File>, unsigned int);
        void write_bank_as_sysex(Glib::RefPtr<Gio::File>, unsigned int);
        void write_bank_as_raw(Glib::RefPtr<Gio::File> file, unsigned int);
        void on_as_raw_event();
        void on_extra_param_event();
        /* VOICE */
        void write_voice_bulk1(unsigned int*, unsigned char*, St_dx7sysex_1*, uint8_t*);
        void write_voice_bulk32(unsigned int*, unsigned char*, St_dx7sysex_1*, uint8_t*);
        void write_voice_as_sysex(Glib::RefPtr<Gio::File>);
        void write_voice_as_raw(Glib::RefPtr<Gio::File>);
        void save_modif_sound();    /* save internally on origin bank */
        void on_save_sound();       /* save internally and write file */
        void write_voices_as_n_sysex(St_dx7sysex_1*);
        /* */
        void save_bank_as(Glib::RefPtr<Gio::File>);
        void clear_sound(St_dx7sysex_1*,uint8_t,bool);     // set 0x00 to all param to voice struct "aka clear struct"
        void set_as_origin_sound(unsigned int);                // set bank_X_modif.sound as bank_X_origin.sound

        /** VOICE  **/
        /* seek voice value from bank file and write it to sound */
        void seek_voice(st_dx7sysex_1*);     // get voice param from file to fill sound struct
        void seek_voice_by_byte(st_dx7sysex_1*);     // get voice param from file to fill sound struct
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
        FunctionPtr mute_hexter_functions[6] = {
            &Dx7interface::on_mute_hexter_op1_event,
            &Dx7interface::on_mute_hexter_op2_event,
            &Dx7interface::on_mute_hexter_op3_event,
            &Dx7interface::on_mute_hexter_op4_event,
            &Dx7interface::on_mute_hexter_op5_event,
            &Dx7interface::on_mute_hexter_op6_event
        };
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
        void on_selected_sound_change(unsigned int,unsigned int);
        sigc::connection slot_selected_sound_change;
        void on_file_select(FunctionPtrFile);
        sigc::connection slot_bank_select;
        /* populate columnview */
        void on_bind_num(const Glib::RefPtr<Gtk::ListItem>&);
        void on_bind_name(const std::shared_ptr<Gtk::ListItem>&);
        void on_setup_sound_number_label(const Glib::RefPtr<Gtk::ListItem>&, Gtk::Align);
        void on_setup_sound_name_label(const Glib::RefPtr<Gtk::ListItem>&, Gtk::Align);
        void on_sound_name_event();
        Glib::ustring check_sound_name(Glib::ustring);
        void set_sound_name(Glib::ustring);
        sigc::connection slot_sound_name_activate;
        sigc::connection slot_sound_name_change;
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
