/*
 * fileio.h $Id: fileio.h,v 1.1 2004-02-10 11:23:57 mvkorpel Exp $
 */

#ifndef __FILEIO_H__
#define __FILEIO_H__

/* Counts the number of words in string s.
 * The string must be null terminated.
 */
int count_words(char *s);

/* Extracts n first words of string s. Returns an array of indices in
 * ascending order. The indices indicate the location of the first and
 * (last+1) characters of each word.
 * The string must be null terminated.
 * Returns NULL if the string contains less than n words.
 */
int *tokenise(char *s, int n);

#endif /* __FILEIO_H__ */
