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
  * 
  */
#pragma once
#ifndef interface_COMMON_H
    #define interface_COMMON_H
    /* Ui dir for gxinterface */
    #define MOD_DIRECTORY PROGRAMNAME_MOD_DIR
    #define MOD_IMG_DIRECTORY PROGRAMNAME_IMG_DIR
    #define MOD_UI_DIRECTORY PROGRAMNAME_UI_DIR

    #ifdef HAVE_CONFIG_H
        #include <config.h>
    #endif
    /* TODO : remove std C in main */
    #include <iostream>
    #ifdef ENABLE_NLS
        #include <libintl.h>
        #include <glibmm/i18n.h>
    #endif
    #include <regex>
    #include <gtkmm-4.0/gtkmm.h>
    //#include <gtkmm/application.h>
    //#include <glibmm-2.68/glibmm.h>
    //#include <giomm-2.68/giomm.h>
    //#include <gdkmm-3.0/gdkmm.h>
    //#include <gtkmm.h>
    #ifdef G_OS_WIN32
        #include <windows.h>
        #define DS ('\\')
        /* CR+LF */
        #define EOL ('\r\n')
    #endif
    #ifdef G_OS_LIN
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
    #define DS ('/')
    using namespace Glib;
    /** convert to string any type of number **/
    template <class paramType>
    Glib::ustring tostr(paramType val) {
        std::ostringstream strm;
        strm << val;
        return strm.str();
    };
#endif /* interface_COMMON_H */

