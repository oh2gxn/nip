/*
 * fileio.c $Id: fileio.c,v 1.4 2004-02-13 14:12:20 mvkorpel Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileio.h"
#include "errorhandler.h"
#include "Graph.h"

#define COMMENT_CHAR '%'
#define LINELENGTH 10000
#define STRING_NODE "node"
#define STRING_STATES "states"
#define STRING_LABEL "label"

int count_words(char *s, int *chars){
  int words = 0, state = 0;
  if(chars)
    *chars = 0;
  while (*s != '\0'){
    if(state == 0){
      if((*s != ' ') && (*s != '\t') && (*s != '\n')){
	words++;	  
	state = 1;
      }
    }
    else if((*s == ' ') || (*s == '\t') || (*s == '\n'))
      state = 0;
    s++;
    (*chars)++;
  }
  return words;
}

int *tokenise(char s[], int n){
  int *indices;
  int i = 0, j = 0, state = 0, arraysize = 2*n;

  if(s == NULL){
    report_error(ERROR_NULLPOINTER, 0);
    return NULL;
  }
  if(n < 1){
    report_error(ERROR_INVALID_ARGUMENT, 0);
    return NULL;
  }

  indices = (int *) calloc(arraysize, sizeof(int));
  /* Couldn't allocate memory */
  if(!indices){
    report_error(ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  while (s[i] != '\0'){
    if(state == 0){
      if((s[i] != ' ') && (s[i] != '\t') && (s[i] != '\n')){
	indices[j++] = i;
	state = 1;
      }
    }
    else if((s[i] == ' ') || (s[i] == '\t') || (s[i] == '\n')){
      indices[j++] = i;
      /* We have found enough words */
      if(j == arraysize)
	break;
      state = 0;
    }
    i++;
  }

  /* Mark the end of a word that extends to the end of the string */
  if(j == arraysize - 1)
    indices[j] = i;
  /* Not enough words */
  else if(j < arraysize - 1){
    free(indices);
    return NULL;
  }
  
  return indices;
}

char **split(char s[], int indices[], int n){
  int i, wordlength, begin, end;
  char **words = (char **) calloc(n, sizeof(char *));
  /* Couldn't allocate memory */
  if(!words){
    report_error(ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  for(i = 0; i < n; i++){
    begin = indices[2*i];
    end = indices[2*i+1];
    wordlength = end - begin;
    words[i] = (char *) calloc(wordlength + 1, sizeof(char));
    /* Couldn't allocate memory */
    if(!words[i]){
      report_error(ERROR_OUTOFMEMORY, 1);
      return NULL;
    }
    strncpy(words[i], &s[begin], wordlength);
    words[i][wordlength] = '\0';
  }

  return words;
}

Graph *read_model(const char *filename){

  FILE *f;
  char s[LINELENGTH];
  int words;
  int *wordbounds;
  int state = 0;
  f=fopen(filename, "r");
  if(!f){
    report_error(ERROR_IO, 1);
    return NULL;
  }

  /* Read a line at a time from the file. */
  while(fgets(s, LINELENGTH, f) != NULL){
    words = count_words(s, NULL);
    wordbounds = tokenise(s, words);

    /* TO DO */

    /* Process the input line with an FSA */
    while(1){
      switch(state){
	/* 0: looking for upper level token "node" */
	case 0 :
	  
	  break;

	case 1 :
	  
	  break;

	case 2 :

	  break;
      }            
    } /* end line processing */

  } /* end of file */

  return NULL;
}
