#!/bin/bash

# Runs test programs in test/*.c
# Quits and returns non-zero in case of any errors.
# Author: Janne Toivola

#######################################################################
# https://tldp.org/LDP/abs/html/debugging.html#ASSERT
assert () #  If condition false, exit from script with appropriate error message.
{
  E_PARAM_ERR=98
  E_ASSERT_FAILED=99

  if [ -z "$3" ] # Not enough parameters passed to assert() function.
  then
    return $E_PARAM_ERR # No damage done.
  fi

  lineno=$3

  diffstr=$(diff $1 $2)
  difflen=$(printf "$diffstr" | wc -l)
  if [ $difflen -gt 0 ]
  then
    echo "Assertion failed:" 1>&2
    echo "$diffstr" 1>&2
    echo "File \"$0\", line $lineno" 1>&2 # Give name of file and line number.
    exit $E_ASSERT_FAILED
  # else
  #   return
  #   and continue executing the script.
  fi
}
#######################################################################

echo 'NIP tests' 1>&2
echo '1. Compile the source code' 1>&2
make

echo '' 1>&2
echo '2. Test string tokenization for parsing models:' 1>&2

of=test/output1.txt
ef=test/expect1.txt
echo "foo (bar) { };" | ./test/stringtest > $of
printf "foo (bar) { };\n\n0 3 4 9 10 11 12 14 \n1 lines, 4 words, 16 chars\n4 first words:\nfoo\n(bar)\n{\n};\n" > $ef
assert $of $ef $LINENO
rm $of $ef

of=test/output2.txt
ef=test/expect2.txt
printf "potential (A | B) {\n  data = (( 1.0 0.0 )\n  (0.1 0.9));\n}\n" | ./test/stringtest > $of
printf "potential (A | B) {\n\n0 9 10 12 13 14 15 17 18 19 \n  data = (( 1.0 0.0 )\n\n2 6 7 8 9 11 12 15 16 19 20 21 \n  (0.1 0.9));\n\n2 6 7 13 \n}\n\n0 1 \n4 lines, 14 words, 62 chars\n14 first words:\npotential\n(A\n|\nB)\n{\ndata\n=\n((\n1.0\n0.0\n)\n(0.1\n0.9));\n}\n" > $ef
assert $of $ef $LINENO
rm $of $ef

# TODO: some 10 layers or units more...
#echo '' 1>&2
#echo '3. Test string tokenization for parsing models:' 1>&2

echo 'OK' 1>&2
