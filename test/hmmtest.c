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

/* hmmtest.c
 *
 * Experimental time slice stuff for Hidden Markov Models (HMM).
 * Prints a MAP estimate for the hidden variable during the first
 * time series in the given data file. 
 *
 * Author: Janne Toivola
 * Version: $Id: hmmtest.c,v 1.52 2010-12-07 17:23:19 jatoivol Exp $
 */

#include <stdlib.h>
#include <assert.h>
#include "nipparsers.h"
#include "nipjointree.h"
#include "nipvariable.h"
#include "nippotential.h"
#include "niperrorhandler.h"
#include "nip.h"


int main(int argc, char *argv[]){

  int i, j, k, n, t = 0;
  double** quotient = NULL;
  double*** result = NULL; /* probs of the hidden variables */

  nip model = NULL;
  nip_clique clique_of_interest = NULL;

  nip_variable temp = NULL;
  nip_variable interesting = NULL;

  time_series *ts_set = NULL;
  time_series ts = NULL;

  /*************************************/
  /* Some experimental timeslice stuff */
  /*************************************/

  /*****************************************/
  /* Parse the model from a Hugin NET file */
  /*****************************************/

  /* -- Start parsing the network definition file */
  if(argc < 3){
    printf("Give the names of the net-file and data file, please!\n");
    return 0;
  }
  else
    model = parse_model(argv[1]);
    
  if(model == NULL)
    return -1;
  /* The input file has been parsed. -- */


  /*****************************/
  /* read the data from a file */
  /*****************************/
  n = read_timeseries(model, argv[2], &ts_set); /* 1. Open */
  if(n == 0){
    free_model(model);
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    fprintf(stderr, "%s\n", argv[2]);
    return -1;
  }
  ts = ts_set[0];

  /* Allocate some space for filtering */
  assert(ts->num_of_hidden > 0);
  result = (double***) calloc(TIME_SERIES_LENGTH(ts) + 1, sizeof(double**));
  quotient = (double**) calloc(ts->num_of_hidden, sizeof(double*));
  if(!(result && quotient)){
    free_model(model);
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return 1;
  }
  for(t = 0; t < TIME_SERIES_LENGTH(ts) + 1; t++){
    result[t] = (double**) calloc(ts->num_of_hidden, sizeof(double*));
    if(!result[t]){
      free_model(model);
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      return 1;
    }

    for(i = 0; i < ts->num_of_hidden; i++){
      result[t][i] = (double*) calloc(NIP_CARDINALITY(ts->hidden[i]), 
				      sizeof(double));
      if(!result[t][i]){
	free_model(model);
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
	return 1;
      }
    }
  }
  for(i = 0; i < ts->num_of_hidden; i++){
    quotient[i] = (double*) calloc(NIP_CARDINALITY(ts->hidden[i]), 
				   sizeof(double));
    if(!quotient[i]){
      free_model(model);
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      return 1;
    }
  }


  /*****************/
  /* Forward phase */
  /*****************/
  printf("## Forward phase ##\n");  

  reset_model(model); /* Reset the clique tree */
  use_priors(model, !NIP_HAD_A_PREVIOUS_TIMESLICE);

  for(t = 0; t < TIME_SERIES_LENGTH(ts); t++){ /* FOR EVERY TIMESLICE */
    
    printf("-- t = %d --\n", t);
        
    /********************/
    /* Do the inference */
    /********************/
    
    make_consistent(model);
    
    
    /* an experimental forward phase (a.k.a. filtering)... */
    /* Calculates the result values */
    for(i = 0; i < ts->num_of_hidden; i++){
      
      /*********************************/
      /* Check the result of inference */
      /*********************************/
      
      /* 1. Decide which variable you are interested in */
      interesting = ts->hidden[i];
      
      /* 2. Find the clique that contains the family of 
       *    the interesting variable */
      clique_of_interest = nip_find_family(model->cliques, 
					   model->num_of_cliques, 
					   interesting);
      if(!clique_of_interest){
	free_model(model);
	free_timeseries(ts);
	printf("In hmmtest.c : No clique found! Sorry.\n");
	return 1;
      }  

      /* 3. Marginalisation (memory for the result must have been allocated) */
      nip_marginalise_clique(clique_of_interest, interesting, result[t][i]);

      /* 4. Normalisation */
      nip_normalise_array(result[t][i], NIP_CARDINALITY(interesting));

      /* 5. Print the result */
      for(j = 0; j < NIP_CARDINALITY(interesting); j++)
	printf("P(%s=%s) = %f\n", nip_variable_symbol(interesting),
	       (interesting->state_names)[j], result[t][i][j]);
      printf("\n");
    }

    if(t < TIME_SERIES_LENGTH(ts)){
      /* forget old evidence */
      reset_model(model);
      use_priors(model, NIP_HAD_A_PREVIOUS_TIMESLICE);

      for(i = 0; i < ts->num_of_hidden; i++){
	/* old posteriors become new priors */
	temp = ts->hidden[i];
	if(temp->next != NULL)
	  nip_update_likelihood(temp->next, result[t][i]);
      }

      nip_global_retraction(model->variables, model->num_of_vars, 
			    model->cliques, model->num_of_cliques);

      /* 0. Put some data in */
      for(i = 0; i < model->num_of_vars - ts->num_of_hidden; i++)
	if(ts->data[t][i] >= 0)
	  nip_enter_index_observation(model->variables, model->num_of_vars,
				      model->cliques, model->num_of_cliques,
				      ts->observed[i],
				      ts->data[t][i]);
    }
  }


  /******************/
  /* Backward phase */
  /******************/

  printf("## Backward phase ##\n");  

  /* forget old evidence */
  reset_model(model);
  use_priors(model, NIP_HAD_A_PREVIOUS_TIMESLICE); /* JJT: Not sure... */

  for(t = TIME_SERIES_LENGTH(ts)-1; t >= 0; t--){ /* FOR EVERY TIMESLICE */

    printf("-- t = %d --\n", t);

    if(t > 0){
      for(i = 0; i < model->num_of_vars - ts->num_of_hidden; i++){
	temp = ts->observed[i];
	assert(temp);
	if(ts->data[t - 1][i] >= 0)
	  nip_enter_index_observation(model->variables, model->num_of_vars, 
				      model->cliques, model->num_of_cliques, 
				      temp, 
				      ts->data[t - 1][i]);
      }

      for(i = 0; i < ts->num_of_hidden; i++){
	temp = ts->hidden[i];
	if(temp->next != NULL)
	  nip_enter_evidence(model->variables, model->num_of_vars, 
			     model->cliques, model->num_of_cliques, 
			     temp->next, result[t-1][i]);
      }
    }

    if(t < TIME_SERIES_LENGTH(ts)){
      
      for(i = 0; i < ts->num_of_hidden; i++){
	temp = ts->hidden[i];
	if(temp->previous != NULL){
	  /* search for the other index */
	  for(k = 0; k < ts->num_of_hidden; k++)
	    if(nip_equal_variables(temp->previous, ts->hidden[k]))
	      break;
	  
	  /* FIXME: Get rid of the quotient array */
	  printf("result[%d][%d][%d] / result[%d][%d][%d]\n", 
		 t+1, i, j, t, k, j);

	  for(j = 0; j < NIP_CARDINALITY(temp); j++)
	    quotient[i][j] = result[t + 1][i][j] / result[t][k][j]; 
	  
	  nip_enter_evidence(model->variables, model->num_of_vars, 
			     model->cliques, model->num_of_cliques, 
			     temp->previous, quotient[i]);
	}
      }
    }
    
    /********************/
    /* Do the inference */
    /********************/
    
    make_consistent(model);
    
    /*********************************/
    /* Check the result of inference */
    /*********************************/
    for(i = 0; i < ts->num_of_hidden; i++){
      
      /* 1. Decide which variable you are interested in */
      interesting = ts->hidden[i];
      
      /* 2. Find the clique that contains the family of 
       *    the interesting variable */
      clique_of_interest = nip_find_family(model->cliques, 
					   model->num_of_cliques, 
					   interesting);
      if(!clique_of_interest){
	free_model(model);
	free_timeseries(ts);
	printf("In hmmtest.c : No clique found! Sorry.\n");
	return 1;
      }  
      
      /* 3. Marginalisation (the memory must have been allocated) */
      nip_marginalise_clique(clique_of_interest, interesting, result[t][i]);
      
      /* 4. Normalisation */
      nip_normalise_array(result[t][i], NIP_CARDINALITY(interesting));
      
      /* 5. Print the result */
      for(j = 0; j < NIP_CARDINALITY(interesting); j++)
	printf("P(%s=%s) = %f\n", nip_variable_symbol(interesting),
	       (interesting->state_names)[j], result[t][i][j]);
      printf("\n");
    }
    
    /* forget old evidence */
    reset_model(model);
    use_priors(model, NIP_HAD_A_PREVIOUS_TIMESLICE);
  }
  
  for(t = 0; t < TIME_SERIES_LENGTH(ts) + 1; t++){
    for(i = 0; i < ts->num_of_hidden; i++)
      free(result[t][i]);
    free(result[t]);
  }
  for(i = 0; i < ts->num_of_hidden; i++)
    free(quotient[i]);

  free(result);
  free(quotient);

  for(i = 0; i < n; i++)
    free_timeseries(ts_set[i]);
  free(ts_set);
  
  free_model(model);

  return 0;
}
