#!/bin/bash
# The name of the script
#SBATCH -J mpi_school_bench
# 10 minutes wall-clock time
#SBATCH -t 00:10:00
# Number of nodes
#SBATCH -N 1
# Request up to 32 tasks (cores) on the school cluster node
#SBATCH --ntasks-per-node=32
#SBATCH -e mpi_school_error.e
#SBATCH -o mpi_school_output.o

# Load MPI module if your school cluster requires it (e.g. OpenMPI or MPICH)
# Un-comment the line below if a module is needed:
# module load openmpi

echo "⚙️ Compiling MPI code for School Cluster..."
mpicc mpi_v1.c -o mpi_v1_bench -lm -O0 -DNX=80000 -DNSTEPS=20000

if [ $? -ne 0 ]; then
    echo "❌ Compilation failed!"
    exit 1
fi

echo "✅ Compilation successful. Starting School Cluster MPI Benchmarks..."

# Array of process counts to test (usually up to 32 cores on standard cluster nodes)
process_counts=(1 2 4 8 16 32)

for p in "${process_counts[@]}"; do
    echo "🏃‍♂️ Running with $p MPI processes..."
    
    # Run using mpirun or srun (depending on cluster configuration)
    mpirun -n $p ./mpi_v1_bench > mpi_school_timing_${p}p.log
    
    echo "✅ Finished $p processes. Log saved to mpi_school_timing_${p}p.log"
done

echo "🎉 All school cluster benchmarks complete!"
