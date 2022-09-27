/**
 * @file
 * @brief Representation of categorical random variables in 
 * Dynamic Bayes Network models
 *
 * @author Janne Toivola
 * @author Mikko Korpela
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

#ifndef __NIPVARIABLE_H__
#define __NIPVARIABLE_H__

/* The name, symbol, statename etc. can be at most 40 characters long...
 * FIXME: problematic from UTF-8 point of view! */
#define NIP_VAR_TEXT_LENGTH 40 ///< Limit variable & state names, FIXME
#define NIP_VAR_MIN_ID     1 ///< Smallest valid variable ID
#define NIP_VAR_INVALID_ID 0 ///< A certainly invalid ID

/* Binary flags used for identifying different roles of variables in DBNs*/
#define NIP_INTERFACE_NONE          0 ///< Not in slicing interface
#define NIP_INTERFACE_INCOMING      1 ///< Inherits evidence from past
#define NIP_INTERFACE_OUTGOING     (1<<1) ///< Carries evidence for future
#define NIP_INTERFACE_OLD_OUTGOING (1<<2) ///< Variables of previous slice

/* Binary flags for marking the variables during various algorithms */
#define NIP_MARK_OFF  1 ///< Variable is unmarked
#define NIP_MARK_ON   (1<<1) ///< Variable is marked
#define NIP_MARK_BOTH (NIP_MARK_OFF | NIP_MARK_ON) ///< Either marked or not

#define NIP_CARDINALITY(v) ( (v)->cardinality ) ///< Number of values

/** Values of the binary flags */
#define NIP_MARK(v) ( (v)->mark ) ///< get marking flags

/** Values of interface flags */
#define NIP_IF(v) ( (v)->interface_status ) ///< get interface flags

/**
 * The struct and typedefs for nip_variable */
typedef struct nip_var {
  unsigned long id; ///< Unique id for every variable
  char* symbol; ///< Short symbol for the variable (string)
  char* name; ///< Label in the Net language (string)

  int cardinality; ///< Number of possible values
  char** state_names; ///< Name of each value (strings)
  double* likelihood; ///< Likelihood of each value
  double* prior; ///< Prior prob. of each value for an indep. variable
  int prior_entered; ///< Tells whether the prior is already in use

  struct nip_var* previous; /**< Pointer to the variable which corresponds to
                               this one in the previous timeslice */
  struct nip_var* next; /**< Pointer to the variable which corresponds to
                           this one in the next timeslice */
  int num_of_parents; ///< Number of parents
  struct nip_var** parents; ///< Array of pointers to the parents
  void* family_clique; ///< Possible reference to the family clique
  int* family_mapping; /**< Maps the variables in the family to the
                          variables in the family clique */

  int interface_status; ///< Bit flags which interfaces the variable belongs to
  char mark; ///< mark for some algorithms (like DFS and data generation)

  int pos_x; ///< Node horizontal position in the Net language
  int pos_y; ///< Node vertical position in the Net language
  /* TODO: stringpairlist application_specific_properties; */
} nip_variable_struct;

typedef nip_variable_struct* nip_variable; ///< variable reference

/**
 * Linked list element for storing nip_variables */
typedef struct nip_varlink {
  nip_variable data;       ///< the variable
  struct nip_varlink* fwd; ///< the next element
  struct nip_varlink* bwd; ///< the previous element
} nip_variable_link_struct;

typedef nip_variable_link_struct* nip_variable_link; ///< list element ref
typedef nip_variable_link nip_variable_iterator; ///< iterator reference

/**
 * Linked list for storing nip_variables */
typedef struct nip_varlist {
  int length;              ///< number of elements
  nip_variable_link first; ///< the beginning of the list
  nip_variable_link last;  ///< the end of the list
} nip_variable_list_struct;

typedef nip_variable_list_struct* nip_variable_list; ///< list reference

/**
 * List element for storing "NIP_next" relations or other strings, 
 * while other variables are yet to be parsed. */
typedef struct nip_iflink {
  nip_variable var; ///< the variable
  char* next; ///< reference to another variable symbol (yet to be created)
  struct nip_iflink* fwd; ///< the next element
  struct nip_iflink* bwd; ///< the previous element
} nip_iflink_struct;

typedef nip_iflink_struct* nip_interface_link; ///< list element reference

/**
 * List for storing "NIP_next" relations or other strings, 
 * while other variables are yet to be parsed. */
typedef struct nip_iflist {
  int length;               ///< number of elements
  nip_interface_link first; ///< the beginning
  nip_interface_link last;  ///< the end
} nip_iflist_struct;

typedef nip_iflist_struct* nip_interface_list; ///< list reference

/**
 * Creates a new variable.
 * @param symbol Short unique string, e.g. A (= char array {'A', '\0'})
 * @param name More verbose name e.g. "rain" or NULL 
 * @param states Array of strings containing the state names
 * @param cardinality The number of states/values the variable can have
 * @return reference to a new variable
 * @see nip_free_variable() */
