/*
 * nip.h $Id: nip.h,v 1.11 2004-10-28 09:11:34 jatoivol Exp $
 */

#ifndef __NIP_H__
#define __NIP_H__

#include "Clique.h"
#include "Variable.h"
#include "errorhandler.h"
#include <stdlib.h>

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

  /* NOTE: Should there be distinct references to the cliques containing 
   * 'nexts' and 'previous' variables? */
}nip_type;

typedef nip_type *Nip;


typedef struct{
  Nip model;          /* The model (contains the variables and state names) */

  Variable *hidden;   /* An array containing the latent variables */

  int num_of_hidden;  /* Number of latent variables */

  Variable *observed; /* An array containing the observed variables ??? */

  int length;
  int **data;         /* The time series */
  /* JJ NOTES: Should there be a cache for extremely large time series? */
}timeseries_type;

typedef timeseries_type *Timeseries;


/* Makes the model forget all the given evidence. */
void reset_model(Nip model);


/* Creates a model according to the net file. 
 * The single parameter is the name of the net file as a string. */
Nip parse_model(char* file);


/* This provides a way to get rid of a model and free some memory. */
void free_model(Nip model);


/* This reads data from the data file and constructs a time series according 
 * to the given model. */
Timeseries read_timeseries(Nip model, char* datafile);


/* A method for freeing the huge chunk of memory used by a time series. */
void free_timeseries(Timeseries ts);


/* Tells the length of the timeseries. */
int timeseries_length(Timeseries ts);


/* A method for reading an observation from the time series. 
 * You'll have to specify the variable. Do not alter the string returned 
 * by the function! The returned value may be NULL if the variable was not 
 * observed at the specified moment in time. The time span is [0, T-1] */
char* get_observation(Timeseries ts, Variable v, int time);


/* A method for setting an observation in the time series. */
int set_observation(Timeseries ts, Variable v, int time, char* observation);


/* This algorithm computes the probability distributions for every 
 * hidden variable and for every time step according to the timeseries.
 * It uses only forward propagation, so the result of time step t 
 * is not affected by the rest of the timeseries. */
SomeDataType forward_inference(Nip model, Timeseries ts);


/*  */
SomeDataType forward_backward_inference(Nip model, Timeseries ts);


/* This is a function for telling the model about observations. 
 * In case of an error, a non-zero value is returned. */
int insert_hard_evidence(Nip model, char* variable, char* observation);


/* If an observation has some uncertainty, the evidence can be inserted 
 * with this procedure. The returned value is 0 if everything went well. */
int insert_soft_evidence(Nip model, char* variable, double* distribution);


Variable get_Variable(Nip model, char* symbol);


void make_consistent(Nip model);





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
 * - print: zero if we don't want the result printed. Non-zero is the opposite.
 * Returns an array of doubles (remember to free the result when not needed).
 * The returned array is of size v->cardinality.
 * In case of problems, NULL is returned.
 */
double *get_probability(Nip model, Variable v, int print);


/*
 * Calculates the joint probability distribution of a set of Variables.
 * The Variables MUST be in the SAME CLIQUE.
 * The join tree MUST be consistent before calling this.
 * Parameters:
 * - model: the model that contains the Variable
 * - vars: the Variables whose distribution we want
 * - num_of_vars: the number of Variables (size of "vars")
 * - print: zero if we don't want the result printed. Non-zero is the opposite.
 * Returns an array of doubles (remember to free the result when not needed).
 * The returned array is of size
 *   vars[0]->cardinality * ... * vars[num_of_vars - 1]->cardinality.
 * The array can be addressed with multiple indices,
 *   i.e. array[i0][i1][i2], where the indices are in the same order as
 *   the array "vars".
 * In case of problems, NULL is returned.
 */
double *get_joint_probability(Nip model, Variable *vars, int num_of_vars,
			      int print);


/*
 * Prints the Cliques in the given Nip model.
 */
void print_Cliques(Nip model);

#endif /* __NIP_H__ */
