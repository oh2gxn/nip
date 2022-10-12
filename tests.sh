#!/usr/bin/env bash

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
  difflen=$(printf "%s" "$diffstr" | wc -l)
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
echo '1. Test low-level string tokenization: src/nipstring.c' 1>&2

if=test/input1.txt
of=test/output1.txt
ef=test/expect1.txt
head -n1 $if | ./test/stringtest > $of
tail -n+2 $if | ./test/stringtest >> $of
assert $of $ef $LINENO
rm $of


echo '' 1>&2
echo '2. Test string tokenization for parsing models: src/nipparsers.c' 1>&2

if=test/input2.net
of=test/output2.txt
ef=test/expect2.txt
./test/parsertest $if > $of
assert $of $ef $LINENO
rm $of


echo '' 1>&2
echo '3. Test string tokenization for parsing data: src/nipparsers.c' 1>&2

if=test/input3.txt
of=test/output3.txt
ef=test/expect3.txt
./test/datafiletest $if > $of
assert $of $ef $LINENO
rm $of


echo '' 1>&2
echo '4. Test belief potentials: src/nippotential.c' 1>&2

of=test/output4.txt
ef=test/expect4.txt
./test/potentialtest > $of
assert $of $ef $LINENO
rm $of


echo '' 1>&2
echo '5. Test join tree inference: src/nipjointree.c' 1>&2

of=test/output5.txt
ef=test/expect5.txt
./test/cliquetest > $of
assert $of $ef $LINENO
rm $of


echo '' 1>&2
echo '6. Test graph operations: src/nipgraph.c' 1>&2

of=test/output6.txt
ef=test/expect6.txt
./test/graphtest > $of
assert $of $ef $LINENO
rm $of


echo '' 1>&2
echo '7. Test net file parser: src/huginnet.y' 1>&2

if=test/input7.net
of=test/output7.txt
ef=test/expect7.txt
./test/bisontest $if M1 2 0.9 > $of
assert $of $ef $LINENO
rm $of


echo '' 1>&2
echo '8. Test time series MAP inference: util/nipmap' 1>&2

if=test/input8.csv
of=test/output8.csv
ef=test/expect8.csv
./util/nipmap test/input7.net $if $of # 2> /dev/null
assert $of $ef $LINENO
rm $of


echo '' 1>&2
echo '9. Test time series inference: util/nipinference' 1>&2

if=test/input8.csv
of=test/output9.csv
ef=test/expect9.csv
./util/nipinference test/input7.net $if P1 $of # 2> /dev/null
assert $of $ef $LINENO
rm $of


echo '' 1>&2
echo '10. Test forward inference: util/nipnext' 1>&2

if=test/input10.csv
of=test/output10.csv
ef=test/expect10.csv
head -n3 test/input8.csv > $if
echo "null,null" >> $if
echo "" >> $if
echo "F,2" >> $if
echo "null,1" >> $if
echo "null,null" >> $if
./util/nipnext test/input7.net $if P1 > $of
assert $of $ef $LINENO
rm $if $of


echo '' 1>&2
echo '11. Test time series sampling: util/nipsample' 1>&2

if=test/input7.net
of=test/output11.csv
ef=test/expect11.csv
./util/nipsample $if 42 52 7 $of # 2> /dev/null
assert $of $ef $LINENO
rm $of


# TODO: some 3 layers or units more...

echo "$(tput setaf 2)OK$(tput sgr0)" 1>&2