nip_variable nip_new_variable(const char* symbol, const char* name, 
			      char** states, int cardinality);

/**
 * Function for copying variables. 
 * Handle with care: ID etc. issues will appear!
 * Returns a new allocated variable, so freeing the original one does 
 * not affect the copy.
 * TODO: remove this, unless some automation for NIP_next required?
 * @param v Variable to copy
 * @return reference to a new variable
 * @see nip_free_variable() */
nip_variable nip_copy_variable(nip_variable v);

/**
 * Frees the memory used by the variable v. 
 * NOTE: remember to REMOVE the variable from all possible lists too.
 * @param v Variable to be freed */
void nip_free_variable(nip_variable v);

/**
 * Method for testing variable equality (not the equality of values).
 * This is needed to keep potentials in order.
 * @param v1 A variable reference
 * @param v2 Another variable reference
 * @return non-zero value if \p v1 and \p v2 have equal IDs, or 0 otherwise
 */
int nip_equal_variables(nip_variable v1, nip_variable v2);

/**
 * Get IDs for keeping variables and thus the potentials ordered.
 * @param v A variable reference
 * @return the unique id of the variable \p v
 */
unsigned long nip_variable_id(nip_variable v);

/**
 * Method for marking a variable.
 * @param v Reference to the variable */
void nip_mark_variable(nip_variable v);

/**
 * Method for unmarking a variable.
 * @param v Reference to the variable */
void nip_unmark_variable(nip_variable v);

/**
 * Method for checking if a variable is marked. 
 * @param v Reference to the variable
 * @return zero if not marked */
int nip_variable_marked(nip_variable v);

/**
 * Returns the symbol of the variable. It is a string reference, 
 * not a new char[], so don't free() it.
 * @param v Reference to the variable
 * @return a string reference (do not free) */
char* nip_variable_symbol(nip_variable v);

/**
 * Gives a numerical representation of the variable state. 
 * This function is useful when parsing data. 
 * @param v Reference to the variable
 * @param state A value represented as a string
 * @return the index in [0 ... <cardinality-1>] or -1 if the 
 * variable doesn't have such a state. */
int nip_variable_state_index(nip_variable v, char *state);

/**
 * The reciprocal of the function above... 
 * @param v Reference to the variable
 * @param index Which state of the variable \p v
 * @return string reference (do not free)
 * @see nip_variable_state_index()
 */
char* nip_variable_state_name(nip_variable v, int index);

/**
 * Method for searching a variable from an array according to the symbol. 
 * @param vars An array of variables
 * @param nvars Size of the array
 * @param symbol The symbol of the searched variable as a string
 * @return reference to the found variable, or NULL */
nip_variable nip_search_variable_array(nip_variable* vars, int nvars, 
				       char *symbol);

/**
 * Gives a variable some new likelihoods. The size of the array
 * must match \p v->cardinality and the contents are copied, so 
 * likelihood parameter array should be freed after use. 
 * @param v The variable to modify
 * @param likelihood array of size v->cardinality
 * @return an error code if a null pointer is given.
 */
int nip_update_likelihood(nip_variable v, double likelihood[]);

/**
 * Sets a uniform likelihood for \p v.
 * @param v The variable to modify */
void nip_reset_likelihood(nip_variable v);

/**
 * Tells how many parents the variable has.
 * @param v Reference to the variable
 * @return number of parent variables in the Bayes net */
int nip_number_of_parents(nip_variable v);

/**
 * Sets the position for GUI (a.k.a node position in Hugin NET file)
 * @param v Reference to the variable
 * @param x Horizontal coordinate
 * @param y Vertical coordinate */
void nip_set_variable_position(nip_variable v, int x, int y);

/**
 * Gets you the position for GUI (a.k.a node position in Hugin NET file)
 * @param v Reference to the variable
 * @param x Pointer where horizontal coordinate written
 * @param y Pointer where vertical coordinate written */
void nip_get_variable_position(nip_variable v, int* x, int* y);

/**
 * Sets the parents for a variable. The variable references to the parents 
 * are copied, so the parents array should be freed after this.
 * @param v The variable
 * @param parents Array of variables (references copied)
 * @param nparents Number of parents
 * @return error code, or 0 if successful */
int nip_set_parents(nip_variable v, nip_variable* parents, int nparents);

/**
 * Tells which variables are the parents of the variable v.
 * @param v The child variable
 * @return reference to the internal data, do not free() */
nip_variable* nip_get_parents(nip_variable v);

/**
 * Method for testing parenthood.
 * @param parent A parent candidate
 * @param child A child candidate
 * @return 1 if \p parent is a parent variable of \p child in the network */
int nip_variable_is_parent(nip_variable parent, nip_variable child);

