#!/bin/bash

# Quick example of NIP utilities
# Author: Janne Toivola
echo 'NIP demo'
echo '1. Compile the source code'
make


echo ''
echo '2. Acquire some data:' 
# Let's sample data from a known DBN model, 365 time series, 24 samples each.
# (this would be acquired elsewhere in an actual application) 
./util/nipsample examples/model.net 365 24 examples/data1.txt
./util/nipsample examples/model.net 365 24 examples/data2.txt

# ...but instead of assuming we can observe all variables, we have only 
# noisy measurements M. Data is space delimited and M1 is the third column.
echo ''
echo '...but hide data about process P'
awk -f util/select.awk var=M1 examples/data1.txt > examples/data1-M1.txt
awk -f util/select.awk var=M1 examples/data2.txt > examples/data2-M1.txt

# Alternatively: hide just some of the data, see hide_values.awk
# awk -f util/hide_values.awk examples/data1.txt > examples/data1-M1.txt
# awk -f util/hide_values.awk examples/data2.txt > examples/data2-M1.txt


echo ''
echo '3. Use EM algorithm to learn a statistical model:'
# Learn (assumably unknown) parameters of empty.net from our incomplete data 
# in data-M1.txt. 
# Magic termination parameters for EM: 
# change threshold = 0.00001, 
# min. log. likelihood per time step = -1.1
./util/niptrain examples/empty.net examples/data1-M1.txt 0.00001 -1.1 examples/trained.net
echo 'See if examples/trained.net makes any sense compared to model.net.'


echo ''
echo '4. Probabilistic inference with the learned model:'
# See if the learned model can infer state of P1 from M1 in data2.txt
./util/nipinference examples/trained.net examples/data2-P1.txt P1 examples/data2-P1-inferred.txt

# ...choose the maximum a posteriori state, instead of just probabilities
./util/nipmap examples/trained.net examples/data2-M1.txt examples/data2-P-map.txt

# ...see what happened
awk -f util/select.awk var=P1 examples/data2.txt > examples/data2-P1.txt
awk -f util/select.awk var=P1 examples/data2-P-map.txt > examples/data2-P1-map.txt
paste -d' ' examples/data2-P1.txt examples/data2-P1-map.txt > examples/predictions.txt
echo 'See if examples/predictions.txt match at all.'
# NOTE: data2-P1-map.txt does NOT contain most likely state sequences
# (like in the result of the Viterbi algorithm), just independently most 
# probable states


### 5. Cross validating accuracy
# Run leave-one-out cross validation test on inferring the state of one
# variable on one sample given the other variables in the rest of the data.  
#./util/nipbenchmark examples/empty.net examples/data1.txt 0.00001 -1 P1 examples/crossvalidation.txt
#echo 'See if examples/crossvalidation.txt makes any sense.'
# TODO: include original data from data-P1.txt also?
