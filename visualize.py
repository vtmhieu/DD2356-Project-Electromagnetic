import matplotlib.pyplot as plt
import numpy as np

# File where the C program's output is saved
data_file = 'output.txt'

try:
    with open(data_file, 'r') as f:
        lines = f.readlines()
        
        # The values are on the second line (index 1)
        # because the first line is the text "Final electric field snapshot..."
        values_str = lines[1].strip().split()
        
        # Convert strings to floats
        E_field = [float(v) for v in values_str]

    # Plot the data
    plt.figure(figsize=(10, 6))
    plt.plot(E_field, label='Electric Field $E(x)$', color='blue', linewidth=2)
    
    plt.xlabel('Grid Point Index ($x$)', fontsize=12)
    plt.ylabel('Amplitude', fontsize=12)
    plt.title('1D FDTD Simulation - Final Electric Field', fontsize=14)
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()
    
    # Save the plot to an image file
    plt.savefig('field_plot.png', dpi=300, bbox_inches='tight')
    print("✅ Plot successfully saved as 'field_plot.png'")

except FileNotFoundError:
    print(f"❌ Error: '{data_file}' not found.")
    print("Please run your C program and save its output first:")
    print("Example: ./originalC > output.txt")
except Exception as e:
    print(f"❌ An error occurred: {e}")
