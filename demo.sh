#!/bin/bash

# Quick example of NIP utilities
# Author: Janne Toivola

### 1. Compile the source code
make



### 2. Acquire some data: 
# Let's sample data from a known DBN model, 365 time series, 24 samples each.
# (this would be acquired elsewhere in an actual application) 
./util/nipsample examples/model.net 365 24 examples/data1.txt
./util/nipsample examples/model.net 365 24 examples/data2.txt

# ...but instead of assuming we can observe every variable,
# we hide some of them. Data is space delimited and L1 is the third column.
gawk -f util/select.awk var=L1 examples/data1.txt > examples/data1-L1.txt
gawk -f util/select.awk var=L1 examples/data2.txt > examples/data2-L1.txt

# Alternatively: hide just some of the data, see hide_values.awk
# gawk -f util/hide_values.awk examples/data1.txt > examples/data1-L1.txt
# gawk -f util/hide_values.awk examples/data2.txt > examples/data2-L1.txt



### 3. Use EM learning algorithm:
# fit (assumably unknown) parameters of empty.net from our incomplete data 
# in data-L1.txt. Magic termination parameters for EM: 
# change threshold = 0.00001, min. log. likelihood per time step = -0.7
./util/niptrain examples/empty.net examples/data1-L1.txt 0.00001 -0.7 examples/trained.net
echo 'See if examples/trained.net makes any sense compared to model.net.'



### 4. Probabilistic inference
# See if the learned model can infer state of A1 from L1 in data2.txt
./util/nipinference examples/trained.net examples/data2-L1.txt A1 examples/data2-A1-inferred.txt

# ...choose the maximum a posteriori state, instead of just probabilities
./util/nipmap examples/trained.net examples/data2-L1.txt examples/data2-A-map.txt

# ...see what happened
gawk -f util/select.awk var=A1 examples/data2.txt > examples/data2-A1.txt
gawk -f util/select.awk var=A1 examples/data2-A-map.txt > examples/data2-A1-map.txt
paste -d' ' examples/data2-A1.txt examples/data2-A1-map.txt > examples/predictions.txt
echo 'See if examples/predictions.txt match at all.'



### 5. Cross validating accuracy
# Run leave-one-out cross validation test on inferring the state of one
# variable on one sample given the other variables in the rest of the data.  
#./util/nipbenchmark examples/empty.net examples/data1.txt 0.00001 -1 A1 examples/crossvalidation.txt
#echo 'See if examples/crossvalidation.txt makes any sense.'
# TODO: include original data from data-A1.txt also?
