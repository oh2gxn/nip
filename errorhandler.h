#ifndef __ERRORHANDLER_H__
#define __ERRORHANDLER_H__

/* Is there something like errno.h ??? */
#define NO_ERROR 0
#define ERROR_NULLPOINTER 1
#define ERROR_DIVBYZERO 2
#define ERROR_INVALID_ARGUMENT 3
#define ERROR_OUTOFMEMORY 4
#define ERROR_IO 5

#define GLOBAL_UPDATE 100
#define GLOBAL_RETRACTION 101

/* Method for reporting an error. 
 * - errorcode is for example ERROR_DIVBYZERO
 * - if verbose is other than 0, a message will be displayed 
 */
void report_error(int errorcode, int verbose);

/* Method for resetting the errorcounter. */
void reset_errorhandler();

/* Method for checking what was the last error */
int check_errortype();

/* Method for checking how many errors have occured */
int check_errorcounter();

#endif







