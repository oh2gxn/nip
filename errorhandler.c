/* Yhtenäinen virhetilanteiden hallintajärjestelmä! */

#include <stdio.h>
#include "errorhandler.h"

/* A global variable for counting errors */
int ERROR_COUNTER = 0;

/* Errorcode of the last error. */
enum errorcodes ERRORCODE = OK;

/* Method for reporting an error. 
- errorcode is for example DIVBYZERO defined in errorhandler.h 
- if verbose is other than 0, a message will be displayed 
*/
void report_error(enum errorcodes errorcode, int verbose){
  ERRORCODE = errorcode;
  ERROR_COUNTER++;
}

/* Method for resetting the errorcounter */
void reset_errorhandler(){
  ERRORCODE = OK;
  ERROR_COUNTER = 0;
}

/* Method for checking what was the last error */
enum errorcodes check_errortype(){
  return ERRORCODE;
}

/* Method for checking how many errors have occured */
int check_errorcounter(){
  return ERROR_COUNTER;
}
