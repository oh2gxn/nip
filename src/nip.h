/*
 * nip.h $Id: nip.h,v 1.24 2005-04-09 01:28:45 jatoivol Exp $
 */

#ifndef __NIP_H__
#define __NIP_H__

#include "Clique.h"
#include "Variable.h"
#include "errorhandler.h"
#include <stdlib.h>

# define FORWARD  1
# define BACKWARD 0

typedef struct{
  int num_of_cliques;
  int num_of_vars;
  Clique *cliques;
  Variable *variables;
  Variable *next;     /* An array of the variables that will substitute 
		       * another one in the next timeslice. */

  Variable *previous; /* An array of the variables that are substituted 
		       * by some variables from the previous timeslice. 
		       * Waste of memory? */

  int num_of_nexts;   /* Number of variables in the 'next' and 'previous' 
		       * arrays*/

  /* Undocumented features: */
  Variable *children;    /* All the variables which have parents */
  Variable *independent; /* ...and those who dont. (Redundant?)  */
  int num_of_children;

  Clique front_clique; /* A memoization reference to the clique which mediates
			* the message to the timeslices in the future */

  Clique tail_clique;  /* The clique which handles the connection to the 
			* timeslices in the past */
}nip_type;

typedef nip_type *Nip;


typedef struct{
  Nip model;          /* The model (contains the variables and state names) */
  Variable *hidden;   /* An array containing the latent variables */
  int num_of_hidden;  /* Number of latent variables */
  Variable *observed; /* An array containing the observed variables ??? */

  int length;
  int **data;         /* The time series */
  /* JJ NOTE: Should there be a cache for extremely large time series? */
}time_series_type;

typedef time_series_type *TimeSeries;


typedef struct{
  Variable *variables; /* variables of interest */
  int num_of_vars;
  int length;     /* length of the time series */
  double ***data; /* probability distribution of every variable at every t */
}uncertain_series_type;

typedef uncertain_series_type *UncertainSeries;


/* Makes the model forget all the given evidence.
 * NOTE: also the priors specified in the NET file are cleared
 * (but remain intact in the Variables) so you'll have to re-enter them
 * as soft evidence. */
void reset_model(Nip model);


/* Creates a model according to the net file. 
 * The single parameter is the name of the net file as a string. */
Nip parse_model(char* file);


/* This provides a way to get rid of a model and free some memory. */
void free_model(Nip model);


/* This reads data from the data file and constructs a time series according 
 * to the given model. */
TimeSeries read_timeseries(Nip model, char* datafile);


/* A method for freeing the huge chunk of memory used by a time series. 
 * Note that this does not free the model. */
void free_timeseries(TimeSeries ts);


/* Tells the length of the time series. */
int timeseries_length(TimeSeries ts);


/* A method for freeing the memory used by an uncertain time series. */
void free_uncertainseries(UncertainSeries ucs);


/* Tells the length of the uncertain time series. */
int uncertainseries_length(UncertainSeries ucs);


/* A method for reading an observation from the time series. 
 * You'll have to specify the variable. DO NOT alter the string returned 
 * by the function! The returned value may be NULL if the variable was not 
 * observed at the specified moment in time. The time span is [0, T-1] */
char* get_observation(TimeSeries ts, Variable v, int time);


/* A method for setting an observation in the time series. */
int set_observation(TimeSeries ts, Variable v, int time, char* observation);


/* This algorithm computes the probability distributions for every 
 * hidden variable and for every time step according to the timeseries.
 * It uses only forward propagation, so the result of time step t 
 * is not affected by the rest of the timeseries. 
 *  You'll have to specify the variables of interest in the vars array 
 * and the number of the variables. */
UncertainSeries forward_inference(TimeSeries ts, Variable vars[], int nvars);


/* This one computes the probability distributions for every 
 * hidden variable and for every time step according to the timeseries.
 * It uses both forward and backward propagation, so the result of time 
 * step t is affected by the whole timeseries. 
 *  You'll have to specify the variables of interest in the vars array 
 * and the number of the variables. */
UncertainSeries forward_backward_inference(TimeSeries ts, 
					   Variable vars[], int nvars);


/* This is a function for telling the model about observations. 
 * In case of an error, a non-zero value is returned. */
int insert_hard_evidence(Nip model, char* variable, char* observation);


/* If an observation has some uncertainty, the evidence can be inserted 
 * with this procedure. The returned value is 0 if everything went well. */
int insert_soft_evidence(Nip model, char* variable, double* distribution);


Variable get_Variable(Nip model, char* symbol);


void make_consistent(Nip model);


/* Computes the most likely state sequence of the variables, given the time
 * series. In other words, this function implements the idea also known as
 * the Viterbi algorithm. (The model is included in the TimeSeries.)  
 * NOTE: this is not implemented yet! */
TimeSeries mlss(Variable vars[], int nvars, TimeSeries ts);


/* Teaches the given model according to the given time series with 
 * EM-algorithm. Returns an error code as an integer. 
 * NOTE:  this is not implemented yet! 
 * NOTE2: the model is included in the TimeSeries */
int em_learn(TimeSeries ts);


/********************************************************************
 * TODO: a set of functions for reading the results of inference from 
 *       the model 
 */

/*
 * Calculates the probability distribution of a Variable.
 * The join tree MUST be consistent before calling this.
 * Parameters:
 * - model: the model that contains the Variable
 * - v: the Variable whose distribution we want
 * Returns an array of doubles (remember to free the result when not needed).
 * The returned array is of size v->cardinality.
 * In case of problems, NULL is returned.
 */
double *get_probability(Nip model, Variable v);


/*
 * Calculates the joint probability distribution of a set of Variables.
 * The Variables MUST be in the SAME CLIQUE.
 * The join tree MUST be consistent before calling this.
 * Parameters:
 * - model: the model that contains the Variable
 * - vars: the Variables whose distribution we want
 * - num_of_vars: the number of Variables (size of "vars")
 * Returns an array of doubles (remember to free the result when not needed).
 * The returned array is of size
 *   vars[0]->cardinality * ... * vars[num_of_vars - 1]->cardinality.
 * The array can be addressed with multiple indices,
 *   i.e. array[i0][i1][i2], where the indices are in the same order as
 *   the array "vars".
 * In case of problems, NULL is returned.
 */
double *get_joint_probability(Nip model, Variable *vars, int num_of_vars);


/*
 * Prints the Cliques in the given Nip model.
 */
void print_Cliques(Nip model);

#endif /* __NIP_H__ */
