#include <stdlib.h>
#include <assert.h>
#include "parser.h"
#include "Clique.h"
#include "Variable.h"
#include "potential.h"
#include "errorhandler.h"
#include "nip.h"


int main(int argc, char *argv[]){

  char** tokens = NULL;
  int i, j, k, l, retval, t = 0;
  double** quotient = NULL;
  double*** result = NULL; /* probs of the hidden variables */

  Nip model = NULL;
  Clique clique_of_interest = NULL;

  Variable *hidden = NULL;
  Variable temp = NULL;
  Variable interesting = NULL;

  Timeseries ts = NULL;

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
  ts = read_timeseries(model, argv[2]); /* 1. Open */
  if(ts == NULL){
    free_model(model);
    report_error(__FILE__, __LINE__, ERROR_FILENOTFOUND, 1);
    fprintf(stderr, "%s\n", argv[2]);
    return -1;
  }


  /* Allocate some space for filtering */
  result = (double***) calloc(ts->datarows + 1, sizeof(double**));
  quotient = (double**) calloc(num_of_hidden, sizeof(double*));
  if(!(result && quotient)){
    free_model(model);
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return 1;
  }
  for(t = 0; t < ts->datarows + 1; t++){
    result[t] = (double**) calloc(num_of_hidden, sizeof(double*));
    if(!result[t]){
      free_model(model);
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return 1;
    }

    for(i = 0; i < num_of_hidden; i++){
      result[t][i] = (double*) calloc(number_of_values(hidden[i]), 
				      sizeof(double));
      if(!result[t][i]){
	free_model(model);
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	return 1;
      }
    }
  }
  for(i = 0; i < num_of_hidden; i++){
    quotient[i] = (double*) calloc(number_of_values(hidden[i]), 
				   sizeof(double));
    if(!quotient[i]){
      free_model(model);
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return 1;
    }
  }


  /*****************/
  /* Forward phase */
  /*****************/
  printf("## Forward phase ##\n");  

  for(t = 0; t <= ts->length; t++){ /* FOR EVERY TIMESLICE */
    
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
      
      /* 1. Decide which Variable you are interested in */
      interesting = ts->hidden[i];
      
      /* 2. Find the Clique that contains the family of 
       *    the interesting Variable */
      clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				       &interesting, 1);
      if(!clique_of_interest){
	free_model(model);
	free_timeseries(ts);
	printf("In hmmtest.c : No clique found! Sorry.\n");
	return 1;
      }  

      /* 3. Marginalisation (memory for the result must have been allocated) */
      marginalise(clique_of_interest, interesting, result[t][i]);

      /* 4. Normalisation */
      normalise(result[t][i], number_of_values(interesting));    

      /* 5. Print the result */
      for(j = 0; j < number_of_values(interesting); j++)
	printf("P(%s=%s) = %f\n", get_symbol(interesting),
	       (interesting->statenames)[j], result[t][i][j]);
      printf("\n");
    }

    if(t < ts->length){
      /* forget old evidence */
      reset_model(model);

      for(i = 0; i < ts->num_of_hidden; i++){
	/* old posteriors become new priors */
	temp = ts->hidden[i];
	if(temp->next != NULL)
	  update_likelihood(temp->next, result[t][i]);
      }

      global_retraction(model->variables, model->num_of_vars, 
			model->cliques, model->num_of_cliques);

      /* 0. Put some data in */
      for(i = 0; i < ts->num_of_nodes; i++)
	if(ts->data[t][i] >= 0)
	  enter_i_observation(model->variables, model->num_of_vars,
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

  for(t = ts->length; t >= 0; t--){ /* FOR EVERY TIMESLICE */

    printf("-- t = %d --\n", t);

    if(t > 0){
      for(i = 0; i < model->num_of_vars - ts->num_of_hidden; i++){
	temp = ts->observed[i];
	assert(temp);
	if(ts->data[t - 1][i] >= 0)
	  enter_i_observation(model->variables, model->num_of_vars, 
			      model->cliques, model->num_of_cliques, 
			      temp, 
			      ts->data[t - 1][i]);
      }

      for(i = 0; i < num_of_hidden; i++){
	temp = ts->hidden[i];
	if(temp->next != NULL)
	  enter_evidence(model->variables, model->num_of_vars, 
			 model->cliques, model->num_of_cliques, 
			 temp->next, result[t-1][i]);
      }
    }
    
    if(t < ts->length){
      
      for(i = 0; i < ts->num_of_hidden; i++){
	temp = ts->hidden[i];
	if(temp->previous != NULL){
	  /* search for the other index */
	  for(k = 0; k < ts->num_of_hidden; k++)
	    if(equal_variables(temp->previous, ts->hidden[k]))
	      break;
	  
	  /* FIXME: Get rid of the quotient array */
	  
	  for(j = 0; j < number_of_values(temp); j++)
	    quotient[i][j] = result[t + 1][i][j] / result[t][k][j]; 
	  
	  enter_evidence(model->variables, model->num_of_vars, 
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
      
      /* 1. Decide which Variable you are interested in */
      interesting = ts->hidden[i];
      
      /* 2. Find the Clique that contains the family of 
       *    the interesting Variable */
      clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				       &interesting, 1);
      if(!clique_of_interest){
	free_model(model);
	free_timeseries(ts);
	printf("In hmmtest.c : No clique found! Sorry.\n");
	return 1;
      }  
      
      /* 3. Marginalisation (the memory must have been allocated) */
      marginalise(clique_of_interest, interesting, result[t][i]);
      
      /* 4. Normalisation */
      normalise(result[t][i], number_of_values(interesting));
      
      /* 5. Print the result */
      for(j = 0; j < number_of_values(interesting); j++)
	printf("P(%s=%s) = %f\n", get_symbol(interesting),
	       (interesting->statenames)[j], result[t][i][j]);
      printf("\n");
    }
    
    /* forget old evidence */
    reset_model(model);
    
  }
  
  for(t = 0; t < ts->length + 1; t++){
    for(i = 0; i < ts->num_of_hidden; i++)
      free(result[t][i]);
    free(result[t]);
  }
  for(i = 0; i < ts->num_of_hidden; i++)
    free(quotient[i]);

  free(result);
  free(quotient);

  free_timeseries(ts);

  free_model(model);

  return 0;
}
