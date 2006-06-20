/* Yhtenäinen virhetilanteiden hallintajärjestelmä! */
/* Pitäisi löytää tälle jotain käyttöä... ja kehittää systeemiä. */
/*http://www.opengroup.org/onlinepubs/007904975/functions/xsh_chap02_03.html*/
/* #include <errno.h> ??? */
#include <stdio.h>
#include "errorhandler.h"

/* A variable for counting errors */
static int ERROR_COUNTER = 0;

/* Errorcode of the last error */
static int ERRORCODE = NO_ERROR; 

void report_error(char *srcFile, int line, int errorcode, int verbose){
  ERRORCODE = errorcode;
  ERROR_COUNTER++;
  if(verbose){
    fprintf(stderr, "In %s (%d): ", srcFile, line);
    switch (errorcode) {
    case NO_ERROR : 
      fprintf(stderr, "O.K.\n"); break;
    case ERROR_NULLPOINTER : 
      fprintf(stderr, "Nullpointer given.\n"); break;
    case ERROR_DIVBYZERO :
      fprintf(stderr, "Division by zero.\n"); break;
    case ERROR_INVALID_ARGUMENT :
      fprintf(stderr, "Invalid argument given.\n"); break;
    case ERROR_OUTOFMEMORY :
      fprintf(stderr, "Malloc or calloc failed.\n"); break;
    case ERROR_IO :
      fprintf(stderr, "I/O failure.\n"); break;
    case ERROR_GENERAL :
      fprintf(stderr, "An error encountered.\n"); break;
    case ERROR_FILENOTFOUND :
      fprintf(stderr, "Requested file not found.\n"); break;
    default : fprintf(stderr, "Something went wrong.\n");
    }
  }
}

void reset_errorhandler(){
  ERRORCODE = NO_ERROR;
  ERROR_COUNTER = 0;
}

int check_errortype(){
  return ERRORCODE;
}

int check_errorcounter(){
  return ERROR_COUNTER;
}
