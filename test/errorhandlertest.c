/*  NIP - Dynamic Bayesian Network library
    Copyright (C) 2012  Janne Toivola

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, see <http://www.gnu.org/licenses/>.
*/

/*
 * errorhandlertest.c
 * $Id: errorhandlertest.c,v 1.4 2008-12-20 13:05:24 jatoivol Exp $
 * This is a small test for the error handler.
 */

#include <stdio.h>
#include <stdlib.h>
#include "errorhandler.h" 

/* Main function for testing */
int main(){

  int e = ERROR_DIVBYZERO;
  int i;
  printf("Raportoidaan virheita.\n");
  for(i = 0; i < 10; i++){
    report_error(__FILE__, __LINE__, e, 0);
    printf("%d virhetta\n", check_errorcounter());
  }
  printf("Nollataan errorhandler.\n");
  reset_errorhandler();
  printf("%d virhetta\n", check_errorcounter());
  return 0;
}
