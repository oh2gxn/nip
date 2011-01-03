/* Unified error handler stuff! */
/*http://www.opengroup.org/onlinepubs/007904975/functions/xsh_chap02_03.html*/
/* #include <errno.h> ??? */

#include "niperrorhandler.h"

/* A variable for counting errors */
static int NIP_ERROR_COUNTER = 0;

/* Errorcode of the last error */
static int NIP_ERRORCODE = NIP_NO_ERROR; 

void nip_report_error(char *srcFile, int line, nip_error_code e, int verbose){
  NIP_ERRORCODE = e;
  NIP_ERROR_COUNTER++;
  if(verbose){
    fprintf(stderr, "In %s (%d): ", srcFile, line);
    switch (e) {
    case NIP_NO_ERROR : 
      fprintf(stderr, "O.K.\n"); break;
    case NIP_ERROR_NULLPOINTER : 
      fprintf(stderr, "Nullpointer given.\n"); break;
    case NIP_ERROR_DIVBYZERO :
      fprintf(stderr, "Division by zero.\n"); break;
    case NIP_ERROR_INVALID_ARGUMENT :
      fprintf(stderr, "Invalid argument given.\n"); break;
    case NIP_ERROR_OUTOFMEMORY :
      fprintf(stderr, "Malloc or calloc failed.\n"); break;
    case NIP_ERROR_IO :
      fprintf(stderr, "I/O failure.\n"); break;
    case NIP_ERROR_GENERAL :
      fprintf(stderr, "An error encountered.\n"); break;
    case NIP_ERROR_FILENOTFOUND :
      fprintf(stderr, "Requested file not found.\n"); break;
    case NIP_ERROR_BAD_LUCK :
      fprintf(stderr, "Random model was born dead.\n"); break;
    default : fprintf(stderr, "Something went wrong.\n");
    }
  }
}

void nip_reset_errorhandler(){
  NIP_ERRORCODE = NIP_NO_ERROR;
  NIP_ERROR_COUNTER = 0;
}

int nip_check_errortype(){
  return NIP_ERRORCODE;
}

int nip_check_errorcounter(){
  return NIP_ERROR_COUNTER;
}
