/* ----------------------------------------------------------------------------
 * gxinterface -- Gtk+ Extended Interface
 * Debug macros Header
 *
 * ----------------------------------------------------------------------------
 * copyright © 2006, 2007, 2008, 2009, 2010 Jérôme BENHAÏM <benhaimjerome@gmail.com>,
 *					  Mirsal ENNAIME <mirsal.ennaime@gmail.com>
 * ----------------------------------------------------------------------------
 *
 *   This file is part of GxInterface.
 *
 *	 GxInterface is free software: you can redistribute it and/or modify
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
#ifndef _TOOLS_DEBUG_H
    #define _TOOLS_DEBUG_H

    #include <stdlib.h>
    #include <stdio.h>

    #define DEBUG_TOOL
    #define DEBUG_TOOL_AS_ERROR

    #ifdef DEBUG_TOOL
    #  ifdef DEBUG_TOOL_AS_ERROR
    #    define LOG_IN()        fprintf ( stderr, " -> %s ligne %i de %s \n", __PRETTY_FUNCTION__, __LINE__ , __FILE__)
    #    define LOG_OUT()       fprintf ( stderr, "<-  %s\n", __PRETTY_FUNCTION__ )
    #    define TRACE(f, ...)   fprintf ( stderr, f "\n", ## __VA_ARGS__ )
    #  else /* DEBUG_TOOL_AS_ERROR */
    #    define LOG_IN()        printf ( " -> %s\n", __class__ )
    #    define LOG_OUT()       printf ( "<-  %s\n", __func__ )
    #    define TRACE(f, ...)   printf ( f "\n", ## __VA_ARGS__ )
    #  endif /* DEBUG_TOOL_AS_ERROR */
    #else  /* DEBUG_TOOL */
    #  define LOG_IN()        (void)(0)
    #  define LOG_OUT()       (void)(0)
    #  define TRACE(f, ...)   (void)(0)
    #endif /* DEBUG_TOOL */
    #define print_error(str, ...)  fprintf(stderr, (str "\n"), ## __VA_ARGS__)
#endif /* _TOOLS_DEBUG_H */
