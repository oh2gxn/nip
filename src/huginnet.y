/* huginnet.y $Id: huginnet.y,v 1.88 2010-11-11 16:38:07 jatoivol Exp $
 * Grammar file for a subset of the Hugin Net language.
 */

%{
#define _GNU_SOURCE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lists.h"
#include "parser.h"
#include "clique.h"
#include "nipvariable.h"
#include "niperrorhandler.h"
#include "Graph.h"

/* The current input file 
 * TODO: get rid of these global variables and make them a struct... */
static FILE *nip_net_file = NULL;

/* Is there a hugin net file open? 0 if no, 1 if yes. 
 * NOTE: could "nip_net_file == NULL" be used for representing the same info? 
 */
static int nip_net_file_open = 0;

/* Global variables for relaying results:
 * Some of the results are returned via global variables and some 
 * as the semantic values of net language constructs. 
 * The functional paradigm would be more elegant... */
static int nip_node_position_x = 100;
static int nip_node_position_y = 100;
static int nip_node_size_x = 80;
static int nip_node_size_y = 60;

static doublelist nip_parsed_doubles = NULL;
static int        nip_data_size      = 0;

static stringlist nip_parsed_strings = NULL;
static char**     nip_statenames = NULL;
static int        nip_n_statenames = 0;

/* All the unrecognized MY_field = "value" pairs */
static stringpairlist nip_ignored_net_fields = NULL;
static stringpairlist nip_ignored_node_fields = NULL;
static stringpairlist nip_ignored_potential_fields = NULL;

static char* nip_label;       /* node label contents */
static char* nip_persistence; /* NIP_next contents   */

static nip_variable_list nip_parsed_vars   = NULL;
static nip_variable_list nip_parent_vars   = NULL;

static Graph* nip_graph = NULL;

static potentialList nip_parsed_potentials = NULL;

static nip_interface_list nip_interface_relations = NULL;

static clique* nip_cliques = NULL;
static int   nip_n_cliques = 0;

static int yylex (void);

static void yyerror (const char *s);  /* Called by yyparse on error */

static int parsed_vars_to_graph(nip_variable_list vl, Graph* g);

static int parsed_potentials_to_jtree(potentialList potentials, 
				      clique* cliques, int ncliques);

static int interface_to_vars(nip_interface_list il, nip_variable_list vl);

static void print_parsed_stuff(potentialList pl);


/* Opens an input file. Returns 0 if file was opened or if some file was
 * already open. Returns ERROR_GENERAL if an error occurred
 * opening the file.
 */
FILE *open_net_file(const char *filename);

/* Closes the current input file (if there is one).
 */
void close_net_file();

/* Gives you the list of variables after yylex() */
nip_variable_list get_parsed_variables (void);

/* Gives you the array of cliques after yylex() */
int get_cliques (clique** clique_array_pointer);

/* Gives you the global parameters of the whole network 
 * (node size is the only mandatory field...) */
void get_parsed_node_size(int* x, int* y);
%}

/* BISON Declarations */
/* These are the data types for semantic values. 
 * NOTE: there could be more of these to get rid of global variables... */
%union {
  double numval;
  double *doublearray;
  char *name;
  char **stringarray;
  nip_variable var;
  /* list of X to get rid of global variables? */
}

/***********************/
/* NOTE ABOUT STRINGS! ********************************************/
/* The memory for the strings is allocated in yylex() and should  */
/* be freed if the string is copied or not used!                  */
/* (The "ownership" of strings changes.)                          */
/******************************************************************/

%token token_net "net"
%token token_class "class"
%token token_node_size "node_size"
%token token_data "data"
%token token_utility "utility"
%token token_decision "decision"
%token token_discrete "discrete"
%token token_continuous "continuous"
%token token_node "node"
%token token_label "label"
%token token_position "position"
%token token_states "states"
%token token_persistence "NIP_next"
%token token_potential "potential"
%token token_normal "normal"
%token <name> QUOTED_STRING
%token <name> UNQUOTED_STRING
%token <numval> NUMBER
%type <stringarray> statesDeclaration
%type <doublearray> dataList
%type <var> nodeDeclaration child
%type <name> labelDeclaration persistenceDeclaration

/* Grammar follows */

%%
input:  nodes potentials {
  if(parsed_vars_to_graph(nip_parsed_vars, nip_graph) != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    YYABORT;
  }

  if(interface_to_vars(nip_interface_relations, nip_parsed_vars) 
     != NIP_NO_ERROR){
    yyerror("Invalid timeslice specification!\nCheck NIP_next declarations.");
    YYABORT;
  }
  nip_free_interface_list(nip_interface_relations);

  nip_n_cliques = find_cliques(nip_graph, &nip_cliques);
  free_graph(nip_graph); /* Get rid of the graph (?) */
  nip_graph = NULL;
  if(nip_n_cliques < 0){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    YYABORT;
  }

  if(parsed_potentials_to_jtree(nip_parsed_potentials, 
				nip_cliques, nip_n_cliques) != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    YYABORT;
  }
#ifdef DEBUG_BISON
  print_parsed_stuff(nip_parsed_potentials);
#endif
  free_potentialList(nip_parsed_potentials); /* frees potentials also */
}

