/* nipvariable.h 
 *
 * Representation of categorical random variables 
 * in Dynamic Bayes Network models
 *
 * Author: Janne Toivola
 * $Id: nipvariable.h,v 1.4 2010-11-25 18:36:40 jatoivol Exp $
 */

#ifndef __NIPVARIABLE_H__
#define __NIPVARIABLE_H__

/* The name, symbol, statename etc. can be at most 40 characters long... */
#define NIP_VAR_TEXT_LENGTH 40
#define NIP_VAR_MIN_ID 1

/* Binary flags used for identifying different roles of variables in DBNs*/
#define NIP_INTERFACE_NONE          0
#define NIP_INTERFACE_INCOMING      1
#define NIP_INTERFACE_OUTGOING     (1<<1)
#define NIP_INTERFACE_OLD_OUTGOING (1<<2)

/* Binary flags for marking the variables during various algorithms */
#define NIP_MARK_OFF  1
#define NIP_MARK_ON   (1<<1)
#define NIP_MARK_BOTH (NIP_MARK_OFF | NIP_MARK_ON)

/* Number of different values a variable can have */
#define NIP_CARDINALITY(v) ( (v)->cardinality )

/* Values of the binary flags */
#define NIP_MARK(v)        ( (v)->mark )

/* enum interface_type {none, old_outgoing, outgoing};
 * typedef enum interface_type interface_flag; */

/* The struct and typedefs for nip_variable */
struct nip_var {
  char* symbol;       /* Short symbol for the variable */
  char* name;         /* Label in the Net language */
  char** state_names; /* A string array with <cardinality> strings */
  int cardinality;    /* Number of possible values */
  unsigned long id;   /* Unique id for every variable */
  double* likelihood; /* Likelihood of each value */
  double* prior;      /* Prior prob. of each value for an indep. variable */
  int prior_entered;  /* Tells whether the prior is already in use */
  struct nip_var* previous; /* Pointer to the variable which corresponds to
			     * this one in the previous timeslice */
  struct nip_var* next;     /* Pointer to the variable which corresponds to
			     * this one in the next timeslice */
  struct nip_var** parents; /* Array of pointers to the parents */
  int num_of_parents;       /* Number of parents */
  void* family_clique;      /* Possible reference to the family clique */
  int* family_mapping;      /* Maps the variables in the family to the
			     * variables in the family clique */
  int interface_status; /* Which interfaces the variable belongs to */
  int pos_x; /* node position by Hugin */
  int pos_y;
  char mark; /* mark for some algorithms (like DFS and data generation) */
  /* TODO: stringpairlist application_specific_properties; */
};
typedef struct nip_var nip_varstruct;
typedef nip_varstruct* nip_variable;



/* Linked list for storing nip_variables */
struct nip_varlink {
  nip_variable data;       /* the variable */
  struct nip_varlink* fwd; /* next element */
  struct nip_varlink* bwd; /* previous element */
};
typedef struct nip_varlink nip_varlinkstruct;
typedef nip_varlinkstruct* nip_variable_link;
typedef nip_variable_link nip_variable_iterator;

struct nip_varlist {
  int length;              /* number of elements */
  nip_variable_link first; /* beginning of the list */
  nip_variable_link last;  /* end of the list */
};
typedef struct nip_varlist nip_varliststruct;
typedef nip_varliststruct* nip_variable_list;



/* List for storing "NIP_next" relations or other strings, 
 * while other variables are yet to be parsed... */
struct nip_iflink {
  nip_variable var; /* the variable */
  char* next;       /* ID reference to another variable */
  struct nip_iflink* fwd; /* next element */
  struct nip_iflink* bwd; /* previous element */
};
typedef struct nip_iflink nip_iflinkstruct;
typedef nip_iflinkstruct* nip_interface_link;

struct nip_iflist {
  int length;
  nip_interface_link first;
  nip_interface_link last;
};
typedef struct nip_iflist nip_ifliststruct;
typedef nip_ifliststruct* nip_interface_list;



/* Creates a new variable. Parameters:
 * - symbol is a short unique string, e.g. A (= char array {'A', '\0'})
 * - name is a more verbose name e.g. "rain" or NULL 
 * - states is an array of strings containing the state names or NULL
 * - cardinality is the number of states/values the variable can have */
nip_variable nip_new_variable(const char* symbol, const char* name, 
			      char** states, int cardinality);


/* Function for copying variable v. 
 * Handle with care: ID etc. issues will appear!
 * Returns a new allocated variable, so freeing the original one does 
 * not affect the copy.
 */
nip_variable nip_copy_variable(nip_variable v);


/* Frees the memory used by the variable v. 
 * NOTE: remember to REMOVE the variable from all possible lists too. */
void nip_free_variable(nip_variable v);


/* Method for testing variable equality (not the equality of values).
 * This is needed to keep potentials in order.
 * Returns non-zero value if v1 and v2 have equal IDs, zero otherwise.
 */
int nip_equal_variables(nip_variable v1, nip_variable v2);


