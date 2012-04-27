#!/bin/bash

# Quick example of NIP utilities
# Author: Janne Toivola

### 1. Compile the source code
make



### 2. Acquire some data: 
# Let's sample data from a known DBN model, 365 time series, 240 samples each.
# (this would be acquired elsewhere in an actual application) 
./util/nipsample examples/model.net 365 240 examples/data.txt

# ...but instead of assuming we can observe every variable,
# we hide some of them. Data is space delimited and L1 is the third column.
cut -d' ' -f3 examples/data.txt > examples/data-L1.txt

# Alternatively: hide just some of the data, see hide_values.awk
# gawk -f hide_values.awk examples/data.txt > examples/data-L1.txt



### 3. Use EM learning algorithm:
# fit (assumably unknown) parameters of empty.net from our incomplete data 
# in data-L1.txt. Magic termination parameters for EM: 
# threshold = 0.5, min. log. likelihood per time step = -1
./util/niptrain examples/empty.net examples/data-L1.txt 0.5 -1 examples/trained.net



### 4. Probabilistic inference
# See if the learned model can infer state of A1 from L1
./util/nipinference examples/trained.net examples/data-L1.txt A1 examples/data-A1-inferred.txt

# ...choose the maximum a posteriori state, instead of just probabilities
./util/nipmap examples/trained.net examples/data-L1.txt examples/data-A-map.txt

# ...see what happened
#cut -d' ' -f2 examples/data.txt > examples/data-A1.txt
#cut -d' ' -f1 examples/data-A-map.txt > examples/data-A1-map.txt
#diff examples/data-A1.txt examples/data-A1-map.txt > examples/predictions.txt
#echo 'examples/predictions.txt makes any sense (labels may have switched)?'



### 5. Cross validating accuracy
# Run leave-one-out cross validation test on inferring the state of one
# variable on one sample given the other variables in the rest of the data.  
#./util/nipbenchmark TODO...
