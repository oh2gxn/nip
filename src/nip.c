/**
 * @file
 * @brief Top level abstractions of the NIP system: probably all you need to use
 *
 * @author Janne Toivola
 * @copyright &copy; 2007,2012 Janne Toivola <br>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. <br>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. <br>
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "nip.h"


/** Write new kind of net files (net language rev.2) */
#define NET_LANG_V2

/** This many probabilities / row of text */
#define POTENTIAL_ELEMENTS_PER_LINE 7

/** Run EM steps at least this many times unless limited by maximum count */
#define MIN_EM_ITERATIONS 3

/*#define DEBUG_NIP*/

/* External Hugin Net parser functions */
#include "huginnet.tab.h" // int yyparse();

/* TODO: Put these guys into a separate shared header?
 * Or find The Proper Way to do stuff without global variables!
 * Hint: %parse-param in huginnet.y? */
FILE* open_net_file(const char *filename);
void close_net_file();
nip_variable_list get_parsed_variables();
int get_cliques(nip_clique** clique_array_pointer);
void get_parsed_node_size(int* x, int* y);


/* Internal helper functions */
static int start_timeslice_message_pass(nip_model model,
                                        nip_direction dir,
                                        nip_potential sepset);
static int finish_timeslice_message_pass(nip_model model,
                                         nip_direction dir,
                                         nip_potential num,
                                         nip_potential den);

static int e_step(time_series ts, nip_potential* parameters,
                  double* loglikelihood);
static int m_step(nip_potential* results, nip_model model);


void reset_model(nip_model model){
  int i, retval;
  nip_variable v;
  for(i = 0; i < model->num_of_vars; i++){
    v = model->variables[i];
    nip_reset_likelihood(v);
    v->prior_entered = 0;
  }
  retval = nip_global_retraction(model->variables, model->num_of_vars,
                                 model->cliques, model->num_of_cliques);
  if(retval != NIP_NO_ERROR)
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
}


void total_reset(nip_model model){
  int i;
  nip_clique c;
  for(i = 0; i < model->num_of_cliques; i++){
    c = model->cliques[i];
    nip_uniform_potential(c->original_p, 1.0);
  }
  /* Q: Reset priors? */
  reset_model(model); /* Could that be enough? */
}


void use_priors(nip_model model, int has_history){
  int i, retval;
  nip_variable v;

  for(i = 0; i < model->num_of_vars - model->num_of_children; i++){
    v = model->independent[i];
    assert(v->prior != NULL);

    /* Prevent priors from being multiplied into the model more than once */
    if(!(v->prior_entered)){
      /* Interface variables have priors only in the first time step
       * (!has_history => first time step) */
      if(!has_history || !(v->interface_status & NIP_INTERFACE_OLD_OUTGOING)){
        /* Reminder: Priors should be multiplied into the tree so that
         *           they are not canceled if evidence is entered, but
         *           there was the idea that global retraction should
         *           cancel priors. Canceling some priors is needed in
         *           time slice models where interface variables will
         *           not actually have priors in subsequent time steps. */
        retval = nip_enter_prior(model->variables, model->num_of_vars,
                                 model->cliques, model->num_of_cliques,
                                 v, v->prior);
        if(retval != NIP_NO_ERROR)
          nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
        v->prior_entered = 1;
      }
    }
  }
}


