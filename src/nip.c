/*
 * nip.c $Id: nip.c,v 1.92 2005-06-30 12:15:37 jatoivol Exp $
 */

#include "nip.h"
#include "parser.h"
#include "clique.h"
#include "variable.h"
#include "errorhandler.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>

/*
#define DEBUG_NIP
*/

/***** 
 * TODO: 

 * - Viterbi algorithm for the ML-estimate of the latent variables
 *   - another forward-like algorithm with elements of dynamic programming
 *   - To save huge amounts of memory, could the process use some kind of 
 *     "timeslice sepsets" for encoding the progress throughout time?

 
 * - EM algorithm for estimating parameters of the model
 *   + Invent a concise and efficient way of computing each of the parameters.
 *     + one kind of solution:
 *       + build it around the forward-backward inference... (E-step)
 *       + due to the fact that the child is always the first variable in 
 *         the potentials defining the model, the M-step is quite trivial
 *
 *   + Find a neat way to replace the original parameters of the model.
 *     + gather a potential for each family of variables
 *     + initialise with saved potentials? (see parser.c line 1115)
 *
 *   - Determine the parameters of the algorithm
 *     - when to stop?
 *       - difference in the average loglikelihood of the timeseries...
 *       - loglikelihood should be calculated during E-step


 * - Function for generating artificial data according to the model
 *   - something like forward_inference and a little loop where you
 *     set values for variables and infer the probabilities for the 
 *     next ones to be set
 *****/

extern int yyparse();

static int start_timeslice_message_pass(nip model, int direction, 
					potential sepset);
static int finish_timeslice_message_pass(nip model, int direction,
					 potential num, potential den);

static int e_step(time_series ts, potential* results, double* loglikelihood);
static int m_step(potential* results, nip model);

static int lottery(double* distribution, int size);

void reset_model(nip model){
  int i, retval;

  for(i = 0; i < model->num_of_vars; i++)
    reset_likelihood(model->variables[i]);
  retval = global_retraction(model->variables, model->num_of_vars, 
			     model->cliques, model->num_of_cliques);
  if(retval != NO_ERROR)
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
}


void total_reset(nip model){
  int i, j;
  clique c;
  for(i = 0; i < model->num_of_cliques; i++){
    c = model->cliques[i];
    for(j = 0; j < c->p->size_of_data; j++){
      c->original_p->data[j] = 1;
    }
  }
  reset_model(model); /* Could that be enough? */
}


void use_priors(nip model, int has_history){
  int i, retval;
  variable v;
  for(i = 0; i < model->num_of_vars - model->num_of_children; i++){
    v = model->independent[i];
    if(!has_history || v->previous == NULL){
      retval = enter_evidence(model->variables, model->num_of_vars, 
			      model->cliques, model->num_of_cliques, 
			      v, v->prior);
      if(retval != NO_ERROR)
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    }
  }
}


nip parse_model(char* file){
  int i, j, k, retval;
  variable temp;
  variable_iterator it;
  nip new = (nip) malloc(sizeof(nip_type));

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }
  
  /* 1. Parse */
  if(open_yyparse_infile(file) != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return NULL;
  }

  retval = yyparse(); /* Reminder: priors not entered yet! */

  close_yyparse_infile();

  if(retval != 0)
    return NULL;

  /* 2. Get the parsed stuff and make a model out of them */
  new->num_of_cliques = get_num_of_cliques();
  new->num_of_vars = total_num_of_vars();
  new->cliques = *get_cliques_pointer();

  new->variables = (variable*) calloc(new->num_of_vars, sizeof(variable));
  if(!(new->variables)){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(new);
    return NULL;
  }
    
  it = get_first_variable();
  temp = next_variable(&it);
  i = 0;
  while(temp != NULL){
    new->variables[i++] = temp;
    temp = next_variable(&it);
  }
  
  /* count the number of nexts and children */
  new->num_of_nexts = 0;
  new->num_of_children = 0;
  for(i = 0; i < new->num_of_vars; i++){
    if(new->variables[i]->next)
      new->num_of_nexts++;

    if(new->variables[i]->parents)
      new->num_of_children++;
  }

  new->next = (variable*) calloc(new->num_of_nexts, sizeof(variable));
  new->previous = (variable*) calloc(new->num_of_nexts, sizeof(variable));
  new->children = (variable*) calloc(new->num_of_children, sizeof(variable));
  /*  printf("\nDEBUG %s (%d): num_of_children = %d\n", __FILE__, __LINE__, 
   *	 new->num_of_children);*/
  new->independent = 
    (variable*) calloc(new->num_of_vars - new->num_of_children, 
		       sizeof(variable));
  if(!(new->independent && new->children && new->previous && new->next)){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(new->variables);
    free(new->next);
    free(new->previous);
    free(new->children);
    free(new);
    return NULL;
  }

  j = 0;
  for(i = 0; i < new->num_of_vars; i++)
    if(new->variables[i]->next){
      new->next[j] = new->variables[i];
      new->previous[j] = new->variables[i]->next;
      j++;
    }

  /* Reminder: (Before I indexed the children with j, the program had 
   * funny crashes on Linux systems :) */
  j = 0; k = 0;
  for(i = 0; i < new->num_of_vars; i++)
    if(new->variables[i]->parents)
      new->children[j++] = new->variables[i]; /* set the children */
    else
      new->independent[k++] = new->variables[i];
  
  new->front_clique = NULL;
  new->tail_clique = NULL;
  get_parser_node_size(&(new->node_size_x), &(new->node_size_y));

  /* Let's check one detail */
  for(i = 0; i < new->num_of_vars - new->num_of_children; i++)  
    assert(new->independent[i]->num_of_parents == 0);

  /* 4. Reset parser globals */
  it = get_last_variable(); /* free the list structure */
  while(it->bwd){
    it = it->bwd;
    free(it->fwd);
  }
  free(it);
  reset_variable_list();
  reset_clique_array();

  return new;
}