/* optional net block */
|  netDeclaration nodes potentials {
  if(parsed_vars_to_graph(nip_parsed_vars, nip_graph) != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    YYABORT;
  }

  if(interface_to_vars(nip_interface_relations, nip_parsed_vars) 
     != NIP_NO_ERROR){
    yyerror("Invalid timeslice specification!\nCheck NIP_next declarations.");
    YYABORT;
  }
  nip_free_interface_list(nip_interface_relations);

  nip_n_cliques = find_cliques(nip_graph, &nip_cliques);
  free_graph(nip_graph); /* Get rid of the graph (?) */
  nip_graph = NULL;
  if(nip_n_cliques < 0){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    YYABORT;
  }

  if(parsed_potentials_to_jtree(nip_parsed_potentials, 
				nip_cliques, nip_n_cliques) != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    YYABORT;
  }
#ifdef DEBUG_BISON
  print_parsed_stuff(nip_parsed_potentials);
#endif
  free_potentialList(nip_parsed_potentials); /* frees potentials also */
}

/* possible old class statement */
| token_class UNQUOTED_STRING '{' parameters nodes potentials '}' {
  free($2); /* the classname is useless */
  if(parsed_vars_to_graph(nip_parsed_vars, nip_graph) != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    YYABORT;
  }

  if(interface_to_vars(nip_interface_relations, nip_parsed_vars) 
     != NIP_NO_ERROR){
    yyerror("Invalid timeslice specification!\nCheck NIP_next declarations.");
    YYABORT;
  }
  nip_free_interface_list(nip_interface_relations);

  nip_n_cliques = find_cliques(nip_graph, &nip_cliques);
  free_graph(nip_graph); /* Get rid of the graph (?) */
  nip_graph = NULL;
  if(nip_n_cliques < 0){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    YYABORT;
  }

  if(parsed_potentials_to_jtree(nip_parsed_potentials, 
				nip_cliques, nip_n_cliques) != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    YYABORT;
  }
#ifdef DEBUG_BISON
  print_parsed_stuff(nip_parsed_potentials);
#endif
  free_potentialList(nip_parsed_potentials); /* frees potentials also */
};


nodes:    /* empty */ { nip_graph = new_graph(nip_parsed_vars->length); }
|         nodeDeclaration nodes {/* a variable added */}
;


potentials:    /* empty */ {/* list of initialisation data ready */}
             | potentialDeclaration potentials {/* potential added */}
;


nodeDeclaration:    token_node UNQUOTED_STRING '{' node_params '}' {
  int i,retval;
  char *label = nip_label;
  char **states = nip_statenames;
  nip_variable v = NULL;
  
  /* have to check that all the necessary fields were included */
  if(label == NULL)
    asprintf(&label, " "); /* default label is empty */

  if(states == NULL){
    free(label); nip_label = NULL;
    asprintf(&label, "NIP parser: The states field is missing (node %s)", $2);
    yyerror(label);
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    free($2);
    free(label);
    free(nip_persistence); nip_persistence = NULL;
    YYABORT;
  }

  v = nip_new_variable($2, label, states, nip_n_statenames);

  if(v == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    free($2);
    free(label); nip_label = NULL;
    free(nip_persistence); nip_persistence = NULL;
    for(i = 0; i < nip_n_statenames; i++)
      free(nip_statenames[i]);
    free(nip_statenames); nip_statenames = NULL; nip_n_statenames = 0;
    YYABORT;
  }
  /* set the parsed position values */
  nip_set_position(v, nip_node_position_x, nip_node_position_y);
  nip_node_position_x = 100; nip_node_position_y = 100; /* reset */

  if(nip_parsed_vars == NULL)
    nip_parsed_vars = nip_new_variable_list();
  nip_append_variable(nip_parsed_vars, v);

  if(nip_persistence != NULL){
    
    if(nip_interface_relations == NULL)
      nip_interface_relations = nip_new_interface_list();
    retval = nip_append_interface(nip_interface_relations, v, nip_persistence);

    if(retval != NIP_NO_ERROR){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
      free($2);
      free(label); nip_label = NULL;
      free(nip_persistence); nip_persistence = NULL;
      for(i = 0; i < nip_n_statenames; i++)
	free(nip_statenames[i]);
      free(nip_statenames); nip_statenames = NULL; nip_n_statenames = 0;
      nip_free_variable(v);
      YYABORT;
    }
  }

  free($2);
  free(label);
  for(i = 0; i < nip_n_statenames; i++)
    free(nip_statenames[i]);
  free(nip_statenames); nip_statenames = NULL; nip_n_statenames = 0;
  nip_persistence = NULL;
  $$ = v;}

