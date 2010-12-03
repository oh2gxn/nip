/* nipjointree.h
 * Functions for handling cliques and sepsets.
 * Includes evidence handling and propagation of information
 * in the join tree. 
 * Authors: Janne Toivola, Mikko Korpela
 * Version: $Id: nipjointree.h,v 1.3 2010-12-03 17:21:28 jatoivol Exp $
 */

#ifndef __NIPJOINTREE_H__
#define __NIPJOINTREE_H__

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "nipvariable.h"
#include "nippotential.h"
#include "niperrorhandler.h"


typedef struct nip_sepsetlink {
  void* data; /* void is needed because of the order of definitions */
  struct nip_sepsetlink *fwd;
  struct nip_sepsetlink *bwd;
} nip_sepsetlink_struct;
typedef nip_sepsetlink_struct* nip_sepset_link;

typedef struct {
  nip_potential p;
  nip_potential original_p;
  nip_variable* variables; /* array size == p->num_of_vars */
  nip_sepset_link sepsets;
  int num_of_sepsets;
  char mark; /* the way to prevent endless loops, either MARK_ON or MARK_OFF */
} nip_clique_struct;
typedef nip_clique_struct* nip_clique;

typedef struct {
  nip_potential old;   /* previous potential */
  nip_potential new;   /* current potential */
  nip_variable* variables; /* size == old->num_of_vars */
  nip_clique *cliques; /* always an array of two (neighbor) cliques */
} nip_sepset_struct;

typedef nip_sepset_struct* nip_sepset;


/* List for storing parsed potentials while constructing the graph etc. 
 * (when the cliques don't exist yet) */
typedef struct nip_potentiallink {
  nip_potential data;
  nip_variable child;
  nip_variable* parents;
  struct nip_potentiallink* fwd;
  struct nip_potentiallink* bwd;
} nip_potential_link_struct;
typedef nip_potential_link_struct* nip_potential_link;

typedef struct {
  int length;
  nip_potential_link first;
  nip_potential_link last;
} nip_potential_list_struct;
typedef nip_potential_list_struct* nip_potential_list;



/* Method for creating cliques (without pointers to sepsets).
 * vars[] can be in any order.
 */
nip_clique nip_new_clique(nip_variable vars[], int num_of_vars);


/* Method for removing cliques and freeing memory. */
void nip_free_clique(nip_clique c);


/* Method for adding a sepset next to a clique: returns an error code */
int nip_add_sepset(nip_clique c, nip_sepset s);


/* Method for creating sepsets: 
   - cliques[] is an array which contains references to BOTH 
     neighbouring cliques */
nip_sepset nip_new_sepset(nip_variable variables[], int num_of_vars, 
			  nip_clique cliques[]);


/* Method for removing sepsets and freeing memory. */
void nip_free_sepset(nip_sepset s);


/* Method for creating potentials with correct structure.
 * - variables[] is an array of the variables in a suitable order
 * - num_of_vars tells how many variables there are
 * - data[] is the data array in the order according to variables */
nip_potential nip_create_potential(nip_variable variables[], 
				   int num_of_vars, 
				   double data[]);


/* Something very dangerous...
double* reorder_potential(nip_variable vars[], nip_potential p); 
*/

/* Method for unmarking a clique: call this to every clique before 
   collecting or distributing evidence. */
void nip_unmark_clique(nip_clique c);


/* Tells how many variables the clique contains. */
int nip_clique_size(nip_clique c);


/* Tells how many variables the sepset contains. */
int nip_sepset_size(nip_sepset s);


/*
 * Call Distribute-Evidence for c. Returns an error code.
 * Remember to UNMARK cliques before calling this.
 */
int nip_distribute_evidence(nip_clique c);


/* Tells if the cliques are connected (in the same join tree).
 * Remember to UNMARK cliques before calling this. 
 */
int nip_cliques_connected(nip_clique one, nip_clique two);


/*
 * Call Collect-Evidence from clique c1 (or nullpointer) for clique c2. 
 * sepset s12 is the sepset between c1 and c2 or nullpointer to get
 * started. Returns an error code.
 * Remember to UNMARK cliques before calling this.
 */
int nip_collect_evidence(nip_clique c1, nip_sepset s12, nip_clique c2);


/*
 * Method for finding out the joint probability distribution of arbitrary
 * variables by making a DFS in the join tree. <start> is the starting 
 * point for the recursion. The variable array <vars> specifies the 
 * variables of interest. The array also specifies the order of variables in 
 * the resulting potential and the size of the array must be specified as 
 * the third parameter <n_vars>. <isect> and <n_isect> are for the recursion 
 * and should be NULL and 0 respectively when used from other functions.
 * (<isect> MAY NOT contain any of the variables in <vars>)
 * NOTE: Remember to unmark all cliques before calling this.
 */
nip_potential nip_gather_joint_probability(nip_clique start, 
					   nip_variable *vars, 
					   int n_vars,
					   nip_variable *isect, 
					   int n_isect);


