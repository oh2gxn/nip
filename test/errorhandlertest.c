/*
 * errorhandlertest.c
 * $Id: errorhandlertest.c,v 1.3 2004-06-21 06:12:12 mvkorpel Exp $
 * Tämä on pieni testi errorhandlerille.
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
