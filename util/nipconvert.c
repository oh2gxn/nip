/* nipconvert.c
 *
 * SYNOPSIS: 
 * CONVERT <MODEL.NET> <IN FORMAT> <IN.TXT> <OUT FORMAT> <OUT.TXT>
 *
 * Converts data between various formats: 
 * currently only univariate data into unary format!
 *
 * EXAMPLE: ./nipconvert m.net univariate data.txt unary udata.txt
 *
 * Author: Janne Toivola
 * Version: $Id: nipconvert.c,v 1.1 2010-12-03 17:21:29 jatoivol Exp $
 */

/* Some experimental copy-paste from nip.c 
 * This one has a lot of unnecessary features and computation 
 * TODO: 
 * - move to nip.c
 * - refactor? 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>

#include "nip.h"
#include "nipvariable.h"

#define UNIVARIATE   1
#define MULTIVARIATE 2
#define UNARY        3
#define INVALID     99

#define S_UNIVARIATE "univariate"
#define S_MULTIVARIATE "multivariate"
#define S_UNARY "unary"
/* BTW: use only ASCII in these strings! */


int write_unary_timeseries(time_series *ts_set, int n_series, char* filename){
  int i, n, t;
  int d;
  int *record;
  int n_observed;
  nip_variable v;
  nip_variable *observed;
  nip_variable *observed_more;
  time_series ts;
  nip the_model;
  FILE *f = NULL;

  /* Check stuff */
  if(!(n_series > 0 && ts_set && filename)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  /* Check all the models are same */
  the_model = ts_set[0]->model;
  for(n = 1; n < n_series; n++){
    if(ts_set[n]->model != the_model){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
      return NIP_ERROR_INVALID_ARGUMENT;
    }
  }

  /* Find out union of observed variables */
  ts = ts_set[0];
  observed = nip_variable_union(ts->observed, NULL, 
				the_model->num_of_vars - ts->num_of_hidden,
				0, &n_observed);
  if(n_observed < 0){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    return NIP_ERROR_GENERAL;
  }

  for(n = 1; n < n_series; n++){
    ts = ts_set[n];
    observed_more = nip_variable_union(observed, ts->observed, n_observed,
				       the_model->num_of_vars - 
				       ts->num_of_hidden,
				       &n_observed);
    free(observed); /* nice to create the same array again and again? */
    observed = observed_more;
  }

  if(n_observed < 1){ /* no observations in any time series? */
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  /** NOTE: Actually this assumes there is only one observed variable! **/

  /* Temporary space for a sorted record (unary values) */
  v = observed[0];
  record = (int*) calloc(NIP_CARDINALITY(v), sizeof(int));
  if(!record){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(observed);
    return NIP_ERROR_OUTOFMEMORY;
  }

  /* Try to open the file for write */
  f = fopen(filename, "w");
  if(!f){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_IO, 1);
    free(observed);
    free(record);
    return NIP_ERROR_IO;
  }

  /* Write names of the variable states */
  for(i = 0; i < NIP_CARDINALITY(v); i++){
    if(i > 0)
      fprintf(f, "%c", FIELD_SEPARATOR);
    fprintf(f, "%s", nip_variable_state_name(v, i));
  }
  fputs("\n", f);

  /* Write the data */
  for(n = 0; n < n_series; n++){ /* for each time series */
    ts = ts_set[n];

    for(t = 0; t < TIME_SERIES_LENGTH(ts); t++){ /* for each time step */

      /* Fill record with indicators of missing data */
      for(i = 0; i < NIP_CARDINALITY(v); i++)
	record[i] = 0;

      /* Extract data from the time series */
      d = ts->data[t][0];
      if(d >= 0 && d < NIP_CARDINALITY(v))
	record[d] = 1;

      /* Print the data */
      for(i = 0; i < NIP_CARDINALITY(v); i++){
	if(i > 0)
	  fprintf(f, "%c", FIELD_SEPARATOR);
	if(record[i])
	  fputs("1", f);
	else
	  fputs("0", f);
      }
      fputs("\n", f);
    }
    fputs("\n", f); /* TS separator */
  }  
  free(observed);
  free(record);

  /* Close the file */
  if(fclose(f)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_IO, 1);
    return NIP_ERROR_IO;
  }
  return NIP_NO_ERROR;
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

  /* read file formats */
  /* Reminder: strcasecmp() is NOT ANSI C. */
  if(strcasecmp(argv[2], S_UNIVARIATE) == 0)
    iformat = UNIVARIATE;
  /* additional formats here */
  else{
    printf("Invalid input file format: %s?\n", argv[2]);
    free_model(model);
    return -1;
  }

  if(strcasecmp(argv[4], S_UNARY) == 0)
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
  k = NIP_NO_ERROR;
  switch (oformat) {
  case UNARY:
    k = write_unary_timeseries(ts_set, n, argv[5]);
    break;
  default:
    ; /* shouldn't happen */
  }
  if(k != NIP_NO_ERROR){
    fprintf(stderr, "Failed to write the data into %s\n", argv[5]);
    nip_report_error(__FILE__, __LINE__, k, 1);
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
