import pandas as pd
import matplotlib.pyplot as plt
import os

# Create plots directory if it doesn't exist
os.makedirs('plots', exist_ok=True)

# Read the data
csv_path = 'data_for_plotting/mpi/school_mpi_scaling_results.csv'
df = pd.read_csv(csv_path)

# Extract data
processes = df['Processes']
execution_time = df['ExecutionTime']
speedup = df['Speedup']
ideal_speedup = processes

# Create a figure with two subplots (side by side)
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# Plot 1: Execution Time
ax1.plot(processes, execution_time, marker='o', linestyle='-', color='b', linewidth=2, markersize=8)
ax1.set_xlabel('Number of MPI Processes', fontsize=12)
ax1.set_ylabel('Execution Time (seconds)', fontsize=12)
ax1.set_title('MPI Execution Time Scaling (School Cluster)', fontsize=14)
ax1.grid(True, linestyle='--', alpha=0.7)
ax1.set_xticks(processes)

# Plot 2: Speedup
ax2.plot(processes, speedup, marker='o', linestyle='-', color='g', linewidth=2, markersize=8, label='Actual Speedup')
ax2.plot(processes, ideal_speedup, linestyle='--', color='r', linewidth=2, label='Ideal Speedup')
ax2.set_xlabel('Number of MPI Processes', fontsize=12)
ax2.set_ylabel('Speedup Factor', fontsize=12)
ax2.set_title('MPI Speedup vs Ideal (School Cluster)', fontsize=14)
ax2.grid(True, linestyle='--', alpha=0.7)
ax2.set_xticks(processes)
ax2.legend(fontsize=12)

# Adjust layout and save
plt.tight_layout()
output_path = 'plots/school_mpi_scaling.png'
plt.savefig(output_path, dpi=300, bbox_inches='tight')
print(f"Plot saved successfully to {output_path}")
