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


int main(int argc, char *argv[]){

  char** tokens = NULL;
  int** data = NULL;
  int* cardinalities = NULL;
  int* temp_vars = NULL;
  int i, j, k, m, n, retval, t = 0;
  int num_of_hidden = 0;
  int num_of_nexts = 0;
  double** result; /* probs of the hidden variables */


  int kludge_card[] = {3, 3};
  double kludge[] = {0.1283333333, 0.0650, 0.8066666667, 
                     0.9033333333, 0.0566666667, 0.0400,
                     0.0793333333, 0.5396666667, 0.3810};
  potential kludge_pot;
  Variable kludge_vars[2];
  

  Nip model = NULL;
  Clique clique_of_interest = NULL;
  
  potential *timeslice_sepsets = NULL;
  /* for reordering sepsets between timeslices
     potential temp_potential;
     potential reordered;
  */
  Variable *hidden = NULL;
  Variable *next = NULL;
  Variable *previous = NULL;
  Variable temp = NULL;
  Variable interesting = NULL;
  Variable_iterator it = NULL;

  datafile* timeseries = NULL;

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




  /* Some kludge stuff to test a hypothesis */
  if(strcmp(argv[1], "htm_timeslice.net") == 0){
    printf(" ** This is only a test! ** Some kludge added! **\n");
    kludge_vars[0] = get_Variable(model, "B0");
    kludge_vars[1] = get_Variable(model, "S0");
    kludge_pot = make_potential(kludge_card, 2, kludge);
    clique_of_interest = find_family(model->cliques, model->num_of_cliques,
				     kludge_vars, 2);
    initialise(clique_of_interest, kludge_vars[0], kludge_vars + 1, 
	       kludge_pot, 1); /* FIXME: How to prevent the persistency of
				* initialisation?  Multidimensional evidence?*/
  }
  /* EOK: End of kludge */




  /*****************************/
  /* read the data from a file */
  /*****************************/
  timeseries = open_datafile(argv[2], ',', 0, 1);
  if(timeseries == NULL){
    report_error(__FILE__, __LINE__, ERROR_FILENOTFOUND, 1);
    fprintf(stderr, "%s\n", argv[2]);
    return -1;
  }


  /* Figure out the number of hidden variables and variables 
   * that substitute some other variable in the next timeslice. */
  it = model->first_var;
  temp = next_Variable(&it);
  while(temp != NULL){
    j = 1;
    for(i = 0; i < timeseries->num_of_nodes; i++)
      if(equal_variables(temp, get_Variable(model, 
					    timeseries->node_symbols[i])))
	j = 0;
    if(j)
      num_of_hidden++;

    if(temp->next)
      num_of_nexts++;

    temp = next_Variable(&it);
  }

  assert(num_of_hidden == (model->num_of_vars - timeseries->num_of_nodes));

  /* Allocate arrays for hidden variables. */
  hidden = (Variable *) calloc(num_of_hidden, sizeof(Variable));
  if(!hidden){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return 1;
  }

  if(num_of_nexts > 0){

    next = (Variable *) calloc(num_of_nexts, sizeof(Variable));
    if(!next){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free(hidden);
      return 1;
    }

    previous = (Variable *) calloc(num_of_nexts, sizeof(Variable));
    if(!previous){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free(hidden);
      free(next);
      return 1;
    }

    cardinalities = (int*) calloc(num_of_nexts, sizeof(int));
    if(!cardinalities){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free(hidden);
      free(next);
      free(previous);
      return 1;
    }
  }

  /* Fill the arrays */
  it = model->first_var;
  temp = next_Variable(&it);
  k = 0;
  m = 0;
  n = 0;
  while(temp != NULL){
    j = 1;
    for(i = 0; i < timeseries->num_of_nodes; i++)
      if(equal_variables(temp, get_Variable(model,
					    timeseries->node_symbols[i]))){
	j = 0;
	break;
      }
    if(j)
      hidden[k++] = temp;

    if(temp->next){
      next[m] = temp;
      previous[m] = temp->next;
      cardinalities[m] = number_of_values(temp);
      m++;
    }

    temp = next_Variable(&it);
  }
  

  /* Check one little detail :) */
  j = 1;
  for(i = 1; i < num_of_nexts; i++)
    if(get_id(previous[i-1]) > get_id(previous[i]))
      j = 0;
  assert(j);


  /* Allocate some space for data */
  data = (int**) calloc(timeseries->datarows, sizeof(int*));
  if(!data){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return 1;
  }
  for(t = 0; t < timeseries->datarows; t++){
    data[t] = (int*) calloc(timeseries->num_of_nodes, sizeof(int));
    if(!(data[t])){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return 1;
    }
  }


  /* Allocate some space for filtering */
  result = (double**) calloc(num_of_hidden, sizeof(double*));
  if(!result){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return 1;
  }
  
  for(i = 0; i < num_of_hidden; i++){
    result[i] = (double*) calloc(number_of_values(hidden[i]), sizeof(double));
    if(!result[i]){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return 1;
    }
  }
  
  
  
  /* Fill the data array */
  for(t = 0; t < timeseries->datarows; t++){
    retval = nextline_tokens(timeseries, ',', &tokens); /* 2. Read */
    assert(retval == timeseries->num_of_nodes);

    /* 3. Put into the data array */
    for(i = 0; i < retval; i++){
      data[t][i] = 
	get_stateindex(get_Variable(model, 
				    timeseries->node_symbols[i]), 
		       tokens[i]);

      /* Q: Should missing data be allowed?   A: Yes. */
      /* assert(data[t][i] >= 0); */
    }

    for(i = 0; i < retval; i++) /* 4. Dump away */
      free(tokens[i]);
  }


  /* Allocate some space for the intermediate potentials */
  timeslice_sepsets = (potential *) calloc(timeseries->datarows + 1, 
					   sizeof(potential));

  /* Initialise intermediate potentials */
  for(t = 0; t <= timeseries->datarows; t++){
    timeslice_sepsets[t] = make_potential(cardinalities, num_of_nexts, NULL);
  }
  free(cardinalities);


      /**************************************/
      /* FIX ME: there's a bug somewhere!!! */
      /**************************************/


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

  for(t = 0; t < timeseries->datarows; t++){ /* FOR EVERY TIMESLICE */
    
    printf("-- t = %d --\n", t+1);


    /* Put some data in */
    for(i = 0; i < timeseries->num_of_nodes; i++)
      if(data[t][i] >= 0)
	enter_i_observation(model->first_var, model->cliques, 
			    model->num_of_cliques, 
			    get_Variable(model, 
					 timeseries->node_symbols[i]), 
			    data[t][i]);



    if(t > 0){
      /* Finish the message pass between timeslices */
      clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				       previous, num_of_nexts);
      assert(clique_of_interest != NULL);
      temp_vars = (int*) calloc(clique_of_interest->p->num_of_vars - 
				num_of_nexts, sizeof(int));
      if(!temp_vars){
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	return 1;
      }
      j = 0; k = 0;
      for(i=0; i < clique_of_interest->p->num_of_vars; i++){
	if(j < num_of_nexts &&
	   equal_variables((clique_of_interest->variables)[i], previous[j]))
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
    for(i = 0; i < num_of_hidden; i++){
      
      /*********************************/
      /* Check the result of inference */
      /*********************************/

      interesting = hidden[i];
      
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
				     next, num_of_nexts);
    assert(clique_of_interest != NULL);
    temp_vars = (int*) calloc(clique_of_interest->p->num_of_vars - 
			      num_of_nexts, sizeof(int));
    if(!temp_vars){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return 1;
    }
    /* NOTE: Let's hope the "next" variables are in correct order! */
    m = 0;
    n = 0;
    for(j=0; j < clique_of_interest->p->num_of_vars; j++){
      if(m < num_of_nexts &&
	 equal_variables((clique_of_interest->variables)[j], next[m])) /* ? */
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
  
  
  for(t = timeseries->datarows - 1; t >= 0; t--){ /* FOR EVERY TIMESLICE */
    
    printf("-- t = %d --\n", t+1);

#ifdef KEIJO
#endif
    /* Put some evidence in */
    for(i = 0; i < timeseries->num_of_nodes; i++)
      if(data[t][i] >= 0)
	enter_i_observation(model->first_var, model->cliques, 
			    model->num_of_cliques, 
			    get_Variable(model, 
					 timeseries->node_symbols[i]), 
			    data[t][i]);

    

    /* Pass the message from the past */
    if(t > 0){
      clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				       previous, num_of_nexts);
      assert(clique_of_interest != NULL);
      temp_vars = (int*) calloc(clique_of_interest->p->num_of_vars - 
				num_of_nexts, sizeof(int));
      if(!temp_vars){
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	return 1;
      }
      j = 0; k = 0;
      for(i=0; i < clique_of_interest->p->num_of_vars; i++){
	if(j < num_of_nexts &&
	   equal_variables((clique_of_interest->variables)[i], previous[j]))
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



    /* a useful inference ? */
    /*make_consistent(model);*/

  

    /* Pass the message from the future */
    if(t < timeseries->datarows - 1){
      clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				       next, num_of_nexts);
      assert(clique_of_interest != NULL);
      temp_vars = (int*) calloc(clique_of_interest->p->num_of_vars - 
				num_of_nexts, sizeof(int));
      if(!temp_vars){
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	return 1;
      }
      j = 0; k = 0;
      for(i=0; i < clique_of_interest->p->num_of_vars; i++){
	if(j < num_of_nexts && 
	   equal_variables((clique_of_interest->variables)[i], next[j]))
	  j++;
	else
	  temp_vars[k++] = i;
      }
      update_potential(timeslice_sepsets[t+1], timeslice_sepsets[t],
		       clique_of_interest->p, temp_vars);
      free(temp_vars);
    }



    /* Do the inference */
    make_consistent(model);
    
    

    /* Print some final results */
    for(i = 0; i < num_of_hidden; i++){
      
      /* 1. Decide which Variable you are interested in */
      interesting = hidden[i];
      
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
				       previous, num_of_nexts);
      assert(clique_of_interest != NULL);
      temp_vars = (int*) calloc(clique_of_interest->p->num_of_vars - 
				num_of_nexts, sizeof(int));
      if(!temp_vars){
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	return 1;
      }
      /* NOTE: Let's hope the "previous" variables are in correct order */
      m = 0;
      n = 0;
      for(j=0; j < clique_of_interest->p->num_of_vars; j++){
	if(m < num_of_nexts &&
	   equal_variables((clique_of_interest->variables)[j], previous[m]))
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

  for(t = 0; t < timeseries->datarows; t++)
    free(data[t]);
  free(data);

  for(i = 0; i < num_of_hidden; i++)
    free(result[i]);
  free(result);

  for(t = 0; t <= timeseries->datarows; t++)
    free_potential(timeslice_sepsets[t]);
  free(timeslice_sepsets);

  close_datafile(timeseries);
  
  free(hidden);
  free(next);
  free(previous);  

  return 0;
}
