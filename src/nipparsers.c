/**
 * @file
 * @brief Basic tokeniser functions for the Hugin Net file parser.
 * Contains also other parser functions and structs for data files.
 *
 * JJ Comment: Currently the parser is ugly as hell...
 *             Get rid of global variables and ad-hoc data structures!
 *             (add_X(), get_X(), and set_X() are probably the worst)
 *
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

#include "nipparsers.h"

/* #define DEBUG_PARSER */

/* #define PRINT_TOKENS */

/* #define DEBUG_DATAFILE */

#define NIP_PARSER_FOUND_DATA 0
#define NIP_PARSER_EXPECT_DATA 1
#define NIP_PARSER_EXPECT_HEADER 2

static int nip_null_observation(char* token);

static void nip_free_data_file(nip_data_file f);

static int nip_count_data_rows(nip_data_file f);
static int nip_set_node_symbols(nip_data_file file, char* line, int* token_bounds, int ntokens);
static int nip_accumulate_state_names(nip_string_list* statenames, char* line, int* token_bounds, int ntokens);

nip_data_file nip_open_data_file(char* filename, char separator,
                                 int write, int nodenames){

  int length_of_name = 0;
  nip_data_file f = NULL;

  f = (nip_data_file) malloc(sizeof(nip_data_file_struct));
  if(f == NULL){
    nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
    return NULL;
  }

  f->name = NULL;
  f->separator = separator;
  f->file = NULL;
  f->write = write ? 1 : 0; /* not bothering with fstat */
  f->is_open = 0;
  f->first_line_labels = (nodenames ? 1 : 0);
  f->current_line = 0;
  f->label_line = -1; /* headerless by default */
  f->ndatarows = 0;
  f->datarows = NULL;
  f->node_symbols = NULL;
  f->num_of_nodes = 0;
  f->node_states = NULL;
  f->num_of_states = NULL;

  if(write)
    f->file = fopen(filename,"w");
  else
    f->file = fopen(filename,"r");

  if (!f->file){
    nip_report_error(__FILE__, __LINE__, EIO, 1);
    free(f);
    return NULL; /* fopen(...) failed */
  }
  else
    f->is_open = 1;

  length_of_name = strlen(filename);
  f->name = (char *) calloc(length_of_name + 1, sizeof(char));
  if(!f->name){
    nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
    fclose(f->file);
    free(f);
    return NULL;
  }
  strcpy(f->name, filename);

  return f;
}

