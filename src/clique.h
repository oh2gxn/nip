/*
 * clique.h $Id: clique.h,v 1.8 2007-03-13 16:48:03 jatoivol Exp $
 */

#ifndef __CLIQUE_H__
#define __CLIQUE_H__

#include "potential.h"
#include "variable.h"

struct sepsetlist {
  void *data; /* void is needed because of the order of definitions */
  struct sepsetlist *fwd;
  struct sepsetlist *bwd;
};

typedef struct sepsetlist element;
typedef element *sepset_link;

typedef struct {
  potential p;
  potential original_p;
  variable *variables; /* p contains num_of_vars */
  sepset_link sepsets;
  int num_of_sepsets;
  char mark; /* the way to prevent endless loops, either MARK_ON or MARK_OFF */
} cliquetype;
typedef cliquetype *clique;

typedef struct {
  potential old;   /* previous potential */
  potential new;   /* current potential */
  variable *variables; /* old and new contain num_of_vars */
  clique *cliques; /* always between two cliques */
} sepsettype;

typedef sepsettype *sepset;


/* List for storing parsed potentials while constructing the graph etc. */
struct potentialLinkType {
  potential data;
  variable child;
  variable* parents;
  struct potentialLinkType *fwd;
  struct potentialLinkType *bwd;
};

typedef struct potentialLinkType potentialLinkStruct;
typedef potentialLinkStruct *potentialLink;

struct potentialListType {
  int length;
  potentialLink first;
  potentialLink last;
};

typedef struct potentialListType potentialListStruct;
typedef potentialListStruct *potentialList;



/* Method for creating cliques (without pointers to sepsets).
 * vars[] can be in any order.
 */
clique make_clique(variable vars[], int num_of_vars);


/* Method for removing cliques and freeing memory. */
void free_clique(clique c);


/* Method for adding a sepset next to a clique: returns an error code */
int add_sepset(clique c, sepset s);


/* Method for creating sepsets: 
   - cliques[] is an array which contains references to BOTH 
     neighbouring cliques */
sepset make_sepset(variable variables[], int num_of_vars, clique cliques[]);


/* Method for removing sepsets and freeing memory. */
void free_sepset(sepset s);


/* Method for creating potentials with correct structure.
 * - variables[] is an array of the variables in a suitable order
 * - num_of_vars tells how many variables there are
 * - data[] is the data array in the order according to variables */
potential create_potential(variable variables[], int num_of_vars, 
			   double data[]);


/* Something very dangerous...
double *reorder_potential(variable vars[], potential p); 
*/

/* Method for unmarking a clique: call this to every clique before 
   collecting or distributing evidence. */
void unmark_clique(clique c);


/* Tells how many variables the clique contains. */
int clique_num_of_vars(clique c);


/* Tells how many variables the sepset contains. */
int sepset_num_of_vars(sepset s);


/*
 * Call Distribute-Evidence for c. Returns an error code.
 * Remember to UNMARK cliques before calling this.
 */
int distribute_evidence(clique c);


/*
 * Call Collect-Evidence from clique c1 (or nullpointer) for clique c2. 
 * sepset s12 is the sepset between c1 and c2 or nullpointer to get
 * started. Returns an error code.
 * Remember to UNMARK cliques before calling this.
 */
int collect_evidence(clique c1, sepset s12, clique c2);


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
potential gather_joint_probability(clique start, variable *vars, int n_vars,
				   variable *isect, int n_isect);


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
int initialise(clique c, variable child, potential p, int transient);


/*
 * This one calculates the probability distribution for a variable v
 * according to the clique c. To make sense, the join tree should be 
 * made consistent before this. 
 * - The result is placed in the array r
 * - The returned value is an error code.
 */
int marginalise(clique c, variable v, double r[]);


/* Method for backing away from impossibilities in observation. */
int global_retraction(variable* vars, int nvars, clique* cliques, 
		      int ncliques);

/* Computes the so called probability mass of a clique tree */
double probability_mass(clique* cliques, int ncliques);

/*
 * Function for entering an observation to a clique tree.
 * The observed state of the variable is given as a string.
 * Returns an error code.
 */
int enter_observation(variable* vars, int nvars, clique* cliques, 
		      int ncliques, variable v, char *state);


/*
 * Function for entering an observation to a clique tree.
 * The observed state of the variable is given as an index.
 * (See get_stateindex() at variable.h)
 * Returns an error code.
 */
int enter_i_observation(variable* vars, int nvars, clique* cliques, 
			int ncliques, variable v, int index);


/*
 * Function for entering evidence to a clique tree.
 * sizeof(evidence) must equal variable->cardinality.
 * This function might do a global retraction.
 * Returns an error code.
 */
int enter_evidence(variable* vars, int nvars, clique* cliques, 
		   int ncliques, variable v, double evidence[]);


/* Finds a clique containing the family of the given variable.
 * Returns NULL if no such clique is found.
 * Parameters:
 *  - cliques : an array of cliques
 *  - num_of_cliques : the size of the array 'cliques'
 *  - var : the variable whose family is to be found
 */
clique find_family(clique *cliques, int num_of_cliques, variable var);


/* Computes or finds a mapping from the family members to the clique 
 * variables, so that you can use general_marginalise to compute 
 * P(child | pa(child)) from the family clique. */
int* find_family_mapping(clique family, variable child);


/* Finds a clique containing the specified set of variables. 
 * Returns NULL if no such clique is found. Note that this is usually
 * more expensive than find_family().
 * Parameters:
 *  - cliques : an array of cliques
 *  - num_of_cliques : the size of the array 'cliques'
 *  - variables : the variables expected to be included in the clique
 *  - num_of_vars : size of the given array of variables
 */
clique find_clique(clique *cliques, int num_of_cliques, 
		   variable *variables, int num_of_vars);


/*
 * Constructs sepsets and inserts them between the cliques to form a
 * join tree.
 * Returns an error code.
 * Parameters:
 *  - cliques : an array of cliques
 *  - num_of_cliques : the number of cliques in the given array
 */
int find_sepsets(clique *cliques, int num_of_cliques);


/*
 * Prints the variables of the given clique.
 */
void print_clique(clique c);


/*
 * Prints all the cliques in the join tree.
 */
void print_cliques();


/*
 * Prints the variables of the given sepset.
 */
void print_sepset(sepset s);


/*
 * Finds the intersection of two cliques. Creates an array of those
 * variables that can be found in both cliques. Remember to free the
 * memory when the array is not needed anymore. n is the size of the
 * intersection.
 * Returns an error code.
 */
int clique_intersection(clique cl1, clique cl2, variable **vars, int *n);


/* Creates an empty list for storing potentials and corresponding variables 
 */
potentialList make_potentialList();


/* Adds potentials to the list */
int append_potential(potentialList l, potential p, 
		     variable child, variable* parents);
int prepend_potential(potentialList l, potential p, 
		      variable child, variable* parents);


/* Frees the memory allocated to a potentialList.
 * NOTE: this frees also the actual potentials and parent variable arrays! 
 * (variables themselves are kept intact) */
void free_potentialList(potentialList l);



#endif /* __CLIQUE_H__ */
