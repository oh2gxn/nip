/*
 * Clique.h $Id: Clique.h,v 1.41 2004-07-09 11:44:17 jatoivol Exp $
 */

#ifndef __CLIQUE_H__
#define __CLIQUE_H__

#include "potential.h"
#include "Variable.h"

struct sepsetlist {
  void *data; /* void is needed because of the order of definitions */
  struct sepsetlist *fwd;
  struct sepsetlist *bwd;
};

typedef struct sepsetlist element;
typedef element *link;

typedef struct {
  potential p;
  potential original_p;
  Variable *variables; /* p contains num_of_vars */
  link sepsets;
  int num_of_sepsets;
  int mark; /* the way to prevent endless loops */
} cliquetype;
typedef cliquetype *Clique;

typedef struct {
  potential old;   /* previous potential */
  potential new;   /* current potential */
  Variable *variables; /* p contains num_of_vars */
  Clique *cliques; /* always between two cliques */
} sepsettype;

typedef sepsettype *Sepset;

/* Method for creating cliques (without pointers to sepsets).
 * vars[] can be in any order.
 */
Clique make_Clique(Variable vars[], int num_of_vars);


/* Method for removing cliques and freeing memory: returns an error code */
int free_Clique(Clique c);


/* Method for adding a sepset next to a clique: returns an error code */
int add_Sepset(Clique c, Sepset s);


/* Method for creating sepsets: 
   - cliques[] is an array which contains references to BOTH 
     neighbouring cliques */
Sepset make_Sepset(Variable variables[], int num_of_vars, Clique cliques[]);


/* Method for removing sepsets and freeing memory: returns an error code */
int free_Sepset(Sepset s);


/* Method for creating potentials with correct structure.
 * - variables[] is an array of the Variables in a suitable order
 * - num_of_vars tells how many variables there are
 * - data[] is the data array in the order according to variables */
potential create_Potential(Variable variables[], int num_of_vars, 
			   double data[]);


/* Method for cleaning potentials and releasing the memory. 
 * Returns an error code. */
int free_Potential(potential p);


/* Tells how many variables the clique contains. */
int clique_num_of_vars(Clique c);


/* Tells how many variables the sepset contains. */
int sepset_num_of_vars(Sepset s);


/* Returns the i:th Variable in a Clique. */
Variable clique_get_Variable(Clique c, int i);


/*
 * Call Distribute-Evidence for the join tree. Returns an error code.
 */
int distribute_evidence();


/*
 * Call Collect-Evidence for the join tree. Returns an error code.
 */
int collect_evidence();


/* Make up a better name for this */
/*
 * !!! p->num_of_vars equals "length of parents + 1" !!! 
 * Sum of the elements in the potential is assumed to be 1. 
 * The "ownership" of the potential changes.
 */
int initialise(Clique c, Variable child, Variable parents[], potential p);


/*
 * This one calculates the probability distribution for a variable v
 * according to the clique c. To make sense, the join tree should be 
 * made consistent before this. 
 * - The result is placed in the array r
 * - The returned value is an error code.
 */
int marginalise(Clique c, Variable v, double r[]);


/*
 * Normalises the array. Divides every member by their sum.
 * The function modifies the given array.
 */
int normalise(double result[], int array_size);


/* Method for backing away from impossibilities in observation. */
int global_retraction(Clique c);


/*
 * Function for entering an observation to a clique tree.
 * The observed state of the variable is given as a string.
 * Returns an error code.
 */
int enter_observation(Variable v, char *state);


/*
 * Function for entering an observation to a clique tree.
 * The observed state of the variable is given as an index.
 * (See get_stateindex() at Variable.h)
 * Returns an error code.
 */
int enter_i_observation(Variable v, int index);


/*
 * Function for entering evidence to a clique tree.
 * sizeof(evidence) must equal variable->cardinality.
 * This function might do a global retraction.
 * Returns an error code.
 */
int enter_evidence(Variable v, double evidence[]);


/* Finds a clique containing a family of variables. Returns the first
 * found Clique that contains all the given variables.
 * Returns NULL if no such Clique is found.
 * Parameters:
 *  - cliques : an array of Cliques
 *  - num_of_cliques : the size of the array 'cliques'
 *  - variables : an array containing the family of variables
 *  - num_of_vars : the size of the array 'variables'
 */
Clique find_family(Clique *cliques, int num_of_cliques,
		   Variable *variables, int num_of_vars);


/*
 * Constructs Sepsets and inserts them between the Cliques to form a
 * join tree.
 * Returns an error code.
 * Parameters:
 *  - cliques : an array of Cliques
 *  - num_of_cliques : the number of Cliques in the given array
 */
int find_sepsets(Clique *cliques, int num_of_cliques);


/*
 * Prints the variables of the given Clique.
 */
void print_Clique(Clique c);


/*
 * Prints all the Cliques in the join tree.
 */
void print_Cliques();


/*
 * Prints the variables of the given Sepset.
 */
void print_Sepset(Sepset s);


/*
 * Finds the intersection of two Cliques. Creates an array of those
 * Variables that can be found in both Cliques. Remember to free the
 * memory when the array is not needed anymore. n is the size of the
 * intersection.
 * Returns an error code.
 */
int clique_intersection(Clique cl1, Clique cl2, Variable **vars, int *n);

int get_num_of_cliques();

Clique **get_cliques_pointer();

void set_num_of_cliques(int n);

#endif /* __CLIQUE_H__ */
