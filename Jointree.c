/*
 * Jointree.c $Id: Jointree.c,v 1.2 2004-02-19 13:18:20 mvkorpel Exp $
 * Saapa n‰hd‰, mik‰ Jointreen rooli tulee olemaan, jos mik‰‰n.
 */

#include <stdlib.h>
#include "Jointree.h"
#include "Clique.h"
#include "errorhandler.h"

Jointree make_Jointree(int num_of_cliques){
  Jointree j = (Jointree) malloc(sizeof(jointreetype));
  j->cliques = (Clique *) calloc(num_of_cliques, sizeof(Clique));

  /* ... */
  return NULL;
}

int free_Jointree(Jointree j){
  if(j == NULL)
    return ERROR_NULLPOINTER;
  free(j->cliques);
  free(j);
  return NO_ERROR;
}

int add_Clique(Jointree j, Clique c){

  return NO_ERROR;
}
