/*
 * fileio.h $Id: fileio.h,v 1.9 2004-06-08 10:04:35 mvkorpel Exp $
 */

#ifndef __FILEIO_H__

#include "Graph.h"

#define __FILEIO_H__
#define LINELENGTH 10000

/* Counts the number of words in string s.
 * The string must be null terminated.
 * If chars is not a null pointer, the number of characters in the string
 * is placed there.
 */
int count_words(const char *s, int *chars);

/* Counts the number of tokens in string s.
 * The string must be null terminated.
 * Tokens are like (white space separated) words, but every
 * '(', ')', '=', '{', '}', ',', and ';'is a separate token.
 * Additionally, quoted strings are each counted as one token.
 * POSSIBLE PROBLEMS with un-paired quotes?
 * If chars is not a null pointer, the number of characters in the string
 * is placed there.
 */
int count_tokens(const char *s, int *chars);

/* Extracts n first words or tokens of string s. Returns an array of indices
 * (length 2*n) in ascending order. The indices indicate the location of
 * the first and (last+1) characters of each word.
 * The string must be null terminated.
 * Returns NULL if the string contains less than n words.
 * If mode == 0, the string is separated into words.
 * If mode == 1, the string is separated into tokens.
 * The definitions of word and token are in count_tokens(...).
 */
int *tokenise(const char s[], int n, int mode);

/* Splits string s into n null terminated words.
 * - indices: an index array (see tokenise(...))
 */
char **split(const char s[], int indices[], int n);

/* Reads the variables of a graph from the given file. Returns a graph
 * containing the variables.
 */
Graph *read_model(const char *filename);

#endif /* __FILEIO_H__ */