|    token_discrete token_node UNQUOTED_STRING '{' node_params '}' {
  int i,retval;
  char *label = nip_label;
  char **states = nip_statenames;
  nip_variable v = NULL;
  
  /* have to check that all the necessary fields were included */
  if(label == NULL)
    asprintf(&label, " "); /* default label is empty */

  if(states == NULL){
    free(label);
    nip_label = NULL;
    asprintf(&label, "NIP parser: The states field is missing (node %s)", $3);
    yyerror(label);
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    free($3);
    free(label);
    free(nip_persistence); nip_persistence = NULL;
    YYABORT;
  }

  v = nip_new_variable($3, label, states, nip_n_statenames);

  if(v == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    free($3);
    free(label);
    nip_label = NULL;
    free(nip_persistence); nip_persistence = NULL;
    for(i = 0; i < nip_n_statenames; i++)
      free(nip_statenames[i]);
    free(nip_statenames); nip_statenames = NULL; nip_n_statenames = 0;
    YYABORT;
  }
  /* set the parsed position values */
  nip_set_position(v, nip_node_position_x, nip_node_position_y);
  nip_node_position_x = 100; nip_node_position_y = 100; /* reset */

  if(nip_parsed_vars == NULL)
    nip_parsed_vars = nip_new_variable_list();
  nip_append_variable(nip_parsed_vars, v);

  if(nip_persistence != NULL){
    if(nip_interface_relations == NULL)
      nip_interface_relations = nip_new_interface_list();
    retval = nip_append_interface(nip_interface_relations, v, nip_persistence);
    if(retval != NIP_NO_ERROR){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
      free($3);
      free(label); nip_label = NULL;
      free(nip_persistence); nip_persistence = NULL;
      for(i = 0; i < nip_n_statenames; i++)
	free(nip_statenames[i]);
      free(nip_statenames); nip_statenames = NULL; nip_n_statenames = 0;
      nip_free_variable(v);
      YYABORT;
    }
  }

  free($3);
  free(label);
  for(i = 0; i < nip_n_statenames; i++)
    free(nip_statenames[i]);
  free(nip_statenames); nip_statenames = NULL; nip_n_statenames = 0;
  nip_persistence = NULL;
  $$ = v;}

| token_continuous token_node UNQUOTED_STRING '{' ignored_params '}' { 
  int i;
  char *label = nip_label;
  free(label); nip_label = NULL;
  asprintf(&label, "NET parser: Continuous variables (node %s) %s", $3, 
	   "are not supported.");
  yyerror(label);
  nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
  free($3);
  free(label);
  free(nip_persistence); nip_persistence = NULL;
  for(i = 0; i < nip_n_statenames; i++)
    free(nip_statenames[i]);
  free(nip_statenames); nip_statenames = NULL; nip_n_statenames = 0;
  YYABORT;
  $$=NULL;}

| token_utility UNQUOTED_STRING '{' ignored_params '}' { 
  int i;
  char *label = nip_label;
  free(label); nip_label = NULL;
  asprintf(&label, "NET parser: Utility nodes (node %s) %s", $2, 
	   "are not supported.");
  yyerror(label);
  nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
  free($2);
  free(label);
  free(nip_persistence); nip_persistence = NULL;
  for(i = 0; i < nip_n_statenames; i++)
    free(nip_statenames[i]);
  free(nip_statenames); nip_statenames = NULL; nip_n_statenames = 0;
  YYABORT;
  $$=NULL;}

| token_decision UNQUOTED_STRING '{' ignored_params '}' { 
  int i;
  char *label = nip_label;

  free(label); nip_label = NULL;
  asprintf(&label, "NET parser: Decision nodes (node %s) %s", $2, 
	   "are not supported.");
  yyerror(label);
  nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
  free($2);
  free(label);
  free(nip_persistence); nip_persistence = NULL;
  for(i = 0; i < nip_n_statenames; i++)
    free(nip_statenames[i]);
  free(nip_statenames); nip_statenames = NULL; nip_n_statenames = 0;
  YYABORT;
  $$=NULL;}
;

ignored_params: /* end of list */
|            unknownDeclaration ignored_params
|            statesDeclaration ignored_params { /*nip_statenames = $1;*/ }
|            labelDeclaration ignored_params { nip_label = $1; }
|            persistenceDeclaration ignored_params { nip_persistence = $1; }
|            positionDeclaration ignored_params
;


