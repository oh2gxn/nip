/*
 * fileio.h $Id: fileio.h,v 1.2 2004-02-12 14:49:19 mvkorpel Exp $
 */

#ifndef __FILEIO_H__
#define __FILEIO_H__

/* Counts the number of words in string s.
 * The string must be null terminated.
 * If chars is not a null pointer, the number of characters in the string
 * is placed there.
 */
int count_words(char *s, int *chars);

/* Extracts n first words of string s. Returns an array of indices
 * (length 2*n) in ascending order. The indices indicate the location of
 * the first and (last+1) characters of each word.
 * The string must be null terminated.
 * Returns NULL if the string contains less than n words.
 */
int *tokenise(char *s, int n);

/* Splits string s into n null terminated words.
 * - indices: an index array (see tokenise(...))
 */
char **split(char s[], int indices[], int n);

#endif /* __FILEIO_H__ */
