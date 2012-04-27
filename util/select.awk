#   NIP - Dynamic Bayesian Network library
#   Copyright (C) 2012  Janne Toivola
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, see <http://www.gnu.org/licenses/>.


# Script for extracting data about one variable
#
# Example: gawk -f select.awk var=X original_data.txt > variable_X.txt
#
# Janne Toivola, 19.06.2006
# $Id: select.awk,v 1.1 2008-12-19 20:53:50 jatoivol Exp $

BEGIN{
# Field Separator, was ", "
  FS = " "

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
