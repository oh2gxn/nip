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
      echo "$(tput setaf 1)Failed$(tput sgr0)" 1>&2
      echo "$diffstr" 1>&2
      echo "File \"$0\", line $lineno" 1>&2 # Give name of file and line number.
      exit $E_ASSERT_FAILED
  else
      echo "$(tput setaf 2)Passed$(tput sgr0)"
      # return and continue executing the script.
  fi
}
#######################################################################

echo 'NIP tests' 1>&2
echo '0. Compile the source code' 1>&2
make


echo '' 1>&2
echo '1. Test low-level string tokenization:' 1>&2

if=test/input1.txt
of=test/output1.txt
ef=test/expect1.txt
head -n1 $if | ./test/stringtest > $of
tail -n+2 $if | ./test/stringtest >> $of
assert $of $ef $LINENO
rm $of


echo '' 1>&2
echo '2. Test string tokenization for parsing models:' 1>&2

if=test/input2.net
of=test/output2.txt
ef=test/expect2.txt
./test/parsertest $if > $of
assert $of $ef $LINENO
rm $of


echo '' 1>&2
echo '3. Test string tokenization for parsing data:' 1>&2

if=test/input3.txt
of=test/output3.txt
ef=test/expect3.txt
./test/datafiletest $if > $of
assert $of $ef $LINENO
rm $of


echo '' 1>&2
echo '4. Test belief potentials:' 1>&2

of=test/output4.txt
ef=test/expect4.txt
./test/potentialtest > $of
assert $of $ef $LINENO
rm $of


echo '' 1>&2
echo '5. Test join tree inference:' 1>&2

of=test/output5.txt
ef=test/expect5.txt
./test/cliquetest > $of
assert $of $ef $LINENO
rm $of


echo '' 1>&2
echo '6. Test graph operations:' 1>&2

of=test/output6.txt
ef=test/expect6.txt
./test/graphtest > $of
assert $of $ef $LINENO
rm $of


# TODO: some 9 layers or units more...

echo "$(tput setaf 2)OK$(tput sgr0)" 1>&2