node_params: /* end of definitions */
|            unknownDeclaration node_params
|            statesDeclaration node_params { /*nip_statenames = $1;*/ }
|            labelDeclaration node_params { nip_label = $1; }
|            persistenceDeclaration node_params { nip_persistence = $1; }
|            positionDeclaration node_params
;


netDeclaration: token_net '{' parameters '}' { /* did enough already */ }
;


parameters:    /* end of definitions */
| nodeSizeDeclaration parameters {}
| unknownDeclaration parameters {}
;


labelDeclaration:     token_label '=' QUOTED_STRING ';' { $$ = $3; }
;


persistenceDeclaration: token_persistence '=' QUOTED_STRING ';' { $$ = $3; }
;


statesDeclaration:    token_states '=' '(' strings ')' ';' { 

  /* makes an array of strings out of the parsed list of strings */
  nip_statenames = list_to_string_array(nip_parsed_strings);
  nip_n_statenames = nip_parsed_strings->length;

  /* free the list (not the strings) */
  empty_stringlist(nip_parsed_strings);
  free(nip_parsed_strings); nip_parsed_strings = NULL;

  if(!nip_statenames){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    YYABORT;
  }

  $$ = nip_statenames;
}
;


positionDeclaration:  token_position '=' '(' NUMBER NUMBER ')' ';' {
  nip_node_position_x = abs((int)$4); 
  nip_node_position_y = abs((int)$5);}
;


nodeSizeDeclaration:  token_node_size '=' '(' NUMBER NUMBER ')' ';' {
  nip_node_size_x = abs((int)$4); 
  nip_node_size_y = abs((int)$5);}
;


unknownDeclaration:  UNQUOTED_STRING '=' value ';' { free($1); }
;


potentialDeclaration: token_potential '(' child '|' symbols ')' '{' dataList '}' { 
  /* fixed ? : parser segfaults if <symbols> is an empty list... */
  int i;
  int retval, size;
  int nparents = 0;
  nip_variable *family = NULL;
  nip_variable *parents = NULL;
  double *doubles = $8;
  char* error = NULL;
  potential p = NULL;

  if(nip_parent_vars != NULL)
    nparents = nip_parent_vars->length;

  family = (nip_variable*) calloc(nparents + 1, sizeof(nip_variable));

  if(nip_parent_vars != NULL)
    parents = nip_variable_list_to_array(nip_parent_vars);

  /* TODO: parser could survive even when "potential(A| )" happens... */
  if(!parents || !family){
    free(family);
    free(parents);
    free(doubles);
    YYABORT;
  }

#ifdef DEBUG_BISON
  printf("nip_symbols_parsed = %d\n", nparents);
#endif

  family[0] = $3;
  size = NIP_CARDINALITY(family[0]);
  for(i = 0; i < nparents; i++){
    family[i + 1] = parents[i];
    size = size * NIP_CARDINALITY(parents[i]);
  }
  /* check that nip_data_size >= product of variable cardinalities! */
  if(size > nip_data_size){
    /* too few elements in the specified potential */
    asprintf(&error, 
	     "NET parser: Not enough elements in potential( %s... )!", 
	     nip_variable_symbol(family[0]));
    yyerror(error);
    free(family);
    free(parents);
    free(doubles);
    YYABORT;
  }

  if(nip_parsed_potentials == NULL)
    nip_parsed_potentials = make_potentialList();

  p = create_potential(family, nparents + 1, doubles);
  normalise_cpd(p); /* useless? */
  retval = append_potential(nip_parsed_potentials, p, family[0], parents);

  free(doubles); /* the data was copied at create_potential */
  nip_empty_variable_list(nip_parent_vars);
  free(family);
  if(retval != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, retval, 1);
    YYABORT;
  }
}
|                     token_potential '(' child ')' '{' dataList '}' {
  int retval;
  nip_variable *family;
  double *doubles = $6;
  char* error = NULL;
  potential p = NULL;
  if(NIP_CARDINALITY($3) > nip_data_size){
    /* too few elements in the specified potential */
    asprintf(&error, 
	     "NET parser: Not enough elements in potential( %s )!", 
	     nip_variable_symbol($3));
    yyerror(error);
    free(doubles);
    YYABORT;
  }
  family = &$3;
  if(nip_parsed_potentials == NULL)
    nip_parsed_potentials = make_potentialList();
  p = create_potential(family, 1, doubles);
  normalise_potential(p); /* <=> normalise_cpd(p) in this case */
  retval = append_potential(nip_parsed_potentials, p, family[0], NULL); 
  free(doubles); /* the data was copied at create_potential */
  if(retval != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, retval, 1);
    YYABORT;
  }
}
|                     token_potential '(' child ')' '{' '}' {
  int retval;
  nip_variable* family;
  family = &$3;

  if(nip_parsed_potentials == NULL)
    nip_parsed_potentials = make_potentialList();
  retval = append_potential(nip_parsed_potentials, 
			    create_potential(family, 1, NULL), 
			    family[0], NULL); 
  if(retval != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    YYABORT;
  }
}
|                     token_potential '(' child '|' symbols ')' '{' '}' {
  /* fixed ? : parser segfaults if <symbols> is an empty list... */
  int i;
  int retval, size;
  int nparents = 0;
  nip_variable *family = NULL;
  nip_variable *parents = NULL;
  potential p = NULL;

  if(nip_parent_vars != NULL)
    nparents = nip_parent_vars->length;

  family = (nip_variable*) calloc(nparents + 1, sizeof(nip_variable));

  if(nip_parent_vars != NULL)
    parents = nip_variable_list_to_array(nip_parent_vars);

  /* TODO: parser could survive even when "potential(A| )" happens... */
  if(!parents || !family){
    free(family);
    free(parents);
    YYABORT;
  }

#ifdef DEBUG_BISON
  printf("nip_symbols_parsed = %d\n", nparents);
#endif

  family[0] = $3;
  size = NIP_CARDINALITY(family[0]);
  for(i = 0; i < nparents; i++){
    family[i + 1] = parents[i];
    size = size * NIP_CARDINALITY(parents[i]);
  }

  if(nip_parsed_potentials == NULL)
    nip_parsed_potentials = make_potentialList();

  p = create_potential(family, nparents + 1, NULL);
  normalise_cpd(p); /* useless? */
  retval = append_potential(nip_parsed_potentials, p, family[0], parents);

  nip_empty_variable_list(nip_parent_vars);
  free(family);
  if(retval != NIP_NO_ERROR){
    nip_report_error(__FILE__, __LINE__, retval, 1);
    YYABORT;
  }
}
;


