#ifndef __ERRORHANDLER_H__
#define __ERRORHANDLER_H__

#define NO_ERROR 0
#define ERROR_NULLPOINTER 1
#define ERROR_DIVBYZERO 2

/* Method for reporting an error. 
- errorcode is for example DIVBYZERO defined in errorhandler.h 
- if verbose is other than 0, a message will be displayed 
*/
void report_error(int errorcode, int verbose);

/* Method for resetting the errorcounter. */
void reset_errorhandler();

/* Method for checking what was the last error */
int check_errortype();

/* Method for checking how many errors have occured */
int check_errorcounter();

#endif







