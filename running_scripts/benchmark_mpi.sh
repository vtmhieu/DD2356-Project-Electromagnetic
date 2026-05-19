#!/bin/bash

# Define where to save the results
OUTPUT_DIR="data_for_plotting/mpi"
OUTPUT_CSV="${OUTPUT_DIR}/mpi_scaling_results.csv"

# Ensure output directories exist
mkdir -p ${OUTPUT_DIR}
mkdir -p plots

# Compile the code with O0 optimization to match the Serial/OpenMP tests
echo "⚙️  Compiling mpi_v1.c with -O0 optimization..."
mpicc mpi_v1.c -o mpi_v1_bench -lm -O0 -DNX=80000 -DNSTEPS=20000

# Check if compilation succeeded
if [ $? -ne 0 ]; then
    echo "❌ Compilation failed!"
    exit 1
fi

echo "✅ Compilation successful. Starting benchmark..."
echo "Processes,ExecutionTime,Speedup,Efficiency" > $OUTPUT_CSV

# Run with 1 process to get the baseline time
echo "🏃‍♂️ Running baseline (1 process)..."
OUTPUT=$(mpirun -n 1 ./mpi_v1_bench)
BASE_TIME=$(echo "$OUTPUT" | grep "Main loop time" | awk '{print $4}')

if [ -z "$BASE_TIME" ]; then
    echo "❌ Failed to capture execution time for 1 process."
    exit 1
fi

echo "⏱️  Baseline time: $BASE_TIME seconds"
echo "1,$BASE_TIME,1.0,1.0" >> $OUTPUT_CSV

# Loop over different numbers of processes (powers of 2)
# Added --oversubscribe in case your local machine doesn't have 16 physical cores
for p in 2 4 8 16; do
    echo "🏃‍♂️ Running with $p processes..."
    
    # Run and capture output. 
    OUTPUT=$(mpirun -n $p --oversubscribe ./mpi_v1_bench 2>/dev/null)
    
    # Extract the main loop time using grep and awk
    TIME=$(echo "$OUTPUT" | grep "Main loop time" | awk '{print $4}')
    
    # Check if time was extracted properly
    if [ -z "$TIME" ]; then
        echo "⚠️  Warning: Error extracting time for $p processes. Skipping..."
        continue
    fi
    
    # Calculate speedup and efficiency using awk
    SPEEDUP=$(awk -v t1=$BASE_TIME -v tp=$TIME 'BEGIN { printf "%.4f", t1/tp }')
    EFFICIENCY=$(awk -v s=$SPEEDUP -v p=$p 'BEGIN { printf "%.4f", s/p }')
    
    echo "⏱️  Time: $TIME s | Speedup: $SPEEDUP x | Efficiency: $EFFICIENCY"
    
    # Append to CSV
    echo "$p,$TIME,$SPEEDUP,$EFFICIENCY" >> $OUTPUT_CSV
done

echo "🎉 Benchmarking complete. Results saved in $OUTPUT_CSV"
echo "You can now plot the results!"
