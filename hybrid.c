#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <mpi.h>

#ifndef NX
#define NX 80000
#endif
#ifndef NSTEPS
#define NSTEPS 20000
#endif
#define DX 1.0
#define DT 0.5
#define PI 3.141592653589793
#define NPLOTTINGS 50
#ifndef THREAD_COUNT
#define THREAD_COUNT 8
#endif

void initialize_fields(double *E, double *H) {
  double center = NX * DX / 2.0;
  for (int i = 0; i < NX; i++) {
    double x = i * DX;
    E[i] = exp(-0.005 * (x - center) * (x - center));
    H[i] = 0.0;
  }
}

void update_H_interior(double *E, double *H, int local_NX) {
  #pragma omp for simd schedule(static)
  for (int i = 2; i < local_NX - 2; i++) {
    H[i] = H[i] + (DT / DX) * (E[i + 1] - E[i]);
  }
}

void update_H_boundary(double *E, double *H, int local_NX, int rank, int size) {
  H[1] = H[1] + (DT / DX) * (E[2] - E[1]);
  if (rank < size - 1)
    H[local_NX - 2] = H[local_NX - 2] + (DT / DX) * (E[local_NX - 1] - E[local_NX - 2]);
  // absorbing condition
  if (rank == size - 1)
    H[local_NX - 1] = H[local_NX - 2];
}

void update_E_interior(double *E, double *H, int local_NX) {
  #pragma omp for simd schedule(static)
  for (int i = 2; i < local_NX - 2; i++) {
    E[i] = E[i] + (DT / DX) * (H[i] - H[i - 1]);
  }
}

void update_E_boundary(double *E, double *H, int local_NX, int rank, int size) {
  if (rank > 0)
    E[1] = E[1] + (DT / DX) * (H[1] - H[0]);
  E[local_NX - 2] = E[local_NX - 2] + (DT / DX) * (H[local_NX - 2] - H[local_NX - 3]);
  // absorbing condition
  if (rank == 0)
    E[0] = E[1];
}