int nip_analyse_data_file(nip_data_file file){

  int i, state = NIP_PARSER_FOUND_DATA;
  int linecounter = 0;
  int tscounter = 0;
  char last_line[MAX_LINELENGTH];
  int num_of_tokens = 0;
  int* token_bounds;
  int empty_lines_read = 0;
  nip_string_list* statenames = NULL;

  if (file->write){
    return 0;
  }
  /* If the file is opened in read mode, check the contents.
   * This includes names of nodes and their states.
   */

  /* the first pass: find header, count columns, count consecutive rows of data */
  file->ndatarows = nip_count_data_rows(file);

  /* allocate file->datarows array */
  file->datarows = (int*) calloc(file->ndatarows, sizeof(int));
  /* NOTE: calloc resets the contents to zero! */
  if(!file->datarows){
    nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
    return -1;
  }

  /* the second pass: determine node labels (header) and unique states */
  linecounter = 0;
  state = NIP_PARSER_EXPECT_DATA;
  if(file->first_line_labels)
    state = NIP_PARSER_EXPECT_HEADER;

  while(fgets(last_line, MAX_LINELENGTH, file->file)){

    num_of_tokens = nip_count_tokens(last_line, NULL, 0,
                                     &(file->separator), 1, 0, 1);
    if(num_of_tokens > 0){
      token_bounds =
        nip_tokenise(last_line, num_of_tokens, 0, &(file->separator), 1, 0, 1);
      if(!token_bounds){
        nip_report_error(__FILE__, __LINE__, nip_check_error_type(), 1);
        return -1;
      }
    }
    (file->current_line)++;

    /* Ignore empty lines...
     * only the empty lines immediately after the node labels...
     * and duplicate empty lines. Otherwise start a new timeseries */
    if(num_of_tokens == 0){
      empty_lines_read++;
      continue; /* empty lines to be ignored */
    }
    else{
      if(empty_lines_read){
        linecounter = 1;
        if(state < NIP_PARSER_EXPECT_DATA)
          tscounter++;
      }
      else
        linecounter++;
      empty_lines_read = 0;
      if(state > NIP_PARSER_FOUND_DATA)
        state--; /* stop ignoring single empty lines */
    }
 
    /* Read node names or make them up. */
    if(state > NIP_PARSER_FOUND_DATA){
      /* the first non-empty line and expecting node names: set file->num_of_nodes */
      if (nip_set_node_symbols(file, last_line, token_bounds, num_of_tokens) < 0) {
        nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
        free(token_bounds);
        return -1;
      }

      /* allocate memory for counting states */
      statenames = (nip_string_list *) calloc(file->num_of_nodes, sizeof(nip_string_list));
      if(!statenames){
        nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
        free(token_bounds);
        return -1;
      }

      for(i = 0; i < file->num_of_nodes; i++)
        statenames[i] = nip_new_string_list();

      file->num_of_states = (int *) calloc(file->num_of_nodes, sizeof(int));
      if(!file->num_of_states){
        nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
        for(i = 0; i < file->num_of_nodes; i++)
          nip_free_string_list(statenames[i]);
        free(statenames);
        free(token_bounds);
        return -1;
      }

      file->node_states = (char ***) calloc(file->num_of_nodes, sizeof(char **));
      if(!file->node_states){
        nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
        for(i = 0; i < file->num_of_nodes; i++)
          nip_free_string_list(statenames[i]);
        free(statenames);
        free(token_bounds);
        return -1;
      }
    }
    else{
      /* Read observations (just in order to see all the different
         kinds of observations for each node). */
      file->datarows[tscounter]++;
      /* i == min(file->num_of_nodes, num_of_tokens) */
      i = (file->num_of_nodes < num_of_tokens) ? file->num_of_nodes : num_of_tokens;
      if (i > 0 &&
          nip_accumulate_state_names(statenames, last_line, token_bounds, i) < 0){
        nip_report_error(__FILE__, __LINE__, nip_check_error_type(), 1);
        for(i = 0; i < file->num_of_nodes; i++)
          nip_free_string_list(statenames[i]);
        free(statenames);
        free(token_bounds);
        return -1;
      }
    }
    free(token_bounds);
  }

  /* Count number of states in each variable and convert lists into arrays */
  for(i = 0; i < file->num_of_nodes; i++){
    file->num_of_states[i] = NIP_LIST_LENGTH(statenames[i]);
    file->node_states[i] = nip_string_list_to_array(statenames[i]);
    nip_empty_string_list(statenames[i]);
    free(statenames[i]);
  }
  free(statenames);

  rewind(file->file);
  file->current_line = 0;

  return tscounter;
}

/*
 * The first pass over a new data file to determine number of time series.
 * Ignores a header row unless f->first_line_labels==0.
 * Not useful for streaming / online processing.
 */
