/*
 * nip.c $Id: nip.c,v 1.203 2007-09-30 18:39:05 jatoivol Exp $
 */

#include "nip.h"
#include "lists.h"
#include "clique.h"
#include "variable.h"
#include "errorhandler.h"
#include "huginnet.tab.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

/* write new kind of net files (net language rev.2) */
#define NET_LANG_V2

#define POTENTIAL_ELEMENTS_PER_LINE 7

#define MIN_EM_ITERATIONS 3

/* #define DEBUG_NIP */

/***********************************************************
 * The time slice concept features some major difficulties 
 * because the actual calculations are done in the join tree
 * instead of the graph. The program should be able to 
 * figure out how the join tree repeats itself and store 
 * some kind of sepsets between the time slices... Note that 
 * there can be only one sepset between two adjacent 
 * time slices, because the join tree can't have loops. This 
 * implies that the variables, which have links to the 
 * variables in the next time slice, should be found in the 
 * same clique.
 *
 * UPDATE: it seems that Mr. Murphy has a solution...
 */


/***** 
 * TODO: 

 * - tiny addition to huginnet.y: potentialDeclaration without data
 *   (uniform potential) AND parents. Copy-paste the similar thing
 *   currently made for priors?

 * - print_model
 *   - writes an input file for graphviz/dot for automated visualization?

 * - automated test scripts
 *   - test models and data
 *   - script with simple diff operations?

 * - BUG: net parser segfaults if <symbols> is an empty list:
 *   "potential( A | ) { }" - it should report an error and quit

 * - Leave-one-out program based on em_test and inftest...
 *   + loo_prediction_test
 *   + for each time series: predict the values of a given variable
 *       given a model learned from other series and 
 *       given data about the other variables
 *   - testing

 * - BUG: some DBN models work, equivalent static ones don't... WHY?
 *   - Does this happen only on MRF-like models? 
 *     Is it because of bad triangulation?

 *   - Should use_priors() be used automatically by reset_model() ?
 *   - Is the condition for entering priors really correct ?
 *   - Make sure priors get multiplied into the model only once!
 *     (previously they were treated like evidence and this wasn't a problem)

 * - A program for computing conditional likelihoods: likelihood.c
 *   + command line parameters:
 *     + "foo.net", the model
 *     + "bar.txt", the data
 *     + "D E F", at least one variable!
 *   + output: the likelihood value "p (d e f | a b c)"
 *   - the same for DBN models???

 * - Use separate structure for the first time step
 *   - How to create the graph for it automatically?
 *   - How to use the new piece for inference in the first step?

 * - Implement some sort of structural learning algorithm?
 *   - NIP_potential_parents = "P1 P2 P3"; ?
 *   - There is a static set of edges and defined 
 *     set of possible additional edges in a graph
 *   - The graph has to be stored in the nip struct ?
 *     - nip struct       <=> net file
 *     - variable struct  <=> node() definition
 *     - potential struct <=> potential() definition???
 *   - What about graphs without any static edges?
 *     - Net file without any potential() definitions?
 *       (or only priors)
 *     - nip struct == a set of graphs and their clique trees?

 * - Parse and preserve other fields specified in Hugin Net files
 *   (currently this program ignores them)
 *   - How to store potential-specific fields?
 *   - What are net{input/output}-fields?

 * - Consistent conventions in naming functions, variables, 
 *   and data structures?
 *   - foo_bar(), dataType, etc.?

 * - Printing potential tables should be contained in potential.c
 *   - Problem: how to include comments about parent values?

 * - Refactorisation of variable_union(), variable_isect(), and
 *   mapper() by replacing a lot of copy-paste code with them...

 * - Get rid of copy-paste stuff in inference procedures... but how?

 * - "Viterbi" algorithm for the ML-estimate of the latent variables
 *   - It is a descendant of the algorithm called "max-product" or "max-sum"
 *   - another forward-like algorithm with elements of dynamic programming
 *   - To save huge amounts of memory, could the process use some kind of 
 *     "timeslice sepsets" for encoding the progress throughout time?
 *   - It has to have the backtracking ability...

 * - Online forward_inference (+ refactor offline forward_inference?)

 * - Online fixed_lag_smoothing...

 * - Refactor the list implementations (ADTs, almost done?)
 *   - potentialList needs to be "hidden" better?
 *   - still massive amounts of copy-paste stuff... let it be?
 *   + Next time: make a general void* list or use C++

 * - Hide time_series and uncertain_series implementations better
 *   - more useful operations for them?

 * - write_X() functions could take file id's instead of file names...
 *   (opening a file or other output would be users responsibility)

 * + BUG: Evidence about variables without parents cancels the prior
 *   + Priors should not be entered as if they were evidence
 *****/

extern FILE *open_net_file(const char *filename);
extern void close_net_file();
extern int yyparse();
extern variablelist get_parsed_variables();
extern int get_cliques(clique** clique_array_pointer);
extern void get_parsed_node_size(int* x, int* y);

static int start_timeslice_message_pass(nip model, direction dir, 
					potential sepset);
static int finish_timeslice_message_pass(nip model, direction dir,
					 potential num, potential den);

static int e_step(time_series ts, potential* parameters, 
		  double* loglikelihood);
static int m_step(potential* results, nip model);



void reset_model(nip model){
  int i, retval;
  variable v;
  for(i = 0; i < model->num_of_vars; i++){
    v = model->variables[i];
    reset_likelihood(v);
    v->prior_entered = 0;
  }
  retval = global_retraction(model->variables, model->num_of_vars, 
			     model->cliques, model->num_of_cliques);
  if(retval != NO_ERROR)
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
}


void total_reset(nip model){
  int i;
  clique c;
  for(i = 0; i < model->num_of_cliques; i++){
    c = model->cliques[i];
    uniform_potential(c->original_p, 1.0);
  }
  /* Q: Reset priors? */
  reset_model(model); /* Could that be enough? */
}


void use_priors(nip model, int has_history){
  int i, retval;
  variable v;

  for(i = 0; i < model->num_of_vars - model->num_of_children; i++){
    v = model->independent[i];
    assert(v->prior != NULL);

    /* Prevent priors from being multiplied into the model more than once */
    if(!(v->prior_entered)){
      /* Interface variables have priors only in the first time step 
       * (!has_history => first time step) */
      if(!has_history || !(v->if_status & INTERFACE_OLD_OUTGOING)){

	/* Reminder: Priors should be multiplied into the tree so that
	 *           they are not canceled if evidence is entered, but
	 *           there was the idea that global retraction should
	 *           cancel priors. Canceling some priors is needed in 
	 *           time slice models where interface variables will 
	 *           not actually have priors in subsequent time steps. */
	
	retval = enter_prior(model->variables, model->num_of_vars, 
			     model->cliques, model->num_of_cliques, 
			     v, v->prior);
	if(retval != NO_ERROR)
	  report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);

	v->prior_entered = 1;
      }
    }
  }
}


nip parse_model(char* file){
  int i, j, k, m, retval;
  variable temp;
  variablelist vl;
  nip new = (nip) malloc(sizeof(nip_type));

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }
  
  /* 1. Parse */
  if(open_net_file(file) == NULL){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return NULL;
  }

  retval = yyparse(); /* Reminder: priors not entered yet?! */

  close_net_file();

  if(retval != 0)
    return NULL;

  /* 2. Get the parsed stuff and make a model out of them */
  new->num_of_cliques = get_cliques(&(new->cliques));
  vl = get_parsed_variables();
  new->num_of_vars = LIST_LENGTH(vl);
  new->variables = list_to_variable_array(vl);
  empty_variablelist(vl); /* free the list? */

  /* count the number of various kinds of "special" variables */
  new->num_of_nexts = 0;
  new->num_of_children = 0;
  new->outgoing_interface_size = 0;
  new->incoming_interface_size = 0;
  for(i = 0; i < new->num_of_vars; i++){
    temp = new->variables[i];

    /* how many belong to the next timeslice */
    if(temp->next)
      new->num_of_nexts++;
    
    /* how many belong to the interfaces */
    if(temp->if_status & INTERFACE_INCOMING)
      new->incoming_interface_size++;
    if(temp->if_status & INTERFACE_OUTGOING)
      new->outgoing_interface_size++;

    /* how many have parents */
    if(temp->parents)
      new->num_of_children++;
    else
      if(temp->prior == NULL){
	fprintf(stderr, "Warning: No prior for the variable %s!\n", 
		temp->symbol);
	temp->prior = (double*) calloc(CARDINALITY(temp), sizeof(double));
	/* this fixes the situation */
      }
  }

  new->next = (variable*) calloc(new->num_of_nexts, sizeof(variable));
  new->previous = (variable*) calloc(new->num_of_nexts, sizeof(variable));
  new->outgoing_interface = (variable*) calloc(new->outgoing_interface_size, 
					       sizeof(variable));
  new->previous_outgoing_interface = 
    (variable*) calloc(new->outgoing_interface_size, sizeof(variable));
  new->incoming_interface = (variable*) calloc(new->incoming_interface_size, 
					       sizeof(variable));
  new->children = (variable*) calloc(new->num_of_children, sizeof(variable));
  new->independent = 
    (variable*) calloc(new->num_of_vars - new->num_of_children, 
		       sizeof(variable));
  if(!(new->independent && 
       new->children && 
       new->outgoing_interface && 
       new->previous_outgoing_interface && 
       new->incoming_interface && 
       new->previous && 
       new->next)){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(new->variables);
    free(new->next);
    free(new->previous);
    free(new->outgoing_interface);
    free(new->previous_outgoing_interface);
    free(new->incoming_interface);
    free(new->children);
    free(new);
    return NULL;
  }

  /* This selects the variables for various special purposes */
  j = 0; k = 0; m = 0;
  for(i = 0; i < new->num_of_vars; i++){
    temp = new->variables[i];

    if(temp->next){
      new->next[j] = temp;
      new->previous[j] = temp->next;
      j++;
    }
    
    if(temp->if_status & INTERFACE_INCOMING)
      new->incoming_interface[k++] = temp;
    if(temp->if_status & INTERFACE_OLD_OUTGOING){
      new->previous_outgoing_interface[m] = temp;
      new->outgoing_interface[m] = temp->previous;
      assert(temp->previous->if_status & INTERFACE_OUTGOING);
      m++;
    }
  }
  assert(m == new->outgoing_interface_size); /* same amount of old & new? */

  /* Reminder: (Before I indexed the children with j, the program had 
   * funny crashes on Linux systems :) */
  j = 0; k = 0;
  for(i = 0; i < new->num_of_vars; i++)
    if(new->variables[i]->parents)
      new->children[j++] = new->variables[i]; /* set the children */
    else
      new->independent[k++] = new->variables[i];

  if(new->outgoing_interface_size > 0){
    new->in_clique = find_clique(new->cliques, 
				 new->num_of_cliques, 
				 new->previous_outgoing_interface, 
				 new->outgoing_interface_size);
    assert(new->in_clique != NULL);
    new->out_clique = find_clique(new->cliques, 
				  new->num_of_cliques, 
				  new->outgoing_interface, 
				  new->outgoing_interface_size);
    assert(new->out_clique != NULL);
  }
  else{
    new->in_clique = NULL;
    new->out_clique = NULL;
  }
  get_parsed_node_size(&(new->node_size_x), &(new->node_size_y));

  /* Let's check one detail */
  for(i = 0; i < new->num_of_vars - new->num_of_children; i++)  
    assert(new->independent[i]->num_of_parents == 0);

  /* 4. Reset parser globals? */
  /*empty_variablelist(vl);*/

