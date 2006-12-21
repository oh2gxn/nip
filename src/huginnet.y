/*
 * huginnet.y $Id: huginnet.y,v 1.73 2006-12-21 17:16:16 jatoivol Exp $
 * Grammar file for a subset of the Hugin Net language.
 */

%{
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lists.h"
#include "parser.h"
#include "clique.h"
#include "variable.h"
#include "errorhandler.h"
#include "Graph.h"

static int nip_node_position_x = 100;
static int nip_node_position_y = 100;
static int nip_node_size_x = 80;
static int nip_node_size_y = 60;

static doublelist nip_parsed_doubles = NULL;
static int        nip_data_size      = 0;

static stringlist nip_parsed_strings = NULL;
static char**     nip_statenames = NULL;
static int        nip_n_statenames = 0;

static char*      nip_label;       /* node label contents */
static char*      nip_persistence; /* NIP_next contents   */

static variablelist nip_parsed_vars   = NULL;
static variablelist nip_parent_vars   = NULL;

static Graph* nip_graph = NULL;

static clique* nip_cliques = NULL;
static int   nip_n_cliques = 0;

static int
yylex (void);

static void
yyerror (const char *s);  /* Called by yyparse on error */

/* Gives you the list of variables after yylex() */
variablelist get_parsed_variables (void);

/* Gives you the array of cliques after yylex() */
int get_cliques (clique** clique_array_pointer);

void get_parsed_node_size(int* x, int* y);
%}

