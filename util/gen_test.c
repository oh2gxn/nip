#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>

#include "nip.h"
#include "variable.h"

/* Generates data according to a given model
 * SYNOPSIS: GEN_TEST <MODEL.NET> <LENGTH> <RESULT.TXT>
 * (resulting data will be written to the file <result.txt>) */

int main(int argc, char *argv[]) {

  int i, n;
  nip model = NULL;
  time_series ts = NULL;
  double d = 0;
  char* tailptr = NULL;

  /** <Some experimental code> **/
  ;
  /** </Some experimental code>**/

  if(argc < 4){
    printf("You must specify: \n"); 
    printf(" - the NET file for the model, \n");
    printf(" - time series length, \n"); 
    printf(" - the resulting data file, please!\n");
    return 0;
  }
  
  /* read the model */
  model = parse_model(argv[1]);
  if(!model){
    printf("Unable to parse the NET file: %s?\n", argv[1]);
    return -1;
  }
  use_priors(model, 1);

  /* read the time series length */
  d = strtod(argv[2], &tailptr);
  n = (int)d;
  if(d <= 0 || d > 1000000  || tailptr == argv[2]){
    printf("Invalid time series length: %s?\n", argv[2]);
    free_model(model);
    return -1;
  }

  /* THE algorithm (may take a while) */
  printf("Generating data... \n");
  ts = generate_data(model, n);
  if(!ts){
    fprintf(stderr, "There were errors during data sampling!\n");
    free_model(model);
    return -1;
  }
  printf("...done.\n");

  /* Write the results to the file */
  i =  write_timeseries(&ts, 1, argv[3]);
  if(i != NO_ERROR){
    fprintf(stderr, "Failed to write the data into %s\n", argv[3]);
    report_error(__FILE__, __LINE__, i, 1);
    free_timeseries(ts);
    free_model(model);
    return -1;
  }

  free_timeseries(ts);
  free_model(model);
  return 0;
}