/**
 * Sets the prior distribution for an (independent) variable v. 
 * You SHOULD NOT set a prior for a variable which has parents.
 * Does not mean that the prior would have been used in any 
 * inference yet, just that the model stores it 
 * (i.e. \p v->prior_entered stays 0).
 * @param v The independent variable
 * @param prior Pointer to \p v->cardinality probabilities
 * @return An error code if anything went wrong */
int nip_set_prior(nip_variable v, double* prior);

/**
 * Tells the prior distribution of a variable. 
 * (NULL if v depends on others)
 * @param v A variable
 * @return reference to internal data, do not free() */
double* nip_get_prior(nip_variable v);

/**
 * Returns a new array (allocates memory!) that contains the given variables
 * in ascending order according to their ID number.
 * @param vars An original array of variable references
 * @param nvars Size of the array
 * @return A new sorted array of variable references
 */
nip_variable* nip_sort_variables(nip_variable* vars, int nvars);

/**
 * Computes the union of given variables (free the result somewhere).
 * Preserves the order of the first input array \p a.
 * @param a A set of variables as an array
 * @param b Another set of variables as an array
 * @param na Size of set \p a
 * @param nb Size of set \p b
 * @param nc Writes the size of the union, or negative if errors
 * @return Reference to a new array, or NULL if the set is empty
 * e.g. b==NULL => nb==0 (and returns copy of \p a)
 */
nip_variable* nip_variable_union(nip_variable* a, nip_variable* b, 
				 int na, int nb, int* nc);

/**
 * Computes the intersection of given variables. 
 * Preserves the order of first input array.
 * @param a A set of variables as an array
 * @param b Another set of variables as an array
 * @param na Size of set \p a
 * @param nb Size of set \p b
 * @param nc Writes the size of the intersection, or negative if errors
 * @return Reference to a new array, or NULL if the set is empty
 */
nip_variable* nip_variable_isect(nip_variable* a, nip_variable* b, 
				 int na, int nb, int* nc);

/**
 * Returns a mapping array which tells the location of subset variables in 
 * the (ordered) set. Free the result somewhere.
 * @param set Array of a superset of variables, ordered by id
 * @param subset Array of a smaller set of variables
 * @param nset Size of superset
 * @param nsubset Size of subset
 * @return New array of \p nsubset indices to \p set */
int* nip_mapper(nip_variable* set, nip_variable* subset, 
		int nset, int nsubset);

/**
 * Creates new lists of variables. 
 * @return empty list of variables */
nip_variable_list nip_new_variable_list();

/**
 * Creates new list of (variable, symbol) pairs
 * @return empty list of (variable, symbol) pairs */
nip_interface_list nip_new_interface_list();

/**
 * Adds a variable to the end of a list
 * @param l A list of variables to extend
 * @param v A variable to add
 * @return error code, or 0 on success */
int nip_append_variable(nip_variable_list l, nip_variable v);

/**
 * Adds a (variable, string) pair to the end of a list
 * @param l A list of (variable,string) pairs to extend
 * @param var A variable to add
 * @param next Symbol of the related variable in the next time slice
 * @return error code, or 0 on success */
int nip_append_interface(nip_interface_list l, nip_variable var, char* next);

/**
 * Adds a variable to the beginning of a list
 * @param l A list of variables to extend
 * @param v A variable to add
 * @return error code, or 0 on success */
int nip_prepend_variable(nip_variable_list l, nip_variable v);

/**
 * Adds a (variable, string) pair to the beginning of a list
 * @param l A list of (variable,string) pairs to extend
 * @param var A variable to add
 * @param next Symbol of the related variable in the next time slice
 * @return error code, or 0 on success */
int nip_prepend_interface(nip_interface_list l, nip_variable var, char* next);

/**
 * Turns a list of variables into a new array.
 * @param l List of variables
 * @return array of variables, free() when necessary */
nip_variable* nip_variable_list_to_array(nip_variable_list l);

/**
 * Makes the list of variables empty, 
 * without freeing the variables themselves.
 * @param l List of variables
 */
void nip_empty_variable_list(nip_variable_list l);

/**
 * Frees the list structure and the strings stored into it, 
 * but doesn't free the variables.
 * @param l List of (variable,string) pairs
 */
void nip_free_interface_list(nip_interface_list l);

/**
 * Function for iterating through a linked list of variables. 
 * @param it Reference to an iterator, which becomes updated
 * @return a reference to variable, or NULL */
nip_variable nip_next_variable(nip_variable_iterator* it);

/**
 * Function for searching a variable with certain symbol in a linked list.
 * @param l List of variables
 * @param symbol The unique symbol of the searched variable
 * @return variable reference or NULL if not found. */
nip_variable nip_search_variable_list(nip_variable_list l, char *symbol);

#endif /* __VARIABLE_H__ */
