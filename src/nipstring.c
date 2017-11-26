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

#include "nipstring.h"
#include <ctype.h>  // isspace
#include <stdlib.h> // calloc, free
#include <string.h> // strncpy
#include "niperrorhandler.h"

/*
#define DEBUG_IO
#include <stdio.h> // fprintf
*/

int nip_count_words(const char *s, int *chars){
  return nip_count_tokens(s, chars, 0, NULL, 0, 0, 1);
}


int nip_count_tokens(const char *s, int *chars, int q_strings,
		     char *separators, int n_separators, int sep_tokens,
		     int wspace_sep){
  int i;
  int tokens = 0, state = 0;
  int separator_found = 0;
  char ch;
  if(chars)
    *chars = 0;

  /*
   * States:
   *   0: waiting for start of token
   *   1: processing token (not quoted string)
   *   2: processing quoted string
   */
  while (*s != '\0'){

    if(separators)
      for(i = 0; i < n_separators; i++)
	if(*s == separators[i]){
	  separator_found = 1;
	  break;
	}

    if(state != 2 && separator_found){
      if(sep_tokens)
	tokens++;
      state = 0;
      separator_found = 0;
    }
    else if(q_strings && state != 2 && (*s == '"')){

      /* Check if we have a matching '"' */
      i = 1;
      while((ch = s[i++]) != '\0')
	if(ch == '"'){

	  /* Quoted string starts from here. */
	  state = 2;
	  tokens++;
	  break;
	}
    }
    else if(state == 0){
      if((wspace_sep && !isspace((int)*s)) ||
	 (!wspace_sep && *s != '\n')){
	tokens++;	  
	state = 1;
      }
    }
    else if((wspace_sep && state == 1 && isspace((int)*s)) ||
	    *s == '\n')
      state = 0;
    else if(q_strings && state == 2 && (*s == '"'))
      state = 0;

    s++;
    if(chars)
      (*chars)++;
  }
  return tokens;
}


int* nip_tokenise(const char s[], int n, int q_strings,
		  char* separators, int n_separators,
		  int sep_tokens, int wspace_sep){
  int* indices;
  int i = 0, j = 0, state = 0, arraysize = 2*n, k;
  int separator_found = 0;
  char ch, ch2;

  if(s == NULL){
    nip_report_error(__FILE__, __LINE__, EFAULT, 0);
    return NULL;
  }
  if(n < 0){
    nip_report_error(__FILE__, __LINE__, EINVAL, 1);
    return NULL;
  }

  if(n == 0)
    return NULL;

  indices = (int *) calloc(arraysize, sizeof(int));

  /* Couldn't allocate memory */
  if(!indices){
    nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
    return NULL;
  }

  while ((ch = s[i]) != '\0'){

    if(separators)
      for(k = 0; k < n_separators; k++)
	if(ch == separators[k]){
	  separator_found = 1;
	  break;
	}

    if(state != 2 && separator_found){
      if(state == 1)
	indices[j++] = i;

      state = 0;

      if(sep_tokens){
	indices[j++] = i;
	indices[j++] = i + 1;
      }

      separator_found = 0;
    }
    else if(q_strings && state != 2 && (ch == '"')){

      /* Check if we have a matching '"' */
      k = i + 1;
      while((ch2 = s[k++]) != '\0')
	if(ch2 == '"'){

	  /* If we were not done processing the previous token,
	   * mark it as done.
	   */
	  if(state == 1)
	    indices[j++] = i;

	  /* Quoted string starts from here. */
	  state = 2;
	  indices[j++] = i;
	  break;
	}
    }
    else if(state == 0){
      if((wspace_sep && !isspace((int)ch)) ||
	 (!wspace_sep && ch != '\n')){
	indices[j++] = i;
	state = 1;
      }
    }
    else if((wspace_sep && state == 1 && isspace((int)ch)) ||
	    ch == '\n'){
      indices[j++] = i;
      state = 0;
    }
    else if(q_strings && state == 2 && (ch == '"')){
      indices[j++] = i + 1;
      state = 0;
    }

    /* Have we found enough words? If so, break out. */
    if(j == arraysize)
      break;
    i++;
#ifdef DEBUG_IO
    fprintf(stderr, "i = %d, state = %d\n", i, state);
#endif
  }

  /* Mark the end of a word that extends to the end of the string */
  if(j == arraysize - 1)
    indices[j] = i;

  /* Not enough words */
  else if(j < arraysize - 1){
#ifdef DEBUG_IO
    fprintf(stderr, "tokenise failed to find enough tokens\n", i);
    fprintf(stderr, "j = %d, arraysize = %d\n", j, arraysize);
    fprintf(stderr, "indices =\n");
    for(i = 0; i < j; i++)
      fprintf(stderr, "%d ", indices[i]);
    fprintf(stderr, "\n");
#endif
    nip_report_error(__FILE__, __LINE__, EINVAL, 1);
    free(indices);
    return NULL;
  }
  
  return indices;
}


char** nip_split(const char s[], int indices[], int n){
  int i, j, wordlength, begin, end;
  char** words = (char **) calloc(n, sizeof(char *));

  /* Couldn't allocate memory */
  if(!words){
    nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
    return NULL;
  }

  for(i = 0; i < n; i++){
    begin = indices[2*i];
    end = indices[2*i+1];
    wordlength = end - begin;
    words[i] = (char *) calloc(wordlength + 1, sizeof(char));

    /* Couldn't allocate memory */
    if(!words[i]){
      nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
      for(j = 0; j < i; j++)
	free(words[j]);
      free(words);
      return NULL;
    }

    strncpy(words[i], &s[begin], wordlength);
    words[i][wordlength] = '\0';
  }

  return words;
}