static int nip_count_data_rows(nip_data_file f) {

  char last_line[MAX_LINELENGTH]; /* buffer */
  int linecounter = 0; /* number of consecutive non-empty lines this far */
  int empty_lines_read = 0; /* number of consecutive empty lines */
  int ndatarows = 0; /* number of groups of consecutive non-empty lines */
  int state = NIP_PARSER_EXPECT_DATA; /* ignores empty lines before data */
  int num_of_tokens = 0;

  if(f->first_line_labels)
    state = NIP_PARSER_EXPECT_HEADER; /* ignores empty lines before node labels */

  while(fgets(last_line, MAX_LINELENGTH, f->file)){
    num_of_tokens = nip_count_tokens(last_line, NULL, 0,
                                     &(f->separator), 1, 0, 1);
    (f->current_line)++;

    /* JJT  1.9.2004: A sort of bug fix. Ignore empty lines */
    /* JJT 22.6.2005: Another fix. Ignore only the empty lines
     * immediately after the node labels... and duplicate empty lines.
     * Otherwise start a new timeseries */
    if(num_of_tokens == 0){
      linecounter = 0;
      empty_lines_read++;
      if(state > NIP_PARSER_FOUND_DATA || empty_lines_read > 1)
        continue; /* empty lines to be ignored */
    }
    else{
      linecounter++;
      empty_lines_read = 0;
      if(state >= NIP_PARSER_EXPECT_DATA){
        if(state >= NIP_PARSER_EXPECT_HEADER){
          f->label_line = f->current_line; // a side-effect
          linecounter = 0; /* not counting header as data */
        }
        state--; /* stop ignoring single empty lines and the node names */
      }
      if(state == NIP_PARSER_FOUND_DATA && linecounter == 1)
        ndatarows++; /* at the first consecutive row of data */
    }
  }

  /* rewind */
  rewind(f->file);
  f->current_line = 0;
  return ndatarows;
}

/* Use the header row as node symbols (column names). Return ntokens, or negative if failed. */
static int nip_set_node_symbols(nip_data_file file, char* line, int* token_bounds, int ntokens){
  int i, dividend, length_of_name;

  file->num_of_nodes = ntokens;
  file->node_symbols = (char **) calloc(ntokens, sizeof(char *));

  if(!file->node_symbols){
    nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
    return -1;
  }

  if(file->first_line_labels){
    for(i = 0; i < ntokens; i++){
      file->node_symbols[i] = (char *) calloc(token_bounds[2*i+1] - token_bounds[2*i] + 1,
                                              sizeof(char));
      if(!file->node_symbols[i]){
        nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
        return -1;
      }

      strncpy(file->node_symbols[i], &(line[token_bounds[2*i]]),
              token_bounds[2*i+1] - token_bounds[2*i]);
      file->node_symbols[i][token_bounds[2*i+1] - token_bounds[2*i]] = '\0';
    }
  }
  else{
    for(i = 0; i < ntokens; i++){
      /* determine length of the made up name */
      dividend = i + 1;
      length_of_name = 5;
      while((dividend /= 10) > 1)
        length_of_name++;

      file->node_symbols[i] = (char *) calloc(length_of_name, sizeof(char));
      if(!file->node_symbols[i]){
        nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
        return -1; /* error horror */
      }

      sprintf(file->node_symbols[i], "node%d", i + 1);
    }
  }
  return ntokens;
}

/* Use a data row as state names. Return number of tokens, or negative if failed. */
static int nip_accumulate_state_names(nip_string_list* statenames, char* line, int* token_bounds, int ntokens){
  int i, count = 0;
  char *token = NULL;

  for(i = 0; i < ntokens; i++){
    token = (char *) calloc(token_bounds[2*i+1] - token_bounds[2*i] + 1,
                            sizeof(char));
    if(!token){
      nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
      return -1;
    }

    strncpy(token, &(line[token_bounds[2*i]]),
            token_bounds[2*i+1] - token_bounds[2*i]);
    token[token_bounds[2*i+1] - token_bounds[2*i]] = '\0';

    /* If the string has not yet been observed, add it to a list 
     * (ownership is passed on, string is not freed here) */
    if(!nip_null_observation(token) && 
       !nip_string_list_contains(statenames[i], token)){
      if(nip_prepend_string(statenames[i], token) != NIP_NO_ERROR){
        nip_report_error(__FILE__, __LINE__, nip_check_error_type(), 1);
        free(token);
        return count;
      }
      else{
        count++;
      }
    }
    else{
      free(token); /* was seen earlier, skip */
    }
  }

  return count;
}

