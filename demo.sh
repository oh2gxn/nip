#!/bin/bash

# Quick example of NIP utilities
# Author: Janne Toivola
echo 'NIP demo'
echo '0. Compile the source code'
make


echo ''
echo '1. Acquire some data:' 
# Let's sample data from a known DBN model, 365 time series, 24 samples each.
# (this would be acquired elsewhere in an actual application) 
./util/nipsample examples/model.net 111 365 24 examples/data-train.csv
./util/nipsample examples/model.net 222 365 24 examples/data-test.csv

# ...but instead of assuming we can observe all variables, we have only 
# noisy measurements M. Data is comma delimited and M1 is the third column.
echo ''
echo '...but hide data about process P'
awk -f util/select.awk var=M1 examples/data-train.csv > examples/data-train-M1.csv
awk -f util/select.awk var=M1 examples/data-test.csv > examples/data-test-M1.csv

# Alternatively: hide just some of the data, see hide_values.awk
# awk -f util/hide_values.awk examples/data-train.csv > examples/data-train-M1.csv
# awk -f util/hide_values.awk examples/data-test.csv > examples/data-test-M1.csv


echo ''
echo '2. Use EM algorithm to learn a statistical model:'
# Learn (assumably unknown) parameters of empty.net
# from our incomplete data in data-M1.csv.
# Magic termination parameters for EM: 
# change threshold = 0.00001, 
# min. log. likelihood per time step = -1.1
./util/niptrain examples/empty.net examples/data-train-M1.csv 42 0.00001 -1.1 examples/trained.net > examples/learning-curve.csv
echo 'See if examples/trained.net makes any sense compared to model.net.'
echo 'See examples/learning-curve.csv for any promising convergence.'


echo ''
echo '3. Probabilistic inference with the learned model:'
# See if the learned model can infer state of P1 from M1 in data-test.csv
./util/nipinference examples/trained.net examples/data-test-M1.csv P1 examples/data-test-P1-inferred.csv

# ...choose the maximum a posteriori state, instead of just probabilities
./util/nipmap examples/trained.net examples/data-test-M1.csv examples/data-test-P-map.csv

# ...see what happened
awk -f util/select.awk var=P1 examples/data-test.csv > examples/data-test-P1.csv
awk -f util/select.awk var=P1 examples/data-test-P-map.csv > examples/data-test-P1-map.csv
paste -d' ' examples/data-test-P1.csv examples/data-test-P1-map.csv > examples/predictions.csv
echo 'See if examples/predictions.csv match at all.'
# NOTE: data-test-P1-map.csv does NOT contain most likely state sequences
# (like in the result of the Viterbi algorithm), just independently most 
# probable states


#echo ''
#echo '4. Cross validating accuracy:'
# Run leave-one-out cross validation test on inferring the state of one
# variable on one sample given the other variables in the rest of the data.  
#./util/nipbenchmark examples/empty.net examples/data-test.csv 0.00001 -1 P1 examples/crossvalidation.csv
#echo 'See if examples/crossvalidation.csv makes any sense.'
# TODO: include original data from data-P1.csv also?