child:        UNQUOTED_STRING { 
  $$ = nip_search_variable_list(nip_parsed_vars, $1);
  /* NOTE: you could check unrecognized child variables here */
  free($1); }
;


symbols:       /* end of list */
             | symbol symbols
;


symbol:       UNQUOTED_STRING { 
	       /* NOTE: inverted list takes care of the correct order... */
	       int retval;
	       if(nip_parent_vars == NULL)
		 nip_parent_vars = nip_new_variable_list();
	       retval = nip_prepend_variable(nip_parent_vars, 
			  nip_search_variable_list(nip_parsed_vars, $1));
	       if(retval != NIP_NO_ERROR){
		 /* NOTE: you could check unrecognized parent variables here */
		 nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
		 YYABORT;
	       }
	       free($1); }
;


strings:       /* end of list */
             | string strings
;


string:        QUOTED_STRING {
	       int retval;

	       if(nip_parsed_strings == NULL)
		 nip_parsed_strings = make_stringlist();

	       retval = append_string(nip_parsed_strings, $1);
	       if(retval != NIP_NO_ERROR){
		 nip_report_error(__FILE__, __LINE__, retval, 1);
		 YYABORT;
	       }
}
;


numbers:     /* end of list */
           | num numbers
           | '(' numbers ')' numbers
;


num:       NUMBER {
	     int retval;

	     /* If the list is not created yet */
	     if(nip_parsed_doubles == NULL)
	       nip_parsed_doubles = make_doublelist();

	     retval = append_double(nip_parsed_doubles, $1);
	     if(retval != NIP_NO_ERROR){
	       nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
	       YYABORT;
	     }
}
;


ignored_numbers:     /* end of list */
           | NUMBER ignored_numbers {}
           | '(' ignored_numbers ')' ignored_numbers
;


value:         QUOTED_STRING { free($1); }
|              ignored_numbers
;


dataList: token_data '=' '(' numbers ')' ';' {
  /* Note: this doesn't normalise them in any way */
  double *doubles = list_to_double_array(nip_parsed_doubles);
  nip_data_size = nip_parsed_doubles->length;
  empty_doublelist(nip_parsed_doubles); 
  free(nip_parsed_doubles); nip_parsed_doubles = NULL;
  if(!doubles){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    YYABORT;
  }
  $$ = doubles;
}
;

%%
/* Lexical analyzer */
/*
 *      yylex is able to parse strings i.e. strings are terminals: 
 *      "this is a string": char* -> [t, h, i, s, ... , g, \0] as lvalue 
 */

#include <ctype.h>

