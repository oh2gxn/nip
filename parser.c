/* Functions for the bison parser. $Id: parser.c,v 1.2 2004-03-16 12:50:07 jatoivol Exp $
 */

#include "parser.h"

/* correctness? */
void add_pvar(Variable var){
  varlink new = (varlink) malloc(sizeof(varelement));
  new->data = var;
  new->fwd = 0;
  new->bwd = last;
  if(first = 0)
    first = new;
  last = new;
  vars_parsed++;
}
