#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "nip.h"
#include "variable.h"

/* Reads an initial guess for the model from a Hugin NET file, 
 * uses EM-algorithm for learning the parameters from the 
 * given data file and writes the resulting model to the 
 * specified output file. 
 * SYNOPSIS: EM_TEST <ORIGINAL.NET> <DATA.TXT> <THRESHOLD> <RESULT>
 * (resulting model will be written to the file <result.net>) */

int main(int argc, char *argv[]) {

  int i, n;
  nip model = NULL;
  time_series *ts_set = NULL;
  double threshold = 0;
  char* tailptr = NULL;

  /* a little test */
/*   printf(" DBL_MIN = %f\n", DBL_MIN); */
/*   printf(" DBL_MAX = %f\n", DBL_MAX); */
/*   printf(" HUGE_VAL = %f\n", HUGE_VAL); */
/*   printf("-HUGE_VAL = %f\n", -HUGE_VAL); */
/*   printf("ln(DBL_MIN) = %f\n", log(DBL_MIN)); */
/*   printf("ln(DBL_MAX) = %f\n", log(DBL_MAX)); */
/*   printf("ln(-DBL_MAX) = %f\n", log(-DBL_MAX)); */

  if(argc < 5){
    printf("Give the names of: \n"); 
    printf(" - the original NET file, \n");
    printf(" - data file, \n"); 
    printf(" - threshold value (0...1), and \n");
    printf(" - name for the resulting model, please!\n");
    return 0;
  }
  
  /* read the model */
  model = parse_model(argv[1]);
  if(!model){
    printf("Unable to parse the NET file: %s?\n", argv[1]);
    return -1;
  }
  use_priors(model, 1);

  /* read the data */
  n = read_timeseries(model, argv[2], &ts_set);
  if(n == 0){
    printf("Unable to parse the data file: %s?\n", argv[2]);
    return -1;
  }

  /* read the threshold value */
  threshold = strtod(argv[3], &tailptr);
  if(threshold <= 0 || threshold > 1  || tailptr == argv[3]){
    printf("Specify a valid threshold value: %s?\n", argv[3]);
    return -1;
  }

  /* THE algorithm (may take a while) */
  printf("Computing... \n");
  i = em_learn(ts_set, n, threshold);
  if(i != NO_ERROR){
    fprintf(stderr, "There were errors during learning:\n");
    report_error(__FILE__, __LINE__, i, 1);
    return -1;
  }
  printf("...done.\n");

  /* Write the results to a NET file */
  i =  write_model(model, argv[4]);
  if(i != NO_ERROR){
    fprintf(stderr, "Failed to write the model into %s.net\n", argv[4]);
    report_error(__FILE__, __LINE__, i, 1);
    return -1;
  }

  for(i = 0; i < n; i++)
    free_timeseries(ts_set[i]);
  free(ts_set);
  free_model(model);

  return 0;
}
