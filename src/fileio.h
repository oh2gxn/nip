/*
 * fileio.h $Id: fileio.h,v 1.11 2004-06-29 11:45:22 mvkorpel Exp $
 */

#ifndef __FILEIO_H__

#define __FILEIO_H__
#define LINELENGTH 10000

/*
 * Counts the number of words in string s. Words are separated by white space.
 * String s must be null terminated. If chars is not a null pointer,
 * the number of characters in the string is placed there.
 */
int count_words(const char *s, int *chars);

/*
 * Counts the number of tokens in a null terminated string.
 * Parameters:
 * - s:
 *     the input string
 * - *chars:
 *     the number of characters in s is stored here
 * - quoted_strings:
 *     if != 0, strings enclosed in "" are considered one token
 * - separators:
 *     Each character in this string acts as a separator between tokens.
 *     White space is a separator, too.
 * - numof_separators:
 *     the number of characters in *separators
 * - sep_tokens:
 *     if != 0, separators are themselves considered to be tokens
 * - wspace_sep:
 *     if != 0, whitespace is a separator of tokens,
 *     but is not considered a token itself
 */
int count_tokens(const char *s, int *chars, int quoted_strings,
		 char *separators, int numof_separators, int sep_tokens,
		 int wspace_sep);

/*
 * Extracts n first words or tokens of string s. Returns an array of indices
 * (length 2*n) in ascending order. The indices indicate the location of
 * the first and (last+1) characters of each word.
 * The string must be null terminated.
 * Returns NULL if the string contains less than n words.
 * See the parameters of count_tokens.
 */
int *tokenise(const char s[], int n, int quoted_strings,
	      char *separators, int numof_separators, int sep_tokens,
	      int wspace_sep);

/*
 * Splits string s into n null terminated tokens.
 * - indices: an index array (see tokenise(...))
 */
char **split(const char s[], int indices[], int n);

#endif /* __FILEIO_H__ */
