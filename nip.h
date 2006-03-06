/*
 * nip.h $Id: nip.h,v 1.43 2006-03-06 17:20:10 jatoivol Exp $
 */

#ifndef __NIP_H__
#define __NIP_H__

#include "clique.h"
#include "variable.h"
#include "errorhandler.h"
#include <stdlib.h>

#define FORWARD  1
#define BACKWARD 0
#define HAD_A_PREVIOUS_TIMESLICE 1

/* "How probable is the impossible" (0 < epsilon << 1) */
#define EPSILON 0.00001

typedef struct{
  int num_of_cliques;
  clique *cliques;
  int num_of_vars;
  variable *variables;
  variable *next;     /* An array of the variables that will substitute 
		       * another one in the next timeslice. */

  variable *previous; /* An array of the variables that are substituted 
		       * by some variables from the previous timeslice. 
		       * Waste of memory? */

  int num_of_nexts;   /* Number of variables in the 'next' and 'previous' 
		       * arrays*/

  /* Undocumented features: */
  int outgoing_interface_size;
  variable *outgoing_interface;
  int incoming_interface_size;
  variable *incoming_interface;

  clique in_clique;  /* Reference to the clique which receives
		      * the message from the past timeslices */

  clique out_clique; /* The clique which handles the connection to the 
		      * timeslices in the future */

  variable *children;    /* All the variables that have parents */
  variable *independent; /* ...and those who dont. (Redundant?)  */
  int num_of_children;

  int node_size_x;
  int node_size_y;
}nip_type;

typedef nip_type *nip;


typedef struct{
  nip model;          /* The model (contains the variables and state names) */
  variable *hidden;   /* An array containing the latent variables */
  int num_of_hidden;  /* Number of latent variables */
  variable *observed; /* An array containing the observed variables ??? */

  int length;
  int **data;         /* The time series */
  /* JJ NOTE: Should there be a cache for extremely large time series? */
}time_series_type;

typedef time_series_type *time_series;


typedef struct{
  variable *variables; /* variables of interest */
  int num_of_vars;
  int length;     /* length of the time series */
  double ***data; /* probability distribution of every variable at every t */
}uncertain_series_type;

typedef uncertain_series_type *uncertain_series;


/* Makes the model forget all the given evidence.
 * NOTE: also the priors specified in the NET file are cleared
 * (but remain intact in the variables) so you'll have to re-enter them
 * as soft evidence. */
void reset_model(nip model);


/* Makes all the conditional probabilities uniform and forgets all evidence.
 * In other words, the model will be as if it was never initialised with 
 * any parameters at all. 
 * (All the variables and the join tree will be there, of course) */
void total_reset(nip model);


/* Enters the priors of independent variables into the model as evidence.
 * This has to be done after reset_model and parse_model if you have any
 * other kind of priors than uniform ones. If you need to suppress the 
 * initialisation of the variables representing another one from the 
 * previous timeslice, use 'has_history == 0'. 
 * (Usually has_history == 1 only for the first timeslice)
 */
void use_priors(nip model, int has_history);


/* Creates a model according to the net file. 
 * The single parameter is the name of the net file as a string. */
nip parse_model(char* file);


/* Writes the parameters of <model> into Hugin NET file named <name>.net */
int write_model(nip model, char* name);


/* This provides a way to get rid of a model and free some memory. */
void free_model(nip model);


/* This reads data from the data file and constructs a time series according 
 * to the given model. */
int read_timeseries(nip model, char* datafile, 
		    time_series **results);


/* This writes the time series data into a file. */
int write_timeseries(time_series ts, char* filename);


/* A method for freeing the huge chunk of memory used by a time series. 
 * Note that this does not free the model. */
void free_timeseries(time_series ts);


/* Tells the length of the time series. */
int timeseries_length(time_series ts);


/* Writes the inferred probabilities of given variable into a file. */
int write_uncertainseries(uncertain_series ucs, variable v, char* filename);


/* A method for freeing the memory used by an uncertain time series. */
void free_uncertainseries(uncertain_series ucs);


