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

echo '' 1>&2
echo '2. Test string tokenization for parsing models:' 1>&2

if=test/model.net
of=test/output3.txt
ef=test/expect3.txt
printf "potential (x1 | x2) { data={\n  0.9 0.1\n  0.1 0.9\n} }\n" > $if
echo "$if:" > $ef
echo "potential" >> $ef
echo "(" >> $ef
echo "x1" >> $ef
echo "|" >> $ef
echo "x2" >> $ef
echo ")" >> $ef
echo "{" >> $ef
echo "data" >> $ef
echo "=" >> $ef
echo "{" >> $ef
echo "0.9" >> $ef
echo "0.1" >> $ef
echo "0.1" >> $ef
echo "0.9" >> $ef
echo "}" >> $ef
echo "}" >> $ef

./test/parsertest $if > $of
assert $of $ef $LINENO
rm $if $of $ef


echo '' 1>&2
echo '3. Test string tokenization for parsing data:' 1>&2

if=test/data.csv
of=test/output4.txt
ef=test/expect4.txt
printf "\n" > $if
printf "x1,x2\n\n" >> $if
printf "1,foo\n" >> $if
printf "1,foo\n" >> $if
printf "1,bar\n" >> $if
printf "\n" >> $if
printf "2,foo\n" >> $if
printf "null,bar\n" >> $if
printf "\n\n" >> $if
printf "2,bar\n" >> $if

echo "Information about the data file:" > $ef
echo "Name: $if" >> $ef
echo "Separator: ," >> $ef
echo "is_open: 1" >> $ef
echo "label_line: 2" >> $ef
echo "Number of sequences: 3" >> $ef
echo "Sequence lengths: 3,2,1" >> $ef
echo "Number of columns: 2" >> $ef
echo "Columns:" >> $ef
echo "- Node x1, states: 2,1" >> $ef
echo "- Node x2, states: bar,foo" >> $ef
echo "" >> $ef
echo "Reading tokens one line at a time:" >> $ef
echo "1,foo" >> $ef
echo "1,foo" >> $ef
echo "1,bar" >> $ef
echo "2,foo" >> $ef
echo "null,bar" >> $ef
echo "2,bar" >> $ef

./test/datafiletest $if > $of
assert $of $ef $LINENO
rm $if $of $ef


echo '' 1>&2
echo '4. Test belief potentials:' 1>&2

of=test/output5.txt
ef=test/expect5.txt
echo "Mapping:" > $ef
parallel echo "P\({1}\, {2}\, {3}\) =" ::: 0 1 ::: 0 1 2 ::: 0 1 2 3 > test/index.txt
parallel echo "12*{1}+4*{2}+{3} | bc" ::: 0 1 ::: 0 1 2 ::: 0 1 2 3 > test/values.txt
paste -d' ' test/index.txt test/values.txt >> $ef
echo "Inverse mapping:" >> $ef
parallel echo "\-\-\> \[{3}\, {2}\, {1}\]" ::: 0 1 2 3 ::: 0 1 2 ::: 0 1 > test/index.txt
paste -d' ' test/values.txt test/index.txt >> $ef
echo "Marginalized:" >> $ef
parallel echo "P\({1}\, {2}\) =" ::: 0 1 ::: 0 1 2 3 > test/index.txt
parallel echo {} ::: 12 15 18 21 48 51 54 57 > test/values.txt
paste -d' ' test/index.txt test/values.txt >> $ef
echo "Copy:" >> $ef
parallel echo "P\({1}\, {2}\, {3}\) =" ::: 0 1 ::: 0 1 2 ::: 0 1 2 3 > test/index.txt
parallel echo "12*{1}+4*{2}+{3} | bc" ::: 0 1 ::: 0 1 2 ::: 0 1 2 3 > test/values.txt
paste -d' ' test/index.txt test/values.txt >> $ef
echo "Sum:" >> $ef
parallel echo "P\({1}\, {2}\, {3}\) =" ::: 0 1 ::: 0 1 2 ::: 0 1 2 3 > test/index.txt
parallel echo "2*\(12*{1}+4*{2}+{3}\) | bc" ::: 0 1 ::: 0 1 2 ::: 0 1 2 3 > test/values.txt
paste -d' ' test/index.txt test/values.txt >> $ef
echo "Margin:" >> $ef
echo "P(+, 0, +) = 120" >> $ef
echo "P(+, 1, +) = 184" >> $ef
echo "P(+, 2, +) = 248" >> $ef
echo "CPD:" >> $ef
parallel echo "P\({3}\, {2}\, {1}\) =" ::: 0 1 2 3 ::: 0 1 2 ::: 0 1 > test/index.txt
parallel echo {} ::: 0.000000 1.000000 0.200000 0.800000 0.285714 0.714286 0.071429 0.928571 0.227273 0.772727 0.300000 0.700000 0.125000 0.875000 0.250000 0.750000 0.312500 0.687500 0.166667 0.833333 0.269231 0.730769 0.323529 0.676471 > test/values.txt
paste -d' ' test/index.txt test/values.txt >> $ef
echo "Belief update from another potential:" >> $ef
parallel echo {} ::: 0.000000 0.250000 0.333333 0.333333 0.666667 0.416667 0.066667 0.254902 0.333333 0.333333 0.600000 0.411765 0.111111 0.259259 0.333333 0.333333 0.555556 0.407407 0.142857 0.263158 0.333333 0.333333 0.523810 0.403509 > test/values.txt
paste -d' ' test/index.txt test/values.txt >> $ef
echo "Belief update from single margin:" >> $ef
parallel echo {} ::: 0.000000 0.516667 0.333333 0.333333 0.322581 0.201613 0.137778 0.526797 0.333333 0.333333 0.290323 0.199241 0.229630 0.535802 0.333333 0.333333 0.268817 0.197133 0.295238 0.543860 0.333333 0.333333 0.253456 0.195246 > test/values.txt
paste -d' ' test/index.txt test/values.txt >> $ef
echo "Normalize:" >> $ef
echo "P(+, 0, +) = 1" >> $ef
echo "P(+, 1, +) = 1" >> $ef
echo "P(+, 2, +) = 1" >> $ef

./test/potentialtest > $of
assert $of $ef $LINENO
rm $of $ef test/index.txt test/values.txt


# TODO: some 9 layers or units more...

echo 'OK' 1>&2
