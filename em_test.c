#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "nip.h"
#include "variable.h"

/* Reads an initial guess for the model from a Hugin NET file, 
 * uses EM-algorithm for learning the parameters from the 
 * given data file and writes the resulting model to the 
 * specified output file. 
 * SYNOPSIS: EM_TEST <ORIGINAL.NET> <DATA.TXT> <THRESHOLD> <RESULT>
 * (resulting model will be written to the file <result.net>) */

int write_model(nip model, char* name){
  int i, j, n;
  FILE *f = NULL;
  char *filename = NULL;
  variable v = NULL;
  int *temp = NULL;
  int *map = NULL;
  clique c = NULL;
  potential p = NULL;

  /* form the filename */
  filename = (char*) calloc(strlen(name) + 5, sizeof(char));
  if(!filename){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }
  strcpy(filename, name);   /* copy the model name */
  strcat(filename, ".net"); /* append with the extension */

  /* open the stream */
  f = fopen(filename, "w");
  if(!f){
    report_error(__FILE__, __LINE__, ERROR_IO, 1);
    free(filename);
    return ERROR_IO;
  }

  /* remove the evidence */
  reset_model(model);

  /* Reminder:
   * fputs("some text", f);
   * fprintf(f, "value = %g", 1.23400000); */
  /*******************************************************/
  /* Replace %f with %g if you don't like trailing zeros */

  /** Lets print the NET file **/

  /** standard stuff in the beginning **/
  fprintf(f, "class %s \n", name);
  fputs("{ \n", f);
  fputs("    inputs = (); \n", f);
  fputs("    outputs = (); \n", f);
  fputs("    node_size = (40 40); \n", f);

  /** the variables **/
  for(i = 0; i < model->num_of_vars; i++){
    v = model->variables[i];
    n = number_of_values(v)-1;
    fputs("\n", f);
    fprintf(f, "    node %s \n", get_symbol(v));
    fputs("    { \n", f);
    fprintf(f, "        label = \"%s\"; \n", v->name);
    /* FIXME: some real positions */
    fprintf(f, "        position = (%d %d); \n", 100, 100);
    fprintf(f, "        states = (");
    for(j = 0; j < n; j++)
      fprintf(f, "\"%s\" ", v->statenames[j]);
    fprintf(f, "\"%s\"); \n", v->statenames[n]);
    if(v->next)
      fprintf(f, "        NIP_next = \"%s\"; \n", get_symbol(v->next));
    fputs("    } \n", f);    
  }

  /** the priors **/
  for(i = 0; i < model->num_of_vars - model->num_of_children; i++){
    v = model->independent[i];
    n = number_of_values(v) - 1;
    fputs("\n", f);
    /* independent variables have priors */
    fprintf(f, "    potential (%s) \n", get_symbol(v));
    fputs("    { \n", f);
    fputs("        data = ( ", f);
    for(j = 0; j < n; j++)
      fprintf(f, "%f  ", v->prior[j]);
    fprintf(f, "%f ); \n", v->prior[n]);
    fputs("    } \n", f);
  }

  /** the potentials **/
  for(i = 0; i < model->num_of_children; i++){
    v = model->children[i];
    n = number_of_parents(v) - 1;
    fputs("\n", f);
    /* child variables have conditional distributions */
    fprintf(f, "    potential (%s | ", get_symbol(v));
    for(j = 0; j < n; j++)
      fprintf(f, "%s ", get_symbol(v->parents[j]));
    fprintf(f, "%s)\n", get_symbol(v->parents[n]));
    fputs("    { \n", f);
    fputs("        data = (", f);

    n++; /* number of parents */
    temp = (int*) calloc(n+1, sizeof(int));
    if(!temp){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      fclose(f);
      free(filename);
      return ERROR_OUTOFMEMORY;
    }

    /* form the potential */
    temp[0] = v->cardinality; /* child must be the first... */
    /* ...then the parents in reverse order */
    for(j = 1; j <= n; j++)
      temp[j] = v->parents[n-j]->cardinality;
    p = make_potential(temp, n+1, NULL);

    /* compute the distribution */
    c = find_family(model->cliques, model->num_of_cliques, v);
    map = find_family_mapping(c, v);
    temp[0] = map[0]; /* reuse the temp array for mapping */
    for(j = 1; j <= n; j++)
      temp[j] = map[1+n-j];
    general_marginalise(c->p, p, temp);

    /* print the stuff */
    n = number_of_values(v);
    for(j = 0; j < p->size_of_data; j++){
      if(j > 0 && j % n == 0)
	fputs("\n                ", f);
      fprintf(f, " %f ", p->data[j]);
    }
    fputs("); \n", f);
    fputs("    } \n", f);
    free(temp);
    free_potential(p);
  }

  fputs("} \n", f); /* the last brace */

  /* close the file */
  if(fclose(f)){
    report_error(__FILE__, __LINE__, ERROR_IO, 1);
    free(filename);
    return ERROR_IO;
  }
  free(filename);
  return NO_ERROR;
}


int main(int argc, char *argv[]) {

  int i;
  nip model = NULL;
  time_series ts = NULL;
  double threshold = 0;
  char* tailptr = NULL;

  if(argc < 5){
    printf("Give the names of: \n"); 
    printf(" - the original NET file, \n");
    printf(" - data file, \n"); 
    printf(" - threshold value (0...1), and \n");
    printf(" - name for the resulting model, please!\n");
    return 0;
  }
  
  /* read the model */
  model = parse_model(argv[1]);
  if(!model){
    printf("Unable to parse the NET file: %s?\n", argv[1]);
    return -1;
  }
  use_priors(model, 1);

  /* read the data */
  ts = read_timeseries(model, argv[2]);
  if(!ts){
    printf("Unable to parse the data file: %s?\n", argv[2]);
    return -1;
  }

  /* read the threshold value */
  threshold = strtod(argv[3], &tailptr);
  if(threshold <= 0 || threshold > 1  || tailptr == argv[3]){
    printf("Specify a valid threshold value: %s?\n", argv[3]);
    return -1;
  }

  /* THE algorithm (may take a while) */
  printf("Computing... \n");
/*   i = em_learn(ts, threshold); */
/*   if(i != NO_ERROR){ */
/*     fprintf(stderr, "There were errors during learning:\n"); */
/*     report_error(__FILE__, __LINE__, i, 1); */
/*     return -1; */
/*   } */
  printf("...done.\n");

  /* Write the results to a NET file */
  i =  write_model(model, argv[4]);
  if(i != NO_ERROR){
    fprintf(stderr, "Failed to write the model into %s.net\n", argv[4]);
    report_error(__FILE__, __LINE__, i, 1);
    return -1;
  }

  return 0;
}
