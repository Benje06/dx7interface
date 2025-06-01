 /* dx7sysex.h -- sysex
 * DX7 syxex interface definition
 * ----------------------------------------------------------------------------
 * copyright (c) 22006, 2007, 2008, 2009, 2010 Jérôme BENHAÏM <benhaimjerome@gmail.com>,
 * 
 * ----------------------------------------------------------------------------
 * This program is free software ; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation ; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY ; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * ----------------------------------------------------------------------------
 */
    #ifndef dx7sysex_H
        #define dx7sysex_H

    typedef struct st_2{
        uint8_t val;
        unsigned int address;
        static const uint8_t max=1;
        static const uint8_t mask=0x01;
    }St_2;

    typedef struct st_4{
        uint8_t val;
        static const uint8_t max=3;
        static const uint8_t mask=0x03;
    }St_4;

    typedef struct st_6{
        uint8_t val;
        static const uint8_t max=5;
        static const uint8_t mask=0x07;
    }St_6;

    typedef struct st_8{
        uint8_t val;
        unsigned int address;
        static const uint8_t max=7;
        static const uint8_t mask=0x07;
    }St_8;

    typedef struct st_14{
        uint8_t val;
        unsigned int address;
        static const uint8_t max=13;
        static const uint8_t mask=0x0F;
    }St_14;

    typedef struct st_15{
        uint8_t val;
        unsigned int address;
        static const uint8_t max=14;
        static const uint8_t mask=0x0F;
    }St_15;

    typedef struct st_32{
        uint8_t val;
        unsigned int address;
        static const uint8_t max=31;
        static const uint8_t mask=0x1F;
    }St_32;

    typedef struct st_49{
        uint8_t val;
        unsigned int address;
        static const uint8_t max=48;
        static const uint8_t mask=0x3F;
    }St_49;

    typedef struct st_100{
        uint8_t val;
        unsigned int address;
        static const uint8_t max=99;
        static const uint8_t mask=0x7F;
    }St_100;

    typedef struct st_128{
        uint8_t val = 0x00;
        unsigned int address;
        static const uint8_t max=127;
        static const uint8_t mask=0x7F;
    }St_128;

    typedef struct st_kls {
        St_100 brk_pt;//  LEV SCL BRK PT 0-99
        St_100 lft_dpth;//  SCL LEFT DEPTH 0-99
        St_100 rght_dpth;//  SCL RGHT DEPTH 0-99
        St_4 rght_curve;//  scl right curve 0-3
        St_4 lft_curve;//  scl left curve 0-3
    }St_kls;

    typedef struct st_op {
        St_100 eg_rt[4];//  op eg rate
        St_100 eg_lvl[4];//  op eg level
        St_14 dtun;// osc detune
        St_8 krs;//  osc rate scale
        St_8 kvs;// key vel sens
        St_4 ams;// amp mod sens
        St_100 lvl;//  output level
        St_32 freq_coarse;//  freq coarse
        St_2  freq_mode;//   freq mode
        St_100 freq_fine;//  freq fine
        St_kls kls;
    }St_op;

    typedef struct st_pitch {
        St_100 eg_rt[4];
        St_100 eg_lvl[4];
    }St_pitch;

    typedef struct st_lfo {
        St_100 speed;
        St_100 delay;
        St_100 pmd;
        St_100 amd;
        St_8 pms;
        St_6 wave;
        St_2 sync;
    }St_lfo;

    typedef struct st_algo {
        St_32 algo;
        St_2  oks;
        St_8 feedback;
        St_49 transpose;
    }St_algo;

    typedef struct st_ctrl {
        St_2  poly_mono;
        St_14 ptch_bnd_rng;
        St_14 ptch_bnd_stp;
        St_2  portamento_md;
        St_2  portamento_glss;
        St_100 portamento_tm;
        St_100 md_whl_rng;
        St_8 md_whl_assgn;
        St_100 foot_rng;
        St_8 foot_assgn;
        St_100 brth_rng;
        St_8 brth_assgn;
        St_100 aftrtch_rng;
        St_8 aftrtch_assgn;
        St_8 voice_attenuation;
    }St_ctrl;

    typedef struct st_extra {
        St_128 mute;
        St_ctrl controller;
    }St_extra;

    typedef struct st_dx7sysex_1 {
        St_op op[6];
        St_pitch pitch;
        St_algo algo;
        St_lfo lfo;
        Glib::ustring name;
        //std::string name;
        uint8_t sum;
        St_extra extra;
        bool modified = false;
    }St_dx7sysex_1;

    template <unsigned int i>
    struct St_dx7sysex {
        // TODO: use vector
        //std::vector<St_dx7sysex_1> sound;
        St_dx7sysex_1 sound[i];
        Glib::ustring name = "";
    };

    typedef struct{
        //std::vector<St_dx7sysex_1> sound;
        St_dx7sysex_1 sound[1]; // Flexible array member
        Glib::ustring name;
    }*Bank_ptr;


#endif /* dx7sysex_H */
