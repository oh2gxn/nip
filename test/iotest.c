/*
 * iotest.c $Id: iotest.c,v 1.1 2004-02-12 14:49:19 mvkorpel Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include "fileio.h"

#define PRINTWORDS 20

/* A test for io functions. Reads from stdio.
 * Currently prints the number of characters, words and lines.
 * Also prints the word boundaries of each line, and prints a few
 * of the first words, showing the wonderful ability to split strings
 * at word boundaries.
 */
int main(){
/*    FILE *f; */
  /* Hopefully we dont't have longer lines than this. */
  char s[10000];
  int chars = 0, words = 0, lines = 0;
  int chartemp, wordtemp, i;
  int *wordbounds;
  char *splitwords[PRINTWORDS];
  char **wordarray;

/*    f=fopen("infile","r"); */
/*    if (!f) */
/*    return 1; */

/*    while(fgets(s, 1000, f) != NULL){ */
  while(gets(s) != NULL){
/*      printf("%s",s); */
    printf("%s\n",s);
    wordtemp = count_words(s, &chartemp);
    wordbounds = tokenise(s, wordtemp);

    if(words < PRINTWORDS){
      wordarray = split(s, wordbounds, wordtemp);
      for(i = 0; i < wordtemp; i++){
	if(words < PRINTWORDS)
	  splitwords[words] = wordarray[i];
	else
	  free(wordarray[i]);
	words++;
      }
    }
    else
      words += wordtemp;

    /* Print the word bounds. */
    if(wordbounds != NULL){
      for(i = 0; i < 2 * wordtemp; i++)
	printf("%d ", wordbounds[i]); 
      printf("\n");
      free(wordbounds);
    }

    chars += chartemp + 1;
    lines++;
  }
/*      fclose(f); */

  /* Print results */
  printf("\n%d lines, %d words, %d chars\n", lines, words, chars);
  if(words < PRINTWORDS)
    wordtemp = words;
  else
    wordtemp = PRINTWORDS;
  printf("%d first words:\n", wordtemp);
  for(i = 0; i < wordtemp; i++)
    printf("%s ", splitwords[i]);
  printf("\n");

  return 0;
}
