/*
 * Variable.h $Id: Variable.h,v 1.31 2004-07-02 11:07:21 jatoivol Exp $
 */

#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#include "potential.h"
#define VAR_SYMBOL_LENGTH 20
#define VAR_NAME_LENGTH 40
#define VAR_STATENAME_LENGTH 20
#define VAR_MIN_ID 1

struct nip_var {
  char symbol[VAR_SYMBOL_LENGTH + 1]; /* short symbol for the node */
  char name[VAR_NAME_LENGTH + 1]; /* label in the Net language*/
  char **statenames; /* a string array with <cardinality> strings */
  int cardinality;
  unsigned long id; /* unique id for every variable */
  double *likelihood; /* likelihood of each value */
  struct nip_var *previous;
  struct nip_var *next;
};

typedef struct nip_var vtype;
typedef vtype *Variable;


struct varlist {
  Variable data;
  struct varlist *fwd;
  struct varlist *bwd;
};

typedef struct varlist varelement;
typedef varelement *varlink;
typedef varlink Variable_iterator; /* FIXME if you will */

/* Creates a new Variable:
 * - symbol is a short name e.g. A (= array [A, \0])
 * - name is a more verbose name e.g. "rain" or NULL 
 * - states is an array of strings containing the state names or NULL
 * - cardinality is the number of states/values the variable has */
Variable new_variable(const char* symbol, const char* name, 
		      char** states, int cardinality);


/* Function for copying a Variable (if needed). Handle with care.
 * v: the Variable to be copied
 * Returns the copy.
 */
Variable copy_variable(Variable v);


/* Frees the memory used by the Variable v. */
void free_variable(Variable v);


/* Method for testing variable equality. 
 * This may be needed to keep potentials in order. INEQUALITIES ???
 */
int equal_variables(Variable v1, Variable v2);


/* An alternative interface for keeping variables 
 * and thus the potentials in order.
 */
unsigned long get_id(Variable v);


/* Returns the symbol of the Variable. It is a string. 
 * (or NULL if nullpointer given) */
char *get_symbol(Variable v);


/* Gives the numerical representation of the variable state. 
 * Numbers are [0 ... <cardinality-1>] or -1 if the Variable doesn't have
 * such a state. This function is needed when parsing data. */
int get_stateindex(Variable v, char *state);


/* Tells the length of the list of Variables. */
int total_num_of_vars();


/* Call this before searching through the list of variables. */
Variable_iterator get_Variable_list();


/* Gives the next Variable in the list of Variables. Returns NULL when 
 * the end of list is reached. */
Variable next_Variable(Variable_iterator *it);


/* Gets the parsed variable according to the symbol. */
Variable get_variable(char *symbol);


/* Gives v a new likelihood array. The size of the array
 * must match v->cardinality
 */
int update_likelihood(Variable v, double likelihood[]);


/* Sets a uniform likelihood for v. */
void reset_likelihood(Variable v);


/* Returns the number of possible values in the Variable v. (-1 if v == NULL)
 */
int number_of_values(Variable v);

#endif /* __VARIABLE_H__ */