/* An alternative interface for keeping variables and thus 
 * the potentials ordered. Returns the unique id of the variable v.
 */
unsigned long nip_variable_id(nip_variable v);


/* Method for marking a variable. */
void nip_mark_variable(nip_variable v);

/* Method for unmarking a variable. */
void nip_unmark_variable(nip_variable v);

/* Method for checking if a variable is marked. 
 * Returns zero if not marked. */
int nip_variable_marked(nip_variable v);


/* Returns the symbol of the variable. It is a string reference, 
 * not a new char[], so don't free() it. */
char* nip_variable_symbol(nip_variable v);


/* Gives the numerical representation of the variable state. 
 * This function is useful when parsing data. Parameters:
 * - v: the variable
 * - state: the value represented as a string
 * Returns the index in [0 ... <cardinality-1>] or -1 if the 
 * variable doesn't have such a state. */
int nip_variable_state_index(nip_variable v, char *state);


/* The reciprocal of the function above... 
 */
char* nip_variable_state_name(nip_variable v, int index);


/* Method for searching a variable from an array according to the symbol. 
 * - vars: array of variables
 * - nvars: size of the array
 * - symbol: the symbol of the variable as a string */
nip_variable nip_search_variable_array(nip_variable* vars, int nvars, 
				       char *symbol);


/* Gives variable v new likelihoods. The size of the array
 * must match v->cardinality and the contents are copied, so 
 * likelihood parameter array should be freed after use. 
 * Returns an error code if a null pointer is given.
 */
int nip_update_likelihood(nip_variable v, double likelihood[]);


/* Sets a uniform likelihood for v. */
void nip_reset_likelihood(nip_variable v);


/* Tells how many parents the variable has. */
int nip_number_of_parents(nip_variable v);


/* Sets the position for GUI (a.k.a node position in Hugin NET file) */
void nip_set_variable_position(nip_variable v, int x, int y);


/* Gets you the position for GUI (a.k.a node position in Hugin NET file) */
void nip_get_variable_position(nip_variable v, int* x, int* y);


/* Sets the parents for a variable. The variable references to the parents 
 * are copied, so the parents array should be freed after this.
 * - v: the variable
 * - parents: array of variables (copied)
 * - nparents: number of parents */
int nip_set_parents(nip_variable v, nip_variable* parents, int nparents);


/* Tells which variables are the parents of the variable v. */
nip_variable* nip_get_parents(nip_variable v);


/* Sets the prior for an (independent) variable v. You SHOULD not 
 * set a prior for a variable which has parents. */
int nip_set_prior(nip_variable v, double* prior);


/* Tells the prior distribution of a variable. 
 * (NULL if v depends on others) */
double* nip_get_prior(nip_variable v);


/* Returns a new array (allocates memory!) that contains the given variables
 * in ascending order according to their ID number.
 */
nip_variable* nip_sort_variables(nip_variable* vars, int nvars);


/* Returns a new array that contains the union of given variables. 
 * Preserves the order of first input array. 
 * - nc is size of the union and negative in case of errors
 * - returned pointer is NULL if the set is empty
 * - b==NULL => nb==0 (and c==a)
 * (free the result somewhere)
 */
nip_variable* nip_variable_union(nip_variable* a, nip_variable* b, 
				 int na, int nb, int* nc);


/* Returns a new array that contains the intersection of given variables. 
 * Preserves the order of first input array.
 * - nc is size of the intersection and negative in case of errors
 * - returned pointer is NULL if the set is empty
 * (free the result somewhere)
 */
nip_variable* nip_variable_isect(nip_variable* a, nip_variable* b, 
				 int na, int nb, int* nc);


/* Returns a mapping array which tells the location of subset variables in 
 * the (ordered) set. (free the result somewhere) */
int* nip_mapper(nip_variable* set, nip_variable* subset, 
		int nset, int nsubset);


/* Creates new (empty) lists of variables. 
 */
nip_variable_list nip_new_variable_list();
nip_interface_list nip_new_interface_list();


/* Methods for adding stuff to the end of the lists 
 */
int nip_append_variable(nip_variable_list l, nip_variable v);
int nip_append_interface(nip_interface_list l, nip_variable var, char* next);


/* Methods for adding stuff to the beginning of the lists 
 */
int nip_prepend_variable(nip_variable_list l, nip_variable v);
int nip_prepend_interface(nip_interface_list l, nip_variable var, char* next);


/* Turns the list into an array. (Allocates memory, free it when necessary)
 */
nip_variable* nip_variable_list_to_array(nip_variable_list l);


/* Makes the list of variables empty, 
 * without freeing the variables themselves.
 */
void nip_empty_variable_list(nip_variable_list l);


/* Frees the list structure and the strings stored into it, 
 * but doesn't free the variables 
 */
void nip_free_interface_list(nip_interface_list l);


/* Functions for iterating and searching... */
nip_variable nip_next_variable(nip_variable_iterator* it);
nip_variable nip_search_variable_list(nip_variable_list l, char *symbol);

#endif /* __VARIABLE_H__ */