#ifdef DEBUG_NIP
  if(new->out_clique){
    printf("Out clique:\n");
    print_clique(new->out_clique);
  }
  printf("Incoming interface:\n");
  for(i = 0; i < new->incoming_interface_size; i++)
    printf("%s, ", new->incoming_interface[i]->symbol);
  printf("\n");
  printf("Outgoing interface:\n");
  for(i = 0; i < new->outgoing_interface_size; i++)
    printf("%s, ", new->outgoing_interface[i]->symbol);
  printf("\n");
  printf("Old outgoing interface:\n");
  for(i = 0; i < new->outgoing_interface_size; i++)
    printf("%s, ", new->previous_outgoing_interface[i]->symbol);
  printf("\n");
#endif

  return new;
}


/* NOTE: part of this stuff should be moved to potential.c etc. */
int write_model(nip model, char* filename){
  int i, j, n;
  int x, y;
  int nparents, nvalues;
  FILE *f = NULL;
  variable v = NULL;
  int *temp = NULL;
  int *map = NULL;
  clique c = NULL;
  potential p = NULL;
  char *indent;

#ifdef NET_LANG_V1
  indent = "    ";
#else
  indent = "";
#endif

  /* open the stream */
  f = fopen(filename, "w");
  if(!f){
    report_error(__FILE__, __LINE__, ERROR_IO, 1);
    return ERROR_IO;
  }

  /* remove the evidence */
  reset_model(model);

  /* Reminder:
   * fputs("some text", f);
   * fprintf(f, "value = %g", 1.23400000); */
  /*******************************************************/
  /* Replace %f with %g if you don't like trailing zeros */

  /** Lets print the NET file **/

  /** standard stuff in the beginning **/
#ifdef NET_LANG_V1
  fprintf(f, "class %s \n", name);
#else
  fputs("net\n", f);
#endif
  fputs("{\n", f);
#ifdef NET_LANG_V1
  fputs("    inputs = ();\n", f);
  fputs("    outputs = ();\n", f);
#endif
  fprintf(f, "    node_size = (%d %d);\n", 
	model->node_size_x, model->node_size_y);
#ifndef NET_LANG_V1
  fputs("}\n", f);
#endif

  /** the variables **/
  for(i = 0; i < model->num_of_vars; i++){
    v = model->variables[i];
    n = CARDINALITY(v)-1;
    get_position(v, &x, &y);
    fputs("\n", f);
    fprintf(f, "%snode %s\n", indent, get_symbol(v));
    fprintf(f, "%s{\n", indent);
    fprintf(f, "%s    label = \"%s\";\n", indent, v->name);
    fprintf(f, "%s    position = (%d %d);\n", indent, x, y);
    fprintf(f, "%s    states = (", indent);
    for(j = 0; j < n; j++)
      fprintf(f, " \"%s\" \n%s              ", v->statenames[j], indent);
    fprintf(f, " \"%s\" );\n", v->statenames[n]);
    if(v->previous)
      fprintf(f, "%s    NIP_next = \"%s\";\n", indent, 
	      get_symbol(v->previous));
    fprintf(f, "%s}\n", indent);
    fflush(f);
  }

  /** the priors **/
  for(i = 0; i < model->num_of_vars - model->num_of_children; i++){
    v = model->independent[i];
    n = CARDINALITY(v);
    fputs("\n", f);
    /* independent variables have priors */
    fprintf(f, "%spotential (%s)\n", indent, get_symbol(v));
    fprintf(f, "%s{\n", indent);
    fprintf(f, "%s    data = ( ", indent);
    for(j = 0; j < n; j++){

      /* limit the length of lines */
      if (j > 0 && (j % POTENTIAL_ELEMENTS_PER_LINE) == 0) 
	fprintf(f, "\n%s             ", indent);

      /* Print the value: change to %g if trailing zeros disturb you */
      fprintf(f, "%f  ", v->prior[j]);

      /* NOTE: fprintf() returns the number of printed characters,
       * and you could use it for limiting the line length smarter. */
    }
    fputs(");\n", f);
    fprintf(f, "%s}\n", indent);
    fflush(f);
  }

  /** the potentials **/
  for(i = 0; i < model->num_of_children; i++){
    v = model->children[i];

    nvalues  = CARDINALITY(v);
    nparents = number_of_parents(v);
    fputs("\n", f);
    /* child variables have conditional distributions */
    fprintf(f, "%spotential (%s | ", indent, get_symbol(v));
    for(j = nparents-1; j > 0; j--){ 
      /* Hugin fellas put parents in reverse order */
      fprintf(f, "%s ", get_symbol(v->parents[j]));
    }
    fprintf(f, "%s)\n", get_symbol(v->parents[0]));
    fprintf(f, "%s{ \n", indent);
    fprintf(f, "%s    data = (", indent);

    temp = (int*) calloc(nparents+1, sizeof(int));
    if(!temp){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      fclose(f);
      return ERROR_OUTOFMEMORY;
    }

    /* form the potential */
    temp[0] = nvalues; /* child must be the first... */
    /* ...then the parents */
    for(j = 0; j < nparents; j++)
      temp[j+1] = CARDINALITY(v->parents[j]);
    p = make_potential(temp, nparents+1, NULL);

    /* compute the distribution */
    c = find_family(model->cliques, model->num_of_cliques, v);
    map = find_family_mapping(c, v);
    general_marginalise(c->original_p, p, map);

    /* normalisation (NOTE: should be part of potential.c) */
    normalise_cpd(p);

    /* print the stuff (TODO: hide the access to private data?) */
    y = 0; /* counter for number of values printed on a line */
    for(j = 0; j < p->size_of_data; j++){

      /* cut long lines and between change in parent values */
      n = (j % nvalues == 0); /* new combination of parent values? */
      if(j > 0 && 
	 (n || (nvalues > POTENTIAL_ELEMENTS_PER_LINE && 
		y % POTENTIAL_ELEMENTS_PER_LINE == 0))){
	if(n){
	  /* print comments about parent values for previous line 
	   * NOTE: the last comment is left out */
	  fputs(" % ", f);
	  inverse_mapping(p, j-1, temp);
	  for(x = nparents-1; x >= 0; x--){
	    fprintf(f, "%s=%s ", get_symbol(v->parents[x]),
		    get_statename(v->parents[x], temp[x+1]));
	  }
	}
	/* cut the line and indent */
	fprintf(f, "\n%s            ", indent);
	y = 0;
      }
      
      /* Print the value... */
      fprintf(f, " %f ", p->data[j]);
      y++;
    }
    fputs(");\n", f);
    fprintf(f, "%s}\n", indent);
    free(temp);
    free_potential(p);
    fflush(f);
  }

#ifdef NET_LANG_V1
  fputs("} \n", f); /* the last brace */
#endif

  /* close the file */
  if(fclose(f)){
    report_error(__FILE__, __LINE__, ERROR_IO, 1);
    return ERROR_IO;
  }
  return NO_ERROR;
}


