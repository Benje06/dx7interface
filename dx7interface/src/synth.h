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
 */
/*TODO :
 *timer
 *delete midi
 *write bank
 *receive sysex
 *charger/decharger synth
 *attache/detache midi one by synth
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
    private:
        Glib::ustring caller="None";
        /*** ALSA MIDI ***/
        snd_seq_t* seq_handle;	                /* handler */
        snd_seq_system_info_t* seq_info;            /* info */
        snd_seq_event_t* ev;                    /* evenement */
        size_t in_buff_size, out_buff_size;	    /* buffers d'entrée et de sortie */
        int port_in=0, port_out=0;	            /* ports d'entréee et de sortie */
        /* seq queue */
        int spfd;                              /* taille de la file de queue */
        struct pollfd *pfd;                    /* array de file de queue du sequenceur */
    protected:
        uint8_t id_fabricant = 0;                                /* selected fabricant */
        uint8_t channel = 0;                                     /* selected channel */
        uint8_t sub_status = 0;                                  /* selected sub_s */
        /* generic error */
        bool error();
        /*** MIDI ***/
        std::string get_event_name(int);
        snd_seq_event_t* get_seq_event_handler();
        snd_seq_t* get_seq_handler();
        snd_seq_system_info_t* get_seq_info();
        size_t* get_seq_buffer_size();
        void connect_midi(Glib::ustring);
        void deconnect_midi();
        void send_midi(char, uint, u_char*);
        /* to be implemented in the module itself*/
        virtual void listen_midi();        /* sound bank */
        /*** THREAD ***/
        virtual bool Run();
};
