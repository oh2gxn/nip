/*
 * nip.h $Id: nip.h,v 1.2 2004-08-09 15:02:24 jatoivol Exp $
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
  varlink first_var;
  varlink last_var;
}nip;


/* Makes the model forget all the given evidence. */
void reset_model(nip model);


/* Creates a model according to the net file. 
 * The single parameter is the name of the net file as a string. */
nip* parse_model(char* file);


/* This provides a way to get rid of a model and free some memory. */
void free_model(nip model);


/* This is a function for telling the model about observations. 
 * In case of an error, a non-zero value is returned. */
int insert_hard_evidence(nip model, char* variable, char* observation);


/* If an observation has some uncertainty, the evidence can be inserted 
 * with this procedure. The returned value is 0 if everything went well. */
int insert_soft_evidence(nip model, char* variable, double* distribution);

Variable get_Variable(nip model, char* symbol);

void make_consistent(nip model);

/********************************************************************
 * TODO: a set of functions for reading the results of inference from 
 *       the model 
 */


#endif /* __NIP_H__ */