void free_model(nip model){
  int i;

  /* 1. Free cliques and adjacent sepsets */
  for(i = 0; i < model->num_of_cliques; i++)
    free_clique(model->cliques[i]);
  free(model->cliques);

  /* 2. Free the variables */
  for(i = 0; i < model->num_of_vars; i++)
    free_variable(model->variables[i]);
  free(model->variables);

  free(model->next);
  free(model->previous);
  free(model->outgoing_interface);
  free(model->previous_outgoing_interface);
  free(model->incoming_interface);
  free(model->children);
  free(model->independent);
  free(model);
}


int read_timeseries(nip model, char* filename, 
		    time_series** results){
  int i, j, k, m, n, N; 
  int obs;
  char** tokens = NULL;
  time_series ts = NULL;
  datafile* df = NULL;
  variable v = NULL;
  
  df = open_datafile(filename, FIELD_SEPARATOR, 0, 1);

  if(df == NULL){
    report_error(__FILE__, __LINE__, ERROR_FILENOTFOUND, 1);
    fprintf(stderr, "%s\n", filename);
    return 0;
  }  

  /* N time series */
  N = df->ndatarows;
  *results = (time_series*) calloc(N, sizeof(time_series));
  if(!*results){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    close_datafile(df);
    return 0;
  }

  for(n = 0; n < N; n++){
    ts = (time_series) malloc(sizeof(time_series_type));
    if(!ts){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free(*results);
      close_datafile(df);
      return 0;
    }
    ts->model = model;
    ts->hidden = NULL;
    ts->observed = NULL;
    ts->data = NULL;
    ts->length = df->datarows[n];
    
    /* Check the contents of data file */
    obs = 0;
    for(i = 0; i < df->num_of_nodes; i++){
      v = model_variable(model, df->node_symbols[i]);
      if(v)
	obs++;
    }
    
    /* Find out how many (totally) latent variables there are. */
    ts->num_of_hidden = model->num_of_vars - obs;
    ts->num_of_observed = obs; /* Must be "final" */
    
    /* Allocate the array for the hidden variables. */
    ts->hidden = (variable *) calloc(ts->num_of_hidden, sizeof(variable));
    if(obs > 0)
      ts->observed = (variable *) calloc(obs, sizeof(variable));
    if(!(ts->hidden && (ts->observed || obs == 0))){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free(ts->hidden);
      free(ts);
      free(*results);
      close_datafile(df);
      return 0;
    }  
    
    /* Set the pointers to the hidden variables. */
    m = 0;
    for(k = 0; k < model->num_of_vars; k++){
      j = 1;
      for(i = 0; i < df->num_of_nodes; i++){
	if(equal_variables(model->variables[k], 
			   model_variable(model, df->node_symbols[i])))
	  j = 0;
      }
      if(j)
	ts->hidden[m++] = model->variables[k];
    }
    
    if(obs > 0){
      k = 0;
      for(i = 0; i < df->num_of_nodes; i++){
	v = model_variable(model, df->node_symbols[i]);
	if(v)
	  ts->observed[k++] = v;
      }
      
      /* Allocate some space for data */
      ts->data = (int**) calloc(ts->length, sizeof(int*));
      if(!(ts->data)){
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	free(ts->hidden);
	free(ts->observed);
	free(ts);
	free(*results);
	close_datafile(df);
	return 0;
      }
      for(i = 0; i < ts->length; i++){
	ts->data[i] = (int*) calloc(obs, sizeof(int));
	if(!(ts->data[i])){
	  report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	  for(j = 0; j < i; j++)
	    free(ts->data[j]);
	  free(ts->data);
	  free(ts->hidden);
	  free(ts->observed);
	  free(ts);
	  free(*results);
	  close_datafile(df);
	  return 0;
	}
      }
      
      /* Get the data */
      for(j = 0; j < ts->length; j++){
	m = nextline_tokens(df, ',', &tokens); /* 2. Read */
	assert(m == df->num_of_nodes);
	
	/* 3. Put into the data array 
	 * (the same loop as above to ensure the data is in 
	 *  the same order as variables) */
	k = 0;
	for(i = 0; i < m; i++){
	  v = model_variable(model, df->node_symbols[i]);
	  if(v)
	    ts->data[j][k++] = get_stateindex(v, tokens[i]);
	  
	  /* Q: Should missing data be allowed?   A: Yes. */
	  /* assert(data[j][i] >= 0); */
	}
	
	for(i = 0; i < m; i++) /* 4. Dump away */
	  free(tokens[i]);
	free(tokens);
      }
    }

    (*results)[n] = ts;
  }
  
  close_datafile(df);
  return N;
}


