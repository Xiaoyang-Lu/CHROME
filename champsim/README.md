# ChampSim
ChampSim is a trace-based simulator for a microarchitecture study

# How To
You need to run build_champsim.sh, which is a shell script to build the simulator. The script takes a single input parameter, which is the L2's prefetching algorithm name. Other parameters can be set directly in the build_champsim.sh.   
The default configuration is:
```
BRANCH=perceptron
L1D_PREFETCHER=next_line
LLC_PREFETCHER=no
LLC_REPLACEMENT=chrome
NUM_CORE=4
```

Please Compile ChampSim using the following command, if you want to use ip_stride as your L2 prefetcher. 
```
$ ./build_champsim.sh ip_stride
```
This should create the ChampSim executable perceptron-next_line-ip_stride-no-chrome-4core inside bin/ directory. 

To run ChampSim, you need program instruction traces. This script takes ./scripts/traces.txt as input and downloads the traces.   
```
cd scripts/
./download_traces.sh
```
To run ChampSim, you could: 
```
./bin/perceptron-next_line-ip_stride-no-chrome-4core -warmup_instructions 10000000 -simulation_instructions 50000000 -traces ./traces/403.gcc-17B.champsimtrace.xz ./traces/403.gcc-17B.champsimtrace.xz ./traces/403.gcc-17B.champsimtrace.xz ./traces/403.gcc-17B.champsimtrace.xz
```
Traces used for the 3rd Data Prefetching Championship (DPC-3) can be found here. (https://dpc3.compas.cs.stonybrook.edu/champsim-traces/speccpu/) A set of traces used for the 2nd Cache Replacement Championship (CRC-2) can be found from this link. (http://bit.ly/2t2nkUj)
