#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "parser.h"
#include "Clique.h"
#include "Variable.h"
#include "potential.h"
#include "errorhandler.h"
#include "nip.h"

/***********************************************************
 * The timeslice concept features some major difficulties 
 * because the actual calculations are done in the join tree
 * instead of the graph. The program should be able to 
 * figure out how the join tree repeats itself and store 
 * some kind of sepsets between the timeslices... Note that 
 * there can be only one sepset between two adjacent 
 * timeslices, because the join tree can't have loops. This 
 * implies that the variables, which have links to the 
 * variables in the next timeslice, can be found in the 
 * same clique.
 */

/*
#define TEST
#define TEST2
*/

int main(int argc, char *argv[]){

  char** tokens = NULL;
  int* cardinalities = NULL;
  int* temp_vars = NULL;
  int i, j, k, m, n, retval, t = 0;
  double** result; /* probs of the hidden variables */  

  Nip model = NULL;
  Clique clique_of_interest = NULL;
  
  potential *timeslice_sepsets = NULL;
  /* for reordering sepsets between timeslices
     potential temp_potential;
     potential reordered;
  */
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
  ts = read_timeseries(model, argv[2]);
  if(ts == NULL){
    report_error(__FILE__, __LINE__, ERROR_FILENOTFOUND, 1);
    fprintf(stderr, "%s\n", argv[2]);
    free_model(model);
    return -1;
  }


  /* Allocate an array */
  if(ts->num_of_nexts > 0){
    cardinalities = (int*) calloc(ts->num_of_nexts, sizeof(int));
    if(!cardinalities){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free_model(model);
      free_timeseries(ts);
      return -1;
    }
  }

  /* Fill the array */
  k = 0;
  for(i = 0; i < ts->num_of_hidden; i++){
    temp = ts->hidden[i];
    if(temp->next)
      cardinalities[k++] = number_of_values(temp);
  }
  

  /* Allocate some space for filtering */
  result = (double**) calloc(ts->num_of_hidden, sizeof(double*));
  if(!result){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return 1;
  }
  
  for(i = 0; i < ts->num_of_hidden; i++){
    result[i] = (double*) calloc(number_of_values(ts->hidden[i]), 
				 sizeof(double));
    if(!result[i]){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return 1;
    }
  }


  /* Allocate some space for the intermediate potentials */
  timeslice_sepsets = (potential *) calloc(ts->length + 1, sizeof(potential));

  /* Initialise intermediate potentials */
  for(t = 0; t <= ts->length; t++){
    timeslice_sepsets[t] = make_potential(cardinalities, ts->num_of_nexts, 
					  NULL);
  }
  free(cardinalities);


  /* FIX ME: there's a bug somewhere!!! */
  /* Nope... It's a feature. */


  /*****************/
  /* Forward phase */
  /*****************/

  /* Try this kind of iteration:
   * + put data in
   * - pass the message from previous sepset to the next one
   * - print the result
   * - reset model
   */


  printf("## Forward phase ##\n");  

  for(t = 0; t < ts->length; t++){ /* FOR EVERY TIMESLICE */
    
    printf("-- t = %d --\n", t+1);


    /* Put some data in */
    for(i = 0; i < model->num_of_vars - ts->num_of_hidden; i++)
      if(ts->data[t][i] >= 0)
	enter_i_observation(model->variables, model->num_of_vars, 
			    model->cliques, model->num_of_cliques, 
			    ts->observed[i], ts->data[t][i]);


    if(t > 0){
      /* Finish the message pass between timeslices */
      clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				       model->previous, model->num_of_nexts);
      assert(clique_of_interest != NULL);
      temp_vars = (int*) calloc(clique_of_interest->p->num_of_vars - 
				model->num_of_nexts, sizeof(int));
      if(!temp_vars){
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	return 1;
      }
      j = 0; k = 0;
      for(i=0; i < clique_of_interest->p->num_of_vars; i++){
	if(j < model->num_of_nexts &&
	   equal_variables((clique_of_interest->variables)[i], 
			   model->previous[j]))
	  j++;
	else {
	  temp_vars[k] = i;
	  k++;
	}
      }
      update_potential(timeslice_sepsets[t - 1], NULL, 
		       clique_of_interest->p, temp_vars);
      free(temp_vars);
    }


    

    /* Do the inference */
    make_consistent(model);




    /* Print some intermediate results */
    for(i = 0; i < ts->num_of_hidden; i++){
      
      /*********************************/
      /* Check the result of inference */
      /*********************************/

      interesting = ts->hidden[i];
      
      /* 1. Find the Clique that contains the family of 
       * the interesting Variables */
      clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				       &interesting, 1);
      assert(clique_of_interest != NULL);
      
      /* 2. Marginalisation (memory for the result must have been allocated) */
      marginalise(clique_of_interest, interesting, result[i]);
      
      /* 3. Normalisation */
      normalise(result[i], number_of_values(interesting));    
      
      /* 4. Print the result */
      for(j = 0; j < number_of_values(interesting); j++)
	printf("P(%s=%s) = %f\n", get_symbol(interesting),
	       (interesting->statenames)[j], result[i][j]);
      printf("\n");

    }


    
    
    /* Start a message pass between timeslices */
    /* NOTE: Let's hope the "next" variables are in the same clique! */
    clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				     model->next, model->num_of_nexts);
    assert(clique_of_interest != NULL);
    temp_vars = (int*) calloc(clique_of_interest->p->num_of_vars - 
			      model->num_of_nexts, sizeof(int));
    if(!temp_vars){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return 1;
    }
    /* NOTE: Let's hope the "next" variables are in correct order! */
    m = 0;
    n = 0;
    for(j=0; j < clique_of_interest->p->num_of_vars; j++){
      if(m < model->num_of_nexts &&
	 equal_variables((clique_of_interest->variables)[j], 
			 model->next[m])) /* ? */
	m++;
      else {
	temp_vars[n] = j;
	n++;
      }
    }
    general_marginalise(clique_of_interest->p, timeslice_sepsets[t],
			temp_vars);
    free(temp_vars);
    


    /* Forget old evidence */
    reset_model(model);
  }
  
  
  
  
  /******************/
  /* Backward phase */
  /******************/

  printf("## Backward phase ##\n");  
  
  /* forget old evidence */
  reset_model(model);
  
  
  for(t = ts->length - 1; t >= 0; t--){ /* FOR EVERY TIMESLICE */
    
    printf("-- t = %d --\n", t+1);


#ifdef TEST2
    if(t == ts->length - 1){ /* The last timeslice */
#endif
    /* Put some evidence in */
    for(i = 0; i < ts->num_of_nodes; i++)
      if(ts->data[t][i] >= 0)
	enter_i_observation(model->variables, model->num_of_vars, 
			    model->cliques, model->num_of_cliques, 
			    ts->observed[i], ts->data[t][i]);
#ifdef TEST2
    }
#endif
    

#ifdef TEST
    if(t == ts->length - 1){ /* The last timeslice */
#endif
    /* Pass the message from the past */
    if(t > 0){
      clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				       model->previous, model->num_of_nexts);
      assert(clique_of_interest != NULL);
      temp_vars = (int*) calloc(clique_of_interest->p->num_of_vars - 
				model->num_of_nexts, sizeof(int));
      if(!temp_vars){
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	return 1;
      }
      j = 0; k = 0;
      for(i=0; i < clique_of_interest->p->num_of_vars; i++){
	if(j < model->num_of_nexts &&
	   equal_variables((clique_of_interest->variables)[i], 
			   model->previous[j]))
	  j++;
	else {
	  temp_vars[k] = i;
	  k++;
	}
      }
      update_potential(timeslice_sepsets[t-1], NULL,
		       clique_of_interest->p, temp_vars);
      
      free(temp_vars);
    }
#ifdef TEST
    }