int write_timeseries(time_series *ts_set, int n_series, char *filename){
  int i, n, t;
  int d;
  int *record;
  int n_observed;
  int *map;
  variable v;
  variable *observed;
  variable *observed_more;
  time_series ts;
  nip the_model;
  FILE *f = NULL;

  /* Check stuff */
  if(!(n_series > 0 && ts_set && filename)){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  /* Check all the models are same */
  the_model = ts_set[0]->model;
  for(n = 1; n < n_series; n++){
    if(ts_set[n]->model != the_model){
      report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
      return ERROR_INVALID_ARGUMENT;
    }
  }

  /* Find out union of observed variables */
  ts = ts_set[0];
  observed = variable_union(ts->observed, NULL, 
			    the_model->num_of_vars - ts->num_of_hidden,
			    0, &n_observed);
  if(n_observed < 0){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return ERROR_GENERAL;
  }

  for(n = 1; n < n_series; n++){
    ts = ts_set[n];
    observed_more = variable_union(observed, ts->observed, n_observed,
				   the_model->num_of_vars - ts->num_of_hidden,
				   &n_observed);
    free(observed); /* nice to create the same array again and again? */
    observed = observed_more;
  }

  if(!n_observed){ /* no observations in any time series? */
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  /* Temporary space for a sorted record (time step) */
  record = (int*) calloc(n_observed, sizeof(int));
  if(!record){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(observed);
    return ERROR_OUTOFMEMORY;
  }

  /* Try to open the file for write */
  f = fopen(filename, "w");
  if(!f){
    report_error(__FILE__, __LINE__, ERROR_IO, 1);
    free(observed);
    free(record);
    return ERROR_IO;
  }

  /* Write names of the variables */
  for(i = 0; i < n_observed; i++){
    v = observed[i];
    if(i > 0)
      fprintf(f, "%c", FIELD_SEPARATOR);
    fprintf(f, "%s", get_symbol(v));
  }
  fputs("\n", f);

  /* Write the data */
  for(n = 0; n < n_series; n++){ /* for each time series */
    ts = ts_set[n];
    map = mapper(observed, ts->observed, n_observed, ts->num_of_observed);

    for(t = 0; t < ts->length; t++){ /* for each time step */

      /* Fill record with indicators of missing data */
      for(i = 0; i < n_observed; i++)
	record[i] = -1;

      /* Extract data from the time series */
      for(i = 0; i < ts->num_of_observed; i++)
	record[map[i]] = ts->data[t][i];

      /* Print the data */
      for(i = 0; i < n_observed; i++){
	v = observed[i];
	d = record[i];
	if(i > 0)
	  fprintf(f, "%c", FIELD_SEPARATOR);
	if(d >= 0)
	  fprintf(f, "%s", get_statename(v, d));      
	else
	  fputs("null", f);
      }
      fputs("\n", f);
    }
    fputs("\n", f); /* TS separator */
    free(map);
  }  
  free(observed);
  free(record);

  /* Close the file */
  if(fclose(f)){
    report_error(__FILE__, __LINE__, ERROR_IO, 1);
    return ERROR_IO;
  }
  return NO_ERROR;
}


void free_timeseries(time_series ts){
  int t;
  if(ts){
    if(ts->data){
      for(t = 0; t < ts->length; t++)
	free(ts->data[t]);
      free(ts->data);
    }
    free(ts->hidden);
    free(ts->observed);
    free(ts);
  }
}


/* Replace this with the macro TIME_SERIES_LENGTH() */
int timeseries_length(time_series ts){
  return ts->length;
}


int write_uncertainseries(uncertain_series *ucs_set, int n_series, 
			  variable v, char *filename){
  int i, n, t, s;
  int *v_index;
  uncertain_series ucs;
  FILE *f = NULL;

  /* Check stuff */
  n = CARDINALITY(v);
  if(!(n>0 && ucs_set && filename && n_series>0)){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  v_index = (int*) calloc(n_series, sizeof(int));
  if(!v_index){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  /* Check that the variable exists in all UCS and which one is it */
  for(s = 0; s < n_series; s++){
    ucs = ucs_set[s];
    v_index[s] = -1; /* linear search */
    for(i=0; i < ucs->num_of_vars; i++){
      if(equal_variables(v, ucs->variables[i])){
	v_index[s] = i;
	break;
      }
    }
    if(v_index[s] < 0){ /* no such variable in the UCS */
      report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
      return ERROR_INVALID_ARGUMENT;
      free(v_index);
    }
  }
  
  /* Try to open the file for write */
  f = fopen(filename, "w");
  if(!f){
    report_error(__FILE__, __LINE__, ERROR_IO, 1);
    free(v_index);
    return ERROR_IO;
  }

  /* Write names of the states */
  for(i = 0; i < n; i++){
    if(i > 0)
      fprintf(f, "%c", FIELD_SEPARATOR);
    fprintf(f, "%s", get_statename(v, i));
  }
  fputs("\n", f);

  /* Write the data: probabilities... */
  for(s = 0; s < n_series; s++){ /* ...for each series... */
    ucs = ucs_set[s];

    for(t = 0; t < ucs->length; t++){ /* ...for each time step... */
      
      for(i = 0; i < n; i++){ /* ...for each state. */
	if(i > 0)
	  fprintf(f, "%c", FIELD_SEPARATOR);
	fprintf(f, "%f", ucs->data[t][v_index[s]][i]);
      }
      fputs("\n", f);
    }
    fputs("\n", f); /* series separator */
  }  

  /* Close the file */
  if(fclose(f)){
    report_error(__FILE__, __LINE__, ERROR_IO, 1);
    free(v_index);
    return ERROR_IO;
  }

  free(v_index);
  return NO_ERROR;
}


void free_uncertainseries(uncertain_series ucs){
  int i, t;
  if(ucs){
    for(t = 0; t < ucs->length; t++){
      for(i = 0; i < ucs->num_of_vars; i++)
	free(ucs->data[t][i]);
      free(ucs->data[t]);
    }
    free(ucs->data);
    free(ucs->variables);
    free(ucs);
  }
}


int uncertainseries_length(uncertain_series ucs){
  return ucs->length;
}


char* get_observation(time_series ts, variable v, int time){
  int i, j = -1;
  for(i = 0; i < ts->model->num_of_vars - ts->num_of_hidden; i++)
    if(equal_variables(v, ts->observed[i]))
      j = i;

  if(j < 0)
    return NULL;

  return v->statenames[ts->data[time][j]];
}


int set_observation(time_series ts, variable v, int time, char* observation){
  int i, j = -1;
  for(i = 0; i < ts->model->num_of_vars - ts->num_of_hidden; i++)
    if(equal_variables(v, ts->observed[i]))
      j = i;

  i = get_stateindex(v, observation);
  /* a valid variable? a valid observation for that variable? */
  if((j < 0) || (i < 0))
    return ERROR_INVALID_ARGUMENT;

  ts->data[time][j] = i;
  return 0;
}


int insert_hard_evidence(nip model, char* varname, char* observation){
  int ret;
  variable v = model_variable(model, varname);
  if(v == NULL)
    return ERROR_INVALID_ARGUMENT;
  ret = enter_observation(model->variables, model->num_of_vars, 
			  model->cliques, model->num_of_cliques, 
			  v, observation);
  if(ret != NO_ERROR)
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);

  make_consistent(model);
  return ret;
}


int insert_soft_evidence(nip model, char* varname, double* distribution){
  int ret;
  variable v = model_variable(model, varname);
  if(v == NULL)
    return ERROR_INVALID_ARGUMENT;
  ret =  enter_evidence(model->variables, model->num_of_vars, 
			model->cliques, model->num_of_cliques, 
			v, distribution);
  make_consistent(model);
  return ret;
}


/* Note that <model> may be different from ts->model, but the variables
 * have to be shared. */
int insert_ts_step(time_series ts, int t, nip model, char mark_mask){
  int i;
  variable v;

  if(t < 0 || t >= timeseries_length(ts)){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }
    
  for(i = 0; i < ts->model->num_of_vars - ts->num_of_hidden; i++){
    v = ts->observed[i];
    if(MARK(v) & mark_mask){ /* Only the suitably marked variables */
      if(ts->data[t][i] >= 0)
	enter_i_observation(model->variables, model->num_of_vars, 
			    model->cliques, model->num_of_cliques, 
			    v, ts->data[t][i]);
    }
  }
  return NO_ERROR;
}


/* note that <model> may be different from ucs->model */
int insert_ucs_step(uncertain_series ucs, int t, nip model, char mark_mask){
  int i, e;
  variable v;

  if(t < 0 || t >= uncertainseries_length(ucs)){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  for(i = 0; i < ucs->num_of_vars; i++){
    v = ucs->variables[i];
    if(MARK(v) & mark_mask){
      e = enter_evidence(model->variables, model->num_of_vars, 
			 model->cliques, model->num_of_cliques, 
			 v, ucs->data[t][i]);
      if(e != NO_ERROR){
	report_error(__FILE__, __LINE__, e, 1);  
	return e;
      }
    }
  }
  return NO_ERROR;
}


/* Starts a message pass between timeslices */
static int start_timeslice_message_pass(nip model, direction dir,
					potential alpha_or_gamma){
  int *mapping;
  clique c;
  int nvars = model->outgoing_interface_size;
  variable *vars;

  /* What if there are no subsequent time slices? */
  if(nvars == 0){
    uniform_potential(alpha_or_gamma, 1.0);
    return NO_ERROR;
  }

  if(dir == forward){
    vars = model->outgoing_interface;
    c = model->out_clique;
  }
  else{
    vars = model->previous_outgoing_interface;
    c = model->in_clique;
  }

  /* map the variables */
  mapping = mapper(c->variables, vars, c->p->num_of_vars, nvars);

  /* the marginalisation */
  general_marginalise(c->p, alpha_or_gamma, mapping);
  free(mapping); /* should mapping-array be a part of sepsets? */

  /* normalisation in order to avoid drifting towards zeros */
  normalise_potential(alpha_or_gamma);

  return NO_ERROR;
}


/* Finishes the message pass between timeslices */
static int finish_timeslice_message_pass(nip model, direction dir,
					 potential num, potential den){
  int *mapping;
  clique c;
  int nvars = model->outgoing_interface_size;
  variable *vars;

  if(nvars == 0) /* independent time slices (multiplication with 1) */
    return NO_ERROR;

  /* Find a suitable clique c */
  if(dir == forward){
    vars = model->previous_outgoing_interface;
    c = model->in_clique;
  }
  else{
    vars = model->outgoing_interface;
    c = model->out_clique;
  }

  /* map the variables */
  mapping = mapper(c->variables, vars, c->p->num_of_vars, nvars);

  /* the multiplication (and division, if den != NULL) */
  update_potential(num, den, c->p, mapping);
  free(mapping);  
  return NO_ERROR;
}


/* forward-only inference consumes constant (1 time slice) amount of memory 
 * + the results (which is linear) */
uncertain_series forward_inference(time_series ts, variable vars[], int nvars,
				   double* loglikelihood){
  int i, t;
  int *cardinalities = NULL;
  double m1, m2;
  variable temp;
  potential alpha = NULL;
  clique clique_of_interest;
  uncertain_series results = NULL;
  nip model = ts->model;

  
  /* Allocate an array */
  if(model->outgoing_interface_size > 0){
    cardinalities = (int*) calloc(model->outgoing_interface_size, 
				  sizeof(int));
    if(!cardinalities){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return NULL;
    }
  }

  /* Fill the array */
  for(i = 0; i < model->outgoing_interface_size; i++)
    cardinalities[i] = CARDINALITY(model->outgoing_interface[i]);

  /* Allocate some space for the results */
  results = (uncertain_series) malloc(sizeof(uncertain_series_type));
  if(!results){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(cardinalities);
    return NULL;
  }
  
  results->variables = (variable*) calloc(nvars, sizeof(variable));
  if(!results->variables){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(results);
    free(cardinalities);
    return NULL;
  }

  /* Copy the references to the variables of interest */
  memcpy(results->variables, vars, nvars*sizeof(variable));
  results->num_of_vars = nvars;
  results->length = ts->length;

  results->data = (double***) calloc(ts->length, sizeof(double**));
  if(!results->data){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(results->variables);
    free(results);
    free(cardinalities);
    return NULL;
  }
  
  for(t = 0; t < results->length; t++){
    results->data[t] = (double**) calloc(nvars, sizeof(double*));
    if(!results->data[t]){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      while(t > 0){
	t--;
	for(i = 0; i < nvars; i++)
	  free(results->data[t][i]);
	free(results->data[t]);
      }
      free(results->data); /* t == -1 */
      free(results->variables);
      free(results);
      free(cardinalities);
      return NULL;
    }

    for(i = 0; i < nvars; i++){
      results->data[t][i] = (double*) calloc(CARDINALITY(vars[i]),
					     sizeof(double));
      if(!results->data[t][i]){
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	while(i > 0)
	  free(results->data[t][--i]);
	free(results->data[t]);
	while(t > 0){
	  t--;
	  for(i = 0; i < nvars; i++)
	    free(results->data[t][i]);
	  free(results->data[t]);
	}
	free(results->data);
	free(results->variables); /* t == -1 */
	free(results);
	free(cardinalities);
	return NULL;
      }
    }
  }

  /* Initialise the intermediate potential */
  alpha = make_potential(cardinalities, model->outgoing_interface_size, NULL);
  free(cardinalities);

  /*****************/
  /* Forward phase */
  /*****************/
  reset_model(model);
  use_priors(model, !HAD_A_PREVIOUS_TIMESLICE);
  if(loglikelihood)
    *loglikelihood = 0; /* init */

  for(t = 0; t < ts->length; t++){ /* FOR EVERY TIMESLICE */

    /* Original order (pre-10.07.2006):
     * - m1
     * - evidence in
     * - finish_timeslice_message_pass
     * - make consistent
     * - m2 
     */

    if(t > 0){ /*  Fwd or Fwd1  */
      /*  clique_in = clique_in * alpha  */
      if(finish_timeslice_message_pass(model, forward, 
				       alpha, NULL    ) != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	free_uncertainseries(results);
	free_potential(alpha);
	return NULL;
      }
    }
    
    /* Likelihood reference... */
    if(loglikelihood){
      make_consistent(model);
      m1 = model_prob_mass(model);
    }

    /* Put some data in */
    insert_ts_step(ts, t, model, MARK_ON); /* only marked variables */

    /* Do the inference */
    make_consistent(model);

    /* Compute loglikelihood if required */
    if(loglikelihood){
      /* Q: Is this L(y(t) | y(0:t-1)) 
       * A: Yes... */
      m2 = model_prob_mass(model);
      if((m1 > 0) && (m2 > 0)){
	*loglikelihood = (*loglikelihood) + (log(m2) - log(m1));
      }
      /* Check for anomalies */
      /*assert(*loglikelihood <= 0.0);*/
      assert(m2 >= 0.0);
      if(m2 == 0){
	*loglikelihood = -DBL_MAX; /* -infinity ? */
      }
#ifdef DEBUG_NIP
      if(t > 0){
	printf("L(y(%d)|y(0:%d)) = %g / %g = %g\n", t, t-1, m2, m1, m2/m1);
	printf("Log.likelihood ln(L(y(%d)|y(0:%d))) = %g\n", t, t-1, 
	       (log(m2) - log(m1)));
      }
      else{
	printf("L(y(0)) = %g / %g = %g\n", m2, m1, m2/m1);
	printf("Log.likelihood ln(L(y(0))) = %g\n", (log(m2) - log(m1)));
      }
#endif
    }

    /* Write the results */
    for(i = 0; i < results->num_of_vars; i++){      
      /* 1. Decide which variable you are interested in */
      temp = results->variables[i];
      
      /* 2. Find the clique that contains the family of 
       *    the interesting variable */
      clique_of_interest = find_family(model->cliques, model->num_of_cliques, 
				       temp);
      assert(clique_of_interest != NULL);
      
      /* 3. Marginalisation (the memory must have been allocated) */
      marginalise(clique_of_interest, temp, results->data[t][i]);
      
      /* 4. Normalisation */
      normalise_array(results->data[t][i], CARDINALITY(temp));
    } 

    /* Start a message pass between time slices (compute new alpha) */
    if(start_timeslice_message_pass(model, forward, 
				    alpha) != NO_ERROR){
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      free_uncertainseries(results);
      free_potential(alpha);
      return NULL;
    }

#ifdef DEBUG_NIP
    /* print alpha */
    m2 = 0;
    for(i = 0; i < alpha->size_of_data; i++)
      m2 += alpha->data[i];
    printf("Sum(alpha) = %g\n", m2);
#endif
   
    /* Forget old evidence */
    reset_model(model);
    use_priors(model, HAD_A_PREVIOUS_TIMESLICE);
  }
  free_potential(alpha); 

  return results;
}


/* This consumes much more memory depending on the size of the 
 * sepsets between time slices. */
uncertain_series forward_backward_inference(time_series ts,
					   variable vars[], int nvars,
					    double* loglikelihood){
  int i, t;
  int *cardinalities = NULL;
  double m1, m2;
  variable temp;
  potential *alpha_gamma = NULL;
  clique clique_of_interest;
  uncertain_series results = NULL;
  nip model = ts->model;

  /* Allocate an array for describing the dimensions of timeslice sepsets */
  if(model->outgoing_interface_size > 0){
    cardinalities = (int*) calloc(model->outgoing_interface_size, sizeof(int));
    if(!cardinalities){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return NULL;
    }
  }

  /* Fill the array */
  for(i = 0; i < model->outgoing_interface_size; i++)
    cardinalities[i] = CARDINALITY(model->outgoing_interface[i]);

  /* Allocate some space for the results */
  results = (uncertain_series) malloc(sizeof(uncertain_series_type));
  if(!results){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(cardinalities);
    return NULL;
  }
  
  results->variables = (variable*) calloc(nvars, sizeof(variable));
  if(!results->variables){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(results);
    free(cardinalities);
    return NULL;
  }

  /* Copy the references to the variables of interest */
  memcpy(results->variables, vars, nvars*sizeof(variable));
  results->num_of_vars = nvars;
  results->length = ts->length;

  results->data = (double***) calloc(ts->length, sizeof(double**));
  if(!results->data){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(results->variables);
    free(results);
    free(cardinalities);
    return NULL;
  }
  
  for(t = 0; t < results->length; t++){
    results->data[t] = (double**) calloc(nvars, sizeof(double*));
    if(!results->data[t]){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      while(t > 0){
	t--;
	for(i = 0; i < nvars; i++)
	  free(results->data[t][i]);
	free(results->data[t]);
      }
      free(results->data); /* t == 0 */
      free(results->variables);
      free(results);
      free(cardinalities);
      return NULL;
    }

    for(i = 0; i < nvars; i++){
      results->data[t][i] = (double*) calloc(CARDINALITY(vars[i]),
					     sizeof(double));
      if(!results->data[t][i]){
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
	while(i > 0)
	  free(results->data[t][--i]);
	free(results->data[t]);
	while(t > 0){
	  t--;
	  for(i = 0; i < nvars; i++)
	    free(results->data[t][i]);
	  free(results->data[t]);
	}
	free(results->data); /* t == 0 */
	free(results->variables);
	free(results);
	free(cardinalities);
	return NULL;
      }
    }
  }

  /* Allocate some space for the intermediate potentials */
  alpha_gamma = (potential *) calloc(ts->length + 1, sizeof(potential));

  /* Initialise intermediate potentials */
  for(t = 0; t <= ts->length; t++){
    alpha_gamma[t] = make_potential(cardinalities, 
				    model->outgoing_interface_size, 
				    NULL);
  }
  free(cardinalities);

  /*****************/
  /* Forward phase */
  /*****************/
  reset_model(model);
  use_priors(model, !HAD_A_PREVIOUS_TIMESLICE);
  if(loglikelihood)
    *loglikelihood = 0; /* init */

  for(t = 0; t < ts->length; t++){ /* FOR EVERY TIMESLICE */
    
    if(t > 0)
      if(finish_timeslice_message_pass(model, forward, 
				       alpha_gamma[t-1], NULL) != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	free_uncertainseries(results);
	for(i = 0; i <= ts->length; i++)
	  free_potential(alpha_gamma[i]);
	free(alpha_gamma);
	return NULL;
      }

    /* Likelihood reference... */
    if(loglikelihood){
      make_consistent(model);
      m1 = model_prob_mass(model);
    }

    /* Put some data in (Q: should this be AFTER message passing?) */
    insert_ts_step(ts, t, model, MARK_ON);
    
    /* Do the inference */
    make_consistent(model);

    /* Compute loglikelihood if required */
    if(loglikelihood){
      /* Q: Is this L(y(t) | y(0:t-1)) 
       * A: Yes... */
      m2 = model_prob_mass(model);
      if((m1 > 0) && (m2 > 0)){
	*loglikelihood = (*loglikelihood) + (log(m2) - log(m1));
      }
      /* Check for anomalies */
      assert(*loglikelihood < 0.0);
      assert(m2 >= 0.0);
      if(m2 == 0){
	*loglikelihood = -DBL_MAX; /* -infinity ? */
      }
    }

    /* Start a message pass between timeslices */
    if(start_timeslice_message_pass(model, forward,
				    alpha_gamma[t]) != NO_ERROR){
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      free_uncertainseries(results);
      for(i = 0; i <= ts->length; i++)
	free_potential(alpha_gamma[i]);
      free(alpha_gamma);
      return NULL;
    }

    /* Forget old evidence */
    reset_model(model);
    if(ts->length > 1)
      use_priors(model, HAD_A_PREVIOUS_TIMESLICE);
    else
      use_priors(model, !HAD_A_PREVIOUS_TIMESLICE);
  }
  
  /******************/
  /* Backward phase */
  /******************/
  for(t = ts->length - 1; t >= 0; t--){ /* FOR EVERY TIMESLICE */
    
    /* Pass the message from the past */
    if(t > 0)
      if(finish_timeslice_message_pass(model, forward, 
				       alpha_gamma[t-1], 
				       NULL)                   != NO_ERROR){
	
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	free_uncertainseries(results);
	for(i = 0; i <= ts->length; i++)
	  free_potential(alpha_gamma[i]);
	free(alpha_gamma);
	return NULL;
      }
    
    /* Put some evidence in */
    insert_ts_step(ts, t, model, MARK_ON);

    /* Pass the message from the future */
    if(t < ts->length - 1)
      if(finish_timeslice_message_pass(model, backward, 
				       alpha_gamma[t+1], /* gamma */
				       alpha_gamma[t]  ) /* alpha */
	 != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	free_uncertainseries(results);
	for(i = 0; i <= ts->length; i++)
	  free_potential(alpha_gamma[i]);
	free(alpha_gamma);
	return NULL;
      }

    /* Do the inference */
    make_consistent(model);

    /* THE CORE: Write the results */
    for(i = 0; i < results->num_of_vars; i++){
      
      /* 1. Decide which variable you are interested in */
      temp = results->variables[i];
      
      /* 2. Find the clique that contains the family of 
       *    the interesting variable */
      clique_of_interest = find_family(model->cliques, 
				       model->num_of_cliques, 
				       temp);
      assert(clique_of_interest != NULL);
      
      /* 3. Marginalisation (the memory must have been allocated) */
      marginalise(clique_of_interest, temp, results->data[t][i]);
      
      /* 4. Normalisation */
      normalise_array(results->data[t][i], CARDINALITY(temp));
    }
    /* End of the CORE */

    /* Pass the message to the past */
    if(t > 0)
      if(start_timeslice_message_pass(model, backward, 
				      alpha_gamma[t]) != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	free_uncertainseries(results);
	for(i = 0; i <= ts->length; i++)
	  free_potential(alpha_gamma[i]);
	free(alpha_gamma);
	return NULL;
      }

    /* forget old evidence */
    reset_model(model);
    if(t > 1)
      use_priors(model, HAD_A_PREVIOUS_TIMESLICE);
    else
      use_priors(model, !HAD_A_PREVIOUS_TIMESLICE);
  }

  /* free the intermediate potentials */
  for(t = 0; t <= ts->length; t++)
    free_potential(alpha_gamma[t]);
  free(alpha_gamma);

  return results;
}


variable model_variable(nip model, char* symbol){
  int i;

  if(!model){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return NULL;
  }

  for(i = 0; i < model->num_of_vars; i++)
    if(strcmp(symbol, model->variables[i]->symbol) == 0)
      return model->variables[i];

  return NULL;
}


void make_consistent(nip model){
  int i;
  for (i = 0; i < model->num_of_cliques; i++)
    unmark_clique(model->cliques[i]);

  if(collect_evidence(NULL, NULL, model->cliques[0]) != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return;
  }

  for (i = 0; i < model->num_of_cliques; i++)
    unmark_clique(model->cliques[i]);

  if(distribute_evidence(model->cliques[0]) != NO_ERROR)
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);

  return;
}


/* Most likely state sequence of the variables given the timeseries. */
time_series mlss(variable vars[], int nvars, time_series ts){
  int i, j, k, l, t;
  time_series mlss;

  /* Allocate some space for the results */
  mlss = (time_series)malloc(sizeof(time_series_type));
  if(!mlss){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  mlss->model = ts->model;
  mlss->num_of_hidden = ts->model->num_of_vars - nvars;
  mlss->num_of_observed = nvars;
  mlss->hidden = (variable*) calloc(mlss->num_of_hidden, sizeof(variable));
  mlss->observed = (variable*) calloc(nvars, sizeof(variable));
  mlss->length = ts->length;
  mlss->data = (int**) calloc(mlss->length, sizeof(int*));
  if(!(mlss->data && mlss->observed && mlss->hidden)){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(mlss->data); /* useless? */
    free(mlss->observed);
    free(mlss->hidden);
    free(mlss);
    return NULL;
  }

  for(t=0; t < mlss->length; t++){
    mlss->data[t] = (int*) calloc(nvars, sizeof(int));
    if(!(mlss->data[t])){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      while(t > 0)
	free(mlss->data[--t]);
      free(mlss->data);
      free(mlss->observed);
      free(mlss->hidden);
      free(mlss);
      return NULL;
    }
  }

  /* Copy the variable references */
  memcpy(mlss->observed, vars, nvars*sizeof(variable));

  /* Find out the "hidden", or more like uninteresting, variables */
  l = 0;
  for(i=0; i < ts->model->num_of_vars; i++){
    if(l == mlss->num_of_hidden)
      break;

    k = 1;
    for(j=0; j < nvars; j++){
      if(equal_variables(ts->model->variables[i], vars[j])){
	k = 0;
	break;
      }
    }
    
    if(k)
      mlss->hidden[l++] = ts->model->variables[i];
  }

  /* TODO: write the algorithm here 
   * - allocate a (massive?) chunk of memory for intermediate results 
   *   - find out the "geometry" of the intermediate data
   *
   * - do a sort of forward inference on the best states 
   *   - for each variable of interest separately? NO !?!?
   *   - by inserting the last state of a hidden variable as hard evidence?
   *     (or is the goal to maximize the probability of observations)
   *
   * - find out the result by iterating backwards the best choices
   */

  /* NOT IMPLEMENTED YET! */
  report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
  return mlss;
}


/* My apologies: this function is probably the worst copy-paste case ever. 
 * Any ideas how to avoid repeating the same parts of code?
 * - function pointers are pretty much out of the question in this case, 
 *   because they can't deliver the results without global variables
 * - some parts of the code could be (and have been) transformed into 
 *   separate procedures */
static int e_step(time_series ts, potential* parameters, 
		  double* loglikelihood){
  int i, t;
  int error;
  int *cardinalities = NULL;
  int nobserved;
  int *data = NULL;
  int *mapping;
  variable* observed = NULL;
  variable v;
  potential *alpha_gamma = NULL;
  potential *results = NULL;
  potential p;
  clique c = NULL;
  nip model = ts->model;
  double m1, m2;

  /* Reserve some memory for computation */
  nobserved = ts->num_of_observed;
  observed = (variable*) calloc(nobserved, sizeof(variable));
  data     = (int*) calloc(nobserved, sizeof(int));
  results = (potential*) calloc(model->num_of_vars, sizeof(potential));
  if(!(results && data && observed && loglikelihood)){
    if(loglikelihood){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      error = ERROR_OUTOFMEMORY;
    }
    else{
      report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
      error = ERROR_INVALID_ARGUMENT;
    }
    free(data);
    free(observed);
    free(results);
    return error;
  }
  for(i = 0; i < model->num_of_vars; i++){
    p = parameters[i];
    results[i] = make_potential(CARDINALITY(p), p->num_of_vars, NULL);
    /* in principle, make_potential can return NULL */
  }  

  /* Allocate some space for the intermediate potentials between timeslices */
  alpha_gamma = (potential *) calloc(ts->length + 1, sizeof(potential));

  /* Allocate an array for describing the dimensions of alpha & gamma */
  if(model->outgoing_interface_size > 0){
    cardinalities = (int*) calloc(model->outgoing_interface_size, 
				  sizeof(int));
    if(!cardinalities){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      for(i = 0; i < model->num_of_vars; i++)
	free_potential(results[i]);
      free(results);
      free(data);
      free(observed);
      free(alpha_gamma);
      return ERROR_OUTOFMEMORY;
    }
  }

  for(i = 0; i < model->outgoing_interface_size; i++)
    cardinalities[i] = CARDINALITY(model->outgoing_interface[i]);

  /* Initialise intermediate potentials */
  for(t = 0; t <= ts->length; t++)
    alpha_gamma[t] = make_potential(cardinalities, 
				    model->outgoing_interface_size, 
				    NULL);
  free(cardinalities);

  /*****************/
  /* Forward phase */
  /*****************/
  reset_model(model);
  use_priors(model, !HAD_A_PREVIOUS_TIMESLICE);
  *loglikelihood = 0; /* init */
  
  for(t = 0; t < ts->length; t++){ /* FOR EVERY TIMESLICE */
    
    if(t > 0){
      if(finish_timeslice_message_pass(model, forward, 
				       alpha_gamma[t-1], NULL) != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	/* i is useless at this point */
	for(i = 0; i < model->num_of_vars; i++)
	  free_potential(results[i]);
	free(results);
	free(data);
	free(observed);
	for(i = 0; i <= ts->length; i++)
	  free_potential(alpha_gamma[i]);
	free(alpha_gamma);
	return ERROR_GENERAL;
      }
    }

    /* Propagate message and make sure sepsets have correct weight */
    make_consistent(model); 

    m1 = model_prob_mass(model);

    insert_ts_step(ts, t, model, MARK_ON); /* Put some data in */

    make_consistent(model); /* Do the inference */

    /* This computes the log likelihood (ratio of probability masses) */
    m2 = model_prob_mass(model);
    if((m1 > 0) && (m2 > 0)){
      *loglikelihood = (*loglikelihood) + (log(m2) - log(m1));
    }

    /* Check for anomalies like invalid initial guess for parameters */
    if((m1 <= 0) || 
       (m2 <= 0) || 
       (*loglikelihood > 0)){
      for(i = 0; i < model->num_of_vars; i++)
	free_potential(results[i]);
      free(results);
      free(data);
      free(observed);
      for(i = 0; i <= ts->length; i++)
	free_potential(alpha_gamma[i]);
      free(alpha_gamma);


      /* DEBUG */
#ifdef DEBUG_NIP
      printf("t  = %d\n", t);
      printf("m1 = %g\n", m1);
      printf("m2 = %g\n", m2);
      printf("ll = %g\n", *loglikelihood);
      for(i = 0; i < model->num_of_cliques; i++){
	print_clique(model->cliques[i]);
	print_potential(model->cliques[i]->original_p);
      }
#endif


      return ERROR_BAD_LUCK;
    }
    
    /* Start a message pass between timeslices */
    if(start_timeslice_message_pass(model, forward,
				    alpha_gamma[t]) != NO_ERROR){
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      for(i = 0; i < model->num_of_vars; i++)
	free_potential(results[i]);
      free(results);
      free(data);
      free(observed);
      for(i = 0; i <= ts->length; i++)
	free_potential(alpha_gamma[i]);
      free(alpha_gamma);
      return ERROR_GENERAL;
    }

    /* Forget old evidence */
    reset_model(model);
    if(ts->length > 1)
      use_priors(model, HAD_A_PREVIOUS_TIMESLICE);
    else
      use_priors(model, !HAD_A_PREVIOUS_TIMESLICE);
  }
  
  /******************/
  /* Backward phase */
  /******************/
  for(t = ts->length - 1; t >= 0; t--){ /* FOR EVERY TIMESLICE */

    /* Pass the message from the past */
    if(t > 0)
      if(finish_timeslice_message_pass(model, forward, 
				       alpha_gamma[t-1], 
				       NULL)            != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	for(i = 0; i < model->num_of_vars; i++)
	  free_potential(results[i]);
	free(results);
	free(data);
	free(observed);
	for(i = 0; i <= ts->length; i++)
	  free_potential(alpha_gamma[i]);
	free(alpha_gamma);
	return ERROR_GENERAL;
      }
    
    /* Put some evidence in */
    insert_ts_step(ts, t, model, MARK_ON); /* only marked variables */
    
    /* Pass the message from the future */
    if(t < ts->length - 1)
      if(finish_timeslice_message_pass(model, backward, 
				       alpha_gamma[t+1], 
				       alpha_gamma[t]) != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	for(i = 0; i < model->num_of_vars; i++)
	  free_potential(results[i]);
	free(results);
	free(data);
	free(observed);
	for(i = 0; i <= ts->length; i++)
	  free_potential(alpha_gamma[i]);
	free(alpha_gamma);
	return ERROR_GENERAL;
      }

    /* Do the inference */
    make_consistent(model);

    /*** THE CORE: Write the results of inference ***/
    for(i = 0; i < model->num_of_vars; i++){
      p = results[i];

      /* 1. Decide which variable you are interested in */
      v = model->variables[i];

      /* JJT 02.11.2006: Skip old interface variables for t > 0 */
      if(t > 0 && (v->if_status & INTERFACE_OLD_OUTGOING))
	continue;
      
      /* 2. Find the clique that contains the family of 
       *    the interesting variable */
      c = find_family(model->cliques, model->num_of_cliques, v);
      assert(c != NULL);
      
      /* 3. General Marginalisation from the timeslice */
      mapping = find_family_mapping(c, v);
      general_marginalise(c->p, p, mapping);


#ifdef DEBUG_NIP
      /* DEBUG */
      printf("Marginalising the family of %s ", v->symbol);
      for(j = 0; j < p->num_of_vars - 1; j++)
	printf("%s ", v->parents[j]->symbol);
      printf("from \n");
      print_clique(c);
      printf("with mapping [");
      for(j = 0; j < p->num_of_vars; j++)
	printf("%d,", mapping[j]);
      printf("]\n");
      /* FIXME: correct mapping??? */
#endif

      
      /********************/
      /* 4. Normalisation */
      /********************/
      normalise_potential(p); /* Does this cause numerical problems? */

      /* 5. THE SUM of expected counts over time */
      sum_potential(parameters[i], p); /* "parameters[i] += p" */
    }
    /*** Finished writing results for this timestep ***/

    /* Pass the message to the past */
    if(t > 0)
      if(start_timeslice_message_pass(model, backward, 
				      alpha_gamma[t]) != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	for(i = 0; i < model->num_of_vars; i++)
	  free_potential(results[i]);
	free(results);
	free(data);
	free(observed);
	for(i = 0; i <= ts->length; i++)
	  free_potential(alpha_gamma[i]);
	free(alpha_gamma);
	return ERROR_GENERAL;
      }

    /* forget old evidence */
    reset_model(model);
    if(t > 1) /* Q: Or t > 0 ?  A: No, t will be t-1 soon... */
      use_priors(model, HAD_A_PREVIOUS_TIMESLICE);
    else
      use_priors(model, !HAD_A_PREVIOUS_TIMESLICE);
  }

  /* free the space for calculations */
  for(i = 0; i < model->num_of_vars; i++)
    free_potential(results[i]);
  free(results);
  free(data);
  free(observed);

  /* free the intermediate potentials */
  for(t = 0; t <= ts->length; t++)
    free_potential(alpha_gamma[t]);
  free(alpha_gamma);

  return NO_ERROR;
}


static int m_step(potential* parameters, nip model){
  int i, j, k;
  int* fam_map;
  clique fam_clique = NULL;
  variable child = NULL;

#ifdef PARAMETER_EPSILON
  /* 0. Make sure there are no zero probabilities 
   * TODO: hide the access to private data... */
  for(i = 0; i < model->num_of_vars; i++){
    k = parameters[i]->size_of_data;
    for(j = 0; j < k; j++)
      if(parameters[i]->data[j] < PARAMETER_EPSILON){
	/*assert(parameters[i]->data[j] > 0.0);*/
	parameters[i]->data[j] = PARAMETER_EPSILON;
    /* Q: Should the tiny value be (inversely) proportional to the number 
     *    of zeros so that the added weight is constant? */
      }
  }
#endif
  
  /* 1. Normalise parameters by dividing with the sums over child variables */
  for(i = 0; i < model->num_of_vars; i++){
    /* k = CARDINALITY(model->variables[i]); */
    /* child is the first variable in the potential? */
    normalise_cpd(parameters[i]);
    /** JJT: Not so sure if this is correct! **/
  }

  /* 2. Reset the clique potentials and everything */
  total_reset(model);

  /* 3. Initialise the model with the new parameters */
  for(i = 0; i < model->num_of_vars; i++){
    child = model->variables[i];

    if(child->num_of_parents > 0){
      fam_clique = find_family(model->cliques, model->num_of_cliques, child);
      fam_map = find_family_mapping(fam_clique, child);

      /* Update the conditional probability distributions (dependencies) */
      j = init_potential(parameters[i], fam_clique->p, fam_map);
      k = init_potential(parameters[i], fam_clique->original_p, fam_map);
      if(j != NO_ERROR || k != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	return ERROR_GENERAL;
      }
      /* couldn't use initialise() here because of the messy order of
       * parameter potentials */
    }
    else{
      /* Update the priors of independent variables */
      total_marginalise(parameters[i], child->prior, 0); /* correct? */
      /*for(j = 0; j < CARDINALITY(child); j++)
	  child->prior[j] = parameters[i]->data[j];*/
    }
  }

  return NO_ERROR;
}


/* Trains the given model (ts[0]->model) according to the given set of 
 * time series (ts[*]) with EM-algorithm. Returns an error code. */
int em_learn(time_series *ts, int n_ts, double threshold,
	     doublelist learning_curve){
  int e, i, n, v;
  int *card;
  int ts_steps;
  double old_loglikelihood; 
  double loglikelihood = -DBL_MAX;
  double probe = 0;
  potential *parameters = NULL;
  nip model = NULL;

  if(!ts[0] || !ts[0]->model){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }
  model = ts[0]->model;

  if(learning_curve != NULL){
    /* Take care it's empty */
    if(LIST_LENGTH(learning_curve) > 0)
      empty_doublelist(learning_curve);
  }

  /* Reserve some memory for calculation */
  parameters = (potential*) calloc(model->num_of_vars, sizeof(potential));
  if(!parameters){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  for(v = 0; v < model->num_of_vars; v++){
    n = model->variables[v]->num_of_parents + 1;
    card = (int*) calloc(n, sizeof(int));
    if(!card){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      while(v > 0)
	free_potential(parameters[--v]);
      free(parameters);
      return ERROR_OUTOFMEMORY;
    }
    /* The child MUST be the first variable in order to normalize
     * potentials reasonably */
    card[0] = CARDINALITY(model->variables[v]);
    for(i = 1; i < n; i++)
      card[i] = CARDINALITY(model->variables[v]->parents[i-1]);
    /* variable->parents should be null only if n==1 
     * => no for-loop => no null dereference */

    parameters[v] = make_potential(card, n, NULL);
    free(card);
  }

  /* Randomize the parameters. (TODO: move this operation to potential.c?)
   * NOTE: parameters near zero are a numerical problem... 
   *       on the other hand, zeros are needed in some cases. 
   *       How to identify a "bad" zero? */
  /*random_seed(NULL);*/
  for(v = 0; v < model->num_of_vars; v++){
    random_potential(parameters[v]);
    /* the M-step will take care of the normalisation */
  }

  /* Compute total number of time steps */
  ts_steps = 0;
  for(n = 0; n < n_ts; n++)
    ts_steps += timeseries_length(ts[n]);  

  /************/
  /* THE Loop */
  /************/
  i = 0;
  do{

    /* M-Step... or at least the last part of it. 
     * On the first iteration this enters the random parameters 
     * into the model. */
    e = m_step(parameters, model);
    if(e != NO_ERROR){
      report_error(__FILE__, __LINE__, e, 1);
      for(v = 0; v < model->num_of_vars; v++){
	free_potential(parameters[v]);
      }
      free(parameters);
      if(learning_curve != NULL)
	empty_doublelist(learning_curve);
      return e;
    }

    old_loglikelihood = loglikelihood;
    loglikelihood = 0.0;

    /* Initialise the parameter potentials to "zero" for  
     * accumulating the "average parameters" in the E-step */
    for(v = 0; v < model->num_of_vars; v++){
      uniform_potential(parameters[v], 1.0); /* Q: Use pseudo counts? */

      /*memset(parameters[v]->data, 0, n * sizeof(double)); BS */

      /* the M-step will take care of the normalisation 
       * (and elimination of zeros ?) */
    }

    /* E-Step: Now this is the heavy stuff..! 
     * (for each time series separately to save memory) */
    for(n = 0; n < n_ts; n++){
      e = e_step(ts[n], parameters, &probe);
      if(e != NO_ERROR){
	if(e != ERROR_BAD_LUCK)
	  report_error(__FILE__, __LINE__, e, 1);
	/* don't report invalid random parameters */
	for(v = 0; v < model->num_of_vars; v++){
	  free_potential(parameters[v]);
	}
	free(parameters);
	if(e != ERROR_BAD_LUCK){
	  if(learning_curve != NULL)
	    empty_doublelist(learning_curve);
	}
	/* else let the list be */

	return e;
      }

      /** DEBUG **/
      assert(-HUGE_DOUBLE < probe  &&  probe <= 0.0  && 
	     probe == probe);
      /* probe != probe  =>  probe == NaN  */

      loglikelihood += probe;
    }

    /* Add an element to the linked list */
    if(learning_curve != NULL){
      e = append_double(learning_curve, loglikelihood / ts_steps);
      if(e != NO_ERROR){
	report_error(__FILE__, __LINE__, e, 1);
	for(v = 0; v < model->num_of_vars; v++){
	  free_potential(parameters[v]);
	}
	free(parameters);
	empty_doublelist(learning_curve);
	return e;
      }
    }

    /* Check if the parameters were valid in any sense */
    if(old_loglikelihood > loglikelihood + (ts_steps * threshold) ||
       loglikelihood > 0 || 
       loglikelihood == -HUGE_DOUBLE){ /* some "impossible" data */

      for(v = 0; v < model->num_of_vars; v++){
	free_potential(parameters[v]);
      }
      free(parameters);
      /* Return the list as it is */
      return ERROR_BAD_LUCK;
    }

    /* Remember this! 
     * (It helps if you insist having a minimum amount of iterations :) */
    i++;

  } while((loglikelihood - old_loglikelihood) > (ts_steps * threshold) || 
	  i < MIN_EM_ITERATIONS);
  /*** When should we stop? ***/

  for(v = 0; v < model->num_of_vars; v++){
    free_potential(parameters[v]);
  }
  free(parameters);

  return NO_ERROR;
}


/* a little wrapper */
double model_prob_mass(nip model){
  double m;
  m = probability_mass(model->cliques, model->num_of_cliques);
  return m;
}


double *get_probability(nip model, variable v){
  clique clique_of_interest;
  double *result;
  int cardinality;

  if(!model){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return NULL;
  }
  if(!v){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return NULL;
  }
  cardinality = CARDINALITY(v);
  result = (double *) calloc(cardinality, sizeof(double));
  if(!result){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  /* 1. Find the clique that contains the interesting variable */
  clique_of_interest = find_family(model->cliques, model->num_of_cliques, v);
  if(!clique_of_interest){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    free(result);
    return NULL;
  }

  /* 2. Marginalisation (the memory must have been allocated) */
  marginalise(clique_of_interest, v, result);

  /* 3. Normalisation */
  normalise_array(result, cardinality);

  /* 4. Return the result */
  return result;
}


potential get_joint_probability(nip model, variable *vars, int num_of_vars){
  potential p;
  int i;

  /* Unmark all cliques */
  for (i = 0; i < model->num_of_cliques; i++)
    unmark_clique(model->cliques[i]);

  /* Make a DFS in the tree... */
  p = gather_joint_probability(model->cliques[0], vars, num_of_vars, NULL, 0);
  if(p == NULL){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return NULL;
  }
  
  /* Normalisation (???) */
  normalise_potential(p);
  return p;
}


/* JJ: this has some common elements with the forward_inference function */
time_series generate_data(nip model, int length){
  int i, j, k, t;
  int *cardinalities = NULL;
  potential alpha = NULL;
  int nvars = model->num_of_vars;
  variable *vars = NULL;
  /* reserved the possibility to pass the set of variables 
   * as a parameter in order to omit part of the data...   */
  variable v;
  time_series ts = NULL;
  double *distribution = NULL;

  vars = (variable*) calloc(nvars, sizeof(variable));
  if(!vars){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  /* sort the variables appropriately */
  for(i = 0; i < model->num_of_vars; i++)
    unmark_variable(model->variables[i]);

  /* first the independent variables... */
  j = 0;
  for(i = 0; i < model->num_of_vars; i++){
    v = model->variables[i];
    if(!v->num_of_parents){
      vars[j++] = v;
      mark_variable(v);
    }
  }
 
  /* ...then the children whose parents are already included */
  while(j < nvars){
    for(i = 0; i < model->num_of_vars; i++){
      v = model->variables[i];
      if(variable_marked(v))
	continue;
      t = 1;
      for(k = 0; k < v->num_of_parents; k++){
	if(!variable_marked(v->parents[k])){
	  t = 0;
	  break;
	}
      }
      if(t){
	vars[j++] = v;
	mark_variable(v);
      }	
    }
  }

  /* allocate memory for the time series */
  ts = (time_series) malloc(sizeof(time_series_type));
  if(!ts){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(vars);
    return NULL;
  }
  ts->model = model;
  ts->hidden = NULL;
  ts->num_of_hidden = model->num_of_vars - nvars;
  ts->num_of_observed = nvars;
  ts->observed = vars;
  ts->length = length;
  ts->data = NULL;

  ts->data = (int**) calloc(ts->length, sizeof(int*));
  if(!ts->data){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(ts);
    free(vars);
    return(NULL);
  }
  for(t = 0; t < ts->length; t++){
    ts->data[t] = (int*) calloc(nvars, sizeof(int));
    if(!(ts->data[t])){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      while(t >= 0)
	free(ts->data[t--]);
      free(ts->data);
      free(ts);
      free(vars);
      return NULL;
    }
  }

  /* create the sepset potential between the time slices */
  if(model->outgoing_interface_size > 0){
    cardinalities = (int*) calloc(model->outgoing_interface_size, 
				  sizeof(int));
    if(!cardinalities){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free_timeseries(ts);
      return NULL;
    }
  }

  for(i = 0; i < model->outgoing_interface_size; i++)
    cardinalities[i] = CARDINALITY(model->outgoing_interface[i]);
  alpha = make_potential(cardinalities, model->outgoing_interface_size, NULL);
  free(cardinalities);
  
  /* new seed number for rand and clear the previous evidence */
  /*random_seed(NULL);*/
  reset_model(model);
  use_priors(model, !HAD_A_PREVIOUS_TIMESLICE);

  /* for each time step */
  for(t = 0; t < ts->length; t++){

    /* influence from the previous time step */
    if(t > 0){
      if(finish_timeslice_message_pass(model, forward, 
				       alpha, NULL) != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	free_timeseries(ts);
	free_potential(alpha);
	return NULL;
      }
    }
    
    /** for each variable */
    for(i = 0; i < nvars; i++){
      make_consistent(model);
      v = vars[i];
      /*** get the probability distribution */
      distribution = get_probability(model, v);
      /*** organize a lottery */
      k = lottery(distribution, CARDINALITY(v));
      free(distribution);
      /*** insert into the time series and the model as evidence */
      ts->data[t][i] = k;
      enter_i_observation(model->variables, model->num_of_vars, 
			  model->cliques, model->num_of_cliques, v, k);
    }
    make_consistent(model);

    /* influence from the current time slice to the next one */
    if(start_timeslice_message_pass(model, forward, alpha) != NO_ERROR){
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      free_timeseries(ts);
      free_potential(alpha);
      return NULL;
    }

    /* Forget old evidence */
    reset_model(model);
    use_priors(model, HAD_A_PREVIOUS_TIMESLICE);
  }
  free_potential(alpha);
  return ts;
}


/* Most of this is borrowed from Jaakko Hollmen. */
long random_seed(long* seedpointer){
  struct tm aika, *aikap;
  time_t aika2;
  long seedvalue;

  if(seedpointer == NULL){
    /* The time since midnight in seconds in the seed for
       the random number generator */
    aika2 = time(NULL);
    aikap = &aika;
    aikap = localtime(&aika2);
    seedvalue = aikap->tm_sec + 60 *aikap->tm_min
      + 3600 * aikap->tm_hour;
    seedvalue ^= (getpid() + (getpid() << 15));
  }
  else
    seedvalue = *seedpointer;
  /*srand48(seedvalue); NON-ANSI! */
  srand(seedvalue);
  return seedvalue;
}


/* Function for generating random discrete values according to a specified 
 * distribution. Values are between 0 and <size>-1 inclusive. */
int lottery(double* distribution, int size){
  int i = 0;
  double sum = 0;
  double r = rand()/(double)RAND_MAX;
  /* double r = drand48(); NON-ANSI! */
  do{
    if(i >= size){
      report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
      return size-1;
    }
    sum += distribution[i++];
  }while(sum < r);
  return i-1;
}


void print_cliques(nip model){
  int i;
  int num_of_cliques;
  sepset s;
  clique c;
  sepset_link sepsetlist;
  clique *cliques;

  if(!model){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return;
  }

  cliques = model->cliques;
  num_of_cliques = model->num_of_cliques;

  printf("Cliques of the model:\n");
  for(i = 0; i < num_of_cliques; i++){
    c = cliques[i];
    print_clique(c);
    print_potential(c->p);
    sepsetlist = c->sepsets;
    while(sepsetlist){
      s = (sepset)sepsetlist->data;
      print_sepset(s);
      print_potential(s->new);      
      sepsetlist = sepsetlist->fwd;
    }
    printf("\n");
  }
}
