#ifndef __NIPERRORHANDLER_H__
#define __NIPERRORHANDLER_H__

/* Is there something like errno.h ?? */
#define NIP_NO_ERROR 0
#define NIP_ERROR_NULLPOINTER 1
#define NIP_ERROR_DIVBYZERO 2
#define NIP_ERROR_INVALID_ARGUMENT 3
#define NIP_ERROR_OUTOFMEMORY 4
#define NIP_ERROR_IO 5
#define NIP_ERROR_GENERAL 6
#define NIP_ERROR_FILENOTFOUND 7
#define NIP_ERROR_BAD_LUCK 8

/* Method for reporting an error. 
 * - srcFile is the source file (__FILE__)
 * - line is the number of the line in the source code (__LINE__)
 * - errorcode is for example NIP_ERROR_DIVBYZERO
 * - if verbose is other than 0, a message will be displayed 
 */
void nip_report_error(char *srcFile, int line, int errorcode, int verbose);

/* Method for resetting the errorcounter. */
void nip_reset_errorhandler();

/* Method for checking what was the last error */
int nip_check_errortype();

/* Method for checking how many errors have occured */
int nip_check_errorcounter();

#endif