int main() {
  int provided;
  MPI_Init_thread(NULL, NULL, MPI_THREAD_FUNNELED, &provided);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  // if (!getenv("OMP_NUM_THREADS"))
    // omp_set_num_threads(THREAD_COUNT);

  int has_extra    = (NX % size) > rank;
  int local_real   = NX / size + (has_extra ? 1 : 0);
  int global_start = rank * (NX / size) + (has_extra ? rank : NX % size);
  int local_NX     = local_real + 2;

  // Gatherv/Scatterv metadata — computed once, reused throughout
  int *recv_counts = (int *)malloc(size * sizeof(int));
  int *displs      = (int *)malloc(size * sizeof(int));
  for (int r = 0; r < size; r++) {
    recv_counts[r] = NX / size + (r < NX % size ? 1 : 0);
    displs[r]      = r * (NX / size) + (r < NX % size ? r : NX % size);
  }

  double allocation_start = MPI_Wtime();
  // rank 0 allocates global arrays for initialization, then scatters
  double *global_E = NULL;
  double *global_H = NULL;
  if (rank == 0) {
    global_E = (double *)malloc(NX * sizeof(double));
    global_H = (double *)malloc(NX * sizeof(double));
  }
  double *E = (double *)malloc(local_NX * sizeof(double));
  double *H = (double *)malloc(local_NX * sizeof(double));
  double allocation_end = MPI_Wtime();
  if (rank == 0) printf("Allocation time: %f seconds\n", allocation_end - allocation_start);

  double initialization_start = MPI_Wtime();
  if (rank == 0)
    initialize_fields(global_E, global_H);
  // scatter real cells into local[1..local_NX-2], leaving ghost cells [0] and [local_NX-1] empty
  MPI_Scatterv(global_E, recv_counts, displs, MPI_DOUBLE,
               &E[1], local_real, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Scatterv(global_H, recv_counts, displs, MPI_DOUBLE,
               &H[1], local_real, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  if (rank == 0) { free(global_E); free(global_H); }
  // ghost cells start as zero; filled on first halo exchange
  E[0] = E[local_NX-1] = 0.0;
  H[0] = H[local_NX-1] = 0.0;
  double initialization_end = MPI_Wtime();
  if (rank == 0) printf("Initialization time: %f seconds\n", initialization_end - initialization_start);

  double saving_start = MPI_Wtime();
  double **E_at_timestep = NULL;
  if (rank == 0) {
    E_at_timestep = (double **) malloc(NPLOTTINGS * sizeof(double*));
    for (int i = 0; i < NPLOTTINGS; i++)
      E_at_timestep[i] = (double *) malloc(NX * sizeof(double));
  }
  double saving_end = MPI_Wtime();
  if (rank == 0) printf("Saving time: %f seconds\n", saving_end - saving_start);

  MPI_Request reqs[4];
  int nreqs;

  // ── Main FDTD loop ────────────────────────────────────────────────────────
  MPI_Barrier(MPI_COMM_WORLD);
  double main_loop_start = MPI_Wtime();
  // #pragma omp parallel
  {
    for (int t = 0; t < NSTEPS; t++) {
      //#pragma omp single
      {
        nreqs = 0;
        if (rank > 0)       MPI_Irecv(&E[0],          1, MPI_DOUBLE, rank-1, 1, MPI_COMM_WORLD, &reqs[nreqs++]);
        if (rank < size-1)  MPI_Irecv(&E[local_NX-1], 1, MPI_DOUBLE, rank+1, 0, MPI_COMM_WORLD, &reqs[nreqs++]);
        if (rank < size-1)  MPI_Isend(&E[local_NX-2], 1, MPI_DOUBLE, rank+1, 1, MPI_COMM_WORLD, &reqs[nreqs++]);
        if (rank > 0)       MPI_Isend(&E[1],          1, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD, &reqs[nreqs++]);
      }

      update_H_interior(E, H, local_NX);

      //#pragma omp single
      {
        MPI_Waitall(nreqs, reqs, MPI_STATUSES_IGNORE);
        update_H_boundary(E, H, local_NX, rank, size);
      }

      //#pragma omp single
      {
        nreqs = 0;
        if (rank > 0)       MPI_Irecv(&H[0],          1, MPI_DOUBLE, rank-1, 1, MPI_COMM_WORLD, &reqs[nreqs++]);
        if (rank < size-1)  MPI_Irecv(&H[local_NX-1], 1, MPI_DOUBLE, rank+1, 0, MPI_COMM_WORLD, &reqs[nreqs++]);
        if (rank < size-1)  MPI_Isend(&H[local_NX-2], 1, MPI_DOUBLE, rank+1, 1, MPI_COMM_WORLD, &reqs[nreqs++]);
        if (rank > 0)       MPI_Isend(&H[1],          1, MPI_DOUBLE, rank-1, 0, MPI_COMM_WORLD, &reqs[nreqs++]);
      }

      update_E_interior(E, H, local_NX);

      //#pragma omp single
      {
        MPI_Waitall(nreqs, reqs, MPI_STATUSES_IGNORE);
        update_E_boundary(E, H, local_NX, rank, size);
      }

      #ifdef ENABLE_PLOTTING
      //#pragma omp single
      if (t % (NSTEPS / NPLOTTINGS) == 0) {
        int snap = t / (NSTEPS / NPLOTTINGS);
        MPI_Gatherv(&E[1], local_real, MPI_DOUBLE,
                    (rank == 0) ? E_at_timestep[snap] : NULL, recv_counts, displs, MPI_DOUBLE,
                    0, MPI_COMM_WORLD);
      }
      #endif
    }
  }

  double main_loop_end = MPI_Wtime();
  if (rank == 0) printf("Main loop time: %f seconds\n", main_loop_end - main_loop_start);

  // ── Verification ─────────────────────────────────────────────────────────
  double verification_start = MPI_Wtime();
  double *global_E_verify = (rank == 0) ? (double *)malloc(NX * sizeof(double)) : NULL;
  MPI_Gatherv(&E[1], local_real, MPI_DOUBLE,
              global_E_verify, (rank == 0) ? recv_counts : NULL, (rank == 0) ? displs : NULL, MPI_DOUBLE,
              0, MPI_COMM_WORLD);

  if (rank == 0) {
    double max_E = 0.0;
    int max_index = 0;
    for (int i = 0; i < NX; i++) {
      if (global_E_verify[i] >= max_E) {
        max_E = global_E_verify[i];
        max_index = i;
      }
    }
    double expected_shift    = (NSTEPS * DT) / DX;
    double initial_center    = NX * DX / 2.0;
    double expected_position = initial_center + expected_shift;
    double actual_position   = max_index * DX;

    printf("Expected position of the maximum electric field: %f\n", expected_position);
    printf("Actual position of the maximum electric field: %f\n", actual_position);
    if (fabs(expected_position - actual_position) < DX) {
      printf("The maximum electric field is at the expected position.\n");
    } else {
      double relative_error = fabs(expected_position - actual_position) / expected_position;
      printf("The maximum electric field is NOT at the expected position.\n");
      printf("Relative error: %f%%\n", relative_error * 100);
    }
    free(global_E_verify);
  }
  double verification_end = MPI_Wtime();
  if (rank == 0) printf("Verification time: %f seconds\n", verification_end - verification_start);

  #ifdef ENABLE_PLOTTING
  if (rank == 0) {
    for (int i = 0; i < NPLOTTINGS; i++) {
      char file_name[100];
      snprintf(file_name, sizeof(file_name), "data_for_plotting/hybrid/E_field_step_%d.txt", i);
      FILE* out_file = fopen(file_name, "w");
      if (out_file == NULL) { printf("Error! Could not open file\n"); exit(-1); }
      for (int j = 0; j < NX; j++)
        fprintf(out_file, "%f ", E_at_timestep[i][j]);
      fclose(out_file);
    }
    for (int i = 0; i < NPLOTTINGS; i++) free(E_at_timestep[i]);
    free(E_at_timestep);
  }
  #endif

  free(recv_counts);
  free(displs);
  free(E);
  free(H);

  MPI_Finalize();
  return 0;
}