int write_model(nip model, char* name){
  int i, j, n;
  int x, y;
  FILE *f = NULL;
  char *filename = NULL;
  variable v = NULL;
  int *temp = NULL;
  int *map = NULL;
  clique c = NULL;
  potential p = NULL;

  /* form the filename */
  filename = (char*) calloc(strlen(name) + 5, sizeof(char));
  if(!filename){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }
  strcpy(filename, name);   /* copy the model name */
  strcat(filename, ".net"); /* append with the extension */

  /* open the stream */
  f = fopen(filename, "w");
  if(!f){
    report_error(__FILE__, __LINE__, ERROR_IO, 1);
    free(filename);
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
  fprintf(f, "class %s \n", name);
  fputs("{ \n", f);
  fputs("    inputs = (); \n", f);
  fputs("    outputs = (); \n", f);
  fprintf(f, "    node_size = (%d %d); \n", 
	model->node_size_x, model->node_size_y);

  /** the variables **/
  for(i = 0; i < model->num_of_vars; i++){
    v = model->variables[i];
    n = number_of_values(v)-1;
    get_position(v, &x, &y);
    fputs("\n", f);
    fprintf(f, "    node %s \n", get_symbol(v));
    fputs("    { \n", f);
    fprintf(f, "        label = \"%s\"; \n", v->name);
    fprintf(f, "        position = (%d %d); \n", x, y);
    fprintf(f, "        states = (");
    for(j = 0; j < n; j++)
      fprintf(f, "\"%s\" \n                  ", v->statenames[j]);
    fprintf(f, "\"%s\"); \n", v->statenames[n]);
    if(v->next)
      fprintf(f, "        NIP_next = \"%s\"; \n", get_symbol(v->next));
    fputs("    } \n", f);
    fflush(f);
  }

  /** the priors **/
  for(i = 0; i < model->num_of_vars - model->num_of_children; i++){
    v = model->independent[i];
    n = number_of_values(v) - 1;
    fputs("\n", f);
    /* independent variables have priors */
    fprintf(f, "    potential (%s) \n", get_symbol(v));
    fputs("    { \n", f);
    fputs("        data = ( ", f);
    for(j = 0; j < n; j++)
      fprintf(f, "%f  ", v->prior[j]);
    fprintf(f, "%f ); \n", v->prior[n]);
    fputs("    } \n", f);
    fflush(f);
  }

  /** the potentials **/
  for(i = 0; i < model->num_of_children; i++){
    v = model->children[i];
    n = number_of_parents(v) - 1;
    fputs("\n", f);
    /* child variables have conditional distributions */
    fprintf(f, "    potential (%s | ", get_symbol(v));
    for(j = n; j > 0; j--) /* Hugin fellas put parents in reverse order */
      fprintf(f, "%s ", get_symbol(v->parents[j]));
    fprintf(f, "%s)\n", get_symbol(v->parents[0]));
    fputs("    { \n", f);
    fputs("        data = (", f);

    n++; /* number of parents */
    temp = (int*) calloc(n+1, sizeof(int));
    if(!temp){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      fclose(f);
      free(filename);
      return ERROR_OUTOFMEMORY;
    }

    /* form the potential */
    temp[0] = v->cardinality; /* child must be the first... */
    /* ...then the parents */
    for(j = 0; j < n; j++)
      temp[j+1] = v->parents[j]->cardinality;
    p = make_potential(temp, n+1, NULL);

    /* compute the distribution */
    c = find_family(model->cliques, model->num_of_cliques, v);
    map = find_family_mapping(c, v);
    general_marginalise(c->p, p, map);

    /* normalisation */
    n = number_of_values(v);    
    for(j = 0; j < p->size_of_data; j += n)
      normalise(p->data + j, n);

    /* print the stuff */
    for(j = 0; j < p->size_of_data; j++){
      if(j > 0 && j % n == 0)
	fputs("\n                ", f);
      fprintf(f, " %f ", p->data[j]);
    }
    fputs("); \n", f);
    fputs("    } \n", f);
    free(temp);
    free_potential(p);
    fflush(f);
  }

  fputs("} \n", f); /* the last brace */

  /* close the file */
  if(fclose(f)){
    report_error(__FILE__, __LINE__, ERROR_IO, 1);
    free(filename);
    return ERROR_IO;
  }
  free(filename);
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
  free(model->children);
  free(model->independent);
  free(model->next);
  free(model->previous);
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
  
  df = open_datafile(filename, ',', 0, 1);

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
	
	/* 3. Put into the data array */
	for(i = 0; i < m; i++){
	  v = model_variable(model, df->node_symbols[i]);
	  if(v)
	    ts->data[j][i] = get_stateindex(v, tokens[i]);
	  
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


int write_timeseries(time_series ts, char *filename){
  int i, n, t;
  int d;
  variable v;
  FILE *f = NULL;

  n = ts->model->num_of_vars - ts->num_of_hidden;
  if(!(n && ts && filename)){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  /* Try to open the file for write */
  f = fopen(filename, "w");
  if(!f){
    report_error(__FILE__, __LINE__, ERROR_IO, 1);
    return ERROR_IO;
  }

  /* Write names of the variables */
  fprintf(f, "%s", get_symbol(ts->observed[0]));
  for(i = 1; i < n; i++){
      v = ts->observed[i];
      fprintf(f, ", %s", get_symbol(v));
  }
  fputs("\n", f);

  /* Write the data */
  for(t = 0; t < ts->length; t++){
    v = ts->observed[0];
    d = ts->data[t][0];
    if(d >= 0)
      fprintf(f, "%s", get_statename(v, d));
    else
      fputs("null", f);

    for(i = 1; i < n; i++){
      v = ts->observed[i];
      d = ts->data[t][i];
      if(d >= 0)
	fprintf(f, ", %s", get_statename(v, d));
      else
	fputs(", null", f);
    }
    fputs("\n", f);
  }
  
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


int timeseries_length(time_series ts){
  return ts->length;
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


/* note that <model> may be different from ts->model */
int insert_ts_step(time_series ts, int t, nip model){
  int i;
  if(t > timeseries_length(ts))
    return ERROR_INVALID_ARGUMENT;
    
  for(i = 0; i < ts->model->num_of_vars - ts->num_of_hidden; i++){
    if(ts->data[t][i] >= 0)
      enter_i_observation(model->variables, model->num_of_vars, 
			  model->cliques, model->num_of_cliques, 
			  ts->observed[i], ts->data[t][i]);
  }
  return NO_ERROR;
}


/* note that <model> may be different from ucs->model */
int insert_ucs_step(uncertain_series ucs, int t, nip model){
  int i, e;
  for(i = 0; i < ucs->num_of_vars; i++){
    e = enter_evidence(model->variables, model->num_of_vars, 
		       model->cliques, model->num_of_cliques, 
		       ucs->variables[i], ucs->data[t][i]);
    if(e != NO_ERROR){
      report_error(__FILE__, __LINE__, e, 1);  
      return e;
    }
  }
  return NO_ERROR;
}


/* Starts a message pass between timeslices */
static int start_timeslice_message_pass(nip model, int direction,
					potential sepset){
  int i, j, k;
  int *mapping;
  clique c;
  variable *vars;
  int nvars = model->num_of_nexts;

  if(direction == FORWARD){
    vars = model->next;
    c = model->front_clique; /* null if not yet memoized */
  }
  else{
    vars = model->previous;
    c = model->tail_clique; /* null if not yet memoized */
  }

  /* some cliques are memoized */
  if(c == NULL){
    c = find_clique(model->cliques, model->num_of_cliques, vars, nvars);

    /* this saves the result */
    if(direction == FORWARD)
      model->front_clique = c; 
    else
      model->tail_clique = c;
  }
  assert(c != NULL);

  mapping = (int*) calloc(nvars, sizeof(int));
  if(!mapping){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  k = 0;
  for(i=0; i < c->p->num_of_vars; i++){
    if(k == nvars)
      break; /* all pointers found */
    for(j=0; j < nvars; j++){
      if(equal_variables((c->variables)[i], vars[j])){
	mapping[j] = i;
	k++;
	break;
      }
    }
  }    

  /* the marginalisation */
  general_marginalise(c->p, sepset, mapping);
  free(mapping); /* should mapping-array be a part of sepsets? */

  /* normalisation in order to avoid drifting towards zeros */
  normalise(sepset->data, sepset->size_of_data);

  /* Q: since potential arrays may be large, should the 
   * sum of elements be normalised to N instead of 1..???     
   */

  return NO_ERROR;
}


/* Finishes the message pass between timeslices */
static int finish_timeslice_message_pass(nip model, int direction,
					 potential num, potential den){
  int i, j, k;
  int *mapping;
  clique c;
  variable *vars;
  int nvars = model->num_of_nexts;

  /* This uses memoization for finding a suitable clique c */
  if(direction == FORWARD){
    vars = model->previous;
    c = model->tail_clique;
  }
  else{
    vars = model->next;
    c = model->front_clique;
  }

  if(c == NULL){
    c = find_clique(model->cliques, model->num_of_cliques, vars, nvars);
    if(direction == FORWARD)
      model->tail_clique = c;
    else
      model->front_clique = c;
  }
  assert(c != NULL);

  mapping = (int*) calloc(nvars, sizeof(int));
  if(!mapping){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }
  k = 0;
  for(i=0; i < c->p->num_of_vars; i++){
    if(k == nvars)
      break; /* all pointers found */
    for(j=0; j < nvars; j++){
      if(equal_variables((c->variables)[i], vars[j])){
	mapping[j] = i;
	k++;
	break;
      }
    }
  }

  /* the multiplication */
  update_potential(num, den, c->p, mapping);
  free(mapping);
  
  return NO_ERROR;
}


/* forward-only inference consumes constant (1 time slice) amount of memory 
 * + the results (which is linear) */
uncertain_series forward_inference(time_series ts, variable vars[], int nvars){
  int i, k, t;
  int *cardinalities = NULL;
  variable temp;
  potential timeslice_sepset = NULL;
  clique clique_of_interest;
  uncertain_series results = NULL;
  nip model = ts->model;
  
  /* Allocate an array */
  if(model->num_of_nexts > 0){
    cardinalities = (int*) calloc(model->num_of_nexts, sizeof(int));
    if(!cardinalities){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return NULL;
    }
  }

  /* Fill the array */
  k = 0;
  for(i = 0; i < ts->num_of_hidden; i++){
    temp = ts->hidden[i];
    if(temp->next)
      cardinalities[k++] = number_of_values(temp);
  }

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
	  free(results->data[t][i]); /* Fixed 15.3.2005. Did it help? */
	free(results->data[t]);
      }
      free(results->data); /* t == -1 */
      free(results->variables);
      free(results);
      free(cardinalities);
      return NULL;
    }

    for(i = 0; i < nvars; i++){
      results->data[t][i] = (double*) calloc(number_of_values(vars[i]),
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
  timeslice_sepset = make_potential(cardinalities, model->num_of_nexts, NULL);
  free(cardinalities);

  /*****************/
  /* Forward phase */
  /*****************/
  reset_model(model);
  use_priors(model, !HAD_A_PREVIOUS_TIMESLICE);

  for(t = 0; t < ts->length; t++){ /* FOR EVERY TIMESLICE */
    
    if(t > 0)
      if(finish_timeslice_message_pass(model, FORWARD, 
				       timeslice_sepset, NULL) != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	free_uncertainseries(results);
	free_potential(timeslice_sepset);
	return NULL;
      }
    
    /* Put some data in (Q: should this be AFTER message passing) */
    insert_ts_step(ts, t, model);
    
    /* Do the inference */
    make_consistent(model);

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
      normalise(results->data[t][i], number_of_values(temp));
    } 
   
    /* Start a message pass between time slices */
    if(start_timeslice_message_pass(model, FORWARD, 
				    timeslice_sepset) != NO_ERROR){
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      free_uncertainseries(results);
      free_potential(timeslice_sepset);
      return NULL;
    }
    /* Forget old evidence */
    reset_model(model);
  }
  free_potential(timeslice_sepset); 

  return results;
}


/* This consumes much more memory depending on the size of the 
 * sepsets between time slices. */
uncertain_series forward_backward_inference(time_series ts,
					   variable vars[], int nvars){
  int i, k, t;
  int *cardinalities = NULL;
  variable temp;
  potential *timeslice_sepsets = NULL;
  clique clique_of_interest;
  uncertain_series results = NULL;
  nip model = ts->model;

  /* Allocate an array for describing the dimensions of timeslice sepsets */
  if(model->num_of_nexts > 0){
    cardinalities = (int*) calloc(model->num_of_nexts, sizeof(int));
    if(!cardinalities){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return NULL;
    }
  }

  /* Fill the array */
  k = 0;
  for(i = 0; i < ts->num_of_hidden; i++){
    temp = ts->hidden[i];
    if(temp->next)
      cardinalities[k++] = number_of_values(temp);
  }

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
      results->data[t][i] = (double*) calloc(number_of_values(vars[i]),
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
  timeslice_sepsets = (potential *) calloc(ts->length + 1, sizeof(potential));

  /* Initialise intermediate potentials */
  for(t = 0; t <= ts->length; t++){
    timeslice_sepsets[t] = make_potential(cardinalities, model->num_of_nexts, 
					  NULL);
  }
  free(cardinalities);


  /*****************/
  /* Forward phase */
  /*****************/
  reset_model(model);
  use_priors(model, !HAD_A_PREVIOUS_TIMESLICE);

  for(t = 0; t < ts->length; t++){ /* FOR EVERY TIMESLICE */
    
    if(t > 0)
      if(finish_timeslice_message_pass(model, FORWARD, 
				       timeslice_sepsets[t-1], 
				       NULL)                   != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	free_uncertainseries(results);
	for(i = 0; i <= ts->length; i++)
	  free_potential(timeslice_sepsets[i]);
	free(timeslice_sepsets);
	return NULL;
      }

    /* Put some data in (Q: should this be AFTER message passing?) */
    insert_ts_step(ts, t, model);    
    
    /* Do the inference */
    make_consistent(model);
    
    /* Start a message pass between timeslices */
    if(start_timeslice_message_pass(model, FORWARD,
				    timeslice_sepsets[t]) != NO_ERROR){
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      free_uncertainseries(results);
      for(i = 0; i <= ts->length; i++)
	free_potential(timeslice_sepsets[i]);
      free(timeslice_sepsets);
      return NULL;
    }

    /* Forget old evidence */
    reset_model(model);
    use_priors(model, HAD_A_PREVIOUS_TIMESLICE);
  }
  
  /******************/
  /* Backward phase */
  /******************/
  
  /* forget old evidence */
  reset_model(model);
  use_priors(model, HAD_A_PREVIOUS_TIMESLICE);
  
  for(t = ts->length - 1; t >= 0; t--){ /* FOR EVERY TIMESLICE */
    
    /* Pass the message from the past */
    if(t > 0)
      if(finish_timeslice_message_pass(model, FORWARD, 
				       timeslice_sepsets[t-1], 
				       NULL)                   != NO_ERROR){

	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	free_uncertainseries(results);
	for(i = 0; i <= ts->length; i++)
	  free_potential(timeslice_sepsets[i]);
	free(timeslice_sepsets);
	return NULL;
      }
    
    /* Pass the message from the future */
    if(t < ts->length - 1)
      if(finish_timeslice_message_pass(model, BACKWARD, 
				       timeslice_sepsets[t+1], 
				       timeslice_sepsets[t]) != NO_ERROR){

	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	free_uncertainseries(results);
	for(i = 0; i <= ts->length; i++)
	  free_potential(timeslice_sepsets[i]);
	free(timeslice_sepsets);
	return NULL;
      }

    /* Put some evidence in */
    insert_ts_step(ts, t, model);

    /* Do the inference */
    make_consistent(model);
    

    /* THE CORE: Write the results */
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
      normalise(results->data[t][i], number_of_values(temp));
    }
    /* End of the CORE */


    if(t > 0)
      if(start_timeslice_message_pass(model, BACKWARD, 
				      timeslice_sepsets[t]) != NO_ERROR){

	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	free_uncertainseries(results);
	for(i = 0; i <= ts->length; i++)
	  free_potential(timeslice_sepsets[i]);
	free(timeslice_sepsets);
	return NULL;
      }

    /* forget old evidence */
    reset_model(model);
    use_priors(model, HAD_A_PREVIOUS_TIMESLICE);
  }

  /* free the intermediate potentials */
  for(t = 0; t <= ts->length; t++)
    free_potential(timeslice_sepsets[t]);
  free(timeslice_sepsets);

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
 * - some parts of the code could be transformed into separate procedures */
static int e_step(time_series ts, potential* parameters, 
		  double* loglikelihood){
  int i, j, k, t, size;
  int *cardinalities = NULL;
  int nobserved;
  int *data = NULL;
  variable* observed = NULL;
  variable temp;
  potential *timeslice_sepsets = NULL;
  potential *results = NULL;
  potential p;
  clique clique_of_interest = NULL;
  nip model = ts->model;

  /* Reserve some memory for calculation */
  nobserved = model->num_of_vars - ts->num_of_hidden;
  observed = (variable*) calloc(nobserved, sizeof(variable));
  data     = (int*) calloc(nobserved, sizeof(int));
  results = (potential*) calloc(model->num_of_vars, sizeof(potential));
  if(!(results && data && observed)){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    free(data);
    free(observed);
    return ERROR_OUTOFMEMORY;
  }
  for(i = 0; i < model->num_of_vars; i++){
    k = model->variables[i]->num_of_parents + 1;
    p = parameters[i];
    results[i] = make_potential(p->cardinality, k, NULL);
    /* Initialise the sum by setting to zero */
    memset(p->data, 0, p->size_of_data * sizeof(double));
  }  

  /* Allocate some space for the intermediate potentials between timeslices */
  timeslice_sepsets = (potential *) calloc(ts->length + 1, sizeof(potential));

  /* Allocate an array for describing the dimensions of timeslice sepsets */
  if(model->num_of_nexts > 0){
    cardinalities = (int*) calloc(model->num_of_nexts, sizeof(int));
    if(!cardinalities){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      for(i = 0; i < model->num_of_vars; i++)
	free_potential(results[i]);
      free(results);
      free(data);
      free(observed);
      free(timeslice_sepsets);
      return ERROR_OUTOFMEMORY;
    }
  }
  k = 0;
  for(i = 0; i < ts->num_of_hidden; i++){
    temp = ts->hidden[i];
    if(temp->next)
      cardinalities[k++] = number_of_values(temp);
  }

  /* Initialise intermediate potentials */
  for(t = 0; t <= ts->length; t++){
    timeslice_sepsets[t] = make_potential(cardinalities, model->num_of_nexts, 
					  NULL);
  }
  free(cardinalities);


  /*****************/
  /* Forward phase */
  /*****************/
  reset_model(model);
  use_priors(model, !HAD_A_PREVIOUS_TIMESLICE);
  *loglikelihood = 0; /* init */
  
  for(t = 0; t < ts->length; t++){ /* FOR EVERY TIMESLICE */
    
    if(t > 0)
      if(finish_timeslice_message_pass(model, FORWARD, 
				       timeslice_sepsets[t-1], 
				       NULL)                   != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	/* i is useless at this point */
	for(i = 0; i < model->num_of_vars; i++)
	  free_potential(results[i]);
	free(results);
	free(data);
	free(observed);
	for(i = 0; i <= ts->length; i++)
	  free_potential(timeslice_sepsets[i]);
	free(timeslice_sepsets);
	return ERROR_GENERAL;
      }

    /* This computes the log likelihood of the ts */
    /*** Watch out for missing data etc. ***/
    j = 0;
    for(i = 0; i < nobserved; i++){
      if(ts->data[t][i] >= 0){
	data[j] = ts->data[t][i];
	observed[j++] = ts->observed[i];
      }
    }
    *loglikelihood = (*loglikelihood) + 
      momentary_loglikelihood(model, observed, data, j);


    /*** DEBUG ***/
/*     printf("loglikelihood became %f at t=%d\n", *loglikelihood, t); */
/*     assert(*loglikelihood > -HUGE_VAL); */


    /* Put some data in */
    insert_ts_step(ts, t, model);
    
    /* Do the inference */
    make_consistent(model);
    
    /* Start a message pass between timeslices */
    if(start_timeslice_message_pass(model, FORWARD,
				    timeslice_sepsets[t]) != NO_ERROR){
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      for(i = 0; i < model->num_of_vars; i++)
	free_potential(results[i]);
      free(results);
      free(data);
      free(observed);
      for(i = 0; i <= ts->length; i++)
	free_potential(timeslice_sepsets[i]);
      free(timeslice_sepsets);
      return ERROR_GENERAL;
    }

    /* Forget old evidence */
    reset_model(model);
    use_priors(model, HAD_A_PREVIOUS_TIMESLICE);
  }
  
  /******************/
  /* Backward phase */
  /******************/
  
  /* forget old evidence */
  reset_model(model);
  use_priors(model, HAD_A_PREVIOUS_TIMESLICE);
  
  for(t = ts->length - 1; t >= 0; t--){ /* FOR EVERY TIMESLICE */
    
    /* Put some evidence in */
    insert_ts_step(ts, t, model);

    /* Pass the message from the past */
    if(t > 0)
      if(finish_timeslice_message_pass(model, FORWARD, 
				       timeslice_sepsets[t-1], 
				       NULL)                   != NO_ERROR){

	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	for(i = 0; i < model->num_of_vars; i++)
	  free_potential(results[i]);
	free(results);
	free(data);
	free(observed);
	for(i = 0; i <= ts->length; i++)
	  free_potential(timeslice_sepsets[i]);
	free(timeslice_sepsets);
	return ERROR_GENERAL;
      }
    
    /* Pass the message from the future */
    if(t < ts->length - 1)
      if(finish_timeslice_message_pass(model, BACKWARD, 
				       timeslice_sepsets[t+1], 
				       timeslice_sepsets[t]) != NO_ERROR){

	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	for(i = 0; i < model->num_of_vars; i++)
	  free_potential(results[i]);
	free(results);
	free(data);
	free(observed);
	for(i = 0; i <= ts->length; i++)
	  free_potential(timeslice_sepsets[i]);
	free(timeslice_sepsets);
	return ERROR_GENERAL;
      }

    /* Do the inference */
    make_consistent(model);
    

    /*** THE CORE: Write the results of inference ***/
    for(i = 0; i < model->num_of_vars; i++){
      p = results[i];

      /* 1. Decide which variable you are interested in */
      temp = model->variables[i];
      
      /* 2. Find the clique that contains the family of 
       *    the interesting variable */
      clique_of_interest = find_family(model->cliques, 
				       model->num_of_cliques, 
				       temp);
      assert(clique_of_interest != NULL);
      
      /* 3. General Marginalisation from the timeslice */
      general_marginalise(clique_of_interest->p, p,
			  find_family_mapping(clique_of_interest, temp));
      
      /* 4. Normalisation ??? */
      size = p->size_of_data;
      k = temp->cardinality;
      for(j = 0; j < size; j = j + k)
	normalise(p->data + j, k);
      /* Not so sure whether this works correctly or not... 
       * I assume that the child variable is the least significant 
       * w.r.t. the address in the potential data. 
       * (as is the case in Hugin net files) 
       */

      /* 5. THE SUM of conditional probabilities over time */
      for(j = 0; j < size; j++){
	parameters[i]->data[j] += p->data[j];
      }
    }
    /*** Finished writing results for this timestep ***/


    if(t > 0)
      if(start_timeslice_message_pass(model, BACKWARD, 
				      timeslice_sepsets[t]) != NO_ERROR){

	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	for(i = 0; i < model->num_of_vars; i++)
	  free_potential(results[i]);
	free(results);
	free(data);
	free(observed);
	for(i = 0; i <= ts->length; i++)
	  free_potential(timeslice_sepsets[i]);
	free(timeslice_sepsets);
	return ERROR_GENERAL;
      }

    /* forget old evidence */
    reset_model(model);
    use_priors(model, HAD_A_PREVIOUS_TIMESLICE);
  }

  /* free the space for calculations */
  for(i = 0; i < model->num_of_vars; i++)
    free_potential(results[i]);
  free(results);
  free(data);
  free(observed);

  /* free the intermediate potentials */
  for(t = 0; t <= ts->length; t++)
    free_potential(timeslice_sepsets[t]);
  free(timeslice_sepsets);

  return NO_ERROR;
}


static int m_step(potential* parameters, nip model){
  int i, j, k;
  int* fam_map;
  clique fam_clique = NULL;
  variable child = NULL;
  
  /* 1. Normalise parameters by dividing with the sums over child variables */
  for(i = 0; i < model->num_of_vars; i++){
    k = number_of_values(model->variables[i]);
    for(j = 0; j < parameters[i]->size_of_data; j = j + k)
      normalise(parameters[i]->data + j, k);
    /* Maybe this works, maybe not... */
  }

  /* Q: Should the clique potential be initially uniform? */
  /* A: Doesn't seem to make sense otherwise...           */
  
  /* 2. Reset the clique potentials and everything */
  total_reset(model);

  /* 3. Initialise the model with the new parameters */
  for(i = 0; i < model->num_of_vars; i++){
    child = model->variables[i];
    fam_clique = find_family(model->cliques, model->num_of_cliques, child);
    fam_map = find_family_mapping(fam_clique, child);

    if(child->num_of_parents > 0){
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
      for(j = 0; j < number_of_values(child); j++)
	child->prior[j] = parameters[i]->data[j];
    }
  }
  return NO_ERROR;
}


/* Teaches the given model (ts->model) according to the given time series 
 * (ts) with EM-algorithm. Returns an error code as an integer. */
int em_learn(time_series *ts, int n_ts, double threshold){
  int i, n, v;
  int *card;
  double old_loglikelihood; 
  double loglikelihood = -DBL_MAX;
  double probe = 0;
  potential *parameters;
  nip model = ts[0]->model;

  /* Reserve some memory for calculation */
  parameters = (potential*) calloc(model->num_of_vars, 
				   sizeof(potential));
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
    card[0] = model->variables[v]->cardinality;
    for(i = 1; i < n; i++)
      card[i] = model->variables[v]->parents[i-1]->cardinality;

    /* variable->parents should be null only if n==1 
     * => no for-loop => no null dereference */
    parameters[v] = make_potential(card, n, NULL);
    free(card);
  }

  /************/
  /* THE Loop */
  /************/
  i = 0;
  do{
    old_loglikelihood = loglikelihood;
    loglikelihood = 0;
    for(n = 0; n < n_ts; n++){
      /* E-Step: Now this is the heavy stuff..! */
      if(e_step(ts[n], parameters, &probe) != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	return ERROR_GENERAL;
      }

      /** DEBUG **/
      assert(probe > -HUGE_VAL && probe <= 0.0);

      loglikelihood += (probe / timeseries_length(ts[n]));
    }

    /* DEBUG */
    printf("Iteration %d: \t average loglikelihood = %f\n", i++, 
           loglikelihood);

    /* NOTE: I'm afraid there's a large possibility to overflow */
    if(loglikelihood - old_loglikelihood == 0 ||
       loglikelihood == -HUGE_VAL)
      break;

    /* M-Step: The parameter estimation... */
    if(m_step(parameters, model) != NO_ERROR){
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      return ERROR_GENERAL;
    }
  }while((loglikelihood - old_loglikelihood) > threshold);
  /*** When should we stop? ***/

  /** <a splendid opportunity to write the parameters to a file> **/

  for(v = 0; v < model->num_of_vars; v++){
    free_potential(parameters[v]);
  }
  free(parameters);
  
  return NO_ERROR;
}


double momentary_loglikelihood(nip model, variable* observed, 
			       int* indexed_data, int n_observed){
  potential p;
  double likelihood;

  if(!observed || !n_observed)
    return -DBL_MAX;

  /* NOTE: the potential array will be ordered according to the 
   * given variable-array, not the same way as clique potentials */
  p = get_joint_probability(model, observed, n_observed); /* EXPENSIVE */
  if(!p){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    return -DBL_MAX;
  }
  
  /* we needed only one of the computed values */
  likelihood = get_pvalue(p, indexed_data);
  free_potential(p); /* Remember to free some memory */

  if(likelihood > 0)
    return log(likelihood); /* natural logarithm (a.k.a. ln) */
  else
    return -DBL_MAX;
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
  cardinality = number_of_values(v);
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
  normalise(result, cardinality);

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
  normalise(p->data, p->size_of_data);
  return p;
}


/* JJ: this has some common elements with the forward_inference function */
time_series generate_data(nip model, int length){
  int i, j, k, t;
  int *cardinalities = NULL;
  potential timeslice_sepset = NULL;
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
  if(model->num_of_nexts > 0){
    cardinalities = (int*) calloc(model->num_of_nexts, sizeof(int));
    if(!cardinalities){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      free_timeseries(ts);
      return NULL;
    }
  }
  k = 0;
  for(i = 0; i < ts->num_of_hidden; i++){
    v = ts->hidden[i];
    if(v->next)
      cardinalities[k++] = number_of_values(v);
  }
  timeslice_sepset = make_potential(cardinalities, model->num_of_nexts, NULL);
  free(cardinalities);
  
  /* new seed number for rand and clear the previous evidence from the model */
  srand(time(0));
  reset_model(model);
  use_priors(model, !HAD_A_PREVIOUS_TIMESLICE);

  /* for each time step */
  for(t = 0; t < ts->length; t++){
    /* influence of the previous time step */
    if(t > 0)
      if(finish_timeslice_message_pass(model, FORWARD, 
				       timeslice_sepset, NULL) != NO_ERROR){
	report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	free_timeseries(ts);
	free_potential(timeslice_sepset);
	return NULL;
      }
    
    /** for each variable */
    for(i = 0; i < nvars; i++){
      make_consistent(model);
      v = vars[i];
      /*** get the probability distribution */
      distribution = get_probability(model, v);

      /*** organize a lottery */
      k = lottery(distribution, number_of_values(v));
      free(distribution);

      /*** insert the data into the time series and the model as evidence */
      ts->data[t][i] = k;
      enter_i_observation(model->variables, model->num_of_vars, 
			  model->cliques, model->num_of_cliques, v, k);
    }
    make_consistent(model);

    /* influence from the current time slice to the next one */
    if(start_timeslice_message_pass(model, FORWARD, 
				    timeslice_sepset) != NO_ERROR){
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      free_timeseries(ts);
      free_potential(timeslice_sepset);
      return NULL;
    }
    /* Forget old evidence */
    reset_model(model);
  }
  free_potential(timeslice_sepset);

  return ts;
}


/* Function for generating random discrete values according to a specified 
 * distribution. Values are between 0 and <size>-1 inclusive. */
static int lottery(double* distribution, int size){
  int i = 0;
  double sum = 0;
  double r = (1.0 * rand()) / RAND_MAX;
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
  clique clique_of_interest;
  sepset_link sepsetlist;
  clique *cliques;

  if(!model){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 1);
    return;
  }

  cliques = model->cliques;
  num_of_cliques = model->num_of_cliques;

  printf("Found cliques:\n");
  for(i = 0; i < num_of_cliques; i++){
    clique_of_interest = cliques[i];
    print_clique(clique_of_interest);
    sepsetlist = clique_of_interest->sepsets;
    while(sepsetlist){
      print_sepset((sepset)sepsetlist->data);
      sepsetlist = sepsetlist->fwd;
    }
    printf("\n");
  }
}