/*
 * Tells if the given token indicates a missing value, a "null observation".
 * The token must be null terminated.
 */
static int nip_null_observation(char *token){

#ifdef DEBUG_DATAFILE
  printf("nip_null_observation called\n");
#endif

  if(token == NULL){
    nip_report_error(__FILE__, __LINE__, EFAULT, 1);
    return 0;
  }
  /* compare to all known labels for missing data! */
  else if((strcmp("N/A", token) == 0) ||
          (strcmp("null", token) == 0) ||
          (strcmp("<null>", token) == 0)){
    return 1;
  }
  else{
    return 0;
  }
}


void nip_close_data_file(nip_data_file file){
  if(!file){
    nip_report_error(__FILE__, __LINE__, EFAULT, 1);
    return;
  }
  if(file->is_open){
    fclose(file->file);
    file->is_open = 0;
  }
  /* Release the memory of file struct. */
  nip_free_data_file(file);
}


/* Frees the memory used by (possibly partially allocated) datafile f. */
static void nip_free_data_file(nip_data_file f){
  int i,j;

  if(!f)
    return;
  free(f->name);
  if(f->node_symbols){
    for(j = 0; j < f->num_of_nodes; j++)
      free(f->node_symbols[j]);
    free(f->node_symbols);
  }
  if(f->node_states){
    for(i = 0; i < f->num_of_nodes; i++){
      for(j = 0; j < f->num_of_states[i]; j++)
        free(f->node_states[i][j]);
      free(f->node_states[i]);
    }
    free(f->node_states);
  }
  free(f->num_of_states);
  free(f->datarows);
  free(f);
}


int nip_next_line_tokens(nip_data_file f, char separator, char ***tokens){

  char line[MAX_LINELENGTH];
  char *token;
  int num_of_tokens;
  int *token_bounds;
  int i, j;

  if(!f || !tokens){
    nip_report_error(__FILE__, __LINE__, EFAULT, 1);
    return -1;
  }

  if(!(f->is_open)){
    nip_report_error(__FILE__, __LINE__, EIO, 1);
    return -1;
  }

  /* seek the first line of data (starting from the current line) */
  num_of_tokens = 0;
  do{
    if(fgets(line, MAX_LINELENGTH, f->file) == NULL)
      break;
    else
      f->current_line++;
    
    /* observed token count */
    num_of_tokens = nip_count_tokens(line, NULL, 0, &separator, 1, 0, 1);
    
    /* Skip the first line if it contains node labels. */
    if((f->current_line == f->label_line)  &&  f->first_line_labels)
      num_of_tokens = 0;

  }while(num_of_tokens < 1);

  if(num_of_tokens == 0)
    return 0;

  token_bounds = nip_tokenise(line, num_of_tokens, 0, &separator, 1, 0, 1);
  if(!token_bounds){
    nip_report_error(__FILE__, __LINE__, nip_check_error_type(), 1);
    return -1;
  }

  /* effective number of tokens from now on: min(f->num_of_nodes, observed) */
  num_of_tokens = f->num_of_nodes<num_of_tokens ? f->num_of_nodes : num_of_tokens;
  
  *tokens = (char **) calloc(num_of_tokens, sizeof(char *)); // TODO: freed where?
  if(!tokens){
    nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
    free(token_bounds);
    return -1;
  }

  for(i = 0; i < num_of_tokens; i++){
    token = (char *) calloc(token_bounds[2*i+1] - token_bounds[2*i] + 1,
                            sizeof(char));
    if(!token){
      nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
      nip_free_data_file(f);
      for(j = 0; j < i; j++)
        free(*tokens[j]);
      free(*tokens);
      free(token_bounds);
      return -1;
    }

    strncpy(token, &(line[token_bounds[2*i]]),
            token_bounds[2*i+1] - token_bounds[2*i]);

    token[token_bounds[2*i+1] - token_bounds[2*i]] = '\0';

    /* Children, remember what papa said: always use parentheses... */
    (*tokens)[i] = token;
  }

  free(token_bounds);

  /* Return the number of acquired tokens. */
  return num_of_tokens;
}


