/* ----------------------------------------------------------------------------
 * Synth.h -- DX7 Graphic interface
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
 * SysEx MSG
 * 0x7E	126	Non-real time
 * 0x7F	127	Real time*
 * Fabricant ID
 * 0x01	1	Sequential Circuits
 * 0x02	2	Big Briar
 * 0x03	3	Octave / Plateau
 * 0x04	4	Moog
 * 0x05	5	Passport Designs
 * 0x06	6	Lexicon
 * 0x07	7	Kurzweil
 * 0x08	8	Fender
 * 0x09	9	Gulbransen
 * 0x0A	10	Delta Labs
 * 0x0B	11	Sound Comp.
 * 0x0C	12	General Electro
 * 0x0D	13	Techmar
 * 0x0E	14	Matthews Research
 * 0x10	16	Oberheim
 * 0x11	17	PAIA
 * 0x12	18	Simmons
 * 0x13	19	Gentle Electric
 * 0x14	20	Fairlight
 * 0x15	21	JL Cooper
 * 0x16	22	Lowery
 * 0x17	23	Lin
 * 0x18	24	Emu
 * 0x1B	27	Peavey
 * 0x20	32	Bon Tempi
 * 0x21	33	S.I.E.L.
 * 0x23	35	SyntheAxe
 * 0x24	36	Hohner
 * 0x25	37	Crumar
 * 0x26	38	Solton
 * 0x27	39	Jellinghous Ms
 * 0x28	40	CTS
 * 0x29	41	PPG
 * 0x2F	47	Elka
 * 0x40	64	Kawai
 * 0x41	65	Roland
 * 0x42	66	Korg
 * 0x43	67	Yamaha
 * 0x44	68	Casio
 * 0x45	69	Akai
 * 0x46 070 Kamiya Studio
 * 0x47 071 Akai
 * 0x48 072 Victor
 * 0x4B 075 Fujitsu
 * 0x4C 076 Sony
 * 0x4E 078 Teac
 * 0x50 080 Matsushita
 * 0x51 081 Fostex
 * 0x52 082 Zoom
  *0x54 084 Matsushita
 * 0x55 085 Suzuki
 * 0x56 086 Fuji Sound
 * 0x57 087 Acoustic Technical Laboratory
 * 0x7E 126 Universal Non Realtime Message (UNRT)
 * 0x7F 127 Universal Realtime Message (URT)
 */
/*TODO :
 *timer
 *delete midi
 *write bank
 *receive sysex
 *charger/decharger synth
 *attache/detache midi one by synth
 * !!! USE flag to prevent midi send !!!
*/
#pragma once
/* sys */
/* app */
#include <gxinterface/0.0.1/common.h>
#include <gxinterface/0.0.1/debug.h>
#include <gxinterface/0.0.1/lang.h>
#include <gxinterface/0.0.1/gxthread.h>

#if defined(__RtMidi__)
        #include <RtMidi.h>
        #define SND_SEQ_EVENT_NOTEON 0x80
        #define SND_SEQ_EVENT_NOTEOFF 0x90
        #define SND_SEQ_EVENT_KEYPRESS 0xA0
        #define SND_SEQ_EVENT_CONTROLLER 0xB0
        #define SND_SEQ_EVENT_PGMCHANGE 0xC0
        #define SND_SEQ_EVENT_CHANPRESS 0xD0
        #define SND_SEQ_EVENT_PITCHBEND 0xE0
        #define SND_SEQ_EVENT_SYSEX 0xF0
        #define SND_SEQ_EVENT_SENSING 0xFE
#endif
#if defined(__ALSA__)
        /*	Alsa */
        #include <alsa/asoundlib.h>
        /* Helpers */
        #include "event_macro_helpers.h"
#endif
/* CONSTANTS */

class Synth : public Thread {
    public:
        Synth(Glib::ustring);
        virtual ~Synth();
    private:
        bool block_midi_msg;
        Glib::ustring caller="None";
        #if defined(__ALSA__)
                /*** ALSA MIDI ***/
                snd_seq_t* seq_handle;                 /* handler */
                snd_seq_system_info_t* seq_info;       /* info */
                snd_seq_event_t* ev;                   /* evenement */
                size_t in_buff_size, out_buff_size;      /* buffers d'entrée et de sortie */
                int client_id=0;
                int port_in=0, port_out=0;             /* ports d'entréee et de sortie */
                /* seq queue */
                int spfd;                              /* taille de la file de queue */
                struct pollfd *pfd;                    /* array de file de queue du sequenceur */
                void list_midi_ports();
                int get_last_interface_with_name(Glib::ustring);
        #endif
        #if defined(__RtMidi__)
                std::string port_in_name;
                std::string port_out_name;
                RtMidiIn* port_in;
                RtMidiOut* port_out;
                static void midiInCallback(double timestamp, std::vector<unsigned char>* message, void* userData);
        #endif
    protected:
        uint8_t id_fabricant = 0x00;              /* selected fabricant */
        uint8_t channel_send = 0x00;              /* selected channel */
        uint8_t channel_receive = 0x00;           /* selected channel */
        uint8_t sub_status = 0x00;                /* selected sub_s */
        uint8_t msb=0x00;
        uint8_t lsb=0x00;
        uint8_t nvoice=0x00;
        /* generic  */
        void init_nls();
        bool error();
        /*** MIDI ***/
        void block_midi();
        void unblock_midi();
        #if defined(__ALSA__)
                /* ALSA */
                void print_event_info(snd_seq_event_t*);
                snd_seq_event_t* get_seq_event_handler();
                snd_seq_t* get_seq_handler();
                snd_seq_system_info_t* get_seq_info();
                std::string get_event_name(int);
                int get_port_out_number();
                int get_port_in_number();
                int get_client_id();
        #endif
        #if defined(__RtMidi__)
                std::vector<unsigned char> message;
                void print_event_info();
                std::string get_event_name(unsigned char);
                struct timespec ts = {0, 1000000L};
        #endif       
        size_t* get_seq_buffer_size();
        void connect_midi(Glib::ustring);
        void deconnect_midi();
        void send_midi(char, unsigned int, unsigned char*);
        /* could be overrride in the synthé module itself*/
        #if defined(__ALSA__)
                virtual void listen_midi();             /* function that handle midi events */
        #endif
        #if defined(__RtMidi__)
                virtual void listen_midi(double, std::vector<unsigned char>*, void*) = 0;
        #endif
        /*** THREAD ***/
        virtual bool Run();
};
