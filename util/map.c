/* maptest.c
 * 
 * Computes the Maximum A Posteriori (MAP) estimate for the values 
 * of hidden variables in a time series. You have to specify net file 
 * describing the model and data file containing the data for the 
 * observed variables. 
 *
 * SYNOPSIS: MAPTEST <MODEL.NET> <INPUT_DATA.TXT> <OUTPUT_DATA.TXT>
 *
 * EXAMPLE: ./maptest filter.net data.txt filtered_data.txt
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "parser.h"
#include "clique.h"
#include "nipvariable.h"
#include "potential.h"
#include "niperrorhandler.h"
#include "nip.h"

/* contains some copy-paste from write_timeseries in nip.c */

int main(int argc, char *argv[]){

  int i, j, k, n, n_max, t = 0;
  double m, m_max;
  FILE *f = NULL;

  nip model = NULL;
  nip_variable temp = NULL;

  time_series ts = NULL;
  time_series *ts_set = NULL;
  uncertain_series ucs = NULL;

  /*****************************************/
  /* Parse the model from a Hugin NET file */
  /*****************************************/
  if(argc < 4){
    printf("Specify the names of the net file and input/output data files.\n");
    return 0;
  }
  else{
    model = parse_model(argv[1]);
  }

  if(model == NULL)
    return -1;

  /*****************************/
  /* read the data from a file */
  /*****************************/
  n_max = read_timeseries(model, argv[2], &ts_set);

  if(n_max < 1){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    fprintf(stderr, "%s\n", argv[2]);
    free_model(model);
    return -1;
  }

  ts = ts_set[0];

  /************************/
  /* open the output file */
  /************************/
  f = fopen(argv[3], "w");
  if(!f){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_IO, 1);
    for(i = 0; i < n_max; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free_model(model);
    return -1;
  }

  if(ts->num_of_hidden == 0){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    fprintf(stderr, "No hidden variables to estimate.\n");
    for(i = 0; i < n_max; i++)
      free_timeseries(ts_set[i]);
    free(ts_set);
    free_model(model);
    return -1;
  }

  /** DEBUG **/
  printf("Observed variables:\n  ");
  for(i = 0; i < model->num_of_vars - ts->num_of_hidden; i++)
    printf("%s ", ts->observed[i]->symbol);
  printf("\nHidden variables:\n  ");
  for(i = 0; i < ts->num_of_hidden; i++)
    printf("%s ", ts->hidden[i]->symbol);
  printf("\n");

  /* Use all the data (mark all variables) */
  for(i = 0; i < model->num_of_vars; i++)
    nip_mark_variable(model->variables[i]);

  /* TODO: replace with the new write_timeseries? */

  /* Write names of the variables */
  fprintf(f, "%s", nip_variable_symbol(ts->hidden[0]));
  for(i = 1; i < ts->num_of_hidden; i++){
    temp = ts->hidden[i];
    fprintf(f, ", %s", nip_variable_symbol(temp));
  }
  fputs("\n", f);

  /**************************************/
  /* The inference for each time series */
  /**************************************/
  printf("## Computing ##\n");  

  for(n = 0; n < n_max; n++){

    printf("Time series %d of %d\r               ", n+1, n_max);

    /* select time series */
    ts = ts_set[n];

    /* the computation of posterior probabilities */
    ucs = forward_backward_inference(ts, ts->hidden, ts->num_of_hidden, NULL);
    
    for(t = 0; t < UNCERTAIN_SERIES_LENGTH(ucs); t++){ /* FOR EACH TIMESLICE */
      
      /* Print the final results */
      for(i = 0; i < ucs->num_of_vars - 1; i++){
	temp = ucs->variables[i];
	
	/* Find the MAP value */
	k = 0; m_max = 0;
	for(j = 0; j < NIP_CARDINALITY(temp); j++){
	  m = ucs->data[t][i][j];
	  if(m > m_max){
	    m_max = m;
	    k = j;
	  }
	}
	fprintf(f, "%s, ", (temp->state_names)[k]);
      }

      i = ucs->num_of_vars - 1;
      /* some copy-paste code for the last variable */
      temp = ucs->variables[i];	
      k = 0; m_max = 0;
      for(j = 0; j < NIP_CARDINALITY(temp); j++){
	m = ucs->data[t][i][j];
	if(m > m_max){
	  m_max = m;
	  k = j;
	}
      }
      fprintf(f, "%s \n", (temp->state_names)[k]);
    }
    fputs("\n", f); /* space between time series */
    free_uncertainseries(ucs); /* remember to free ucs */
  }

  printf("\n"); /* new line for the prompt */

  /* close the file */
  if(fclose(f)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_IO, 1);
    return -1;
  }

  /* free some memory */
  for(i = 0; i < n_max; i++)
    free_timeseries(ts_set[i]);
  free(ts_set);
  free_model(model);
  
  return 0;
}
