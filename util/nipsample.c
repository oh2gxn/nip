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

/* nipsample.c
 * 
 * Samples data according to a given model.
 *
 * SYNOPSIS: 
 * NIPSAMPLE <MODEL.NET> <SERIES> <SAMPLES> <RESULT.TXT>
 *
 * - Structure of the model is read from the file <MODEL.NET>
 * - integer <SERIES> specifies the number of time series
 * - integer <SAMPLES> specifies how many samples for each time series
 * - resulting data will be written to the file <RESULT.TXT>
 *
 * EXAMPLE: ./nipsample weather.net 52 7 year.txt  
 *
 * Author: Janne Toivola
 * Version: $Id: nipsample.c,v 1.1 2010-12-03 17:21:29 jatoivol Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>

#include "nip.h"
#include "nipvariable.h"


int main(int argc, char *argv[]) {

  int i, n, t;
  nip_model model = NULL;
  time_series *ts_set = NULL;
  time_series ts = NULL;
  double d = 0;
  char* tailptr = NULL;
  long seed;

  /** <Some experimental code> **/
  ;
  /** </Some experimental code>**/

  printf("nipsample:\n");

  if(argc < 5){
    printf("You must specify: \n"); 
    printf(" - the NET file for the model, \n");
    printf(" - number of time series, \n");
    printf(" - time series length, \n"); 
    printf(" - the resulting data file, please!\n");
    return 0;
  }
  
  /* read the model */
  model = parse_model(argv[1]);
  if(!model){
    printf("  Unable to parse the NET file: %s?\n", argv[1]);
    return -1;
  }

  /* read how many time series to generate */
  d = strtod(argv[2], &tailptr);
  n = (int)d;
  if(d <= 0 || d > 1000000  || tailptr == argv[2]){
    printf("  Invalid number of time series: %s?\n", argv[2]);
    free_model(model);
    return -1;
  }

  /* read the time series length */
  d = strtod(argv[3], &tailptr);
  t = (int)d;
  if(d <= 0 || d > 1000000  || tailptr == argv[3]){
    printf("  Invalid time series length: %s?\n", argv[3]);
    free_model(model);
    return -1;
  }

  /* THE algorithm (may take a while) */
  printf("  Generating data... \n");

  seed = random_seed(NULL);
  printf("  Random seed = %ld\n", seed);

  ts_set = (time_series*) calloc(n, sizeof(time_series));
  if(!ts_set){
    fprintf(stderr, "Ran out of memory!\n");    
    free_model(model);
    return -1;
  }
  for(i = 0; i < n; i++){
    ts = generate_data(model, t);
    if(!ts){
      fprintf(stderr, "There were errors during data sampling!\n");
      while(i > 0)
	free_timeseries(ts_set[--i]);
      free(ts_set);
      free_model(model);
      return -1;
    }
    ts_set[i] = ts;
  }
  printf("  ...done.\n");

  /* Write the results to the file */
  i =  write_timeseries(ts_set, n, argv[4]);
  if(i != NIP_NO_ERROR){
    fprintf(stderr, "Failed to write the data into %s\n", argv[4]);
    nip_report_error(__FILE__, __LINE__, i, 1);
    while(n > 0)
      free_timeseries(ts_set[--n]);
    free(ts_set);
    free_model(model);
    return -1;
  }

  while(n > 0)
    free_timeseries(ts_set[--n]);
  free(ts_set);
  free_model(model);
  return 0;
}
