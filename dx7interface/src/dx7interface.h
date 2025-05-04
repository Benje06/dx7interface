/* ----------------------------------------------------------------------------
 * Dx7interface.h -- DX7 Graphic interface
 * dx7 interface Headers                                             header
 * ----------------------------------------------------------------------------
 * copyright © 2006-2025  Jérôme BENHAÏM <benhaimjerome@gmail.com>,
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
*/
#pragma once
#define MODULE_NAME "Dx7interface"
/* define for export type */
#define DX7_1 1
#define DX7_32 2
#define DX7_128 3
#define DX7_RAW 4
#define DX7_SYX 5
/* sys */
//#include <memory>
/*** APP ***/
#define _USE_MATH_DEFINES
#include <cmath>
//#include <numbers> std=c++20 std::numbers::pi
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
//#include <gio/gio.h>

/** CONSTANTS **/
#define DATA_DIR PROGRAMNAME_DATA_DIR
#if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
        #define UI PROGRAMNAME_UI_DIR"dx7interface-0.0.1-simplify.ui"
#else
        #define UI PROGRAMNAME_UI_DIR"dx7interface-0.0.1-simplify_4.8.ui"
#endif
#define CSSFILE PROGRAMNAME_UI_DIR"theme.css"
#ifdef __linux__
    #define FONT "raster-fonts-6x8"
#endif
#if (defined(__WIN32) || defined(__MINGW32__))
    #define FONT "usr\\share\\fonts\\dx7interface\\Araster-fonts-6x8.ttf"
#endif
#define ICON ""

extern "C" {
    std::tuple<std::shared_ptr<void>, Gx_module::St_mod_options> LoadPlug(uint8_t);
};

class Dx7interface : public Gx_module, public Synth {
    public:
        Dx7interface(Glib::ustring ui, uint8_t index);
        virtual ~Dx7interface();
        //void add_action();

    private:
        /**** Generic ****/
        std::shared_ptr<int> pending_remove = std::make_shared<int>(0); // counter storing box remove pending task

        using FunctionPtr = void (Dx7interface::*)(bool);                             /* abstract for function without parameters */
        using FunctionPtrInt = void (Dx7interface::*)(int);                       /* abstract for function with int parameter */

        #ifdef __linux__ 
            /*** ALSA MIDI ***/
            snd_seq_t* seq_handle = nullptr;                /* handler */
            snd_seq_system_info_t* info = nullptr;          /* info */
            snd_seq_event_t* ev = nullptr;                  /* evenement */
            size_t in_buff_size, out_buff_size;             /* buffer d'entré et de sortie */
            std::vector<uint8_t> sysex_buffer;              // Buffer to store SysEx fragments
        #endif
        /* Boolean */
        bool send_extra_params = false;         // set if send_extra paraameter is activate
        bool write_extra_params = false;        // set if send_extra paraameter is activate
        bool unmooved_sound = true;
        bool send_bank = false;

        /*** Dx7 specific ***/
        bool mode_tf1 = false;                  // mode tf1 = fonction parameter by sound
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

        /* BANK LISTVIEW */
        void create_bank_voices_list();
        Glib::RefPtr<Gio::ListStore<SoundBankItem>> bank_data_model=nullptr; /* liste des nom des sons de la banque chargé */
        Glib::RefPtr<Gtk::SingleSelection> bank_selection_model=nullptr;
        Glib::RefPtr<Gtk::SignalListItemFactory> bank_factory=nullptr;
        /* update hte listview model */
        void update_modified();
        template<class ListStoreType>
        void update_data_model_number(Glib::RefPtr<Gio::ListStore<ListStoreType>> data_model,
                               Glib::ustring number);
        template<class ListStoreType>
        void update_data_model(Glib::RefPtr<Gio::ListStore<ListStoreType>> data_model,
                               Glib::ustring sound_name);
        template<class ListStoreType>
        void update_data_model_full(Glib::RefPtr<Gio::ListStore<ListStoreType>> data_model,
                                    BankVariant& bank_modif_dest);
        template<class ListStoreType>
        void update_param_data_model(Glib::RefPtr<Gio::ListStore<ListStoreType>> data_model,
                                     unsigned int index_element,
                                     Glib::ustring name);

