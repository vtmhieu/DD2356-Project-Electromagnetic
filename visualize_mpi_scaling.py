import pandas as pd
import matplotlib.pyplot as plt
import os

# Read the benchmarking results
csv_path = 'data_for_plotting/mpi/mpi_scaling_results.csv'
if not os.path.exists(csv_path):
    print(f"Error: {csv_path} not found. Please run the benchmark script first.")
    exit(1)

df = pd.read_csv(csv_path)

# Create a plots directory if it doesn't exist
os.makedirs('plots', exist_ok=True)

# Set up the figure with two subplots (Execution Time and Speedup/Efficiency)
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))

# --- Plot 1: Execution Time ---
ax1.plot(df['Processes'], df['ExecutionTime'], marker='o', linestyle='-', color='b', linewidth=2, markersize=8)
ax1.set_title('MPI Execution Time vs. Number of Processes', fontsize=14)
ax1.set_xlabel('Number of Processes (p)', fontsize=12)
ax1.set_ylabel('Execution Time (seconds)', fontsize=12)
ax1.grid(True, linestyle='--', alpha=0.7)
ax1.set_xticks(df['Processes'])

# --- Plot 2: Speedup ---
ax2.plot(df['Processes'], df['Speedup'], marker='s', linestyle='-', color='g', linewidth=2, markersize=8, label='Actual Speedup')
# Plot ideal speedup line
ax2.plot(df['Processes'], df['Processes'], linestyle='--', color='r', linewidth=2, label='Ideal Linear Speedup')

ax2.set_title('MPI Speedup vs. Number of Processes', fontsize=14)
ax2.set_xlabel('Number of Processes (p)', fontsize=12)
ax2.set_ylabel('Speedup (S = T1 / Tp)', fontsize=12)
ax2.grid(True, linestyle='--', alpha=0.7)
ax2.set_xticks(df['Processes'])
ax2.legend(fontsize=12)

# Save the plot
plt.tight_layout()
plot_path = 'plots/mpi_scaling.png'
plt.savefig(plot_path, dpi=300)
print(f"🎉 Plot successfully saved to: {plot_path}")
