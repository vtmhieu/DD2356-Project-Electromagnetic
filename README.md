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

3. **Model the inter-thread communication overhead and analyze how it may affect scalability**

    It is known that the computation time in the serial version is 

    T_serial = T_compute

    In the parallel version, we have:

    T_parallel = T_compute / P + T_overhead

    where P is the number of threads, and T_overhead is the time spent on inter-thread communication, synchronization and coherence cost (didn't know how this was called but Gemini helped me, or hallucinated hehe).

    inter-thread communication is done through the cache lines. So, whenever a thread is writting to a cache line, it is pulling it from the memory and blocking other thread's access to it.
    Whenever a core is writting to a cache line that is shared (multiple cores are working on the same data taken from L3), it is in fact trying to write to a copy it has in L1. Before it can do that, it sends a request for ownership to the other cores. Hence, if other cores were reading or tryint to write to that data, then they would have to flush and wait for the changes. This is what happens behind false sharing. This is the cost of coherence. A good thing is that in this case we don't have any inter-thread communication cost, and only have synchronization cost.

    **Synchronization cost**
    The syncrhonization cost for pragma omp for is the barrier in the end (as all threads have to finish before the next iteration).
    
    The serial code is taking approximatelly 5.3 seconds.
    That would mean, that with 8 threads running at the same time, the new hypotethical time would be 5.3/8 = 0.66 seconds.
    The time it takes the openMP version is around 0.865 seconds on averge. That would mean that the overhead is of around 0.2 seconds.
    This overhead comes from creating and deleting threads multiple times. It comes from the barriers that are at every `#pragma parallel for loop`.

    **How would it affect scalability?**

    This overhead will be a big problem for the increasing number of steps. At each step, the threads and barriers are created. With the increase of NX, the overhead will remain constant, as all threads intersect in the cache the same amount of times. Also, they spawn the same number of times. The only problem would be if some threads are slower than others and will finish a lot faster, but for this a `schedule(guided)` will sove it.
    We also tested how it would be to create all the threads a single time and use them afterwards

    **Regarding other implementations for OpenMP**
    We also tried multiple variants for the program. we tried spawning all the threads a single time, as you can see inside `artifacts_for_testing`:

    ```c
    // Main FDTD loop
    #pragma omp parallel 
    {
    for (int t = 0; t < NSTEPS; t++) {
        update_H(){
                #pragma omp for simd schedule(static)
                for (int i = 0; i < NX - 1; i++) {}
            }
        }
    }
    ```
    This resulted in consistently higher results by 0.1 seconds.


    **Regarding potential problems regarding performance**
    1. cold start
        the size of the whole memory is 80K elems * 8 bytes (as we have double). This means it would either way not be able to fir inside the L1 cache or the L2 cache. Cache will either way be evicted and fetched back NSTEPS times. There is no need to worry about the cold start, what we have a "cold start" tens of thousends of times.

    2. clock granularity
        the total amount of time approaches 1 second for the omp version, hence it will not be a problem (only if we decide to measure every loop). 
    3. Smart compiler
        we are printing the E vector in the end. So no optimization by the compiler.
    4. avoid inference by
        running on dardel and hoping for the best, also running multiple times.


4. **Evaluate parallel speedup compared to the serial code on the 3 computing systems**

    For this task, we decided to run each algorithm on every machine 3 times, and take for each one of them the lowest time (the STEAM method hehe). We used the tool utility of `time`.

    **Local Machine**
    for the openMP version: 0.847s
    To mention that the openMP version with 16 threads has the time: 0.855

    For the serial version: 5.318s

    Speedup: 5.318 / 0.847 = 6.27; for 8 threads. Great success!


    **Dardel**
    On dardel, one can as easily as run:
    `sbatch running_scripts/dardel_run.sh`
    This function will print inside `serial_timing.log` and `omp_timing.log`. Hence, let's take the lowest time
    for the openMP version (8 threads): 2.5 seconds
    For the openMP 4 threads version:3.01s

    Will not even test for 16 threads

    For the serial version:10.662s (the timinds differ by around 0.02 seconds, which is nice).

    Speedup: 


    **School Cluster**
    for the openMP version: 
    For the openMP 16 threads version:

    For the serial version:

    Speedup: 