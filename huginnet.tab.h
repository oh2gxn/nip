/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     token_net = 258,
     token_class = 259,
     token_node_size = 260,
     token_data = 261,
     token_utility = 262,
     token_decision = 263,
     token_discrete = 264,
     token_continuous = 265,
     token_node = 266,
     token_label = 267,
     token_position = 268,
     token_states = 269,
     token_persistence = 270,
     token_potential = 271,
     token_normal = 272,
     QUOTED_STRING = 273,
     UNQUOTED_STRING = 274,
     NUMBER = 275
   };
#endif
/* Tokens.  */
#define token_net 258
#define token_class 259
#define token_node_size 260
#define token_data 261
#define token_utility 262
#define token_decision 263
#define token_discrete 264
#define token_continuous 265
#define token_node 266
#define token_label 267
#define token_position 268
#define token_states 269
#define token_persistence 270
#define token_potential 271
#define token_normal 272
#define QUOTED_STRING 273
#define UNQUOTED_STRING 274
#define NUMBER 275




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 27 "huginnet.y"
typedef union YYSTYPE {
  double numval;
  double *doublearray;
  char *name;
  char **stringarray;
  variable variable;
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 86 "huginnet.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



