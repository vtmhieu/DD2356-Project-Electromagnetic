#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NX
#define NX 80000      // Total number of spatial points
#endif
#ifndef NSTEPS
#define NSTEPS 20000 // Number of time steps
#endif
#define DX 1.0      // Spatial step size
#define DT 0.5      // Time step size (should satisfy the CFL condition)
#define PI 3.141592653589793
#define NPLOTTINGS 50

// Function to initialize the fields
void initialize_fields_mpi(double *E, double *H, int local_start, int N_local) {
  double center = NX * DX / 2.0;
  for (int i = 1; i <= N_local; i++) {
    int global_idx = local_start + i - 1;
    double x = global_idx * DX;
    E[i] = exp(-0.005 * (x - center) * (x - center));
    H[i] = 0.0;
  }
  // Initialize halos to 0
  E[0] = 0.0; E[N_local + 1] = 0.0;
  H[0] = 0.0; H[N_local + 1] = 0.0;
}

int main(int argc, char **argv) {
  int rank, size;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int remainder = NX % size;
  // Compute local bounds for domain decomposition
  int local_start = rank * (NX / size) + (rank < remainder ? rank : remainder);
  int local_end = local_start + (NX / size) + (rank < remainder ? 1 : 0);
  int N_local = local_end - local_start;

  // Allocate fields with 2 extra elements for halos (index 0 and index N_local + 1)
  double *E = (double *)malloc((N_local + 2) * sizeof(double));
  double *H = (double *)malloc((N_local + 2) * sizeof(double));
  if (E == NULL || H == NULL) {
    fprintf(stderr, "Memory allocation failed on rank %d\n", rank);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  #ifdef ENABLE_PLOTTING
  double **E_at_timestep = NULL;
  int *recvcounts = NULL;
  int *displs = NULL;
  
  if (rank == 0) {
    E_at_timestep = (double **)malloc(NPLOTTINGS * sizeof(double*));
    for (int i = 0; i < NPLOTTINGS; i++) {
      E_at_timestep[i] = (double *)malloc(NX * sizeof(double));
    }
    recvcounts = (int *)malloc(size * sizeof(int));
    displs = (int *)malloc(size * sizeof(int));
    for (int p = 0; p < size; p++) {
      int p_start = p * (NX / size) + (p < remainder ? p : remainder);
      int p_end = p_start + (NX / size) + (p < remainder ? 1 : 0);
      recvcounts[p] = p_end - p_start;
      displs[p] = p_start;
    }
  }
  #endif

  MPI_Barrier(MPI_COMM_WORLD);
  double start_time = MPI_Wtime();

  initialize_fields_mpi(E, H, local_start, N_local);

  double init_time = MPI_Wtime() - start_time;

  MPI_Barrier(MPI_COMM_WORLD);
  double loop_start_time = MPI_Wtime();

  for (int t = 0; t < NSTEPS; t++) {
    // Halo exchange for E: Update H requires E[i+1] from right neighbor
    MPI_Sendrecv(&E[1], 1, MPI_DOUBLE, rank > 0 ? rank - 1 : MPI_PROC_NULL, 0,
                 &E[N_local + 1], 1, MPI_DOUBLE, rank < size - 1 ? rank + 1 : MPI_PROC_NULL, 0,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Update H
    for (int i = 1; i <= N_local; i++) {
      int global_idx = local_start + i - 1;
      if (global_idx == NX - 1) continue;
      H[i] = H[i] + (DT / DX) * (E[i + 1] - E[i]);
    }
    // Absorbing boundary condition for the rightmost element
    if (rank == size - 1) {
      H[N_local] = H[N_local - 1];
    }

    // Halo exchange for H: Update E requires H[i-1] from left neighbor
    MPI_Sendrecv(&H[N_local], 1, MPI_DOUBLE, rank < size - 1 ? rank + 1 : MPI_PROC_NULL, 1,
                 &H[0], 1, MPI_DOUBLE, rank > 0 ? rank - 1 : MPI_PROC_NULL, 1,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Update E
    for (int i = 1; i <= N_local; i++) {
      int global_idx = local_start + i - 1;
      if (global_idx == 0) continue;
      E[i] = E[i] + (DT / DX) * (H[i] - H[i - 1]);
    }
    // Absorbing boundary condition for the leftmost element
    if (rank == 0) {
      E[1] = E[2];
    }

    #ifdef ENABLE_PLOTTING
    if (t % (NSTEPS / NPLOTTINGS) == 0) {
      int plot_idx = t / (NSTEPS / NPLOTTINGS);
      // Gather local segments of E to the root rank
      MPI_Gatherv(&E[1], N_local, MPI_DOUBLE, 
                  rank == 0 ? E_at_timestep[plot_idx] : NULL, 
                  recvcounts, displs, MPI_DOUBLE, 
                  0, MPI_COMM_WORLD);
    }
    #endif
  }

  MPI_Barrier(MPI_COMM_WORLD);
  double loop_time = MPI_Wtime() - loop_start_time;

  #ifdef ENABLE_PLOTTING
  if (rank == 0) {
    for (int i = 0; i < NPLOTTINGS; i++) {
      char file_name[100];
      snprintf(file_name, sizeof(file_name), "data_for_plotting/mpi/E_field_step_%d.txt", i);
      FILE* out_file = fopen(file_name, "w");
      if (out_file != NULL) {
        for (int j = 0; j < NX; j++) {
          fprintf(out_file, "%f ", E_at_timestep[i][j]);
        }
        fclose(out_file);
      } else {
        printf("Error opening file: %s. Please create data_for_plotting/mpi directory.\n", file_name);
      }
    }
  }
  #endif

  // Verification step: Find max E and its global index locally
  double local_max_E = -1.0;
  int local_max_idx = -1;
  for (int i = 1; i <= N_local; i++) {
    if (E[i] >= local_max_E) {
      local_max_E = E[i];
      local_max_idx = local_start + i - 1;
    }
  }

  // Gather all local maxes and indices to rank 0 to accurately find the rightmost peak
  double *all_max_E = NULL;
  int *all_max_idx = NULL;
  if (rank == 0) {
    all_max_E = (double *)malloc(size * sizeof(double));
    all_max_idx = (int *)malloc(size * sizeof(int));
  }

  MPI_Gather(&local_max_E, 1, MPI_DOUBLE, all_max_E, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Gather(&local_max_idx, 1, MPI_INT, all_max_idx, 1, MPI_INT, 0, MPI_COMM_WORLD);

  if (rank == 0) {
    double global_max_E = -1.0;
    int global_max_idx = -1;
    for (int p = 0; p < size; p++) {
      if (all_max_E[p] >= global_max_E) {
        global_max_E = all_max_E[p];
        global_max_idx = all_max_idx[p];
      }
    }

    double expected_shift = (NSTEPS * DT) / DX;
    double initial_center = NX * DX / 2.0;
    double expected_position = initial_center + expected_shift;
    double actual_position = global_max_idx * DX;

    printf("--- Performance Metrics ---\n");
    printf("Initialization time: %f seconds\n", init_time);
    printf("Main loop time: %f seconds\n", loop_time);
    printf("--- Verification ---\n");
    printf("Expected position: %f\n", expected_position);
    printf("Actual position: %f\n", actual_position);
    
    if (fabs(expected_position - actual_position) < DX) {
      printf("The maximum electric field is at the expected position.\n");
    } else {
      double relative_error = fabs(expected_position - actual_position) / expected_position;
      printf("The maximum electric field is NOT at the expected position.\n");
      printf("Relative error: %f%%\n", relative_error * 100);
    }
    
    free(all_max_E);
    free(all_max_idx);
  }

  // Cleanup
  free(E);
  free(H);
  #ifdef ENABLE_PLOTTING
  if (rank == 0) {
    for (int i = 0; i < NPLOTTINGS; i++) free(E_at_timestep[i]);
    free(E_at_timestep);
    free(recvcounts);
    free(displs);
  }
  #endif

  MPI_Finalize();
  return 0;
}
