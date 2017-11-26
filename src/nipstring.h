/**
 * @file
 * @brief String tokeniser for parsers
 * 
 * @author Mikko Korpela
 * @author Janne Toivola
 * @copyright &copy; 2007,2012 Janne Toivola <br>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. <br>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. <br>
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __NIPSTRING_H__
#define __NIPSTRING_H__

/**
 * Counts the number of words in string s. Words are separated by whitespace.
 * String \p s must be null terminated. If \p chars is not a null pointer,
 * the number of characters in the string is placed there.
 * @param s Null terminated string
 * @param chars Pointer to a place where character count can be written, or NULL
 * @return number of whitespace delimited words
 */
int nip_count_words(const char *s, int *chars);

/**
 * Counts the number of tokens in a null terminated string.
 * @param s The input string
 * @param chars Pointer for writing the number of characters in \p s
 * @param q_strings If != 0, "quoted strings" are considered one token
 * @param separators Each byte in this array acts as a separator between 
 * tokens. Whitespace is a separator, too.
 * @param n_separators Size of \p separators array
 * @param sep_tokens If != 0, separators are themselves considered to be tokens
 * @param wspace_sep If != 0, whitespace is a separator of tokens,
 * but is not considered a token itself
 * @return number of found tokens
 * @see isspace() in <ctype.h>
 */
int nip_count_tokens(const char *s, int *chars, int q_strings,
		     char *separators, int n_separators, int sep_tokens,
		     int wspace_sep);

/**
 * Extracts \p n first words or tokens of string \p s. 
 * Returns an array of indices (length 2*n) in ascending order. 
 * The indices indicate the location of the first and (last+1) 
 * characters of each word.
 * The string must be null terminated.
 * Returns NULL if the string contains less than \p n words.
 * @param s The input string
 * @param n The number of tokens to look for
 * @param q_strings If != 0, "quoted strings" are considered one token
 * @param separators Each byte in this array acts as a separator between 
 * tokens. Whitespace is a separator, too.
 * @param n_separators Size of \p separators array
 * @param sep_tokens If != 0, separators are themselves considered to be tokens
 * @param wspace_sep If != 0, whitespace is a separator of tokens,
 * but is not considered a token itself
 * @return new array of character indices, free it when done
 * @see count_tokens() */
int* nip_tokenise(const char s[], int n, int q_strings,
		  char *separators, int n_separators, int sep_tokens,
		  int wspace_sep);

/**
 * Splits string \p s into \p n null terminated tokens.
 * @param s The original string to split
 * @param indices Array of start and end indices from tokeniser
 * @param n Number of resulting strings, half the size of \p indices
 * @return new array of new strings, free all of them when done
 * @see nip_tokenise() */
char** nip_split(const char s[], int indices[], int n);

#endif /* __NIPSTRING_H__ */
