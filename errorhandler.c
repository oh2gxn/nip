/* Yhtenäinen virhetilanteiden hallintajärjestelmä! */
/* Pitäisi löytää tälle jotain käyttöä... ja kehittää systeemiä. */
/*http://www.opengroup.org/onlinepubs/007904975/functions/xsh_chap02_03.html*/
/* #include <errno.h> ??? */
#include <stdio.h>
#include "errorhandler.h"

/* A global variable for counting errors */
int ERROR_COUNTER = 0;

/* Errorcode of the last error */
int ERRORCODE = NO_ERROR; 

/* Method for reporting an error. 
- errorcode is for example ERROR_DIVBYZERO defined in errorhandler.h 
- if verbose is other than 0, a message will be displayed 
*/
void report_error(int errorcode, int verbose){
  ERRORCODE = errorcode;
  ERROR_COUNTER++;
}

/* Method for resetting the errorcounter */
void reset_errorhandler(){
  ERRORCODE = NO_ERROR;
  ERROR_COUNTER = 0;
}

/* Method for checking what was the last error */
int check_errortype(){
  return ERRORCODE;
}

/* Method for checking how many errors have occured */
int check_errorcounter(){
  return ERROR_COUNTER;
}
