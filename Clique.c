#include "Clique.h"
#include "Variable.h"
#include "potential.h"

/* Method for creating cliques (without pointers to sepsets) 
   - MVK: variables[] is an array of Variables (which are pointers) */
Clique make_Clique(Variable variables[], int num_of_vars){
  Clique c = (Clique) malloc(sizeof(cliquetype));
  int *cardinality = (int *) calloc(num_of_vars, sizeof(int));
  int i;
  for(i = 0; i < num_of_vars; i++){
    cardinality[i] = variables[i]->cardinality;
  }
  c->p = make_potential(cardinality, num_of_vars);
  free(cardinality); /* the array was copied ? */
  c->variables = variables; /* BUT THIS IS NOT copied */
  c->sepsets = 0;
  c->mark = 0;
  return c;
}


/* Method for removing cliques and freeing memory: returns an error code */
int free_Clique(Clique c){
  /* clean the list of sepsets */
  link l1 = c->sepsets;
  link l2 = c->sepsets;
  while(l1 != 0){
    l2 = l1->fwd;
    free(l1);
    l1=l2;
  }

  /* clean the rest */
  free_potential(c->p);
  free(c->variables); /* is this a good idea ? */
  free(c);
  return 0;
}


/* Method for adding a sepset next to a clique: returns an error code */
int add_Sepset(Clique c, Sepset s){
  link new = (link) malloc(sizeof(element));
  new->data = s;
  new->fwd = c->sepsets;
  new->bwd = 0;
  if(c->sepsets != 0)
    c->sepsets->bwd = new;
  c->sepsets = new;
  return 0;
}


/* Method for creating sepsets: 
   - cliques[] is an array which contains references to BOTH 
     neighbouring cliques */
Sepset make_Sepset(Variable variables[], int num_of_vars, Clique cliques[]){
  Sepset s = (Sepset) malloc(sizeof(sepsettype));
  int *cardinality = (int *) calloc(num_of_vars, sizeof(int));
  int i;
  for(i = 0; i < num_of_vars; i++){
    cardinality[i] = variables[i]->cardinality;
  }
  s->old = make_potential(cardinality, num_of_vars);
  s->new = make_potential(cardinality, num_of_vars);
  free(cardinality); /* the array was copied ? */
  s->variables = variables; /* <- */
  s->cliques = cliques; /*  <---- but these arrays are not copied !? */
  return s;
}


/* Method for removing sepsets and freeing memory: returns an error code */
int free_Sepset(Sepset s){
  free_potential(s->old);
  free_potential(s->new);
  free(s->variables);
  free(s->cliques);
  free(s);
  return 0;
}


/* Method for unmarking a clique: call this to every clique before 
   collecting or distributing evidence. Returns an error code */
int unmark_Clique(Clique c){
  c->mark = 0;
  return 0;
}


/* Call Distribute-Evidence for c. Returns an error code. */
int distribute_evidence(Clique c){
  /* mark */
  c->mark = 1;

  /* pass the messages */
  link l = c->sepsets;
  Sepset s;
  while (l != 0){
    s = l->data;
    if(s->cliques[0]->mark == 0)
      message_pass(c, s, s->cliques[0]); /* pass a message */
    else if(s->cliques[1]->mark == 0)
      message_pass(c, s, s->cliques[1]); /* pass a message */
    l = l->fwd;
  }

  /* call neighboring cliques */
  l = c->sepsets;
  while (l != 0){
    s = l->data;
    if(s->cliques[0]->mark == 0)
      distribute_evidence(s->cliques[0]);
    else if(s->cliques[1]->mark == 0)
      distribute_evidence(s->cliques[1]);
    l = l->fwd;
  }
  return 0;
}


/* Call Collect-Evidence from Clique c1 (or nullpointer) for Clique c2. 
   Sepset s12 is the sepset between c1 and c2 or nullpointer to get
   started. Returns an error code. */
int collect_evidence(Clique c1, Sepset s12, Clique c2){
  /* mark */
  c2->mark = 1;

  /* call neighboring cliques */
  link l = c2->sepsets;
  Sepset s;
  while (l != 0){
    s = l->data;
    if(s->cliques[0]->mark == 0)
      collect_evidence(c2, s, s->cliques[0]);
    else if(s->cliques[1]->mark == 0)
      collect_evidence(c2, s, s->cliques[1]);
    l = l->fwd;
  }

  /* pass the message to c1 */
  if((c1 != 0) && (s12 != 0))
    message_pass(c2, s12, c1);

  return 0;
}


/* Method for passing messages between cliques. Returns an error code. */
int message_pass(Clique c1, Sepset s, Clique c2){
  int i, j = 0, k = 0;
  int source_vars[c1->p->num_of_vars - s->new->num_of_vars];
  int extra_vars[c2->p->num_of_vars - s->new->num_of_vars];
  /* save the newer potential as old by switching the pointers */
  Sepset temp;
  temp = s->old;
  s->old = s->new;
  s->new = temp;

  /* marginalise (projection)
     first: select the variables */
  for(i=0; i < c1->p->num_of_vars; i++){
    if(j < s->new->num_of_vars &&
       equal_variables(*((c1->variables)[i]), *((s->variables)[j])))
      j++;
    else {
      source_vars[k] = i;
      k++;
    }
  } /* then: do da job */
  general_marginalise(c1->p, s->new, source_vars);

  j = 0; k = 0;
  /* update (absorption) 
     first: select the variables */
  for(i=0; i < c2->p->num_of_vars; i++){
    if(j < s->new->num_of_vars &&
       equal_variables(*((c2->variables)[i]), *((s->variables)[j])))
      j++;
    else {
      extra_vars[k] = i;
      k++;
    }
  } /* rest the case */
  update_potential(s->new, s->old, c2->p, extra_vars);
  return 0;
}

/* This one calculates the probability distribution for a variable v
   according to the clique c. To make sense, the join tree should be 
   made consistent before this. 
- The result is placed in v->likelihood
- The returned value is an error code. */
int marginalise(Clique c, Variable v){

  /* JJT: Is this the correct idea? */
  
  
  return 0;
}

/* TODO  Returns an error code. */
int insert_evidence(Clique c, double *data){
  /* copy the pointer or the data? */
  return 0;
}

