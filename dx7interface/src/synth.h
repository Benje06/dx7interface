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
 * 0x02	2	Big B*riar
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
#include <uchar.h>
#include <gxinterface/0.0.1/common.h>
#include <gxinterface/0.0.1/debug.h>
#include <gxinterface/0.0.1/lang.h>
/**/
#include <gxinterface/0.0.1/gxthread.h>
/*	Alsa */
#include <alsa/asoundlib.h>
/** CONSTANTS **/
#include "event_macro_helpers.h"

class Synth : public Thread {
    public:
        Synth(Glib::ustring);
        virtual ~Synth();
        void block_midi();
        void unblock_midi();
    private:
        bool block_midi_msg;
        Glib::ustring caller="None";
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

    protected:
        uint8_t id_fabricant = 0x00;              /* selected fabricant */
        uint8_t channel_send = 0x00;              /* selected channel */
        uint8_t channel_receive = 0x00;           /* selected channel */
        uint8_t sub_status = 0x00;                /* selected sub_s */
        uint8_t msb=0x00;
        uint8_t lsb=0x00;
        uint8_t nvoice=0x00;
        /* generic error */
        bool error();
        /*** MIDI ***/
        void print_event_info(snd_seq_event_t*);
        int get_port_out_number();
        int get_port_in_number();
        int get_client_id();
        std::string get_event_name(int);
        snd_seq_event_t* get_seq_event_handler();
        snd_seq_t* get_seq_handler();
        snd_seq_system_info_t* get_seq_info();
        size_t* get_seq_buffer_size();
        void connect_midi(Glib::ustring);
        void deconnect_midi();
        void send_midi(char, uint, u_char*);
        /* could be overrride in the synthé module itself*/
        virtual void listen_midi();             /* function that handle midi events */
        /*** THREAD ***/
        virtual bool Run();
};
