/*
 * fileio.c $Id: fileio.c,v 1.2 2004-02-10 11:25:52 mvkorpel Exp $
 */

#include <stdio.h>
#include "fileio.h"

int count_words(char *s){
  int words = 0, state = 0;
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
  }
  return words;
}

int *tokenise(char *s, int n){
  
  return NULL;
}
