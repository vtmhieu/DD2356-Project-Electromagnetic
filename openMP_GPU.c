#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

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
#define DTDX (DT / DX)

void initialize_fields(double *E, double *H) {
  double center = NX * DX / 2.0;
  for (int i = 0; i < NX; i++) {
    double x = i * DX;
    E[i] = exp(-0.005 * (x - center) * (x - center));
    H[i] = 0.0;
  }
}

int main() {

  double *E = (double *)malloc(NX * sizeof(double));
  double *H = (double *)malloc(NX * sizeof(double));
  if (E == NULL || H == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    return 1;
  }

  initialize_fields(E, H);

  double **E_at_timestep = (double **)malloc(NPLOTTINGS * sizeof(double *));
  for (int i = 0; i < NPLOTTINGS; i++)
    E_at_timestep[i] = (double *)malloc(NX * sizeof(double));

  // copy everything to the gpu
  #pragma omp target data map(tofrom: E[0:NX], H[0:NX])
  {
    for (int t = 0; t < NSTEPS; t++) {

      #pragma omp target teams distribute parallel for schedule(static)
      for (int i = 0; i < NX - 1; i++) {
        H[i] = H[i] + DTDX * (E[i + 1] - E[i]);
      }

      // do this directly on the gpu as we should't bring data back and forth
      #pragma omp target 
      H[NX - 1] = H[NX - 2];

      #pragma omp target teams distribute parallel for schedule(static)
      for (int i = 1; i < NX; i++) {
        E[i] = E[i] + DTDX * (H[i] - H[i - 1]);
      }

      // idem, do on gpu
      #pragma omp target
      E[0] = E[1];

      #ifdef ENABLE_PLOTTING
      if (t % (NSTEPS / NPLOTTINGS) == 0) {
        #pragma omp target update from(E[0:NX])
        memcpy(E_at_timestep[t / (NSTEPS / NPLOTTINGS)], E, sizeof(double) * NX);
      }
      #endif
    }
  }

  // verify the correctness
  double max_E = 0.0;
  int max_index = 0;
  for (int i = 0; i < NX; i++) {
    if (E[i] >= max_E) {
      max_E = E[i];
      max_index = i;
    }
  }

  double expected_shift    = (NSTEPS * DT) / DX;
  double initial_center    = NX * DX / 2.0;
  double expected_position = initial_center + expected_shift;
  double actual_position   = max_index * DX;

  printf("Expected position of the maximum electric field: %f\n", expected_position);
  printf("Actual position of the maximum electric field:   %f\n", actual_position);
  if (fabs(expected_position - actual_position) < DX) {
    printf("The maximum electric field is at the expected position.\n");
  } else {
    double relative_error = fabs(expected_position - actual_position) / expected_position;
    printf("The maximum electric field is NOT at the expected position.\n");
    printf("Relative error: %f%%\n", relative_error * 100);
  }

  // do the plot
  #ifdef ENABLE_PLOTTING
  for (int i = 0; i < NPLOTTINGS; i++) {
    char file_name[100];
    snprintf(file_name, sizeof(file_name), "data_for_plotting/gpu/E_field_step_%d.txt", i);
    FILE *out_file = fopen(file_name, "w");
    if (out_file == NULL) { printf("Error! Could not open file\n"); exit(-1); }
    for (int j = 0; j < NX; j++)
      fprintf(out_file, "%f ", E_at_timestep[i][j]);
    fclose(out_file);
  }
  #endif

  for (int i = 0; i < NPLOTTINGS; i++) free(E_at_timestep[i]);
  free(E_at_timestep);
  free(E);
  free(H);

  return 0;
}