/* BISON Declarations */
/* These are the data types for semantic values. */
%union {
  double numval;
  double *doublearray;
  char *name;
  char **stringarray;
  variable variable;
  /* list of X to save global variables? */
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
%type <variable> nodeDeclaration child
%type <name> labelDeclaration persistenceDeclaration

/* Grammar follows */

%%
input:  nodes potentials {
  if(parsedVars2Graph(nip_parsed_vars, nip_graph) != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    YYABORT;
  }

  if(time2Vars(nip_parsed_vars) != NO_ERROR){
    yyerror("Invalid timeslice specification!\nCheck NIP_next declarations.");
    YYABORT;
  }
  reset_timeinit();

  nip_n_cliques = find_cliques(nip_graph, &nip_cliques);
  free_graph(nip_graph); /* Get rid of the graph (?) */
  nip_graph = NULL;
  if(nip_n_cliques < 0){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    YYABORT;
  }

  if(parsedPots2JTree(nip_cliques, nip_n_cliques) != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    YYABORT;
  }
#ifdef DEBUG_BISON
  print_parsed_stuff();
#endif
  reset_initData();}

/* optional net block */
|  netDeclaration nodes potentials {
  if(parsedVars2Graph(nip_parsed_vars, nip_graph) != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    YYABORT;
  }

  if(time2Vars(nip_parsed_vars) != NO_ERROR){
    yyerror("Invalid timeslice specification!\nCheck NIP_next declarations.");
    YYABORT;
  }
  reset_timeinit();

  nip_n_cliques = find_cliques(nip_graph, &nip_cliques);
  free_graph(nip_graph); /* Get rid of the graph (?) */
  nip_graph = NULL;
  if(nip_n_cliques < 0){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    YYABORT;
  }

  if(parsedPots2JTree(nip_cliques, nip_n_cliques) != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    YYABORT;
  }
#ifdef DEBUG_BISON
  print_parsed_stuff();
#endif
  reset_initData();}

/* possible old class statement */
| token_class UNQUOTED_STRING '{' parameters nodes potentials '}' {
  free($2); /* the classname is useless */
  if(parsedVars2Graph(nip_parsed_vars, nip_graph) != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    YYABORT;
  }

  if(time2Vars(nip_parsed_vars) != NO_ERROR){
    yyerror("Invalid timeslice specification!\nCheck NIP_next declarations.");
    YYABORT;
  }
  reset_timeinit();

  nip_n_cliques = find_cliques(nip_graph, &nip_cliques);
  free_graph(nip_graph); /* Get rid of the graph (?) */
  nip_graph = NULL;
  if(nip_n_cliques < 0){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    YYABORT;
  }

  if(parsedPots2JTree(nip_cliques, nip_n_cliques) != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    YYABORT;
  }
#ifdef DEBUG_BISON
  print_parsed_stuff();
#endif
  reset_initData();}
;


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
  variable v = NULL;
  
  /* have to check that all the necessary fields were included */
  if(label == NULL)
    asprintf(&label, " "); /* default label is empty */

  if(states == NULL){
    free(label); nip_label = NULL;
    asprintf(&label, "NIP parser: The states field is missing (node %s)", $2);
    yyerror(label);
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    free($2);
    free(label);
    free(nip_persistence); nip_persistence = NULL;
    YYABORT;
  }

  v = new_variable($2, label, states, nip_n_statenames);

  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    free($2);
    free(label); nip_label = NULL;
    free(nip_persistence); nip_persistence = NULL;
    for(i = 0; i < nip_n_statenames; i++)
      free(nip_statenames[i]);
    free(nip_statenames); nip_statenames = NULL; nip_n_statenames = 0;
    YYABORT;
  }
  /* set the parsed position values */
  set_position(v, nip_node_position_x, nip_node_position_y);
  nip_node_position_x = 100; nip_node_position_y = 100; /* reset */

  if(nip_parsed_vars == NULL)
    nip_parsed_vars = make_variablelist();
  append_variable(nip_parsed_vars, v);

  if(nip_persistence != NULL){
    retval = add_time_init(v, nip_persistence);
    if(retval != NO_ERROR){
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      free($2);
      free(label); nip_label = NULL;
      free(nip_persistence); nip_persistence = NULL;
      for(i = 0; i < nip_n_statenames; i++)
	free(nip_statenames[i]);
      free(nip_statenames); nip_statenames = NULL; nip_n_statenames = 0;
      free_variable(v);
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
  variable v = NULL;
  
  /* have to check that all the necessary fields were included */
  if(label == NULL)
    asprintf(&label, " "); /* default label is empty */

  if(states == NULL){
    free(label);
    nip_label = NULL;
    asprintf(&label, "NIP parser: The states field is missing (node %s)", $3);
    yyerror(label);
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    free($3);
    free(label);
    free(nip_persistence); nip_persistence = NULL;
    YYABORT;
  }

  v = new_variable($3, label, states, nip_n_statenames);

  if(v == NULL){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
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
  set_position(v, nip_node_position_x, nip_node_position_y);
  nip_node_position_x = 100; nip_node_position_y = 100; /* reset */

  if(nip_parsed_vars == NULL)
    nip_parsed_vars = make_variablelist();
  append_variable(nip_parsed_vars, v);

  if(nip_persistence != NULL){
    retval = add_time_init(v, nip_persistence);
    if(retval != NO_ERROR){
      report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
      free($3);
      free(label); nip_label = NULL;
      free(nip_persistence); nip_persistence = NULL;
      for(i = 0; i < nip_n_statenames; i++)
	free(nip_statenames[i]);
      free(nip_statenames); nip_statenames = NULL; nip_n_statenames = 0;
      free_variable(v);
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
  report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
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
  report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
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
  report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
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
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
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
  int i;
  int retval, size;
  int nparents = nip_parent_vars->length;
  variable *family = NULL;
  variable *parents = NULL;
  double *doubles = $8;
  char* error = NULL;

  family = (variable*) calloc(nparents + 1, sizeof(variable));
  parents = list_to_variable_array(nip_parent_vars);

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
  size = number_of_values(family[0]);
  for(i = 0; i < nparents; i++){
    family[i + 1] = parents[i];
    size = size * number_of_values(parents[i]);
  }
  /* check that nip_data_size >= product of variable cardinalities! */
  if(size > nip_data_size){
    /* too few elements in the specified potential */
    asprintf(&error, 
	     "NET parser: Not enough elements in potential( %s... )!", 
	     get_symbol(family[0]));
    yyerror(error);
    free(family);
    free(parents);
    free(doubles);
    YYABORT;
  }

  retval = add_initData(create_potential(family, nparents + 1, doubles),
			family[0], parents);
  free(doubles); /* the data was copied at create_potential */
  empty_variablelist(nip_parent_vars);
  free(family);
  if(retval != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    YYABORT;
  }
}
|                     token_potential '(' child ')' '{' dataList '}' {
  int retval;
  variable *family;
  double *doubles = $6;
  char* error = NULL;
  if(number_of_values($3) > nip_data_size){
    /* too few elements in the specified potential */
    asprintf(&error, 
	     "NET parser: Not enough elements in potential( %s )!", 
	     get_symbol($3));
    yyerror(error);
    free(doubles);
    YYABORT;
  }
  family = &$3;
  retval = add_initData(create_potential(family, 1, doubles), 
			family[0], NULL); 
  free(doubles); /* the data was copied at create_potential */
  if(retval != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    YYABORT;
  }
}
|                     token_potential '(' child ')' '{' '}' {
  int retval;
  variable* family;
  family = &$3;
  retval = add_initData(create_potential(family, 1, NULL), 
			family[0], NULL); 
  if(retval != NO_ERROR){
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
    YYABORT;
  }
}
;


child:        UNQUOTED_STRING { 
  $$ = get_parser_variable(nip_parsed_vars, $1);
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
		 nip_parent_vars = make_variablelist();
	       retval = prepend_variable(nip_parent_vars, 
			  get_parser_variable(nip_parsed_vars, $1));
	       if(retval != NO_ERROR){
		 /* NOTE: you could check unrecognized parent variables here */
		 report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
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
	       if(retval != NO_ERROR){
		 report_error(__FILE__, __LINE__, retval, 1);
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
	     if(retval != NO_ERROR){
	       report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
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
    report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
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
  char *token = next_token(&tokenlength);
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
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
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
	report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
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
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
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


/* Gives you the list of variables after yylex() */
variablelist get_parsed_variables (void){
  return nip_parsed_vars;
}


/* Gives you the array of cliques after yylex() */
int get_cliques (clique** clique_array_pointer){
  *clique_array_pointer = nip_cliques;
  return nip_n_cliques;
}


void get_parsed_node_size(int* x, int* y){
  *x = nip_node_size_x;
  *y = nip_node_size_y;
}
