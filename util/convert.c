#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>

#include "nip.h"
#include "variable.h"

#define UNIVARIATE   1
#define MULTIVARIATE 2
#define UNARY        3
#define INVALID     99

#define S_UNIVARIATE "univariate"
#define S_MULTIVARIATE "multivariate"
#define S_UNARY "unary"

/* Converts data between various formats: 
 * currently only univariate data into unary format
 *
 * SYNOPSIS: CONVERT <MODEL.NET> <IN FORMAT> <IN.TXT> <OUT FORMAT> <OUT.TXT>
 */

/* Some experimental copy-paste from nip.c */
int write_unary_timeseries(time_series *ts_set, int n_series, char* filename){
  /* TODO */
  return 0;
}


int main(int argc, char *argv[]) {

  int i, k, n=0;
  int iformat, oformat;
  nip model = NULL;
  time_series* ts_set = NULL;

  if(argc < 6){
    printf("You must specify: \n"); 
    printf(" - the NET file for the model, \n");
    printf(" - input format ('univariate'), \n");
    printf(" - input file name, \n");
    printf(" - output format ('unary'), \n");
    printf(" - output file name, please!\n");
    return 0;
  }
  
  /* read the model */
  model = parse_model(argv[1]);
  if(!model){
    printf("Unable to parse the NET file: %s?\n", argv[1]);
    return -1;
  }
  use_priors(model, 1); /* ? */

  /* read file formats */
  if(strcmp(argv[2], S_UNIVARIATE) == 0)
    iformat = UNIVARIATE;
  /* additional formats here */
  else{
    printf("Invalid input file format: %s?\n", argv[2]);
    free_model(model);
    return -1;
  }

  if(strcmp(argv[4], S_UNARY) == 0)
    oformat = UNARY;
  /* additional formats here */
  else{
    printf("Invalid output file format: %s?\n", argv[4]);
    free_model(model);
    return -1;
  }

  /* Read the input data file */
  switch (iformat) {
  case UNIVARIATE:
  case MULTIVARIATE:
    n = read_timeseries(model, argv[3], &ts_set);
    break;
  default:
    n = 0; /* should be impossible */
  }
  if(n < 1){
    fprintf(stderr, "There were errors while reading %s\n", argv[3]);
    free_model(model);
    /* no ts_set to free (?) */
    return -1;
  }

  /* Write the results to the file */
  k = NO_ERROR;
  switch (oformat) {
  case UNARY:
    k = write_unary_timeseries(ts_set, n, argv[5]);
    break;
  default:
    ; /* shouldn't happen */
  }
  if(k != NO_ERROR){
    fprintf(stderr, "Failed to write the data into %s\n", argv[3]);
    report_error(__FILE__, __LINE__, k, 1);
    for(i = 0; i < n; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free_model(model);
    return -1;
  }

  for(i = 0; i < n; i++)
    free_timeseries(ts_set[i]);
  free(ts_set);
  free_model(model);
  return 0;
}
