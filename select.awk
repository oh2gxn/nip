# Script for extracting data about one variable
#
# Example: gawk -f select.awk var=X original_data.txt > variable_X.txt
#
# Janne Toivola, 19.06.2006
# $Id: select.awk,v 1.1 2007-08-09 14:53:52 jatoivol Exp $

BEGIN{
# Field Separator is ", "
  FS = ", "

# Reset line counter (only non-empty lines)
  line = 0;
}

{
# replicate empty lines...
  if (NF == 0) {
    print $0;
  }
  
# ...else for each field
  for (i = 1; i <= NF; i++) {
    if (line == 0) {
      name[i] = $i; # establish field->name mapping
    }

    if (name[i] == var){
      printf "%s", $i;
    }
    if (i == NF) {
      printf "\n";
      line++;
    }
  }
}
