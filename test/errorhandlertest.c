/* Tämä on pieni testi errorhandlerille. */

#include <stdio.h>
#include <stdlib.h>
#include "errorhandler.h" 

/* Main function for testing */
int main(){

  enum errorcodes e = DIVBYZERO;
  int i;
  printf("Raportoidaan virheita.\n");
  for(i = 0; i < 10; i++){
    report_error(e, 0);
    printf("%d virhetta\n", check_errorcounter());
  }
  printf("Nollataan errorhandler.\n");
  reset_errorhandler();
  printf("%d virhetta\n", check_errorcounter());
  return 0;
}