static int
yylex (void)
{
  int tokenlength;
  int retval = 0;
  char *token = next_token(&tokenlength, nip_net_file);
  char *nullterminated;
  char *endptr;
  double numval;

  /* EOF or error */
  if(tokenlength <= 0)
    return 0;

  /* Single character */
  else if(tokenlength == 1){
    retval = *token;

    nullterminated = (char *) calloc(2, sizeof(char));
    if(!nullterminated){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      free(token);
      return 0; /* In the case of an (unlikely) error, stop the parser */
    }
    nullterminated[0] = *token;
    nullterminated[1] = '\0';

    /* Single letter ('A' - 'Z' or 'a' - 'z') is UNQUOTED_STRING. */
    if(isalpha((int)*token)){
      yylval.name = nullterminated;
      free(token);

      return UNQUOTED_STRING;
    }

    /* Single digit ('0' - '9') is NUMBER. */
    else if(isdigit((int)*token)){
      yylval.numval = strtod(nullterminated, 0);
      free(token);
      free(nullterminated);
      return NUMBER;
    }

    /* Other chars (';' '(', ')', etc. ) */
    else {
      free(token);
      free(nullterminated);
      return retval;
    }
  }

  /* Multicharacter tokens */
  else{
    /* Literal string tokens */ 

    /* net */
    if(tokenlength == 3 && strncmp("net", token, 3) == 0){
      free(token);
      return token_net;
    }

    if(tokenlength == 4){
      /* node */
      if(strncmp("node", token, 4) == 0){
	free(token);
	return token_node;
      }
      /* data */
      else if(strncmp("data", token, 4) == 0){
	free(token);
	return token_data;
      }
    }

    if(tokenlength == 5){
      /* label */
      if(strncmp("label", token, 5) == 0){
	free(token);
	return token_label;
      }
      /* class */
      else if(strncmp("class", token, 5) == 0){
	free(token);
	return token_class;
      }
    }

    if(tokenlength == 6){
      /* states */
      if(strncmp("states", token, 6) == 0){
	free(token);
	return token_states;
      }
      /* normal */
      else if(strncmp("normal", token, 6) == 0){
	free(token);
	return token_normal;
      }
    }

    if(tokenlength == 7){
      /* utility */
      if(strncmp("utility", token, 7) == 0){
	free(token);
	return token_utility;
      }      
    }

    if(tokenlength == 8){
      /* position */
      if(strncmp("position", token, 8) == 0){
	free(token);
	return token_position;
      }
      /* decision */
      else if(strncmp("decision", token, 8) == 0){
	free(token);
	return token_decision;
      }
      /* discrete */
      else if(strncmp("discrete", token, 8) == 0){
	free(token);
	return token_discrete;
      }
      /* NIP_next */
      else if(strncmp("NIP_next", token, 8) == 0){
	free(token);
	return token_persistence;
      }
    }

    if(tokenlength == 9){ 
      /* node_size */
      if(strncmp("node_size", token, 9) == 0){
	free(token);
	return token_node_size;
      }
      /* potential */
      else if(strncmp("potential", token, 9) == 0){
	free(token);
	return token_potential;
      }
    }

    /* continuous */
    if(tokenlength == 10 &&
       strncmp("continuous", token, 10) == 0){
      free(token);
      return token_continuous;
    }

    /* End of literal string tokens */

    /* Regular tokens (not literal string) */
    /* QUOTED_STRING (enclosed in double quotes) */
    if(token[0] == '"' &&
       token[tokenlength - 1] == '"'){
      nullterminated = (char *) calloc(tokenlength - 1, sizeof(char));
      if(!nullterminated){
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
	free(token);
	return 0; /* In the case of an (unlikely) error, stop the parser */
      }
      /* For the semantic value of the string, strip off double quotes
       * and insert terminating null character. */
      strncpy(nullterminated, &(token[1]), tokenlength - 2);
      nullterminated[tokenlength - 2] = '\0';
      yylval.name = nullterminated;

      free(token);

      return QUOTED_STRING;
    }

    nullterminated = (char *) calloc(tokenlength + 1, sizeof(char));
    if(!nullterminated){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      free(token);
      return 0; /* In the case of an (unlikely) error, stop the parser */
    }
    strncpy(nullterminated, token, tokenlength);
    nullterminated[tokenlength] = '\0';

    /* NUMBER ? */
    numval = strtod(nullterminated, &endptr);
    /* No error, so the token is a valid double. */
    if(!(nullterminated == endptr && numval == 0)){
      yylval.numval = numval;
      free(token);
      free(nullterminated);
      return NUMBER;
    }

    /* Everything else is UNQUOTED_STRING */
    yylval.name = nullterminated;

    free(token);

    return UNQUOTED_STRING;

  }

}


static void
yyerror (const char *s)  /* Called by yyparse on error */
{
  printf ("%s\n", s);
}


