# Creates incomplete data by hiding part of the values 
# in a data file. Values are hidden randomly according to given observation
# probability for each variable. 
# 
# 100% means that all the samples are kept the way they are
#
# 0% means that the variable is entirely hidden and the variable is renamed
# with prefix "hidden_". Samples are kept intact.
#
# Negative observation probability causes the variable be completely hidden
# and left out of the output.
#
# Otherwise only some of the samples are hidden randomly 
# and designated with "null".
#
#
# Example: gawk -f hide_values.awk original_data.txt > partial_data.txt
#
# Janne Toivola, 28.04.2006
# - No special treatment for completely hidden variables: 09.10.2006
# - Drop completely hidden variables from the output: 11.11.2006
# $Id: hide_values.awk,v 1.1 2008-12-19 20:53:50 jatoivol Exp $

BEGIN{
# Field Separator is ", "
  FS = ", "

# Reset line counter (only non-empty lines)
  line = 0;

# Observation probabilities (integer in [0,100])
  prob["A0"] =  -1;
  prob["A1"] = 100;
  prob["B0"] =  -1;
  prob["B1"] =  50;
  prob["C0"] =  -1;
  prob["C1"] =  -1;
  prob["D0"] =  -1;
  prob["D1"] =  20;
  prob["V0"] =  -1;
  prob["S0"] =  -1;
  prob["Y0"] =  -1;
  prob["V1"] =  -1;
  prob["S1"] =  10;
  prob["Y1"] = 100;
# feel free to add variables...
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
      if (prob[name[i]] == 0){
	printf "hidden_%s", $i;
      }
      else if (prob[name[i]] > 0){
	printf "%s", $i;
      }
    }
    else{ # Randomly observed sample
      p = int(rand() * 100); # integer in [0,99]
# print observed samples and completely hidden variables
      if (p < prob[name[i]] || prob[name[i]] == 0) {
	printf "%s", $i; # observation
      }
      else if (prob[name[i]] > 0){
	printf "null"; # missing value
      }
    }
    if (i == NF) {
      printf "\n";
      line++;
    }
    else if (prob[name[i]] >= 0){
      printf FS;
    }
  }
}
