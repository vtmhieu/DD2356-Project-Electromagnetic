import matplotlib.pyplot as plt
import numpy as np
import re

# File where the C program's output is saved

# open all the files in data_for_plotting/serial and plot them in one figure and save the figure as field_plot.png
folder = 'data_for_plotting/serial/'
output_graph = 'plots/field_plot.png'

# get all the files in the folder
import os
files = [f for f in os.listdir(folder) if f.endswith('.txt')]
files.sort(key=lambda f: int(re.search(r'\d+', f).group()))
num_of_files = len(files)

# Create a single figure for all plots
plt.figure(figsize=(10, 4 * num_of_files))

# for each file, read the data and plot it
for i, file in enumerate(files):
    data_file = os.path.join(folder, file)
    try:
        with open(data_file, 'r') as f:
            lines = f.readlines()
            
            # The values are on the second line (index 1)
            # because the first line is the text "Final electric field snapshot..."
            values_str = lines[0].strip().split()
            
            # Convert strings to floats
            E_field = [float(v) for v in values_str]

        # Plot the data
        plt.subplot(num_of_files, 1, i + 1) 
        plt.plot(E_field, label=f'File: {file}', color='blue', linewidth=2)
        
        plt.xlabel('Grid Point Index ($x$)')
        plt.ylabel('Amplitude')
        plt.title(f'Snapshot: {file}')
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.legend(loc='upper right')

    except FileNotFoundError:
        print(f"Error: '{data_file}' not found.")
        print("Please run your C program and save its output first:")
        print("Example: ./originalC > output.txt")
    except Exception as e:
        print(f"An error occurred: {e}")

# Adjust layout to prevent labels from overlapping
plt.tight_layout()
plt.savefig(output_graph, dpi=300, bbox_inches='tight')
print(f"Plot saved as '{output_graph}'")