/* Puts the variables into the graph */
static int parsed_vars_to_graph(nip_variable_list vl, Graph* g){
  int i, retval;
  nip_variable v;
  nip_variable_iterator it;
  potentialLink initlist = nip_parsed_potentials->first;

  /*assert(vl != NULL);*/
  it = vl->first;
  v = nip_next_variable(&it);
  
  /* Add parsed variables to the graph. */
  while(v != NULL){
    retval = add_variable(g, v);
    if(retval != NIP_NO_ERROR){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
      return NIP_ERROR_GENERAL;
    }
    v = nip_next_variable(&it);
  }

  /* Add child - parent relations to the graph. */
  while(initlist != NULL){  
    for(i = 0; i < initlist->data->num_of_vars - 1; i++){
      retval = add_child(g, initlist->parents[i], initlist->child);
      if(retval != NIP_NO_ERROR){

	if(g == NULL)
	  printf("DEBUG\n");

	assert(initlist->parents[i] != NULL); /* FAILS! */

	printf("Child:  %s\n", initlist->child->symbol);
	printf("Parent: %s\n", initlist->parents[i]->symbol);

	nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
	return NIP_ERROR_GENERAL;
      }
    }
    
    /* Add child - parent relations to variables themselves also */
    nip_set_parents(initlist->child, initlist->parents, 
		    initlist->data->num_of_vars - 1);

    initlist = initlist->fwd;
  }
  return NIP_NO_ERROR;
}


/* Initialises the join tree (clique array) with parsed potentials. 
 * NOTE: the priors of independent variables are not entered into the 
 * join tree (as evidence), but are stored into the variable though.
 * Returns an error code. (0 is O.K.) */
static int parsed_potentials_to_jtree(potentialList potentials, 
				      clique* cliques, int ncliques){
  int retval;
  potentialLink initlist = potentials->first;
  clique fam_clique = NULL;

  while(initlist != NULL){
    fam_clique = find_family(cliques, ncliques, initlist->child);

    if(fam_clique != NULL){
      if(initlist->data->num_of_vars > 1){
	/* Conditional probability distributions are initialised into
	 * the jointree potentials */
	retval = initialise(fam_clique, initlist->child, 
			    initlist->data, 0); /* THE job */
	if(retval != NIP_NO_ERROR){
	  nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
	  return NIP_ERROR_GENERAL;
	}
      }
      else{
	/* Priors of the independent variables are stored into the variable 
	 * itself, but NOT entered into the model YET. */
	/*retval = enter_evidence(vars, nvars, nip_cliques, 
	 *			nip_num_of_cliques, initlist->child, 
	 *			initlist->data->data); OLD STUFF */
	retval = nip_set_prior(initlist->child, initlist->data->data);
	if(retval != NIP_NO_ERROR){
	  nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
	  return NIP_ERROR_GENERAL;
	}
      }
    }
    else
      fprintf(stderr, "In %s (%d): find_family failed!\n", __FILE__, __LINE__);
    initlist = initlist->fwd;
  }
  return NIP_NO_ERROR;
}


