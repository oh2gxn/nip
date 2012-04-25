/*  NIP - Dynamic Bayesian Network library
    Copyright (C) 2012  Janne Toivola

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, see <http://www.gnu.org/licenses/>.
*/

/* Simple error reporting utilities
 * Author: Janne Toivola
 * Version: $Id: niperrorhandler.h,v 1.4 2011-01-23 23:01:47 jatoivol Exp $
 * */

#ifndef __NIPERRORHANDLER_H__
#define __NIPERRORHANDLER_H__

#include <stdio.h>

/* Is there something like errno.h ?? */
typedef enum nip_error_code_enum {
  NIP_NO_ERROR = 0,
  NIP_ERROR_NULLPOINTER = 1,
  NIP_ERROR_DIVBYZERO = 2,
  NIP_ERROR_INVALID_ARGUMENT = 3,
  NIP_ERROR_OUTOFMEMORY = 4,
  NIP_ERROR_IO = 5,
  NIP_ERROR_GENERAL = 6,
  NIP_ERROR_FILENOTFOUND = 7,
  NIP_ERROR_BAD_LUCK = 8
} nip_error_code;


/* Method for reporting an error. 
 * - srcFile is the source file (__FILE__)
 * - line is the number of the line in the source code (__LINE__)
 * - error code e is for example NIP_ERROR_DIVBYZERO
 * - if verbose is other than 0, a message will be displayed 
 */
void nip_report_error(char *srcFile, int line, nip_error_code e, int verbose);

/* Method for resetting the errorcounter. */
void nip_reset_error_handler();

/* Method for checking what was the last error */
nip_error_code nip_check_error_type();

/* Method for checking how many errors have occured */
int nip_check_error_counter();

#endif
