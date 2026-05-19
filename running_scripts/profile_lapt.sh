#!/bin/bash -l
# The -l above is required to get the full environment with modules

# The name of the script is myjob
#SBATCH -J myjob
# 10 minutes wall-clock time will be given to this job
#SBATCH -t 00:10:00
#SBATCH -A edu26.DD2356
# Number of nodes
#SBATCH -p shared
# we have to specify from the start the number of cores we will use (which is computed based on nr of tasks, cpu's per task, nodes)
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --nodes=1
#SBATCH -e error_file.e

gcc originalC.c -o originalC -lm -O0 -DNX=400 -DNSTEPS=1000
# Basic timing
time ./originalC

# hardware counters (cache misses, instructions, cycles)
perf stat -e cache-misses,cache-references,L1-dcache-loads,L1-dcache-load-misses,cycles,instructions ./originalC

sudo perf stat -e L1-dcache-load-misses,L1-dcache-store-misses,LLC-loads,LLC-load-misses ./originalC

sudo perf stat -e branch-instructions,branch-misses ./originalC

sudo perf stat -e dTLB-loads,dTLB-load-misses,dTLB-stores,dTLB-store-misses ./originalC

# find hotspots
perf record -g ./originalC
perf report


valgrind --tool=cachegrind ./originalC
cg_annotate cachegrind.out.<pid>