        /*** MIDI LEARN ***/
        bool midi_learn=false;
        static const int max_param_nb = 168;
        std::vector<int> midi_param{std::vector<int>(max_param_nb, -1)};
        std::vector<std::vector<int>> midi_learned;

        /** READ/WRITE/CLEAN MIDI LEARN config file **/
        /* read */
        sigc::connection slot_midi_learn_load;
        void on_midi_learn_param_select();
        void read_midi_learned_param(unsigned int data_stream_index,
                                     Glib::RefPtr<Gio::File> file);
        /* write */
        sigc::connection slot_midi_learn_save;
        void on_midi_learn_param_save();
        void save_midi_learned_param(unsigned int data_stream_index,
                                     Glib::RefPtr<Gio::File> param_file);
        /* clean */
        void clean_midi_learn();

        /* EVENT MIDI LEARN */
        void on_midi_learn_event();
        void on_add_midi_learn_event();
        void add_midi_learned(int param_number,
                              int function_index);
        void rem_midi_learned(int param_number,
                              int function_index);
        void add_midi_learn_param_widget(Glib::ustring function_name,
                                         Glib::ustring param_number,
                                         int selected_item);
        /* CREATE MIDI LEARN LIST */
        void create_param_list();
        Glib::RefPtr<Gio::ListStore<ParamItem>> param_data_model=nullptr; /* liste des noms des sons de la banque chargée */
        Glib::RefPtr<Gtk::SingleSelection> param_selection_model=nullptr;
        Glib::RefPtr<Gtk::SignalListItemFactory> param_factory=nullptr;
        void on_bind_param_name(const Glib::RefPtr<Gtk::ListItem>&);
        void on_setup_param_label(const Glib::RefPtr<Gtk::ListItem>&, Gtk::Align);
        /* List of internal function that drive the UI */
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
        /* Translation for the User presentation */
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

        /*** POPHOVER MENU ***/
        void create_popover_menu();
        Glib::RefPtr<Gio::SimpleActionGroup> action_group=nullptr;
        Gtk::PopoverMenu* m_popover_menu = nullptr;

        /*** SAVE DIALOG ***/
        void create_dialogs();
        unsigned int action_type = ACT_OPEN;
        unsigned int save_index = 0;
        Gtk::CheckButton* checkbutton_bulk = nullptr;
        void set_param() override;
        void set_dialog(Glib::ustring title) override;
        void OpenDialogFileSelect(unsigned int data_stream_index,
                                  std::function<void(unsigned int data_stream_index,
                                                     Glib::RefPtr<Gio::File> file)>);
        void OpenDialogFileSave(unsigned int data_stream_index,
                                std::function<void(unsigned int data_stream_index,
                                                   Glib::RefPtr<Gio::File> file)>);
        void on_file_save(unsigned int data_stream_index,
                         Glib::RefPtr<Gio::File> file);

        /*** TODO: MOOVE THEM TO GX_MODULE ***/
        #if (GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 10)
        Gtk::FileDialog* file_dialog_param_select = nullptr;
        Gtk::FileDialog* file_dialog_param_save = nullptr;
        #else
        Gtk::FileChooserDialog* file_dialog_param_select = nullptr;
        Gtk::FileChooserDialog* file_dialog_param_save = nullptr;
        Gtk::Button* button_accept = nullptr;
        #endif
        Glib::RefPtr<Gio::File> initial_folder_open_param=nullptr;
        Glib::RefPtr<Gio::File> initial_folder_save_param=nullptr;
        Glib::RefPtr<Gio::DataInputStream> data_stream_param=nullptr;

        /*** THREAD ***/
        bool Run();    /* Thread function  */
        bool Run2();    /* Thread function  */
        /*** MIDI ***/
        #ifdef __linux__
                void listen_midi() override;
        #endif
        #if (defined(__WIN32) || defined(__MINGW32__) && defined(__RtMidi__))
                void listen_midi(double timestamp,
                                 std::vector<unsigned char>* message,
                                 void* userData) override;
        #endif
        /* FILE */
        // call Gx_Module function of the same name
        void read_file_as_datastream(unsigned int data_stream_index,
                                    Glib::RefPtr<Gio::File> file,
                                    std::function<void(unsigned int data_stream_index,
                                                       Glib::RefPtr<Gio::File> file)>);
       