/*
 * Initialises the clique c with the potential p, which describes the 
 * conditional probability of the child variable given its parents. 
 * This is basically a potential multiplication so that the clique keeps 
 * the parameters even when retraction is used. If transient==1, the 
 * potential can be "wiped" with retraction...
 * !!! p->num_of_vars equals "number of parents + 1" !!! 
 * Sum of the elements in the potential is assumed to be 1. 
 * The "ownership" of the potential changes.
 */
int nip_init_clique(nip_clique c, nip_variable child, 
		    nip_potential p, int transient);


/*
 * This one calculates the probability distribution for a variable v
 * according to the clique c. To make sense, the join tree should be 
 * made consistent before this. 
 * - The result is placed in the array r
 * - The returned value is an error code.
 */
int nip_marginalise_clique(nip_clique c, nip_variable v, double r[]);


/* Method for backing away from impossibilities in observation. */
int nip_global_retraction(nip_variable* vars, int nvars, 
			  nip_clique* cliques, int ncliques);

/* Computes the so called probability mass of a clique tree */
double nip_probability_mass(nip_clique* cliques, int ncliques);

/*
 * Function for entering an observation to a clique tree.
 * The observed state of the variable is given as a string.
 * Returns an error code.
 */
int nip_enter_observation(nip_variable* vars, int nvars, 
			  nip_clique* cliques, int ncliques, 
			  nip_variable v, char *state);


/*
 * Function for entering an observation to a clique tree.
 * The observed state of the variable is given as an index.
 * (See get_stateindex() at variable.h)
 * Returns an error code.
 */
int nip_enter_index_observation(nip_variable* vars, int nvars, 
				nip_clique* cliques, int ncliques, 
				nip_variable v, int index);


/* Function for entering evidence to a clique tree. 
 * Resetting the model or entering some new evidence about the 
 * variable cancels the effects of this operation.
 * Size of the evidence array must equal v->cardinality.
 * This function might do a global retraction.
 * Returns an error code.
 */
int nip_enter_evidence(nip_variable* vars, int nvars, 
		       nip_clique* cliques, int ncliques, 
		       nip_variable v, double evidence[]);


/* Function for entering a prior to a clique tree. 
 * Resetting the cliques cancels the effect of this operation, 
 * but entering evidence about the variable does not.
 * Size of the prior array must equal v->cardinality.
 * Returns an error code.
 */
int nip_enter_prior(nip_variable* vars, int nvars, 
		    nip_clique* cliques, int ncliques, 
		    nip_variable v, double prior[]);


/* Finds a clique containing the family of the given variable.
 * Returns NULL if no such clique is found.
 * Parameters:
 *  - cliques : an array of cliques
 *  - num_of_cliques : the size of the array 'cliques'
 *  - var : the variable whose family is to be found
 */
nip_clique nip_find_family(nip_clique* cliques, int ncliques, 
			   nip_variable var);


/* Computes or finds a mapping from the family members to the clique 
 * variables, so that you can use general_marginalise to compute 
 * P(child | pa(child)) from the family clique. */
int* nip_find_family_mapping(nip_clique family, nip_variable child);


/* Finds a clique containing the specified set of variables. 
 * Returns NULL if no such clique is found. Note that this is usually
 * more expensive than find_family().
 * Parameters:
 *  - cliques : an array of cliques
 *  - num_of_cliques : the size of the array 'cliques'
 *  - variables : the variables expected to be included in the clique
 *  - num_of_vars : size of the given array of variables
 */
nip_clique nip_find_clique(nip_clique* cliques, int ncliques, 
			   nip_variable* variables, int nvars);


/*
 * Prints the variables of the given clique.
 */
void nip_fprintf_clique(FILE* stream, nip_clique c);


/*
 * Prints all the cliques in the join tree.
 */
void nip_fprintf_cliques(FILE* stream);


/*
 * Prints the variables of the given sepset.
 */
void nip_fprintf_sepset(FILE* stream, nip_sepset s);


/*
 * Finds the intersection of two cliques. Creates an array of those
 * variables that can be found in both cliques. Remember to free the
 * memory when the array is not needed anymore. n is the size of the
 * intersection.
 * Returns an error code.
 */
int nip_clique_intersection(nip_clique cl1, nip_clique cl2, 
			    nip_variable **vars, int *n);


/* Creates an empty list for storing potentials and corresponding variables 
 */
nip_potential_list nip_new_potential_list();


/* Adds potentials to the list */
int nip_append_potential(nip_potential_list l, nip_potential p, 
			 nip_variable child, nip_variable* parents);
int nip_prepend_potential(nip_potential_list l, nip_potential p, 
			  nip_variable child, nip_variable* parents);


/* Frees the memory allocated to a potentialList.
 * NOTE: this frees also the actual potentials and parent variable arrays! 
 * (variables themselves are kept intact) */
void nip_free_potential_list(nip_potential_list l);



#endif /* __NIPJOINTREE_H__ */
