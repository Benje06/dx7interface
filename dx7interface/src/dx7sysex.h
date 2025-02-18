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

    typedef struct st_1{
        guint8 val;
        guint address;
        static const guint8 max=1;
        static const guint8 mask=0x01;
    }St_1;

    typedef struct st_3{
        guint8 val;
        static const guint8 max=3;
        static const guint8 mask=0x03;
    }St_3;

    typedef struct st_5{
        guint8 val;
        static const guint8 max=5;
        static const guint8 mask=0x07;
    }St_5;

    typedef struct st_7{
        guint8 val;
        guint address;
        static const guint8 max=7;
        static const guint8 mask=0x07;
    }St_7;

    typedef struct st_14{
        guint8 val;
        guint address;
        static const guint8 max=14;
        static const guint8 mask=0x0F;
    }St_14;

    typedef struct st_31{
        guint8 val;
        guint address;
        static const guint8 max=31;
        static const guint8 mask=0x1F;
    }St_31;

    typedef struct st_48{
        guint8 val;
        guint address;
        static const guint8 max=48;
        static const guint8 mask=0x3F;
    }St_48;

    typedef struct st_99{
        guint8 val;
        guint address;
        static const guint8 max=99;
        static const guint8 mask=0x7F;
    }St_99;

    typedef struct st_127{
        guint8 val;
        guint address;
        static const guint8 max=127;
        static const guint8 mask=0x7F;
    }St_99;

    typedef struct st_kls {
        St_99 brk_pt;//  LEV SCL BRK PT 0-99
        St_99 lft_dpth;//  SCL LEFT DEPTH 0-99
        St_99 rght_dpth;//  SCL RGHT DEPTH 0-99
        St_3 rght_curve;//  scl right curve 0-3
        St_3 lft_curve;//  scl left curve 0-3
    }St_kls;

    typedef struct st_op {
        St_99 eg_rt[4];//  op eg rate
        St_99 eg_lvl[4];//  op eg level
        St_99 dtun;// osc detune
        St_7 krs;//  osc rate scale
        St_7 kvs;// key vel sens
        St_3 ams;// amp mod sens
        St_99 lvl;//  output lev
        St_31 freq_coarse;//  freq coarse
        St_1 freq_mode;//   freq mode
        St_99 freq_fine;//  freq fine
        St_kls kls;
    }St_op;

    typedef struct st_pitch {
        St_99 eg_rt[4];
        St_99 eg_lvl[4];
    }St_pitch;

    typedef struct st_lfo {
        St_99 speed;
        St_99 delay;
        St_99 pmd;
        St_99 amd;
        St_7 pms;
        St_5 wave;
        St_1 sync;
    }St_lfo;

    typedef struct st_algo {
        St_31 algo;
        St_1 oks;
        St_7 feedback;
        St_48 transpose;
    }St_algo;

    typedef struct st_funct {
        St_7 feedback;
        St_48 transpose;
    }St_funct;

    typedef struct st_extra {
        St_127 mute;
        St_funct functions;
    }St_extra;

    typedef struct st_dx7sysex_1 {
        St_op op[6];
        St_pitch pitch;
        St_algo algo;
        St_lfo lfo;
        Glib::ustring name;
        guint8 sum;
        St_extra extra;
    }St_dx7sysex_1;

    template <guint i>
    struct St_dx7sysex{
        St_dx7sysex_1 sound[i];
    };

    /*typedef struct st_dx7sysex_32{
        St_dx7sysex sound[32];
    }St_dx7sysex_32;

    typedef struct St_dx7sysex_128{
        St_dx7sysex sound[128];
    }St_dx7sysex_128;
    */
#endif /* dx7sysex_H */
