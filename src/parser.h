/* Definitions for the bison parser. $Id: parser.h,v 1.1 2004-03-16 10:34:22 mvkorpel Exp $
 */

#ifndef __PARSER_H__
#define __PARSER_H__

#include "Variable.h"
#include "potential.h"

struct varlist {
  Variable data;
  struct varlist *fwd;
  struct varlist *bwd;
};

typedef struct varlist varelement;
typedef varelement *varlink;

static varlink first;
static varlink last;
static int vars_parsed;

void add_pvar(Variable var);

#endif /* __PARSER_H__ */
