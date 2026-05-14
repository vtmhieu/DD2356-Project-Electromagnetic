# DD2356-Project-Electromagnetic
This application simulates electromagnetic wave propagation using the Finite-Difference Time-Domain (FDTD) method.

## Electromagnetics Test Case for Correctness

### Test Case Description

*   **Initial Condition:**
    The electric field is initialized with a Gaussian pulse centered at $x = NX \times DX/2$, while the magnetic field starts at zero.

*   **Expected Behavior:**
    With simple absorbing boundaries, the Gaussian pulse should split into two pulses that propagate outward at the numerical speed determined by the CFL condition. For our parameters:

    *   **Propagation Speed:** The wave speed is approximately $c \approx DX/DT$ (typically normalized to 1 in a properly scaled FDTD simulation).
    *   **Pulse Displacement:** After NSTEPS time steps, the peak of the pulse is expected to have shifted by roughly $NSTEPS/2$ grid points in each direction.

### Verification Steps

1.  **Visual Inspection:**
    Plot the electric field $E(x)$ at various time steps (e.g., $t = 0, t = NSTEPS/2, t = NSTEPS$). Confirm that the Gaussian pulse splits into two symmetric pulses propagating in opposite directions.

    For this, we decided to modify the serial code in order to save the electric field on 10 instances inside a matrix. After the computation finishes, we are writting the results to files inside `data_for_plotting/serial`. Afterwards, we are plotting the data using `visualize.py`. The picture will be saved inside `plots` folder.

    ```bash
    gcc originalC.c -o originalC -lm
    ./originalC
    python visualize.py
    ```

2.  **Peak Position Check:**
    Determine the index of the maximum electric field at the final time step. Compare this position with the expected shift from the initial center.

    For this, reference the folder `artifacts_for_testing`. File `originalC_big_vector.c`. For this, we inputed the field being a lot larger than the distance that the wave is meant to travel. If the wave would hit a wall, it would be "reflected". Hence, `artifacts_for_testing.c` can be compiled, and ran and show the serial sanity check. We kept it as an artifact, as, if we decreased the size of the vector for later testing purpopses, we couldn't compute the peak position from math.

    ```bash
    gcc artifacts_for_testing/originalC_big_vector.c -o originalC_big_vector -lm
    ./originalC_big_vector
    ```

3.  **Quantitative Comparison:**
    Optionally, calculate the relative error between the simulated peak amplitude and a reference solution (analytical or from a validated simulation). The error should remain within acceptable bounds (e.g., less than 5%) for a proper CFL setting.

    It can be seen in the aforementioned file, the compilation and running steps. The results are the same for both methods. In case they will no longe rbe the same, `originalC_big_vector.c` will compute the relative error `abs(right_val - rong_val)/right_val`.

## Project Plan & Logistics

### 📋 Tasks Chosen
*   **Baseline:** Validate serial code, profile performance, estimate speedup bounds.
*   **OpenMP:** Parallelize E/H updates, analyze scalability and overhead.
*   **MPI:** Domain decomposition, halo exchange, strong/weak scaling.
*   **Hybrid MPI+OpenMP:** Tune (P, N), analyze scalability for fixed cores.
*   **Optimization:** Address bottlenecks, improve memory access, GPU offloading.

### ⚠️ Challenges
*   Data dependencies between E and H updates.
*   MPI communication overhead (halo exchange).
*   Memory bandwidth limitation (stencil-like pattern).
*   Hybrid tuning complexity.
*   GPU data transfer overhead.

### 📅 Timeline

| Phase | Focus Area |
| :--- | :--- |
| **Week 1** | Baseline, profiling, correctness |
| **Week 2** | MPI + OpenMP implementation |
| **Week 3** | Optimization, vectorization |
| **Week 4** | Scaling tests, evaluation |

### 👥 Responsibilities
*   **Hieu:** OpenMP, plotting
*   **Serban:** MPI, correctness
*   **Teddy:** Optimization, profiling

### Baseline C/C++ Implementation

1. **Identify useful metrics for performance profiling and present their measurement on the 3 computing systems**


