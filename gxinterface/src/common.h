/* ----------------------------------------------------------------------------
 * gxinterface -- Gtk+ Extended Interface
 * Common C++ tools Header
 *
 * ----------------------------------------------------------------------------
 * copyright © 2006, 2007, 2008, 2009, 2010Jérôme BENHAÏM <benhaimjerome@gmail.com>
 *
 * ----------------------------------------------------------------------------
 *
 *   This file is part of GxInterface.
 *
 *	 GxInterface  is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ----------------------------------------------------------------------------
 */

 /* Define by Operating System : 
  * 	- include
  * 	- constant :
  * 		DS		Directory Separator
  * 		EOL		End Of Line
  * 	- generic function :
  * 		tostr<T_>(V_) convert to string a variable V_ of type T_
  *         constexpr std::size_t str_const_hash(const char* str) convert str to a hash
  *         constexpr std::size_t operator""_hash(const char* str, std::size_t) HASH operator for switch case use
  */
#pragma once
#ifndef interface_COMMON_H
    #define interface_COMMON_H

    #ifdef HAVE_CONFIG_H
        #include <config.h>
    #else
        #define MOD_DIRECTORY PROGRAMNAME_MOD_DIR
        #define MOD_IMG_DIR PROGRAMNAME_IMG_DIR
        #define MOD_DATA_DIRECTORY PROGRAMNAME_DATA_DIR
        #define MOD_UI_DIRECTORY PROGRAMNAME_UI_DIR
    #endif
    #define STRINGIFY(x) #x
    #define TOSTRING(x) STRINGIFY(x)
    #include "debug.h"

    #include <iostream>
    #include <filesystem>
    #include <regex>
    #include <chrono>
    #include <ctime>
    #include <iomanip>
    #include <sstream>
    #include <string>

    #if defined(__WIN32) || defined(__MINGW32__)
        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>
        #undef ERROR
        #undef IN
        #undef OUT
        #undef WINDING
        #undef IGNORE
        #undef near
        #define DS "\\"
        /* CR+LF */
        #undef EOL
        #define EOL "\r\n"
        #include <system_error>
        #include <locale>
        /*typedef unsigned int uint;
        typedef unsigned char u_char;
        typedef unsigned long ulong;*/
    #endif
    #ifdef __linux__
        /* LF */
        #define EOL ('\n')
        /* directory separator */
        #define DS ('/')
    #endif
    #ifdef G_OS_UNIX
        /* LF */
        #define EOL ('\n')
        #define DS ('/')
    #endif
    #ifdef G_OS_OSX
        /* LF */
        #define EOL ('\n')
        #define DS ('/')
    #endif
    #ifdef G_OS_MAC
        /* CR */
        #define EOL ('\r')
        #define DS ('/')
    #endif

    #ifdef ENABLE_NLS
        #include <libintl.h>
        #include <glibmm/i18n.h>
    #endif
    #include <gtkmm-4.0/gtkmm.h>
    #include <giomm/settings.h>
    #include <glibmm.h>

	/*** CONSTANTS ***/
	// for action_type
	#define ACT_OPEN 1
	#define ACT_SAVE 2
	#define ACT_INSERT 3 
	#define ACT_DELETE 4
	#define ACT_REPLACE 5
	#define ACT_MOOVE 6
	#define ACT_WARNING 7

    /*** String Convert ***/
    /* convert to string any type of number */
    template <class paramType>
    Glib::ustring tostr(paramType val) {
        std::ostringstream strm;
        strm << val;
        return strm.str();
    };
    /* convert glib::ustring to ascii-7bit */
    inline std::string str_to_ascii(const Glib::ustring& input) {
        std::string result;
        for( auto ch : input ){
            if( ch < 128 ){ // Keep ASCII characters
                result += ch;
            } else { // Replace non-ASCII characters with '?'
                result += '?';
            };
        };
        return result;
    }
    // used to hash MACRO value to array (ex: use to convert MACRO int type of snd_seq_event_type to text message with the name of the macro)
    constexpr std::size_t str_const_hash(const char* str) {
        // Implement a simple compile-time hash function
        std::size_t h = 0;
        for (; *str; ++str) {
            h = h * 31 + *str;
        };
        return h;
    };
    constexpr std::size_t operator""_hash(const char* str, std::size_t) {
        return str_const_hash(str);
    };
    inline std::string get_time(){
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        
        std::tm tm_buffer;
        #if defined(__WIN32) || defined(__MINGW32__)
            localtime_s(&tm_buffer,&now_time);
        #else
            localtime_r(&now_time, &tm_buffer);  // POSIX thread-safe version
        #endif
        std::ostringstream ostr;
        ostr << std::put_time(&tm_buffer, "%Y-%m-%d %H:%M:%S");
        return ostr.str();
    };
    inline std::string error(std::string from, std::string what, std::string why){
        std::string msg = _("From: ") + from + "\n\t"
                        + what + "\n\t"
                        +_("Reason => ") + why;
        return msg;
    };
    /*** INIT NLS ***/
    inline void init_nls(){
        #ifdef ENABLE_NLS
            setlocale (LC_ALL, "");
            std::locale::global(std::locale(""));
            textdomain (GETTEXT_PACKAGE);
            bindtextdomain (GETTEXT_PACKAGE, PROGRAMNAME_LOCALEDIR);
            bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        #endif
    };

    #include "logger.h"
#endif /* interface_COMMON_H */

