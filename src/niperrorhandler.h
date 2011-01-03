/* Simple error reporting utilities
 * Author: Janne Toivola
 * Version: $Id: niperrorhandler.h,v 1.3 2011-01-03 18:04:55 jatoivol Exp $
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
void nip_reset_errorhandler();

/* Method for checking what was the last error */
int nip_check_errortype();

/* Method for checking how many errors have occured */
int nip_check_errorcounter();

#endif