        /**** SOUND BANK ****/
        /** SET/LOAD **/
        void set_default_values();
        void select_voice(unsigned int position);
        void clean_bank();  // read reset1.syx reset32.syx reset128.syx (empty file 0x00 of specified number of voice)
        void set_bank(unsigned int, Glib::RefPtr<Gio::File> file);
        void set_bank_name(Glib::ustring name);
        void set_bank_sounds(unsigned int datat_stream_index,
                             Glib::RefPtr<Gio::File> file);
        void receive_bank(std::vector<uint8_t> sysex_buffer);
        void receive_voice(St_dx7sysex_1* sound,
                           std::vector<uint8_t> sysex_buffer);          // get voice param from midi message to fill sound struct
        void receive_voice_by_byte(St_dx7sysex_1* sound,
                                   std::vector<uint8_t> sysex_buffer);  // get voice param from midi message to fill sound struct
        void receive_paramters(St_dx7sysex_1* sound,
                               std::vector<uint8_t> sysex_buffer);
        /** UPDATE **/
        void update_bank_modif();
        /** RESTORE **/
        void restore_origin(unsigned int type);
        void on_restore_bank();
        void on_restore_sound();
        /** REPLACE **/
        void replace_sound(unsigned int data_stream_index,
                           Glib::RefPtr<Gio::File> file);
        void on_replace_sound(unsigned int data_stream_index,
                              Glib::RefPtr<Gio::File> file);
        /** DELETE **/
        void on_delete_sound();
        /** INSERT AT **/
        void on_insert_at();                                                                    // Open dialog insert at position
        void on_insert_sound(unsigned int data_stream_index,
                             Glib::RefPtr<Gio::File> file);                                          // call load_file->insert_at
        void insert_at(unsigned int data_stream_index,
                       Glib::RefPtr<Gio::File> file);    // do the insert
        /** PREPARE BANK call all necessarry to insert at: get_bank_[src|dest] , copy, moove, read_voice **/
        void prepare_bank(unsigned int data_stream_index,
                          unsigned int nb_snd_in_file,
                          unsigned int pos_end_ins,
                          bool byte_flag);
        /* GET BANKS */
        /* source */
        std::pair<Dx7interface::BankVariant,
                  Dx7interface::BankVariant> get_banks_source();
        /* destination */
        std::tuple<unsigned int,
                   std::pair<Dx7interface::BankVariant,Dx7interface::BankVariant>> get_banks_dest(unsigned int pos_end_ins);
        /* COPY/MOOVE/READ */
        void copy_bank(BankVariant& bank_origin_src,
                       BankVariant& bank_origin_dest,
                       BankVariant& bank_modif_src,
                       BankVariant& bank_modif_dest,
                       unsigned int src_size,
                       unsigned int dst_size);
        void moove_sound(BankVariant& bank_origin_dest,
                         BankVariant& bank_modif_dest,
                         unsigned int end_insert,
                         int nb_snd);
        void read_voice(unsigned int data_stream_index,
                        BankVariant& bank_origin_dest,
                        BankVariant& bank_modif_dest,
                        unsigned int max_write_pos,
                        bool byte_flag); // call seek_voice or seek_voice_by_byte

        void set_init_voice_in_origin();                    // load init voice to bank_1_origin