### OpenMP 
1. **Identify compute-intensive parts and implement OpenMP parallelization**
    In order to identify compute-intensive parts, we need to look statically first.

    First of all, we have the `initialize_fields()` function that initializes `Nx` data points. This is done in O(N), where N is Nx.

    The main loop of the code is
    ```c
    for (int t = 0; t < NSTEPS; t++) {
        update_H(E, H);
        update_E(E, H);
    }
    ```
    With `update_H` and `update_E` being composed of a single for loop that is computationally expensive
    ```c
    for (int i = 0; i < NX - 1; i++) 
        H[i] = H[i] + (DT / DX) * (E[i + 1] - E[i]);
    ```

    The time complexity of this operation is O(Nx * Nsteps). Both of them being bigger than 1K. Hence, in the for loops where we are using NPLOTTINGS, we can directly ignore it and consider it a constant.

    The last part
    ```c
    // for each of the steps, open a file to write them
    for (int i = 0; i < NPLOTTINGS; i++)
        for (int j = 0; j < NX; j++)
            fprintf(out_file, "%f ", E_at_timestep[i][j]);
    ```
    is going to have an overhead that is pretty big, given that opening multiple files and writing inside them linearly is costly.

    Another solution would have been to write on every iteration of the main loop, which is unacceptable.
    
    Now, let's use some tooling to see the computationally expensive parts.

    ```bash
    gcc -g -O0 originalC.c -o originalC -lm
    perf record -g ./originalC
    perf report
    ```

    The result was:

    | Function | Percentage |
    |----------|------------|
    | main | 96.74% |
    | fprintf | 54.30% |
    | update_E | 16.84% |
    | update_H | 15.53% |
    | fclose | 3.78% |
    | printf | 1.30% |
    | fopen | 1.25% |

    Most of the time is being spent writing to file, so let's increase the Nx and the number of steps

    | Function | Percentage |
    |----------|------------|
    | main | 99.76% |
    | update_E | 46.68% |
    | update_H | 46.15% |
    | fprintf | 6.36% |

    This is better. This would mean that the minimum amount of time that we could occupy afterwards would be 10%.

    After some modifications to the c code (some precompilation techniques that can hide the printf functions), we got rid of the 
    printf time. We put some precompilation techniques in order to hide the printf. Now most of the compute time will be used by the electric and magnetic flux functions.

    Now, we can implement the openMP paralelization. As we can see, there is a data dependency. First of all, update__E depends on update_H (vector E depends on vector H). If at step i, vector E is updated, then at step i + 1, vector H will be updated based on vector E. So, we cannot paralelize the step for loop. 

    By looking at the update_E function:
    ```c
    for (int i = 1; i < NX; i++) {
        E[i] = E[i] + (DT / DX) * (H[i] - H[i - 1]);
    }
    ```
    We can see that it is very easy to paralelize it. We can use as many threads as there are elements inside the vector (or 4 times less threads and also use vector registers). 
    ```c
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < NX - 1; i++)
    ```
    We are using static as all operations take the same amount of time, on average all threads will finish simultaneously (on a supercomputer on a node that does only this).

    so, for running the program, one can do:

    ```bash
    gcc -fopenmp openMP_v1.c -o openMP_v1 -lm -DTHREAD_COUNT=16
    ./openMP_v1
    ```
2. **Verify the correctness of implementation**
    For printing the output, the flag `-DENABLE_PLOTTING` needs to be added. This will save the electric field in 50 instances. Afterwards the plotting script can be ran, which will save the plot in `plots/field_plot_omp.png`.

    ```bash
    gcc -fopenmp openMP_v1.c -o openMP_v1 -lm -DTHREAD_COUNT=16 -DENABLE_PLOTTING
    ./openMP_v1
    python visualize.py
    ```

    We can see that we have the same result. This can be seen either by looking at the plot, or by reading the expected position and the actual position (the relative error), which is the same for the serial and the omp versions:

    ```bash
    Expected position of the maximum electric field: 50000.000000
    Actual position of the maximum electric field: 49994.000000
    The maximum electric field is NOT at the expected position.
    Relative error: 0.012000%
    ```
    Hence, it is correct.