nip_model parse_model(char* file){
  int i, j, k, m, retval;
  nip_variable temp;
  nip_variable_list vl;
  nip_model new = (nip_model) malloc(sizeof(nip_model_struct));

  if(!new){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  /* 1. Parse */
  if(open_net_file(file) == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    return NULL;
  }

  retval = yyparse(); /* Reminder: priors not entered yet?! */

  close_net_file();

  if(retval != 0)
    return NULL;

  /* 2. Get the parsed stuff and make a model out of them */
  new->num_of_cliques = get_cliques(&(new->cliques));
  vl = get_parsed_variables();
  new->num_of_vars = NIP_LIST_LENGTH(vl);
  new->variables = nip_variable_list_to_array(vl);
  nip_empty_variable_list(vl);
  free(vl);

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
    if(temp->interface_status & NIP_INTERFACE_INCOMING)
      new->incoming_interface_size++;
    if(temp->interface_status & NIP_INTERFACE_OUTGOING)
      new->outgoing_interface_size++;

    /* how many have parents */
    if(temp->parents)
      new->num_of_children++;
    else
      if(temp->prior == NULL){
        fprintf(stderr, "Warning: No prior for the variable %s!\n",
                temp->symbol);
        temp->prior = (double*) calloc(NIP_CARDINALITY(temp), sizeof(double));
        /* this fixes the situation */
      }
  }

  new->next = (nip_variable*) calloc(new->num_of_nexts, sizeof(nip_variable));
  new->previous = (nip_variable*) calloc(new->num_of_nexts,
                                         sizeof(nip_variable));
  new->outgoing_interface = (nip_variable*)
    calloc(new->outgoing_interface_size, sizeof(nip_variable));
  new->previous_outgoing_interface =
    (nip_variable*) calloc(new->outgoing_interface_size, sizeof(nip_variable));
  new->incoming_interface =
    (nip_variable*) calloc(new->incoming_interface_size, sizeof(nip_variable));
  new->children = (nip_variable*) calloc(new->num_of_children,
                                         sizeof(nip_variable));
  new->independent =
    (nip_variable*) calloc(new->num_of_vars - new->num_of_children,
                           sizeof(nip_variable));
  if(!(new->independent &&
       new->children &&
       new->outgoing_interface &&
       new->previous_outgoing_interface &&
       new->incoming_interface &&
       new->previous &&
       new->next)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
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
      /* NOTE: this is coupled with temp->next->previous */
      j++;
    }

    if(temp->interface_status & NIP_INTERFACE_INCOMING)
      new->incoming_interface[k++] = temp;
    if(temp->interface_status & NIP_INTERFACE_OLD_OUTGOING){
      new->previous_outgoing_interface[m] = temp;
      new->outgoing_interface[m] = temp->next;
      /* IMPORTANT: temp and temp->next have the same index m */
      assert(temp->next->interface_status & NIP_INTERFACE_OUTGOING);
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
    new->in_clique = nip_find_clique(new->cliques,
                                     new->num_of_cliques,
                                     new->previous_outgoing_interface,
                                     new->outgoing_interface_size);
    assert(new->in_clique != NULL);
    new->out_clique = nip_find_clique(new->cliques,
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
  /*nip_empty_variable_list(vl); free(vl);*/

#ifdef DEBUG_NIP
  if(new->out_clique){
    printf("Out clique:\n");
    nip_fprintf_clique(stdout, new->out_clique);
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
int write_model(nip_model model, char* filename){
  int i, j, n;
  int x, y;
  int nparents, nvalues, multiparent = 0;
  FILE *f = NULL;
  nip_variable v = NULL;
  int *temp = NULL;
  int *map = NULL;
  nip_clique c = NULL;
  nip_potential p = NULL;
  char *indent;

#ifdef NET_LANG_V1
  indent = "    ";
#else
  indent = "";
#endif

  /* open the stream */
  f = fopen(filename, "w");
  if(!f){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_IO, 1);
    return NIP_ERROR_IO;
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
    n = NIP_CARDINALITY(v)-1;
    nip_get_variable_position(v, &x, &y);
    fputs("\n", f);
    fprintf(f, "%snode %s\n", indent, nip_variable_symbol(v));
    fprintf(f, "%s{\n", indent);
    fprintf(f, "%s    label = \"%s\";\n", indent, v->name);
    fprintf(f, "%s    position = (%d %d);\n", indent, x, y);
    fprintf(f, "%s    states = (", indent);
    for(j = 0; j < n; j++)
      fprintf(f, " \"%s\" \n%s              ", v->state_names[j], indent);
    fprintf(f, " \"%s\" );\n", v->state_names[n]);
    if(v->next)
      fprintf(f, "%s    NIP_next = \"%s\";\n", indent,
              nip_variable_symbol(v->next));
    fprintf(f, "%s}\n", indent);
    fflush(f);
  }

  /** the priors **/
  for(i = 0; i < model->num_of_vars - model->num_of_children; i++){
    v = model->independent[i];
    n = NIP_CARDINALITY(v);
    fputs("\n", f);
    /* independent variables have priors */
    fprintf(f, "%spotential (%s)\n", indent, nip_variable_symbol(v));
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

    nvalues  = NIP_CARDINALITY(v);
    nparents = nip_number_of_parents(v);
    if (nparents > 1)
      multiparent = 1;
    else
      multiparent = 0;

    /* child variables have conditional distributions */
    fputs("\n", f);
    fprintf(f, "%spotential (%s | ", indent, nip_variable_symbol(v));
    for(j = nparents-1; j > 0; j--){
      /* Hugin fellas put parents in reverse order */
      fprintf(f, "%s ", nip_variable_symbol(v->parents[j]));
    }
    fprintf(f, "%s)\n", nip_variable_symbol(v->parents[0]));
    fprintf(f, "%s{ \n", indent);
    fprintf(f, "%s    data = (", indent);
    if (multiparent)
      fputs("(", f);

    temp = (int*) calloc(nparents+1, sizeof(int));
    if(!temp){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      fclose(f);
      return NIP_ERROR_OUTOFMEMORY;
    }

    /* form the potential */
    temp[0] = nvalues; /* child must be the first... */
    /* ...then the parents */
    for(j = 0; j < nparents; j++)
      temp[j+1] = NIP_CARDINALITY(v->parents[j]);
    p = nip_new_potential(temp, nparents+1, NULL);

    /* compute the distribution */
    c = nip_find_family(model->cliques, model->num_of_cliques, v);
    map = nip_find_family_mapping(c, v);
    nip_general_marginalise(c->original_p, p, map);
    /* TODO: keep parameters/pseudocounts as model state, separate from join tree */

    /* normalisation (NOTE: should be part of potential.c) */
    nip_normalise_cpd(p);

    /* print the stuff (TODO: hide the access to private data?) */
    y = 0; /* counter for number of values printed on a line */
    for(j = 0; j < p->size_of_data; j++){
      /* cut long lines and between change in parent values */
      n = (j % nvalues == 0); /* new combination of parent values? */
      if(j > 0 &&
         (n || (nvalues > POTENTIAL_ELEMENTS_PER_LINE &&
                y % POTENTIAL_ELEMENTS_PER_LINE == 0))){
        if(n){
          if (multiparent)
            fputs(")(", f);
          /* print comments about parent values for previous line
           * NOTE: the last comment is left out */
          fputs(" % ", f);
          nip_inverse_mapping(p, j-1, temp);
          for(x = nparents-1; x >= 0; x--){
            fprintf(f, "%s=%s ", nip_variable_symbol(v->parents[x]),
                    nip_variable_state_name(v->parents[x], temp[x+1]));
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
    if (multiparent)
      fputs(")", f);
    fputs(");\n", f);
    fprintf(f, "%s}\n", indent);
    free(temp);
    nip_free_potential(p);
    fflush(f);
  }

#ifdef NET_LANG_V1
  fputs("} \n", f); /* the last brace */
#endif

  /* close the file */
  if(fclose(f)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_IO, 1);
    return NIP_ERROR_IO;
  }
  return NIP_NO_ERROR;
}


void free_model(nip_model model){
  int i;

  if (!model)
    return;

  /* 1. Free cliques and adjacent sepsets */
  for(i = 0; i < model->num_of_cliques; i++)
    nip_free_clique(model->cliques[i]);
  free(model->cliques);

  /* 2. Free the variables */
  for(i = 0; i < model->num_of_vars; i++)
    nip_free_variable(model->variables[i]);
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


int read_timeseries(nip_model model, char* filename, time_series** results,
                    int (*ts_progress)(int, int)){
  int i, j, k, m, n, N;
  int obs;
  char** tokens = NULL;
  time_series ts = NULL;
  nip_data_file df = NULL;
  nip_variable v = NULL;

  df = nip_open_data_file(filename, NIP_FIELD_SEPARATOR, 0, 1);
  if(df == NULL){
    nip_report_error(__FILE__, __LINE__, ENOENT, 1);
    fprintf(stderr, "%s\n", filename);
    return 0;
  }
  if(nip_analyse_data_file(df) < 0){
    nip_report_error(__FILE__, __LINE__, EIO, 1);
    return 0;
  }

  /* N time series */
  N = df->ndatarows;
  *results = (time_series*) calloc(N, sizeof(time_series));
  if(!*results){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    nip_close_data_file(df);
    return 0;
  }

  for(n = 0; n < N; n++){
    ts = (time_series) malloc(sizeof(time_series_struct));
    if(!ts){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      free(*results);
      nip_close_data_file(df);
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
    ts->hidden = (nip_variable *) calloc(ts->num_of_hidden,
                                         sizeof(nip_variable));
    if(obs > 0)
      ts->observed = (nip_variable *) calloc(obs, sizeof(nip_variable));
    if(!(ts->hidden && (ts->observed || obs == 0))){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      free(ts->hidden);
      free(ts);
      free(*results);
      nip_close_data_file(df);
      return 0;
    }

    /* Set the pointers to the hidden variables. */
    m = 0;
    for(k = 0; k < model->num_of_vars; k++){
      j = 1;
      for(i = 0; i < df->num_of_nodes; i++){
        if(nip_equal_variables(model->variables[k],
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
        /* note that these are coupled with ts->data */
      }

      /* Allocate some space for data */
      ts->data = (int**) calloc(ts->length, sizeof(int*));
      if(!(ts->data)){
        nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
        free(ts->hidden);
        free(ts->observed);
        free(ts);
        free(*results);
        nip_close_data_file(df);
        return 0;
      }
      for(i = 0; i < ts->length; i++){
        ts->data[i] = (int*) calloc(obs, sizeof(int));
        if(!(ts->data[i])){
          nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
          for(j = 0; j < i; j++)
            free(ts->data[j]);
          free(ts->data);
          free(ts->hidden);
          free(ts->observed);
          free(ts);
          free(*results);
          nip_close_data_file(df);
          return 0;
        }
      }

      /* Get the data */
      for(j = 0; j < ts->length; j++){
        /* 2. Read */
        m = nip_next_line_tokens(df, NIP_FIELD_SEPARATOR, &tokens);

        if(m != df->num_of_nodes){
          fprintf(stderr, "Warning: (%s): time series %d (t=%d) ",
                  filename, n, j);
          fprintf(stderr, "has %d tokens, ", m);
          fprintf(stderr, "%d expected instead.\n", df->num_of_nodes);
        }

        /* 3. Put into the data array
         * (the same loop as above to ensure the data is in
         *  the same order as variables ts->observed) */
        k = 0;
        for(i = 0; i < df->num_of_nodes; i++){
          v = model_variable(model, df->node_symbols[i]);
          if(i == m)
            break; /* the line was too short */
          if(v)
            ts->data[j][k++] = nip_variable_state_index(v, tokens[i]);
          /* note that these are coupled with ts->observed */

          /* Q: Should missing data be allowed?   A: Yes. */
          /* assert(data[j][i] >= 0); */
        }

        for(i = 0; i < m; i++) /* 4. Dump away */
          free(tokens[i]);
        free(tokens);
      }
    }

    (*results)[n] = ts;
    if(ts_progress != NULL)
      ts_progress(n, ts->length);
  }

  nip_close_data_file(df);
  return N;
}


int write_timeseries(time_series *ts_set, int n_series, char *filename){
  int i, n, t;
  int d;
  int *record;
  int n_observed;
  int *map;
  nip_variable v;
  nip_variable *observed;
  nip_variable *observed_more;
  time_series ts;
  nip_model the_model;
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
    observed_more =
      nip_variable_union(observed, ts->observed, n_observed,
                         the_model->num_of_vars - ts->num_of_hidden,
                         &n_observed);
    free(observed); /* nice to create the same array again and again? */
    observed = observed_more;
  }

  if(!n_observed){ /* no observations in any time series? */
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  /* Temporary space for a sorted record (time step) */
  record = (int*) calloc(n_observed, sizeof(int));
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

  /* Write names of the variables */
  for(i = 0; i < n_observed; i++){
    v = observed[i];
    if(i > 0)
      fprintf(f, "%c", NIP_FIELD_SEPARATOR);
    fprintf(f, "%s", nip_variable_symbol(v));
  }
  fputs("\n", f);

  /* Write the data */
  for(n = 0; n < n_series; n++){ /* for each time series */
    ts = ts_set[n];
    map = nip_mapper(observed, ts->observed, n_observed, ts->num_of_observed);

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
          fprintf(f, "%c", NIP_FIELD_SEPARATOR);
        if(d >= 0)
          fprintf(f, "%s", nip_variable_state_name(v, d));
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
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_IO, 1);
    return NIP_ERROR_IO;
  }
  return NIP_NO_ERROR;
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
  if(!ts)
    return 0;
  return ts->length;
}


int write_uncertainseries(uncertain_series *ucs_set, int n_series,
                          nip_variable v, char *filename){
  int i, n, t, s;
  int *v_index;
  uncertain_series ucs;
  FILE *f = NULL;

  /* Check stuff */
  n = NIP_CARDINALITY(v);
  if(!(n>0 && ucs_set && filename && n_series>0)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  v_index = (int*) calloc(n_series, sizeof(int));
  if(!v_index){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NIP_ERROR_OUTOFMEMORY;
  }

  /* Check that the variable exists in all UCS and which one is it */
  for(s = 0; s < n_series; s++){
    ucs = ucs_set[s];
    v_index[s] = -1; /* linear search */
    for(i=0; i < ucs->num_of_vars; i++){
      if(nip_equal_variables(v, ucs->variables[i])){
        v_index[s] = i;
        break;
      }
    }
    if(v_index[s] < 0){ /* no such variable in the UCS */
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
      return NIP_ERROR_INVALID_ARGUMENT;
      free(v_index);
    }
  }

  /* Try to open the file for write */
  f = fopen(filename, "w");
  if(!f){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_IO, 1);
    free(v_index);
    return NIP_ERROR_IO;
  }

  /* Write names of the states */
  for(i = 0; i < n; i++){
    if(i > 0)
      fprintf(f, "%c", NIP_FIELD_SEPARATOR);
    fprintf(f, "%s", nip_variable_state_name(v, i));
  }
  fputs("\n", f);

  /* Write the data: probabilities... */
  for(s = 0; s < n_series; s++){ /* ...for each series... */
    ucs = ucs_set[s];

    for(t = 0; t < ucs->length; t++){ /* ...for each time step... */

      for(i = 0; i < n; i++){ /* ...for each state. */
        if(i > 0)
          fprintf(f, "%c", NIP_FIELD_SEPARATOR);
        fprintf(f, "%f", ucs->data[t][v_index[s]][i]);
      }
      fputs("\n", f);
    }
    fputs("\n", f); /* series separator */
  }

  /* Close the file */
  if(fclose(f)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_IO, 1);
    free(v_index);
    return NIP_ERROR_IO;
  }

  free(v_index);
  return NIP_NO_ERROR;
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
  if(!ucs)
    return 0;
  return ucs->length;
}


char* get_observation(time_series ts, nip_variable v, int time){
  int i, j = -1;
  for(i = 0; i < ts->model->num_of_vars - ts->num_of_hidden; i++)
    if(nip_equal_variables(v, ts->observed[i]))
      j = i;

  if(j < 0)
    return NULL;

  if (time < 0 || ts->length <= time)
    return NULL;

  return v->state_names[ts->data[time][j]];
}


double* get_posterior(uncertain_series ucs, nip_variable v, int time){
  int i, j = -1;
  for(i = 0; i < ucs->num_of_vars; i++)
    if(nip_equal_variables(v, ucs->variables[i]))
      j = i;

  if(j < 0)
    return NULL;

  if (time < 0 || ucs->length <= time)
    return NULL;

  return ucs->data[time][j];
}


int set_observation(time_series ts, nip_variable v, int time,
                    char* observation){
  int i, j = -1;
  for(i = 0; i < ts->model->num_of_vars - ts->num_of_hidden; i++)
    if(nip_equal_variables(v, ts->observed[i]))
      j = i;

  i = nip_variable_state_index(v, observation);
  /* a valid variable? a valid observation for that variable? */
  if((j < 0) || (i < 0))
    return NIP_ERROR_INVALID_ARGUMENT;

  ts->data[time][j] = i; // FIXME: check time < ts->length ?
  return 0;
}


int insert_hard_evidence(nip_model model, char* varname, char* observation){
  int ret;
  nip_variable v = model_variable(model, varname);
  if(v == NULL)
    return NIP_ERROR_INVALID_ARGUMENT;
  ret = nip_enter_observation(model->variables, model->num_of_vars,
                              model->cliques, model->num_of_cliques,
                              v, observation);
  if(ret != NIP_NO_ERROR)
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);

  make_consistent(model);
  return ret;
}


int insert_soft_evidence(nip_model model, char* varname, double* distribution){
  int ret;
  nip_variable v = model_variable(model, varname);
  if(v == NULL)
    return NIP_ERROR_INVALID_ARGUMENT;
  ret =  nip_enter_evidence(model->variables, model->num_of_vars,
                            model->cliques, model->num_of_cliques,
                            v, distribution);
  make_consistent(model);
  return ret;
}


/* Note that <model> may be different from ts->model, but the variables
 * have to be shared. */
int insert_ts_step(time_series ts, int t, nip_model model, char mark_mask){
  int i;
  nip_variable v;

  if(t < 0 || t >= timeseries_length(ts)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  for(i = 0; i < ts->model->num_of_vars - ts->num_of_hidden; i++){
    v = ts->observed[i];
    if(NIP_MARK(v) & mark_mask){ /* Only the suitably marked variables */
      if(ts->data[t][i] >= 0)
        nip_enter_index_observation(model->variables, model->num_of_vars,
                                    model->cliques, model->num_of_cliques,
                                    v, ts->data[t][i]);
    }
  }
  return NIP_NO_ERROR;
}


/* note that <model> may be different from ucs->model */
int insert_ucs_step(uncertain_series ucs, int t, nip_model model, char mark_mask){
  int i, e;
  nip_variable v;

  if(t < 0 || t >= uncertainseries_length(ucs)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  for(i = 0; i < ucs->num_of_vars; i++){
    v = ucs->variables[i];
    if(NIP_MARK(v) & mark_mask){
      e = nip_enter_evidence(model->variables, model->num_of_vars,
                             model->cliques, model->num_of_cliques,
                             v, ucs->data[t][i]);
      if(e != NIP_NO_ERROR){
        nip_report_error(__FILE__, __LINE__, e, 1);
        return e;
      }
    }
  }
  return NIP_NO_ERROR;
}


/* Starts a message pass between timeslices */
static int start_timeslice_message_pass(nip_model model,
                                        nip_direction dir,
                                        nip_potential alpha_or_gamma){
  int* mapping;
  nip_clique c;
  int nvars = model->outgoing_interface_size;
  nip_variable* vars;

  /* What if there are no subsequent time slices? */
  if(nvars == 0){
    nip_uniform_potential(alpha_or_gamma, 1.0);
    return NIP_NO_ERROR;
  }

  if(dir == FORWARD){
    vars = model->outgoing_interface;
    c = model->out_clique;
  }
  else{
    vars = model->previous_outgoing_interface;
    c = model->in_clique;
  }

  /* map the variables */
  mapping = nip_mapper(c->variables, vars, NIP_DIMENSIONALITY(c->p), nvars);

  /* the marginalisation */
  nip_general_marginalise(c->p, alpha_or_gamma, mapping);
  free(mapping); /* should mapping-array be a part of sepsets? */

  /* normalisation in order to avoid drifting towards zeros */
  nip_normalise_potential(alpha_or_gamma);

  return NIP_NO_ERROR;
}


/* Finishes the message pass between timeslices */
static int finish_timeslice_message_pass(nip_model model,
                                         nip_direction dir,
                                         nip_potential num,
                                         nip_potential den){
  int* mapping;
  nip_clique c;
  int nvars = model->outgoing_interface_size;
  nip_variable* vars;

  if(nvars == 0) /* independent time slices (multiplication with 1) */
    return NIP_NO_ERROR;

  /* Find a suitable clique c */
  if(dir == FORWARD){
    vars = model->previous_outgoing_interface;
    c = model->in_clique;
  }
  else{
    vars = model->outgoing_interface;
    c = model->out_clique;
  }

  /* map the variables */
  mapping = nip_mapper(c->variables, vars, NIP_DIMENSIONALITY(c->p), nvars);

  /* the multiplication (and division, if den != NULL) */
  nip_update_potential(num, den, c->p, mapping);
  free(mapping);
  return NIP_NO_ERROR;
}


/* forward-only inference consumes constant (1 time slice) amount of memory
 * + the results (which is linear) */
uncertain_series forward_inference(time_series ts, nip_variable vars[],
                                   int nvars, double* loglikelihood){
  int i, t;
  int* cardinalities = NULL;
  double m1, m2;
  nip_variable temp;
  nip_potential alpha = NULL;
  nip_clique clique_of_interest;
  uncertain_series results = NULL;
  nip_model model = ts->model;

  /* Allocate an array */
  if(model->outgoing_interface_size > 0){
    cardinalities = (int*) calloc(model->outgoing_interface_size, sizeof(int));
    if(!cardinalities){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      return NULL;
    }
  }

  /* Fill the array */
  for(i = 0; i < model->outgoing_interface_size; i++)
    cardinalities[i] = NIP_CARDINALITY(model->outgoing_interface[i]);

  /* Allocate some space for the results */
  results = (uncertain_series) malloc(sizeof(uncertain_series_struct));
  if(!results){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(cardinalities);
    return NULL;
  }

  results->variables = (nip_variable*) calloc(nvars, sizeof(nip_variable));
  if(!results->variables){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(results);
    free(cardinalities);
    return NULL;
  }

  /* Copy the references to the variables of interest */
  memcpy(results->variables, vars, nvars*sizeof(nip_variable));
  results->num_of_vars = nvars;
  results->length = ts->length;

  results->data = (double***) calloc(ts->length, sizeof(double**));
  if(!results->data){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(results->variables);
    free(results);
    free(cardinalities);
    return NULL;
  }

  for(t = 0; t < results->length; t++){
    results->data[t] = (double**) calloc(nvars, sizeof(double*));
    if(!results->data[t]){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
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
      results->data[t][i] = (double*) calloc(NIP_CARDINALITY(vars[i]),
                                             sizeof(double));
      if(!results->data[t][i]){
        nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
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
  alpha = nip_new_potential(cardinalities, model->outgoing_interface_size, NULL);
  free(cardinalities);

  /*****************/
  /* Forward phase */
  /*****************/
  reset_model(model);
  use_priors(model, !NIP_HAD_A_PREVIOUS_TIMESLICE);
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
      if(finish_timeslice_message_pass(model, FORWARD, alpha, NULL) != NIP_NO_ERROR){
        nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
        free_uncertainseries(results);
        nip_free_potential(alpha);
        return NULL;
      }
    }

    /* Likelihood reference... */
    if(loglikelihood){
      make_consistent(model);
      m1 = model_prob_mass(model);
    }

    /* Put some data in */
    insert_ts_step(ts, t, model, NIP_MARK_ON); /* only marked variables */

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
        printf("Log.likelihood ln(L(y(%d)|y(0:%d))) = %g\n",
               t, t-1, (log(m2) - log(m1)));
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
      clique_of_interest = nip_find_family(model->cliques,
                                           model->num_of_cliques,
                                           temp);
      assert(clique_of_interest != NULL);

      /* 3. Marginalisation (the memory must have been allocated) */
      nip_marginalise_clique(clique_of_interest, temp, results->data[t][i]);

      /* 4. Normalisation */
      nip_normalise_array(results->data[t][i], NIP_CARDINALITY(temp));
    }

    /* Start a message pass between time slices (compute new alpha) */
    if(start_timeslice_message_pass(model, FORWARD, alpha) != NIP_NO_ERROR){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
      free_uncertainseries(results);
      nip_free_potential(alpha);
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
    use_priors(model, NIP_HAD_A_PREVIOUS_TIMESLICE);
  }
  nip_free_potential(alpha);

  return results;
}


/* This consumes much more memory depending on the size of the
 * sepsets between time slices. */
uncertain_series forward_backward_inference(time_series ts,
                                            nip_variable vars[], int nvars,
                                            double* loglikelihood){
  int i, t;
  int *cardinalities = NULL;
  double m1, m2;
  nip_variable temp;
  nip_potential *alpha_gamma = NULL;
  nip_clique clique_of_interest;
  uncertain_series results = NULL;
  nip_model model = ts->model;

  /* Allocate an array for describing the dimensions of timeslice sepsets */
  if(model->outgoing_interface_size > 0){
    cardinalities = (int*) calloc(model->outgoing_interface_size, sizeof(int));
    if(!cardinalities){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      return NULL;
    }
  }

  /* Fill the array */
  for(i = 0; i < model->outgoing_interface_size; i++)
    cardinalities[i] = NIP_CARDINALITY(model->outgoing_interface[i]);

  /* Allocate some space for the results */
  results = (uncertain_series) malloc(sizeof(uncertain_series_struct));
  if(!results){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(cardinalities);
    return NULL;
  }

  results->variables = (nip_variable*) calloc(nvars, sizeof(nip_variable));
  if(!results->variables){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(results);
    free(cardinalities);
    return NULL;
  }

  /* Copy the references to the variables of interest */
  memcpy(results->variables, vars, nvars*sizeof(nip_variable));
  results->num_of_vars = nvars;
  results->length = ts->length;

  results->data = (double***) calloc(ts->length, sizeof(double**));
  if(!results->data){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(results->variables);
    free(results);
    free(cardinalities);
    return NULL;
  }

  for(t = 0; t < results->length; t++){
    results->data[t] = (double**) calloc(nvars, sizeof(double*));
    if(!results->data[t]){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
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
      results->data[t][i] = (double*) calloc(NIP_CARDINALITY(vars[i]),
                                             sizeof(double));
      if(!results->data[t][i]){
        nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
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
  alpha_gamma = (nip_potential *) calloc(ts->length + 1, sizeof(nip_potential));

  /* Initialise intermediate potentials */
  for(t = 0; t <= ts->length; t++){
    alpha_gamma[t] = nip_new_potential(cardinalities,
                                       model->outgoing_interface_size,
                                       NULL);
  }
  free(cardinalities);

  /*****************/
  /* Forward phase */
  /*****************/
  reset_model(model);
  use_priors(model, !NIP_HAD_A_PREVIOUS_TIMESLICE);
  if(loglikelihood)
    *loglikelihood = 0; /* init */

  for(t = 0; t < ts->length; t++){ /* FOR EVERY TIMESLICE */
    if(t > 0)
      if(finish_timeslice_message_pass(model, FORWARD,
                                       alpha_gamma[t-1], NULL) != NIP_NO_ERROR){
        nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
        free_uncertainseries(results);
        for(i = 0; i <= ts->length; i++)
          nip_free_potential(alpha_gamma[i]);
        free(alpha_gamma);
        return NULL;
      }

    /* Likelihood reference... */
    if(loglikelihood){
      make_consistent(model);
      m1 = model_prob_mass(model);
    }

    /* Put some data in (Q: should this be AFTER message passing?) */
    insert_ts_step(ts, t, model, NIP_MARK_ON);

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
      if(m2 == 0.0){
        *loglikelihood = -DBL_MAX; /* -infinity, does this underflow ? */
      }
    }

    /* Start a message pass between timeslices */
    if(start_timeslice_message_pass(model, FORWARD,
                                    alpha_gamma[t]) != NIP_NO_ERROR){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
      free_uncertainseries(results);
      for(i = 0; i <= ts->length; i++)
        nip_free_potential(alpha_gamma[i]);
      free(alpha_gamma);
      return NULL;
    }

    /* Forget old evidence */
    reset_model(model);
    if(ts->length > 1)
      use_priors(model, NIP_HAD_A_PREVIOUS_TIMESLICE);
    else
      use_priors(model, !NIP_HAD_A_PREVIOUS_TIMESLICE);
  }

  /******************/
  /* Backward phase */
  /******************/
  for(t = ts->length - 1; t >= 0; t--){ /* FOR EVERY TIMESLICE */
    /* Pass the message from the past */
    if(t > 0)
      if(finish_timeslice_message_pass(model, FORWARD,
                                       alpha_gamma[t-1],
                                       NULL) != NIP_NO_ERROR){
        nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
        free_uncertainseries(results);
        for(i = 0; i <= ts->length; i++)
          nip_free_potential(alpha_gamma[i]);
        free(alpha_gamma);
        return NULL;
      }

    /* Put some evidence in */
    insert_ts_step(ts, t, model, NIP_MARK_ON);

    /* Pass the message from the future */
    if(t < ts->length - 1)
      if(finish_timeslice_message_pass(model, BACKWARD,
                                       alpha_gamma[t+1], /* gamma */
                                       alpha_gamma[t]  ) /* alpha */
         != NIP_NO_ERROR){
        nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
        free_uncertainseries(results);
        for(i = 0; i <= ts->length; i++)
          nip_free_potential(alpha_gamma[i]);
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
      clique_of_interest = nip_find_family(model->cliques,
                                           model->num_of_cliques,
                                           temp);
      assert(clique_of_interest != NULL);

      /* 3. Marginalisation (the memory must have been allocated) */
      nip_marginalise_clique(clique_of_interest, temp, results->data[t][i]);

      /* 4. Normalisation */
      nip_normalise_array(results->data[t][i], NIP_CARDINALITY(temp));
    }
    /* End of the CORE */

    /* Pass the message to the past */
    if(t > 0)
      if(start_timeslice_message_pass(model, BACKWARD,
                                      alpha_gamma[t]) != NIP_NO_ERROR){
        nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
        free_uncertainseries(results);
        for(i = 0; i <= ts->length; i++)
          nip_free_potential(alpha_gamma[i]);
        free(alpha_gamma);
        return NULL;
      }

    /* forget old evidence */
    reset_model(model);
    if(t > 1)
      use_priors(model, NIP_HAD_A_PREVIOUS_TIMESLICE);
    else
      use_priors(model, !NIP_HAD_A_PREVIOUS_TIMESLICE);
  }

  /* free the intermediate potentials */
  for(t = 0; t <= ts->length; t++)
    nip_free_potential(alpha_gamma[t]);
  free(alpha_gamma);

  return results;
}


nip_variable model_variable(nip_model model, char* symbol){
  int i;

  if(!model){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NULL;
  }

  for(i = 0; i < model->num_of_vars; i++)
    if(strcmp(symbol, model->variables[i]->symbol) == 0)
      return model->variables[i];

  return NULL;
}


void make_consistent(nip_model model){
  int i;
  for (i = 0; i < model->num_of_cliques; i++)
    nip_unmark_clique(model->cliques[i]);

  if(nip_collect_evidence(NULL, NULL, model->cliques[0]) != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    return;
  }

  for (i = 0; i < model->num_of_cliques; i++)
    nip_unmark_clique(model->cliques[i]);

  if(nip_distribute_evidence(model->cliques[0]) != NIP_NO_ERROR)
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);

  return;
}


/* Most likely state sequence of the variables given the timeseries. */
time_series mlss(nip_variable vars[], int nvars, time_series ts){
  int i, j, k, l, t;
  time_series mlss;

  /* Allocate some space for the results */
  mlss = (time_series)malloc(sizeof(time_series_struct));
  if(!mlss){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  mlss->model = ts->model;
  mlss->num_of_hidden = ts->model->num_of_vars - nvars;
  mlss->num_of_observed = nvars;
  mlss->hidden = (nip_variable*) calloc(mlss->num_of_hidden,
                                        sizeof(nip_variable));
  mlss->observed = (nip_variable*) calloc(nvars, sizeof(nip_variable));
  mlss->length = ts->length;
  mlss->data = (int**) calloc(mlss->length, sizeof(int*));
  if(!(mlss->data && mlss->observed && mlss->hidden)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(mlss->data); /* useless? */
    free(mlss->observed);
    free(mlss->hidden);
    free(mlss);
    return NULL;
  }

  for(t=0; t < mlss->length; t++){
    mlss->data[t] = (int*) calloc(nvars, sizeof(int));
    if(!(mlss->data[t])){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
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
  memcpy(mlss->observed, vars, nvars*sizeof(nip_variable));

  /* Find out the "hidden", or more like uninteresting, variables */
  l = 0;
  for(i=0; i < ts->model->num_of_vars; i++){
    if(l == mlss->num_of_hidden)
      break;

    k = 1;
    for(j=0; j < nvars; j++){
      if(nip_equal_variables(ts->model->variables[i], vars[j])){
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
  nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
  return mlss;
}


/* My apologies: this function is probably the worst copy-paste case ever.
 * Any ideas how to avoid repeating the same parts of code?
 * - function pointers are pretty much out of the question in this case,
 *   because they can't deliver the results without global variables
 * - some parts of the code could be (and have been) transformed into
 *   separate procedures */
static int e_step(time_series ts, nip_potential* parameters, double* loglikelihood){
  int i, t;
#ifdef DEBUG_NIP
  int j;
#endif
  int* cardinalities = NULL;
  int nobserved;
  int* data = NULL;
  int* mapping;
  nip_variable* observed = NULL;
  nip_variable v;
  nip_potential* alpha_gamma = NULL;
  nip_potential* results = NULL;
  nip_potential p;
  nip_clique c = NULL;
  nip_model model = ts->model;
  double m1, m2;
  int error;

  /* Reserve some memory for computation */
  nobserved = ts->num_of_observed;
  observed = (nip_variable*) calloc(nobserved, sizeof(nip_variable));
  data     = (int*) calloc(nobserved, sizeof(int));
  results = (nip_potential*) calloc(model->num_of_vars, sizeof(nip_potential));
  if(!(results && data && observed && loglikelihood)){
    if(loglikelihood){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      error = NIP_ERROR_OUTOFMEMORY;
    }
    else{
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
      error = NIP_ERROR_INVALID_ARGUMENT;
    }
    free(data);
    free(observed);
    free(results);
    return error;
  }
  for(i = 0; i < model->num_of_vars; i++){
    p = parameters[i];
    results[i] = nip_new_potential(NIP_CARDINALITY(p),
                                   NIP_DIMENSIONALITY(p),
                                   NULL);
    /* in principle, new_potential can return NULL */
  }

  /* Allocate some space for the intermediate potentials between timeslices */
  alpha_gamma = (nip_potential *) calloc(ts->length + 1,
                                         sizeof(nip_potential));

  /* Allocate an array for describing the dimensions of alpha & gamma */
  if(model->outgoing_interface_size > 0){
    cardinalities = (int*) calloc(model->outgoing_interface_size, sizeof(int));
    if(!cardinalities){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      for(i = 0; i < model->num_of_vars; i++)
        nip_free_potential(results[i]);
      free(results);
      free(data);
      free(observed);
      free(alpha_gamma);
      return NIP_ERROR_OUTOFMEMORY;
    }
  }

  for(i = 0; i < model->outgoing_interface_size; i++)
    cardinalities[i] = NIP_CARDINALITY(model->outgoing_interface[i]);

  /* Initialise intermediate potentials */
  for(t = 0; t <= ts->length; t++)
    alpha_gamma[t] = nip_new_potential(cardinalities,
                                       model->outgoing_interface_size,
                                       NULL);
  free(cardinalities);

  /*****************/
  /* Forward phase */
  /*****************/
  reset_model(model);
  use_priors(model, !NIP_HAD_A_PREVIOUS_TIMESLICE);
  *loglikelihood = 0; /* init */

  for(t = 0; t < ts->length; t++){ /* FOR EVERY TIMESLICE */
    if(t > 0){
      if(finish_timeslice_message_pass(model, FORWARD,
                                       alpha_gamma[t-1], NULL) != NIP_NO_ERROR){
        nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
        /* i is useless at this point */
        for(i = 0; i < model->num_of_vars; i++)
          nip_free_potential(results[i]);
        free(results);
        free(data);
        free(observed);
        for(i = 0; i <= ts->length; i++)
          nip_free_potential(alpha_gamma[i]);
        free(alpha_gamma);
        return NIP_ERROR_GENERAL;
      }
    }

    /* Propagate message and make sure sepsets have correct weight */
    make_consistent(model);

    m1 = model_prob_mass(model);

    insert_ts_step(ts, t, model, NIP_MARK_ON); /* Put some data in */

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
        nip_free_potential(results[i]);
      free(results);
      free(data);
      free(observed);
      for(i = 0; i <= ts->length; i++)
        nip_free_potential(alpha_gamma[i]);
      free(alpha_gamma);

      /* DEBUG */
#ifdef DEBUG_NIP
      printf("t  = %d\n", t);
      printf("m1 = %g\n", m1);
      printf("m2 = %g\n", m2);
      printf("ll = %g\n", *loglikelihood);
      for(i = 0; i < model->num_of_cliques; i++){
        nip_fprintf_clique(stdout, model->cliques[i]);
        nip_fprintf_potential(stdout, model->cliques[i]->original_p);
      }
#endif

      return NIP_ERROR_BAD_LUCK;
    }

    /* Start a message pass between timeslices */
    if(start_timeslice_message_pass(model, FORWARD,
                                    alpha_gamma[t]) != NIP_NO_ERROR){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
      for(i = 0; i < model->num_of_vars; i++)
        nip_free_potential(results[i]);
      free(results);
      free(data);
      free(observed);
      for(i = 0; i <= ts->length; i++)
        nip_free_potential(alpha_gamma[i]);
      free(alpha_gamma);
      return NIP_ERROR_GENERAL;
    }

    /* Forget old evidence */
    reset_model(model);
    if(ts->length > 1)
      use_priors(model, NIP_HAD_A_PREVIOUS_TIMESLICE);
    else
      use_priors(model, !NIP_HAD_A_PREVIOUS_TIMESLICE);
  }

  /******************/
  /* Backward phase */
  /******************/
  for(t = ts->length - 1; t >= 0; t--){ /* FOR EVERY TIMESLICE */
    /* Pass the message from the past */
    if(t > 0)
      if(finish_timeslice_message_pass(model, FORWARD,
                                       alpha_gamma[t-1],
                                       NULL) != NIP_NO_ERROR){
        nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
        for(i = 0; i < model->num_of_vars; i++)
          nip_free_potential(results[i]);
        free(results);
        free(data);
        free(observed);
        for(i = 0; i <= ts->length; i++)
          nip_free_potential(alpha_gamma[i]);
        free(alpha_gamma);
        return NIP_ERROR_GENERAL;
      }

    /* Put some evidence in */
    insert_ts_step(ts, t, model, NIP_MARK_ON); /* only marked variables */

    /* Pass the message from the future */
    if(t < ts->length - 1)
      if(finish_timeslice_message_pass(model, BACKWARD,
                                       alpha_gamma[t+1],
                                       alpha_gamma[t]) != NIP_NO_ERROR){
        nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
        for(i = 0; i < model->num_of_vars; i++)
          nip_free_potential(results[i]);
        free(results);
        free(data);
        free(observed);
        for(i = 0; i <= ts->length; i++)
          nip_free_potential(alpha_gamma[i]);
        free(alpha_gamma);
        return NIP_ERROR_GENERAL;
      }

    /* Do the inference */
    make_consistent(model);

    /*** THE CORE: Write the results of inference ***/
    for(i = 0; i < model->num_of_vars; i++){
      p = results[i];

      /* 1. Decide which variable you are interested in */
      v = model->variables[i];

      /* JJT 02.11.2006: Skip old interface variables for t > 0 */
      if(t > 0 && (v->interface_status & NIP_INTERFACE_OLD_OUTGOING))
        continue;

      /* 2. Find the clique that contains the family of
       *    the interesting variable */
      c = nip_find_family(model->cliques, model->num_of_cliques, v);
      assert(c != NULL);

      /* 3. General Marginalisation from the timeslice */
      mapping = nip_find_family_mapping(c, v);
      nip_general_marginalise(c->p, p, mapping);

#ifdef DEBUG_NIP
      /* DEBUG */
      printf("Marginalising the family of %s ", v->symbol);
      for(j = 0; j < NIP_DIMENSIONALITY(p) - 1; j++)
        printf("%s ", v->parents[j]->symbol);
      printf("from \n");
      nip_fprintf_clique(stdout, c);
      printf("with mapping [");
      for(j = 0; j < NIP_DIMENSIONALITY(p); j++)
        printf("%d,", mapping[j]);
      printf("]\n");
      /* FIXME: correct mapping??? */
#endif

      /********************/
      /* 4. Normalisation */
      /********************/
      nip_normalise_potential(p); /* Does this cause numerical problems? */

      /* 5. THE SUM of expected counts over time */
      nip_sum_potential(parameters[i], p); /* "parameters[i] += p" */
    }
    /*** Finished writing results for this timestep ***/

    /* Pass the message to the past */
    if(t > 0)
      if(start_timeslice_message_pass(model, BACKWARD,
                                      alpha_gamma[t]) != NIP_NO_ERROR){
        nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
        for(i = 0; i < model->num_of_vars; i++)
          nip_free_potential(results[i]);
        free(results);
        free(data);
        free(observed);
        for(i = 0; i <= ts->length; i++)
          nip_free_potential(alpha_gamma[i]);
        free(alpha_gamma);
        return NIP_ERROR_GENERAL;
      }

    /* forget old evidence */
    reset_model(model);
    if(t > 1) /* Q: Or t > 0 ?  A: No, t will be t-1 soon... */
      use_priors(model, NIP_HAD_A_PREVIOUS_TIMESLICE);
    else
      use_priors(model, !NIP_HAD_A_PREVIOUS_TIMESLICE);
  }

  /* free the space for calculations */
  for(i = 0; i < model->num_of_vars; i++)
    nip_free_potential(results[i]);
  free(results);
  free(data);
  free(observed);

  /* free the intermediate potentials */
  for(t = 0; t <= ts->length; t++)
    nip_free_potential(alpha_gamma[t]);
  free(alpha_gamma);

  return NIP_NO_ERROR;
}


static int m_step(nip_potential* parameters, nip_model model){
  int i, j, k;
  int* fam_map;
  nip_clique fam_clique = NULL;
  nip_variable child = NULL;

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
    /* NOTE: parameter potentials have children as the 1st dimension */
    nip_normalise_cpd(parameters[i]);
  }

  /* 2. Reset the clique potentials and everything */
  total_reset(model);

  /* 3. Initialise the model with the new parameters */
  for(i = 0; i < model->num_of_vars; i++){
    child = model->variables[i];

    if(nip_number_of_parents(child) > 0){
      /* NOTE: parameters have children as the 1st dimension,
       * but cliques have dimensions ordered by variable ID */
      fam_clique = nip_find_family(model->cliques,
                                   model->num_of_cliques,
                                   child);
      fam_map = nip_find_family_mapping(fam_clique, child);

      /* Update the conditional probability distributions (dependencies) */
      j = nip_init_potential(parameters[i], fam_clique->p, fam_map);
      k = nip_init_potential(parameters[i], fam_clique->original_p, fam_map);
      if(j != NIP_NO_ERROR || k != NIP_NO_ERROR){
        nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
        return NIP_ERROR_GENERAL;
      }
    }
    else{
      /* Update the priors of independent variables */
      nip_total_marginalise(parameters[i], child->prior, 0);
    }
  }

  return NIP_NO_ERROR;
}


/* Trains the given model (ts[0]->model) according to the given set of
 * time series (ts[*]) with EM-algorithm. Returns an error code. */
int em_learn(nip_model model, time_series* ts, int n_ts, int have_random_init,
             long max_iterations, double threshold,
             nip_double_list learning_curve, nip_convergence* stopping_criterion,
             int (*em_progress)(nip_double_list, double), int (*ts_progress)(int, int)){
  int i, n, v;
  int *card, *mapping;
  int ts_steps;
  double old_loglikelihood;
  double loglikelihood = -DBL_MAX;
  double probe = 0;
  nip_potential* parameters = NULL;
  nip_clique clique;
  int e, converged;

  if(!ts[0] || !model){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  /* Reserve some memory for calculation */
  parameters = (nip_potential*) calloc(model->num_of_vars, sizeof(nip_potential));
  if(!parameters){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NIP_ERROR_OUTOFMEMORY;
  }

  for(v = 0; v < model->num_of_vars; v++){
    n = nip_number_of_parents(model->variables[v]) + 1;
    card = (int*) calloc(n, sizeof(int));
    if(!card){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      while(v > 0)
        nip_free_potential(parameters[--v]);
      free(parameters);
      return NIP_ERROR_OUTOFMEMORY;
    }
    /* The child MUST be the first variable in order to normalize potentials */
    card[0] = NIP_CARDINALITY(model->variables[v]);
    for(i = 1; i < n; i++)
      card[i] = NIP_CARDINALITY(model->variables[v]->parents[i-1]);
    parameters[v] = nip_new_potential(card, n, NULL);
    free(card);
  }

  /* Randomize the parameters. (TODO: move this operation to potential.c?)
   * NOTE: parameters near zero are a numerical problem...
   *       on the other hand, zeros are needed in some cases.
   *       How to identify a "bad" zero? */
  if(have_random_init)
    for(v = 0; v < model->num_of_vars; v++){
      nip_random_potential(parameters[v]);
      /* the M-step will take care of the normalisation */
    }
  else
    for(v = 0; v < model->num_of_vars; v++){
      clique = nip_find_family(model->cliques, model->num_of_cliques,
                               model->variables[v]);
      if(!clique){
        nip_report_error(__FILE__, __LINE__, EINVAL, 1);
        while(v > 0)
          nip_free_potential(parameters[--v]);
        free(parameters);
        return NIP_ERROR_OUTOFMEMORY;
      }
      mapping = nip_find_family_mapping(clique, model->variables[v]);
      nip_general_marginalise(clique->original_p, parameters[v], mapping);
      // TODO: minor drop in learning curve when continuing, but recovered during the 3 minimum iterations ?
      /* TODO: keep parameters/pseudocounts as model state, separate from join tree */
      /* the M-step will take care of the normalisation?
      nip_normalise_cpd(p); */
    }

  /* Compute total number of time steps */
  ts_steps = 0;
  for(n = 0; n < n_ts; n++)
    ts_steps += timeseries_length(ts[n]);

  /************/
  /* THE Loop */
  /************/
  i = 0; converged = 0;
  do{

    /* M-Step... or at least the last part of it.
     * On the first iteration this enters the random parameters
     * into the model. */
    e = m_step(parameters, model);
    if(e != NIP_NO_ERROR){
      nip_report_error(__FILE__, __LINE__, e, 1);
      for(v = 0; v < model->num_of_vars; v++){
        nip_free_potential(parameters[v]);
      }
      free(parameters);
      if(learning_curve != NULL)
        nip_empty_double_list(learning_curve);
      return e;
    }

    old_loglikelihood = loglikelihood;
    loglikelihood = 0.0;

    /* Initialise the parameter potentials to "zero" for
     * accumulating the "average parameters" in the E-step */
    for(v = 0; v < model->num_of_vars; v++){
      nip_uniform_potential(parameters[v], 1.0); /* Q: Use pseudo counts? */
    }

    /* E-Step: Now this is the heavy stuff..!
     * (for each time series separately to save memory) */
    for(n = 0; n < n_ts; n++){
      e = e_step(ts[n], parameters, &probe);
      if(e != NIP_NO_ERROR){
        if(e != NIP_ERROR_BAD_LUCK)
          nip_report_error(__FILE__, __LINE__, e, 1);
        /* don't report invalid random parameters */
        for(v = 0; v < model->num_of_vars; v++){
          nip_free_potential(parameters[v]);
        }
        free(parameters);
        if(e != NIP_ERROR_BAD_LUCK){
          if(learning_curve != NULL)
            nip_empty_double_list(learning_curve);
        }
        /* else let the list be */

        return e;
      }

      /** DEBUG **/
      assert(-HUGE_DOUBLE < probe  &&  probe <= 0.0  && probe == probe);
      /* probe != probe  =>  probe == NaN  */

      loglikelihood += probe;
      if(ts_progress != NULL)
        ts_progress(n, ts[n]->length);
    }

    /* Add an element to the linked list */
    if(learning_curve != NULL){
      e = em_progress(learning_curve, loglikelihood / ts_steps);
      if(e != NIP_NO_ERROR){
        nip_report_error(__FILE__, __LINE__, e, 1);
        for(v = 0; v < model->num_of_vars; v++){
          nip_free_potential(parameters[v]);
        }
        free(parameters);
        nip_empty_double_list(learning_curve);
        return e;
      }
    }

    /* Check if the parameters were valid in any sense */
    if(old_loglikelihood > loglikelihood + (ts_steps * threshold) ||
       loglikelihood > 0 ||
       loglikelihood == -HUGE_DOUBLE){ /* some "impossible" data */
      for(v = 0; v < model->num_of_vars; v++){
        nip_free_potential(parameters[v]);
      }
      free(parameters);
      /* Return the list as it is */
      return NIP_ERROR_BAD_LUCK;
    }

    /* Remember this!
     * (It helps if you insist having a minimum amount of iterations :) */
    i++;

    /* Check for convergence or other stopping criteria */
    if (i >= MIN_EM_ITERATIONS) {
      if ((loglikelihood - old_loglikelihood) > (ts_steps * threshold)) {
        if (i >= max_iterations) {
          converged = 1;
          if (stopping_criterion)
            *stopping_criterion = ITERATIONS;
        }
      } else {
        converged = 1;
        if (stopping_criterion)
          *stopping_criterion = DELTA;
      }
    } // else: force minimum number of iterations

  } while (!converged);

  for(v = 0; v < model->num_of_vars; v++){
    nip_free_potential(parameters[v]);
  }
  free(parameters);

  return NIP_NO_ERROR;
}


/* a little wrapper */
double model_prob_mass(nip_model model){
  double m;
  m = nip_probability_mass(model->cliques, model->num_of_cliques);
  return m;
}


double* get_probability(nip_model model, nip_variable v){
  nip_clique clique_of_interest;
  double *result;
  int cardinality;

  if(!model){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NULL;
  }
  if(!v){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NULL;
  }
  cardinality = NIP_CARDINALITY(v);
  result = (double *) calloc(cardinality, sizeof(double));
  if(!result){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  /* 1. Find the clique that contains the interesting variable */
  clique_of_interest = nip_find_family(model->cliques,
                                       model->num_of_cliques, v);
  if(!clique_of_interest){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    free(result);
    return NULL;
  }

  /* 2. Marginalisation (the memory must have been allocated) */
  nip_marginalise_clique(clique_of_interest, v, result);

  /* 3. Normalisation */
  nip_normalise_array(result, cardinality);

  /* 4. Return the result */
  return result;
}


nip_potential get_joint_probability(nip_model model,
                                    nip_variable *vars,
                                    int nvars){
  nip_potential p;
  int i;

  /* Unmark all cliques */
  for (i = 0; i < model->num_of_cliques; i++)
    nip_unmark_clique(model->cliques[i]);

  /* Make a DFS in the tree... */
  p = nip_gather_joint_probability(model->cliques[0],
                                   vars, nvars, NULL, 0);
  if(p == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    return NULL;
  }

  /* Normalisation (???) */
  nip_normalise_potential(p);
  return p;
}


/* JJ: this has some common elements with the forward_inference function */
time_series generate_data(nip_model model, int length){
  int i, j, k, t;
  int *cardinalities = NULL;
  nip_potential alpha = NULL;
  int nvars = model->num_of_vars;
  nip_variable *vars = NULL;
  /* reserved the possibility to pass the set of variables
   * as a parameter in order to omit part of the data...   */
  nip_variable v;
  time_series ts = NULL;
  double *distribution = NULL;

  vars = (nip_variable*) calloc(nvars, sizeof(nip_variable));
  if(!vars){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  /* sort the variables appropriately */
  for(i = 0; i < model->num_of_vars; i++)
    nip_unmark_variable(model->variables[i]);

  /* first the independent variables... */
  j = 0;
  for(i = 0; i < model->num_of_vars; i++){
    v = model->variables[i];
    if(nip_number_of_parents(v) == 0){
      vars[j++] = v;
      nip_mark_variable(v);
    }
  }

  /* ...then the children whose parents are already included */
  while(j < nvars){
    for(i = 0; i < model->num_of_vars; i++){
      v = model->variables[i];
      if(nip_variable_marked(v))
        continue;
      t = 1;
      for(k = 0; k < nip_number_of_parents(v); k++){
        if(!nip_variable_marked(v->parents[k])){
          t = 0;
          break;
        }
      }
      if(t){
        vars[j++] = v;
        nip_mark_variable(v);
      }
    }
  }

  /* allocate memory for the time series */
  ts = (time_series) malloc(sizeof(time_series_struct));
  if(!ts){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
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
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(ts);
    free(vars);
    return(NULL);
  }
  for(t = 0; t < ts->length; t++){
    ts->data[t] = (int*) calloc(nvars, sizeof(int));
    if(!(ts->data[t])){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
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
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      free_timeseries(ts);
      return NULL;
    }
  }

  for(i = 0; i < model->outgoing_interface_size; i++)
    cardinalities[i] = NIP_CARDINALITY(model->outgoing_interface[i]);
  alpha = nip_new_potential(cardinalities, model->outgoing_interface_size, NULL);
  free(cardinalities);

  /* new seed number for rand and clear the previous evidence */
  /*random_seed(NULL);*/
  reset_model(model);
  use_priors(model, !NIP_HAD_A_PREVIOUS_TIMESLICE);

  /* for each time step */
  for(t = 0; t < ts->length; t++){

    /* influence from the previous time step */
    if(t > 0){
      if(finish_timeslice_message_pass(model, FORWARD,
                                       alpha, NULL) != NIP_NO_ERROR){
        nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
        free_timeseries(ts);
        nip_free_potential(alpha);
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
      k = lottery(distribution, NIP_CARDINALITY(v));
      free(distribution);
      /*** insert into the time series and the model as evidence */
      ts->data[t][i] = k;
      nip_enter_index_observation(model->variables, model->num_of_vars,
                                  model->cliques, model->num_of_cliques, v, k);
    }
    make_consistent(model);

    /* influence from the current time slice to the next one */
    if(start_timeslice_message_pass(model, FORWARD, alpha) != NIP_NO_ERROR){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
      free_timeseries(ts);
      nip_free_potential(alpha);
      return NULL;
    }

    /* Forget old evidence */
    reset_model(model);
    use_priors(model, NIP_HAD_A_PREVIOUS_TIMESLICE);
  }
  nip_free_potential(alpha);
  return ts;
}


/* Most of this is borrowed from Jaakko Hollmen. */
long random_seed(long* seedpointer){
  struct tm aika, *aikap;
  time_t aika2;
  long seedvalue;

  if(seedpointer == NULL){
    /* The time since midnight in seconds in the seed for the random
       number generator. Make sure not re-seeding more often than once
       per second! */
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
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
      return size-1;
    }
    sum += distribution[i++];
  } while(sum < r);
  return i-1;
}


void print_cliques(nip_model model){
  int i;
  int num_of_cliques;
  nip_sepset s;
  nip_clique c;
  nip_sepset_link sepsetlist;
  nip_clique *cliques;

  if(!model){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return;
  }

  cliques = model->cliques;
  num_of_cliques = model->num_of_cliques;

  printf("Cliques of the model:\n");
  for(i = 0; i < num_of_cliques; i++){
    c = cliques[i];
    nip_fprintf_clique(stdout, c);
    nip_fprintf_potential(stdout, c->p);
    sepsetlist = c->sepsets;
    while(sepsetlist){
      s = (nip_sepset)sepsetlist->data;
      nip_fprintf_sepset(stdout, s);
      nip_fprintf_potential(stdout, s->new);
      sepsetlist = sepsetlist->fwd;
    }
    printf("\n");
  }
}