        /**** SEND / SAVE / WRITE ****/
        /* SEND */
        void on_send_bank();                                // send bank over midi
        void send_voice(st_dx7sysex_1* sound);                    // send voice over midi
        void send_extra_parameters(st_dx7sysex_1* sound);         // send voice parameter over midi
        /* SAVE */
        void on_save_bank();
        void save_bank_as(Glib::RefPtr<Gio::File> file);
        void save_modif_sound();                            // save internally on origin bank
        void on_save_sound();                               // save internally and write file
        void on_as_raw_event();                             // event to manage save dialog parameter  ( save as raw or bulk )
        void on_extra_param_event();                        // event to manage save dialog parameter ( save extra parameters )
        /* WRITE */
        void write_bank(unsigned int data_stream_index,
                        Glib::RefPtr<Gio::File> file,
                        unsigned int index);
        void write_bank_as_sysex(unsigned int data_stream_index,
                                 Glib::RefPtr<Gio::File> file,
                                 unsigned int index);
        void write_bank_as_raw(unsigned int data_stream_index,
                               Glib::RefPtr<Gio::File> file,
                               unsigned int index);
        void write_voice_bulk1(unsigned int* msg_index,
                               unsigned char* msg,
                               St_dx7sysex_1* sound,
                               uint8_t* voice_checksum);
        void write_voice_bulk32(unsigned int* msg_index,
                                unsigned char* msg,
                                St_dx7sysex_1* sound,
                                uint8_t* voice_checksum);
        void write_voice_as_sysex(unsigned int data_stream_index,
                                  Glib::RefPtr<Gio::File> file);
        void write_voice_as_raw(unsigned int data_stream_index,
                                Glib::RefPtr<Gio::File> file);
        void write_voices_as_n_sysex(St_dx7sysex_1* sound);
        void write_voice_extra_parameters(st_dx7sysex_1* sound,
                                          unsigned char*,
                                          unsigned int*);
        /*** SET / SEEK VOICE ***/
        /* seek voice value from bank file and set it to sound */
        void seek_voice(Glib::RefPtr<Gio::DataInputStream> data_stream,
                        st_dx7sysex_1* sound);     // get voice param from file to fill sound struct
        void seek_voice_by_byte(Glib::RefPtr<Gio::DataInputStream> data_stream,
                                st_dx7sysex_1* sound);     // get voice param from file to fill sound struct
        void seek_parameters(Glib::ustring file_base,
                             St_dx7sysex_1* sound); // get sound parameter from file to fill sound extra param struct
        void seek_voice_parameters(St_dx7sysex_1* sound);
        /* set voice value from sound in bank to the interface */
        void set_voice(st_dx7sysex_1* sound);               // set voice in GUI
        void set_voice_parameters(St_dx7sysex_1* sound);    // set sound parameter

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
        int* get_cr_visible_size(const Cairo::RefPtr<Cairo::Context>& cr,
                                 Glib::ustring name);  /* retourne la taille de sla zone visible */
        /* ADSR */
        void redraw_all_curve();
        void draw_background(const Cairo::RefPtr<Cairo::Context>& cr);                                 /* dessine le fond */
        void draw_grid(const Cairo::RefPtr<Cairo::Context>& cr,
                       double width,
                       double height);                       /* dessisne la grille */
        void draw_adsr(const Cairo::RefPtr<Cairo::Context>& cr,
                       double width,
                       double height,
                       Glib::ustring name);        /* dessine la courbe */
        void draw_point(const Cairo::RefPtr<Cairo::Context>& cr,
                        double x,
                        double y,
                        double width,
                        bool orange);          /* dessine un point */
        void draw_note_off(const Cairo::RefPtr<Cairo::Context>& cr,
                          double x_noteoff,
                          double height);
        /* Level Scaling */
        void draw_kls(const Cairo::RefPtr<Cairo::Context>& cr,
                      double width,
                      double height,
                      Glib::ustring);         /* dessine la courbe */
        void draw_keyboard(const Cairo::RefPtr<Cairo::Context>& cr,
                           double width,
                           double height,
                           Glib::ustring num_op);    /*dessine le clavier */
        void draw_axis(const Cairo::RefPtr<Cairo::Context>& cr,
                       double width,
                       double height);                       /* dessine les axes */
        void draw_kls_curve(const Cairo::RefPtr<Cairo::Context>& cr,
                            Glib::ustring type_curve,
                            double width,
                            double height,
                            double dpth,
                            Glib::ustring dir,
                            double lvl); /* draw kls curve type */

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
        void mouse_click(int n_press,
                         double x,
                         double y,
                         Glib::ustring name);
        void mouse_click_release(int n_press,
                                 double x,
                                 double y,
                                 Glib::ustring name);
        /* Mouse mooves */
        Glib::RefPtr<Gtk::EventControllerMotion> controller_mouse_moove_op1;
        Glib::RefPtr<Gtk::EventControllerMotion> controller_mouse_moove_op2;
        Glib::RefPtr<Gtk::EventControllerMotion> controller_mouse_moove_op3;
        Glib::RefPtr<Gtk::EventControllerMotion> controller_mouse_moove_op4;
        Glib::RefPtr<Gtk::EventControllerMotion> controller_mouse_moove_op5;
        Glib::RefPtr<Gtk::EventControllerMotion> controller_mouse_moove_op6;
        Glib::RefPtr<Gtk::EventControllerMotion> controller_mouse_moove_pitch;
        void mouse_mooves(double x,
                          double y,
                          Glib::ustring name);