static int interface_to_vars(nip_interface_list il, nip_variable_list vl){
  int i, m;
  nip_variable v1, v2;
  nip_interface_link initlist = NULL;
  nip_variable_iterator it = NULL;
  
  if(il == NULL)
    return NIP_NO_ERROR;

  if(vl == NULL){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  /* Add time relations to variables. */
  initlist = LIST_ITERATOR(il);
  while(initlist != NULL){
    v1 = initlist->var;
    v2 = nip_search_variable_list(vl, initlist->next);
    if(v1->cardinality == v2->cardinality){
      /* check if the interface variables really match */
      for(i = 0; i < v1->cardinality; i++){
	if(strcmp(nip_get_state_name(v1, i), nip_get_state_name(v2, i))){
	  fprintf(stderr, 
		  "Warning: Corresponding variables %s and %s\n", 
		  nip_variable_symbol(v1), nip_variable_symbol(v2));
	  fprintf(stderr, 
		  "have different kind of states %s and %s!\n", 
		  nip_get_state_name(v1,i), nip_get_state_name(v2,i));
	}
      }
      /* the core */
      v2->next = v1;     /* v2 belongs to V(t)   */
      v1->previous = v2; /* v1 belongs to V(t-1) */
    }
    else{
      fprintf(stderr, 
	      "NET parser: Invalid 'NIP_next' field for node %s. ",
	      nip_variable_symbol(v1));
      fprintf(stderr, 
	      "Node %s does not match with it.",
	      nip_variable_symbol(v2));
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
      return NIP_ERROR_GENERAL;
    }
    initlist = initlist->fwd;
  }

  /* Find out which variables belong to incoming/outgoing interfaces */
  it = LIST_ITERATOR(vl);
  v2 = nip_next_variable(&it); /* get a variable */
  while(v2 != NULL){
    m = 0;

    for(i = 0; i < v2->num_of_parents; i++){ /* for each parent */
      v1 = v2->parents[i];

      /* Condition for belonging to the incoming interface */
      if(v1->previous != NULL && 
	 v2->previous == NULL){ 
	/* v2 has the parent v1 in the previous time slice */
	v1->interface_status |= NIP_INTERFACE_OLD_OUTGOING;
	v1->previous->interface_status |= NIP_INTERFACE_OUTGOING;
	v2->interface_status |= NIP_INTERFACE_INCOMING;
	m = 1;
	/* break; ?? */

#ifdef DEBUG_BISON
	fprintf(stdout, 
	      "NET parser: Node %s in I_{t}->\n",
	      nip_variable_symbol(v1->previous));
	fprintf(stdout, 
	      "NET parser: Node %s in I_{t-1}->\n",
	      nip_variable_symbol(v1));
	fprintf(stdout, 
	      "NET parser: Node %s in I_{t}<-\n",
	      nip_variable_symbol(v2));
#endif
      }
    }
    if(m){ /* parents of v2 in this time slice belong to incoming interface */
      for(i = 0; i < v2->num_of_parents; i++){
	v1 = v2->parents[i];
	if(v1->previous == NULL){ 
	  /* v1 (in time slice t) is married with someone in t-1 */
	  v1->interface_status |= NIP_INTERFACE_INCOMING;

#ifdef DEBUG_BISON
	fprintf(stdout, 
	      "NET parser: Node %s in I_{t}<-\n",
	      nip_variable_symbol(v1));
#endif
	}
      }
    }
    v2 = nip_next_variable(&it);
  }
  return NIP_NO_ERROR;
}


static void print_parsed_stuff(potentialList pl){
  int i, j;
  unsigned long temp;
  potentialLink list = pl->first;

  /* Traverse through the list of parsed potentials. */
  while(list != NULL){
    int* indices; 
    int* temp_array;
    nip_variable* variables;

    if((indices = (int *) calloc(list->data->num_of_vars,
				 sizeof(int))) == NULL){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      return;
    }

    if((temp_array = (int *) calloc(list->data->num_of_vars,
				    sizeof(int))) == NULL){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      free(indices);
      return;
    }

    if((variables = (nip_variable *) calloc(list->data->num_of_vars,
					sizeof(nip_variable))) == NULL){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      free(indices);
      free(temp_array);
      return;
    }
    
    variables[0] = list->child;
    for(i = 1; i < list->data->num_of_vars; i++)
      variables[i] = (list->parents)[i - 1];

    /* reorder[i] is the place of i:th variable (in the sense of this program) 
     * in the array variables[] */
    
    /* init (needed?) */
    for(i = 0; i < list->data->num_of_vars; i++)
      temp_array[i] = 0;
    
    /* Create the reordering table: O(num_of_vars^2) i.e. stupid but working.
     * Note the temporary use of indices array. */
    for(i = 0; i < list->data->num_of_vars; i++){
      temp = nip_variable_id(variables[i]);
      for(j = 0; j < list->data->num_of_vars; j++){
	if(nip_variable_id(variables[j]) > temp)
	  temp_array[j]++; /* counts how many greater variables there are */
      }
    }
    
    /* Go through every number in the potential array. */
    for(i = 0; i < list->data->size_of_data; i++){
      
      inverse_mapping(list->data, i, indices);

      printf("P( %s = %s", list->child->symbol, 
	     (list->child->state_names)[indices[temp_array[0]]]);

      if(list->data->num_of_vars > 1)
	printf(" |");

      for(j = 0; j < list->data->num_of_vars - 1; j++)
	printf(" %s = %s", ((list->parents)[j])->symbol,
	       (((list->parents)[j])->state_names)[indices[temp_array[j + 1]]]);
      
      printf(" ) = %.2f \n", (list->data->data)[i]);
    }
    list = list->fwd;
    
    free(indices);
    free(temp_array);
    free(variables);
  }
}


FILE *open_net_file(const char *filename){
  if(!nip_net_file_open){
    nip_net_file = fopen(filename,"r");
    if (!nip_net_file){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_IO, 1);
      return NULL; /* fopen(...) failed */
    }
    else{
      nip_net_file_open = 1;
      /* nip_read_line = 1; Was this necessary? JJT 5.1.2007 */
    }
  }
  return nip_net_file;
}


void close_net_file(){
  if(nip_net_file_open){
    fclose(nip_net_file);
    nip_net_file_open = 0;
  }
}


/* Gives you the list of variables after yyparse() */
nip_variable_list get_parsed_variables (void){
  return nip_parsed_vars;
}


/* Gives you the array of cliques after yyparse() */
int get_cliques (clique** clique_array_pointer){
  *clique_array_pointer = nip_cliques;
  return nip_n_cliques;
}


void get_parsed_node_size(int* x, int* y){
  *x = nip_node_size_x;
  *y = nip_node_size_y;
}