char* nip_next_hugin_token(FILE* f, int* token_length){

  /* The last line read from the file */
  static char last_line[MAX_LINELENGTH];

  /* How many tokens left in the current line of input? */
  static int tokens_left;

  /* Pointer to the index array of token boundaries */
  static int* indexarray = NULL;

  /* Pointer to the index array of token boundaries
   * (not incremented, we need this for free() ) */
  static int* indexarray_original = NULL;

  /* Should a new line be read in when the next token is requested? */
  static int nip_read_line = 1; 
  /* FIXME: this should be a part of the given file struct? */

  /* The token we return */
  char* token;

  /* Return if some evil mastermind gives us a NULL pointer */
  if(!token_length){
    if(indexarray_original){
      free(indexarray_original);
      indexarray = NULL;
      indexarray_original = NULL;
    }

    return NULL;
  }

  /* Return if input file is not open */
  if(f == NULL){
    if(indexarray_original){
      free(indexarray_original);
      indexarray = NULL;
      indexarray_original = NULL;
    }

    return NULL;
  }

  /* Read new line if needed and do other magic... */
  while(nip_read_line){
    /* Read the line and check for EOF */
    if(!(fgets(last_line, MAX_LINELENGTH, f))){

      if(indexarray_original){
        free(indexarray_original);
        indexarray = NULL;
        indexarray_original = NULL;
      }

      *token_length = 0;
      return NULL;
    }
    /* how many tokens in the line? */
    tokens_left = nip_count_tokens(last_line, NULL, 1, "(){}=,;", 7, 1, 1);

    /* Check whether the line has any tokens. If not, read a new line
     * (go to beginning of loop) */
    if(tokens_left > 0){
      /* Adjust pointer to the beginning of token boundary index array */
      if(indexarray_original){
        free(indexarray_original);
        indexarray = NULL;
        indexarray_original = NULL;
      }

      indexarray = nip_tokenise(last_line, tokens_left, 1, "(){}=,;", 7, 1, 1);
      indexarray_original = indexarray;

      /* If tokenise failed, return NULL, *token_length = 0 */
      if(!indexarray){
        nip_report_error(__FILE__, __LINE__, nip_check_error_type(), 1);
        *token_length = 0;
        return NULL;
      }

      /* Ignore lines that have COMMENT_CHAR as first non-whitespace char */
      if(last_line[indexarray[0]] == NIP_COMMENT_CHAR)
        nip_read_line = 1;
      else
        nip_read_line = 0;
    }
  }

  *token_length = indexarray[1] - indexarray[0];
  token = (char *) calloc(*token_length + 1, sizeof(char));
  if(!token){
    if(indexarray_original){
      free(indexarray_original);
      indexarray = NULL;
      indexarray_original = NULL;
    }
    nip_report_error(__FILE__, __LINE__, ENOMEM, 1);
    *token_length = -1;
    return NULL;
  }

  /* Copy the token */
  strncpy(token, &(last_line[indexarray[0]]), *token_length);

  /* NULL terminate the token. */
  token[*token_length] = '\0';

  indexarray += 2;
  
  /* If all the tokens have been handled, read a new line next time */
  if(--tokens_left == 0)
    nip_read_line = 1;

  /* Still some tokens left. Check for COMMENT_CHAR. */
  else if(last_line[indexarray[0]] == NIP_COMMENT_CHAR)
    nip_read_line = 1;

#ifdef PRINT_TOKENS
  printf("%s\n", token);
#endif

  return token;
}
