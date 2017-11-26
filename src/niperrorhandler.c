/**
 * @file
 * @brief Runtime error reporting, where needed
 *
 * @author Janne Toivola
 * @author Mikko Korpela
 * @copyright &copy; 2007,2012 Janne Toivola <br>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. <br>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. <br>
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "niperrorhandler.h"
#include <errno.h>

/* TODO: compiler flag for suppressing stdio / error messages */
#include <stdio.h>

/* A variable for counting errors, in case more than 1 allowed */
static int NIP_ERROR_COUNTER = 0;

/* Error code of the last error, not just errno */
static int NIP_ERROR_CODE = 0;

int nip_report_error(char *srcFile, int line, int error, int verbose){
  NIP_ERROR_CODE = error;
  NIP_ERROR_COUNTER++;
  if(verbose){
    fprintf(stderr, "In %s (%d): ", srcFile, line);
    switch (error) {
    case 0 :
      fprintf(stderr, "O.K.\n"); break;
    case EFAULT : 
      fprintf(stderr, "Nullpointer given.\n"); break; // the only bad addresses
    case EDOM :
      fprintf(stderr, "Argument outside the defined domain.\n"); break;
    case EINVAL :
      fprintf(stderr, "Invalid argument given.\n"); break;
    case ENOMEM :
      fprintf(stderr, "Failed to allocate memory.\n"); break;
    case EIO :
      fprintf(stderr, "I/O failure.\n"); break;
    case ENOENT :
      fprintf(stderr, "Requested file not found.\n"); break;
    default : fprintf(stderr, "Something went wrong.\n");
    }
  }
  return error;
}

void nip_reset_error_handler(){
  NIP_ERROR_CODE = 0;
  NIP_ERROR_COUNTER = 0;
}

int nip_check_error_type(){
  return NIP_ERROR_CODE;
}

int nip_check_error_counter(){
  return NIP_ERROR_COUNTER;
}
