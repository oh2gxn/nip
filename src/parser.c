/* Functions for the bison parser. $Id: parser.c,v 1.1 2004-03-16 10:34:22 mvkorpel Exp $
 */

#include "parser.h"

void add_pvar(Variable var){
  link new = (link) malloc(sizeof(element));
  new->data = s;
  new->fwd = c->sepsets;
  new->bwd = 0;
  if(c->sepsets != 0)
    c->sepsets->bwd = new;
  c->sepsets = new;
  return 0;
}
