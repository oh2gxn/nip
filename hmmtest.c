#include <stdlib.h>
#include <assert.h>
#include "parser.h"
#include "Clique.h"
#include "Variable.h"
#include "potential.h"
#include "errorhandler.h"

extern int yyparse();


int main(int argc, char *argv[]){

  char** tokens;
  int** data;
  int i, j, k, l, retval, t = 0;
  int num_of_hidden = 0;
  int nip_num_of_cliques;
  double* quotient;
  double*** filtered; /* probs of the hidden variables */
  double*** smoothed; /* probs of the hidden variables (given all data) */

  Clique *nip_cliques;
  Clique clique_of_interest;

  Variable *hidden;
  Variable temp;
  Variable interesting;
  Variable_iterator it;

  datafile* timeseries;


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
  else if(open_yyparse_infile(argv[1]) != NO_ERROR)
    return -1;

  retval = yyparse();

  close_yyparse_infile();

  if(retval != 0)
    return retval;
  /* The input file has been parsed. -- */


  /* Get references to the results of parsing */
  nip_cliques = *get_cliques_pointer();
  nip_num_of_cliques = get_num_of_cliques();


  /*****************************/
  /* read the data from a file */
  /*****************************/
  timeseries = open_datafile(argv[2], ',', 0, 1); /* 1. Open */
  if(timeseries == NULL){
    report_error(__FILE__, __LINE__, ERROR_FILENOTFOUND, 1);
    fprintf(stderr, "%s\n", argv[2]);
    return -1;
  }



  /* Figure out the number of hidden variables and variables that substitute
   * some other variable in the next timeslice. */
  it = get_Variable_list();
  temp = next_Variable(&it);
  while(temp != NULL){
    j = 1;
    for(i = 0; i < timeseries->num_of_nodes; i++)
      if(equal_variables(temp, get_variable(timeseries->node_symbols[i])))
	j = 0;
    if(j)
      num_of_hidden++;
    temp = next_Variable(&it);
  }

  assert(num_of_hidden == (total_num_of_vars() - timeseries->num_of_nodes));

  /* Allocate arrays for hidden variables. */
  hidden = (Variable *) calloc(num_of_hidden, sizeof(Variable));
  if(!hidden){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return 1;
  }

  /* Fill the arrays */
  it = get_Variable_list();
  temp = next_Variable(&it);
  i = 0;
  l = 0;
  while(temp != NULL){
    j = 1;
    for(k = 0; k < timeseries->num_of_nodes; k++)
      if(equal_variables(temp, get_variable(timeseries->node_symbols[k])))
	j = 0;
    if(j)
      hidden[l++] = temp;
    temp = next_Variable(&it);
  }
  

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
  filtered = (double***) calloc(timeseries->datarows + 1, sizeof(double**));
  smoothed = (double***) calloc(timeseries->datarows + 1, sizeof(double**));
  if(!(filtered && smoothed)){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return 1;
  }
  for(t = 0; t < timeseries->datarows + 1; t++){
    filtered[t] = (double**) calloc(num_of_hidden, sizeof(double*));
    smoothed[t] = (double**) calloc(num_of_hidden, sizeof(double*));
    if(!(filtered[t] && smoothed[t])){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return 1;
    }

    for(i = 0; i < num_of_hidden; i++){
      filtered[t][i] = (double*) calloc( number_of_values(hidden[i]), 
					 sizeof(double));
      smoothed[t][i] = (double*) calloc( number_of_values(hidden[i]), 
					 sizeof(double));
      if(!(filtered[t][i] && smoothed[t][i])){
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	return 1;
      }
    }
  }

  
  /* Fill the data array */
  for(t = 0; t < timeseries->datarows; t++){
    retval = nextline_tokens(timeseries, ',', &tokens); /* 2. Read */
    assert(retval == timeseries->num_of_nodes);

    /* 3. Put into the data array */
    for(i = 0; i < retval; i++){
      data[t][i] = 
	get_stateindex(get_variable((timeseries->node_symbols)[i]), tokens[i]);
      assert(data[t][i] >= 0);
    }

    for(i = 0; i < retval; i++) /* 4. Dump away */
      free(tokens[i]);
  }



  /*****************/
  /* Forward phase */
  /*****************/
  printf("## Forward phase ##\n");  

  for(t = 0; t <= timeseries->datarows; t++){ /* FOR EVERY TIMESLICE */
    
    printf("-- t = %d --\n", t);
        
    /********************/
    /* Do the inference */
    /********************/
    
    /* 1. Unmark all Cliques */
    for(i = 0; i < nip_num_of_cliques; i++)
      unmark_Clique(nip_cliques[i]);
    
    /* 2. Collect evidence */
    collect_evidence(NULL, NULL, nip_cliques[0]);
    
    /* 3. Unmark all Cliques */
    for(i = 0; i < nip_num_of_cliques; i++)
      unmark_Clique(nip_cliques[i]);
    
    /* 4. Distribute evidence */
    distribute_evidence(nip_cliques[0]);
    
    
    /* an experimental forward phase (a.k.a. filtering)... */
    /* Calculates the filtered values */
    for(i = 0; i < num_of_hidden; i++){
      
      /*********************************/
      /* Check the result of inference */
      /*********************************/
      
      /* 1. Decide which Variable you are interested in */
      interesting = hidden[i];
      
      /* 2. Find the Clique that contains the family of 
       *    the interesting Variable */
      clique_of_interest = find_family(nip_cliques, nip_num_of_cliques, 
				       &interesting, 1);
      if(!clique_of_interest){
	printf("In hmmtest.c : No clique found! Sorry.\n");
	return 1;
      }  
      
      /* 3. Marginalisation (memory for the result must have been allocated) */
      marginalise(clique_of_interest, interesting, filtered[t][i]);
      
      /* 4. Normalisation */
      normalise(filtered[t][i], number_of_values(interesting));    
      
      /* 5. Print the result */
      for(j = 0; j < number_of_values(interesting); j++)
	printf("P(%s=%s) = %f\n", get_symbol(interesting),
	       (interesting->statenames)[j], filtered[t][i][j]);
      printf("\n");
    }
    
    /* forget old evidence */
    it = get_Variable_list();
    while((temp = next_Variable(&it)) != NULL)
      reset_likelihood(temp);
    
    for(i = 0; i < num_of_hidden; i++){
      /* old posteriors become new priors */
      temp = hidden[i];
      if(temp->next != NULL)
	update_likelihood(temp->next, filtered[t][i]);
    }
    
    global_retraction(nip_cliques[0]);

    /* 0. Put some data in */
    if(t < timeseries->datarows)
      for(i = 0; i < timeseries->num_of_nodes; i++)
	enter_i_observation(get_variable((timeseries->node_symbols)[i]), 
			    data[t][i]);
  }
  
  
  
  
  /******************/
  /* Backward phase */
  /******************/
  
  printf("## Backward phase ##\n");  

  /* forget old evidence */
  it = get_Variable_list();
  while((temp = next_Variable(&it)) != NULL)
    reset_likelihood(temp);
  global_retraction(nip_cliques[0]);


  for(t = timeseries->datarows; t >= 0; t--){ /* FOR EVERY TIMESLICE */
    
    printf("-- t = %d --\n", t);
    

    for(i = 0; i < num_of_hidden; i++){
      temp = hidden[i];
      if(temp->next == NULL)
	enter_evidence(temp, filtered[t][i]);
    }


    if(t < timeseries->datarows){
      for(i = 0; i < num_of_hidden; i++){
	temp = hidden[i];
	if(temp->previous != NULL){
	  /* search for the other index */
	  for(k = 0; k < num_of_hidden; k++)
	    if(equal_variables(temp->previous, hidden[k]))
	      break;


	  quotient = (double *) calloc(number_of_values(temp), sizeof(double));
	  if(!data){
	    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	    return 1;
	  }
	  
	  for(j = 0; j < number_of_values(temp); j++){
	    quotient[j] = smoothed[t + 1][i][j] / filtered[t][k][j]; 
	  }
	  enter_evidence(temp->previous, quotient);	  
	  free(quotient);
	}
      }
    }

    /********************/
    /* Do the inference */
    /********************/
    
    /* 1. Unmark all Cliques */
    for(i = 0; i < nip_num_of_cliques; i++)
      unmark_Clique(nip_cliques[i]);

    /* 2. Collect evidence */
    collect_evidence(NULL, NULL, nip_cliques[0]);

    /* 3. Unmark all Cliques */
    for(i = 0; i < nip_num_of_cliques; i++)
      unmark_Clique(nip_cliques[i]);

    /* 4. Distribute evidence */
    distribute_evidence(nip_cliques[0]);


  
    /*********************************/
    /* Check the result of inference */
    /*********************************/
    for(i = 0; i < num_of_hidden; i++){
      
      /* 1. Decide which Variable you are interested in */
      interesting = hidden[i];

      /* 2. Find the Clique that contains the family of 
       *    the interesting Variable */
      clique_of_interest = find_family(nip_cliques, nip_num_of_cliques, 
				       &interesting, 1);
      if(!clique_of_interest){
	printf("In hmmtest.c : No clique found! Sorry.\n");
	return 1;
      }  
      
      /* 3. Marginalisation (the memory must have been allocated) */
      marginalise(clique_of_interest, interesting, smoothed[t][i]);
      
      /* 4. Normalisation */
      normalise(smoothed[t][i], number_of_values(interesting));
	
      /* 5. Print the result */
      for(j = 0; j < number_of_values(interesting); j++)
	printf("P(%s=%s) = %f\n", get_symbol(interesting),
	       (interesting->statenames)[j], smoothed[t][i][j]);
      printf("\n");
    }
    

    /* forget old evidence */
    it = get_Variable_list();
    while((temp = next_Variable(&it)) != NULL)
      reset_likelihood(temp);
    
    global_retraction(nip_cliques[0]);
   
  }
  
  
  return 0;
}

