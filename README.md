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
    gcc artifacts_for_testing/originalC_big_vector.c -o a.out -lm
    ./a.out
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