        /*** UI ***/
        FunctionPtr mute_by_level_functions[6] = {
            &Dx7interface::mute_op1_by_level,
            &Dx7interface::mute_op2_by_level,
            &Dx7interface::mute_op3_by_level,
            &Dx7interface::mute_op4_by_level,
            &Dx7interface::mute_op5_by_level,
            &Dx7interface::mute_op6_by_level
        };
        void on_mute_op1_event();
        sigc::connection slot_on_mute_op1;
        void on_mute_op2_event();
        sigc::connection slot_on_mute_op2;

        void on_checkbutton_mute_by_level_event();
        /** EVENTS / SIGNAL **/
        void block_ui();                       /* block all interface events */
        void unblock_ui();                     /* ... */
        void attach_signals() override;
        void dettach_signals() override;
        void attach_action_group_signals();

        /** Drawing **/
        void attach_drawarea_signals();
        void on_draw_algo(const Cairo::RefPtr<Cairo::Context>& cr,
                          double width,
                          double height);
        void on_draw_lfo(const Cairo::RefPtr<Cairo::Context>& cr,
                         double width,
                         double height);
        void on_draw_pitch_event(const Cairo::RefPtr<Cairo::Context>& cr,
                                 int width,
                                 int height);
        void on_draw_op_event(const Cairo::RefPtr<Cairo::Context>& cr,
                              int width,
                              int height,
                              Glib::ustring num_op);
        void on_draw_kls_event(const Cairo::RefPtr<Cairo::Context>& cr,
                               int width,
                               int height,
                               Glib::ustring num_op);

        /*** EVENTS and SIGC ::connection slot for blocking ***/
        /** BANK **/
        void on_bank_reveal();
        sigc::connection slot_bank_reveal;
        void on_columnview_right_click(int n_press,
                                       double x,
                                       double y);
        sigc::connection slot_columnview_right_click;
        void on_selected_sound_change(unsigned int num,
                                      unsigned int nb_elmnt);
        sigc::connection slot_selected_sound_change;
        sigc::connection slot_bank_select;
        sigc::connection slot_file_dialog_select;
        /* populate columnview */
        void on_bind_num(const Glib::RefPtr<Gtk::ListItem>& list_item);
        void on_bind_name(const std::shared_ptr<Gtk::ListItem>& list_item);
        void on_setup_sound_number_label(const Glib::RefPtr<Gtk::ListItem>& list_item, Gtk::Align);
        void on_setup_sound_name_label(const Glib::RefPtr<Gtk::ListItem>& list_item, Gtk::Align);
        void on_sound_name_event();
        Glib::ustring check_sound_name(Glib::ustring ssound_name); // check valid caracter for soundname entered in textbox
        void set_sound_name(Glib::ustring sound_name);
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
        void mute_op1_by_level(bool unmute);
        void mute_op2_by_level(bool unmute);
        void mute_op3_by_level(bool unmute);
        void mute_op4_by_level(bool unmute);
        void mute_op5_by_level(bool unmute);
        void mute_op6_by_level(bool unmute);
        void on_mute_op_by_level_event();
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
        void on_lvl_op1_event(bool mute_by_level);
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
        void on_lvl_op2_event(bool mute_by_level);
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
        void on_lvl_op3_event(bool mute_by_level);
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
        void on_lvl_op4_event(bool mute_by_level);
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
        void on_lvl_op5_event(bool mute_by_level);
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
        void on_lvl_op6_event(bool mute_by_level);
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
