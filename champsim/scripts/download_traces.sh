#!/bin/bash

mkdir -p $PWD/../traces
i=0
while read LINE
do
    wget -P $PWD/../traces -c https://dpc3.compas.cs.stonybrook.edu/champsim-traces/speccpu/$LINE
done < traces.txt