#endif


    /* a useful inference ? */
    /*make_consistent(model);*/

  

    /* Pass the message from the future */
    if(t < ts->length - 1){
      clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				       model->next, model->num_of_nexts);
      assert(clique_of_interest != NULL);
      temp_vars = (int*) calloc(clique_of_interest->p->num_of_vars - 
				model->num_of_nexts, sizeof(int));
      if(!temp_vars){
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	return 1;
      }
      j = 0; k = 0;
      for(i=0; i < clique_of_interest->p->num_of_vars; i++){
	if(j < model->num_of_nexts && 
	   equal_variables((clique_of_interest->variables)[i], model->next[j]))
	  j++;
	else
	  temp_vars[k++] = i;
      }
#ifdef TEST
      update_potential(timeslice_sepsets[t+1], NULL,
		       clique_of_interest->p, temp_vars);
#else
      update_potential(timeslice_sepsets[t+1], timeslice_sepsets[t],
		       clique_of_interest->p, temp_vars);
#endif
      free(temp_vars);
    }



    /* Do the inference */
    make_consistent(model);
    
    

    /* Print some final results */
    for(i = 0; i < ts->num_of_hidden; i++){
      
      /* 1. Decide which Variable you are interested in */
      interesting = ts->hidden[i];
      
      /* 2. Find the Clique that contains the family of 
       *    the interesting Variable */
      clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				       &interesting, 1);
      assert(clique_of_interest != NULL);
      
      /* 3. Marginalisation (the memory must have been allocated) */
      marginalise(clique_of_interest, interesting, result[i]);
      
      /* 4. Normalisation */
      normalise(result[i], number_of_values(interesting));
      
      /* 5. Print the result */
      for(j = 0; j < number_of_values(interesting); j++)
	printf("P(%s=%s) = %f\n", get_symbol(interesting),
	       (interesting->statenames)[j], result[i][j]);
      printf("\n");
    }
    


    if(t > 0){
      /* Start a message pass between timeslices */
      clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				       model->previous, model->num_of_nexts);
      assert(clique_of_interest != NULL);
      temp_vars = (int*) calloc(clique_of_interest->p->num_of_vars - 
				model->num_of_nexts, sizeof(int));
      if(!temp_vars){
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	return 1;
      }
      /* NOTE: Let's hope the "previous" variables are in correct order */
      m = 0;
      n = 0;
      for(j=0; j < clique_of_interest->p->num_of_vars; j++){
	if(m < model->num_of_nexts &&
	   equal_variables((clique_of_interest->variables)[j], 
			   model->previous[m]))
	  m++;
	else {
	  temp_vars[n] = j;
	  n++;
	}
      }
      general_marginalise(clique_of_interest->p, timeslice_sepsets[t],
			  temp_vars);
      free(temp_vars);
    }


    /* forget old evidence */
    reset_model(model);
  }



  free_model(model);

  for(i = 0; i < ts->num_of_hidden; i++)
    free(result[i]);
  free(result);

  for(t = 0; t <= ts->length; t++)
    free_potential(timeslice_sepsets[t]);
  free(timeslice_sepsets);

  free_timeseries(ts);
  
  return 0;
}
