#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

// Function to initialize the electric field with a Gaussian pulse
void initialize_fields(double *E, double *H) {
  // Center the Gaussian pulse in the middle of the domain
  double center = NX * DX / 2.0;
  for (int i = 0; i < NX; i++) {
    double x = i * DX;
    E[i] = exp(-0.005 * (x - center) * (x - center));
    H[i] = 0.0;
  }
}

// Function to update the magnetic field H
void update_H(double *E, double *H) {
  // Update H from 0 to NX-2 (using forward differences)
  for (int i = 0; i < NX - 1; i++) {
    H[i] = H[i] + (DT / DX) * (E[i + 1] - E[i]);
  }
  // Simple absorbing boundary condition:
  H[NX - 1] = H[NX - 2];
}

// Function to update the electric field E
void update_E(double *E, double *H) {
  // Update E from 1 to NX-1 (using backward differences)
  for (int i = 1; i < NX; i++) {
    E[i] = E[i] + (DT / DX) * (H[i] - H[i - 1]);
  }
  // Simple absorbing boundary condition:
  E[0] = E[1];
}

int main() {

  // initizalization timing
  clock_t start_time = clock();
  // Allocate fields
  double *E = (double *)malloc(NX * sizeof(double));
  double *H = (double *)malloc(NX * sizeof(double));
  if (E == NULL || H == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    return 1;
  }
  clock_t end_time = clock();
  double allocation_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
  printf("Allocation time: %f seconds\n", allocation_time);

  start_time = clock();
  // Initialize fields
  initialize_fields(E, H);
   end_time = clock();
  double initialization_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
  printf("Initialization time: %f seconds\n", initialization_time);

  start_time = clock();
  // save the electric field at various steps
  double **E_at_timestep = (double **) malloc(NPLOTTINGS * sizeof(double*));
  for (int i = 0; i < NPLOTTINGS; i++) {
    E_at_timestep[i] = (double *) malloc(NX * sizeof(double));
  }
  end_time = clock();
  double saving_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
  printf("Saving time: %f seconds\n", saving_time);

  // main loop timing
  start_time = clock();
  // Main FDTD loop
  for (int t = 0; t < NSTEPS; t++) {
    update_H(E, H);
    update_E(E, H);
    #ifdef ENABLE_PLOTTING
    if (t % (NSTEPS / NPLOTTINGS) == 0) {
      memcpy(E_at_timestep[t / (NSTEPS / NPLOTTINGS)], E, sizeof(double) * NX);
    }
    #endif
  }
  end_time = clock();
  double main_loop_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
  printf("Main loop time: %f seconds\n", main_loop_time);

  // TODO: I don't understand why we have this
  // // Output final snapshot of the electric field for verification
  // printf("Final electric field snapshot:\n");
  // for (int i = 0; i < NX; i++) {
  //   printf("%f ", E[i]);
  // }
  // printf("\n");

  #ifdef ENABLE_PLOTTING
  // for each of the steps, open a file to write them
  for (int i = 0; i < NPLOTTINGS; i++) {
    // open file inside data_for_plotting/serial
    char file_name[100];
    snprintf(file_name, sizeof(file_name), "data_for_plotting/serial/E_field_step_%d.txt", i);
    FILE* out_file = fopen(file_name, "w");
    // test file is not null
    if (out_file == NULL)
      {  
        printf("Error! Could not open file\n");
        exit(-1); // must include stdlib.h
      }

    for (int j = 0; j < NX; j++) {
      fprintf(out_file, "%f ", E_at_timestep[i][j]);
    }
    fclose(out_file);
  }
  #endif


  // get the index of the maximum electric field. Compare it to the expected shift from 
  // the initial center

  // verification timing
  start_time = clock();
  // get the maximum electric field index
  double max_E = 0.0;
  int max_index = 0;
  for (int i = 0; i < NX; i++) {
    if (E[i] >= max_E) {
      max_E = E[i];
      max_index = i;
    }
  }

  // expected shift from the initial center
  double expected_shift = (NSTEPS * DT) / DX; // distance = speed * time, speed = DX/DT
  double initial_center = NX * DX / 2.0;
  double expected_position = initial_center + expected_shift;
  double actual_position = max_index * DX;

  // compare them and write appropiate message
  printf("Expected position of the maximum electric field: %f\n", expected_position);
  printf("Actual position of the maximum electric field: %f\n", actual_position);
  if (fabs(expected_position - actual_position) < DX) {
    printf("The maximum electric field is at the expected position.\n");
  } else {
    // compute the relative error between them
    double relative_error = fabs(expected_position - actual_position) / expected_position;
    printf("The maximum electric field is NOT at the expected position.\n");
    printf("Relative error: %f%%\n", relative_error * 100);
  }

  for (int i = 0; i < NPLOTTINGS; i++) {
    free(E_at_timestep[i]);
  }
  free(E);
  free(E_at_timestep);
  free(H);
  end_time = clock();
  double verification_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
  printf("Verification time: %f seconds\n", verification_time);


  return 0;
}