/* Definitions for the bison parser. $Id: parser.h,v 1.2 2004-03-16 12:50:07 jatoivol Exp $
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

static varlink first = 0;
static varlink last = 0;
static int vars_parsed = 0;

void add_pvar(Variable var);

#endif /* __PARSER_H__ */
