#ifndef __ERRORHANDLER_H__
#define __ERRORHANDLER_H__

enum errorcodes { OK, NULLPOINTER, DIVBYZERO };

/* Method for reporting an error. 
- errorcode is for example DIVBYZERO defined in errorhandler.h 
- if verbose is other than 0, a message will be displayed 
*/
void report_error(enum errorcodes errorcode, int verbose);

/* Method for resetting the errorcounter. */
void reset_errorhandler();

/* Method for checking what was the last error */
enum errorcodes check_errortype();

/* Method for checking how many errors have occured */
int check_errorcounter();

#endif







