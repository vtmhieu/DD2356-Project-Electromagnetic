#!/bin/bash -l
# The -l above is required to get the full environment with modules

# The name of the script is mpi_bench
#SBATCH -J mpi_bench
# 15 minutes wall-clock time will be given to this job
#SBATCH -t 00:15:00
#SBATCH -A edu26.DD2356
# Number of nodes
#SBATCH -p shared
# Request 64 tasks (MPI processes) so we can scale up to 64
#SBATCH --ntasks=64
#SBATCH --nodes=1
#SBATCH -e mpi_error_file.e

# On Dardel, the 'cc' compiler wrapper automatically links MPI 
# as long as the cray-mpich module is loaded (default).
echo "⚙️ Compiling MPI code..."
cc mpi_v1.c -o mpi_v1_bench -lm -O0 -DNX=80000 -DNSTEPS=20000

if [ $? -ne 0 ]; then
    echo "❌ Compilation failed!"
    exit 1
fi

echo "✅ Compilation successful. Starting MPI Benchmarks on Dardel..."

# Array of process counts to test
process_counts=(1 2 4 8 16 32 64)

for p in "${process_counts[@]}"; do
    echo "🏃‍♂️ Running with $p MPI processes..."
    
    # We use srun instead of mpirun on Dardel Slurm clusters.
    # The output is redirected to separate log files for analysis.
    srun -n $p ./mpi_v1_bench > mpi_timing_${p}p.log
    
    echo "✅ Finished $p processes. Log saved to mpi_timing_${p}p.log"
done

echo "🎉 All benchmarks complete!"
