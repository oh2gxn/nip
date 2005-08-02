/* em_test.c
 *
 * Reads the structure for the model from a Hugin NET file, 
 * uses EM-algorithm for learning the parameters from the 
 * given data file and writes the estimated model to the 
 * specified output file. 
 *
 * SYNOPSIS: EM_TEST <ORIGINAL.NET> <DATA.TXT> <THRESHOLD> <RESULT>
 * (resulting model will be written to the file <RESULT>.net) 
 *
 * EXAMPLE: ./em_test model1.net data.txt 0.00001 model2
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "nip.h"
#include "variable.h"

int main(int argc, char *argv[]) {

  int i, n;
  nip model = NULL;
  time_series *ts_set = NULL;
  time_series ts;
  double threshold = 0;
  char* tailptr = NULL;

  if(argc < 5){
    printf("You must specify: \n"); 
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

  /* print a summary about the variables */
  ts = ts_set[0];
  printf("Hidden variables are:\n");
  for(i = 0; i < ts->num_of_hidden; i++)
    printf("  %s", get_symbol(ts->hidden[i]));
  printf("\nObserved variables are:\n");
  for(i = 0; i < model->num_of_vars - ts->num_of_hidden; i++)
    printf("  %s", get_symbol(ts->observed[i]));
  printf("\n");

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