/* Tells the length of the uncertain time series. */
int uncertainseries_length(uncertain_series ucs);


/* A method for reading an observation from the time series. 
 * You'll have to specify the variable. DO NOT alter the string returned 
 * by the function! The returned value may be NULL if the variable was not 
 * observed at the specified moment in time. The time span is [0, T-1] */
char* get_observation(time_series ts, variable v, int time);


/* A method for setting an observation in the time series. */
int set_observation(time_series ts, variable v, int time, char* observation);


/* This is a function for telling the model about observations. 
 * In case of an error, a non-zero value is returned. */
int insert_hard_evidence(nip model, char* varname, char* observation);


/* If an observation has some uncertainty, the evidence can be inserted 
 * with this procedure. The returned value is 0 if everything went well. */
int insert_soft_evidence(nip model, char* varname, double* distribution);


/* Method for inserting all the evidence at a specified step <t> in 
 * a time series <ts> into the (time slice) model <ts->model>. */
int insert_ts_step(time_series ts, int t, nip model);


/* Method for inserting all the evidence at a specified step <t> in 
 * an uncertain time series <ucs> into the model <ucs->model>. */
int insert_ucs_step(uncertain_series ucs, int t, nip model);


/* This algorithm computes the probability distributions for every 
 * hidden variable and for every time step according to the timeseries.
 * It uses only forward propagation, so the result of time step t 
 * is not affected by the rest of the timeseries. 
 *  You'll have to specify the variables of interest in the vars array 
 * and the number of the variables. */
uncertain_series forward_inference(time_series ts, variable vars[], int nvars);


/* This one computes the probability distributions for every 
 * hidden variable and for every time step according to the timeseries.
 * It uses both forward and backward propagation, so the result of time 
 * step t is affected by the whole timeseries. 
 *  You'll have to specify the variables of interest in the vars array 
 * and the number of the variables. */
uncertain_series forward_backward_inference(time_series ts, 
					   variable vars[], int nvars);


/* Fetches you the variable with a given name. */
variable model_variable(nip model, char* symbol);


/* Makes the join tree consistent i.e. does the inference on a single 
 * timeslice (useful after inserting evidence). */
void make_consistent(nip model);


/* Computes the most likely state sequence of the variables, given the time
 * series. In other words, this function implements the idea also known as
 * the Viterbi algorithm. (The model is included in the time_series.)  
 * NOTE: this is not implemented yet! */
time_series mlss(variable vars[], int nvars, time_series ts);


/* Teaches the given model according to the given time series with 
 * EM-algorithm. Returns an error code as an integer. 
 * NOTE:  this is not implemented yet! 
 * NOTE2: the model is included in the time_series */
int em_learn(time_series *ts, int n_ts, double threshold);


/* Tells the likelihood of observations (not normalised). 
 * You must normalise the result with the mass computed before 
 * the evidence was put in. */
double model_prob_mass(nip model);


/*
 * Calculates the probability distribution of a variable.
 * The join tree MUST be consistent before calling this.
 * Parameters:
 * - model: the model that contains the variable
 * - v: the variable whose distribution we want
 * Returns an array of doubles (remember to free the result when not needed).
 * The returned array is of size v->cardinality.
 * In case of problems, NULL is returned.
 */
double *get_probability(nip model, variable v);


/*
 * Calculates the joint probability distribution of a set of variables.
 * The join tree MUST be consistent before calling this.
 * Parameters:
 * - model: the model that contains the variables
 * - vars: the variables whose distribution we want
 * - num_of_vars: the number of variables (size of "vars")
 * Returns the result as a potential 
 * (remember to free the result when not needed).
 * The variables of the potential are in the same order as they were given.
 * In case of problems, NULL is returned.
 */
potential get_joint_probability(nip model, variable *vars, int num_of_vars);


/* Generates time series data according to a model. */
time_series generate_data(nip model, int length);


/* Sets the seed number for drand48 & co. */
void random_seed();

/* For generating single observations. */
int lottery(double* distribution, int size);


/*
 * Prints the cliques in the given nip model.
 */
void print_cliques(nip model);

#endif /* __NIP_H__ */
