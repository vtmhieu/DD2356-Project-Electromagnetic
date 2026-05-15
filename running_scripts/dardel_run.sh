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


cc originalC.c -o originalC -lm

echo "Starting Serial Benchmarks"
for i in $(seq 1 10); do 
  { time ./originalC > /dev/null; } 2>> serial_timing.log
done

cc -fopenmp openMP_v1.c -o openMP_v1 -lm -DTHREAD_COUNT=8

echo "Starting OpenMP Benchmarks"
for i in $(seq 1 10); do 
  { time ./openMP_v1 > /dev/null; } 2>> omp_timing_8t.log
done

cc -fopenmp openMP_v1.c -o openMP_v1 -lm -DTHREAD_COUNT=16

echo "Starting OpenMP Benchmarks"
for i in $(seq 1 10); do 
  { time ./openMP_v1 > /dev/null; } 2>> omp_timing_16t.log
done


cc -fopenmp openMP_v1.c -o openMP_v1 -lm -DTHREAD_COUNT=32

echo "Starting OpenMP Benchmarks"
for i in $(seq 1 10); do 
  { time ./openMP_v1 > /dev/null; } 2>> omp_timing_32t.log
done


cc -fopenmp openMP_v1.c -o openMP_v1 -lm -DTHREAD_COUNT=64

echo "Starting OpenMP Benchmarks"
for i in $(seq 1 10); do 
  { time ./openMP_v1 > /dev/null; } 2>> omp_timing_64t.log
done