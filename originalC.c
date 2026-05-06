#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define NX 400      // Total number of spatial points
#define NSTEPS 1000 // Number of time steps
#define DX 1.0      // Spatial step size
#define DT 0.5      // Time step size (should satisfy the CFL condition)
#define PI 3.141592653589793

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
  // Allocate fields
  double *E = (double *)malloc(NX * sizeof(double));
  double *H = (double *)malloc(NX * sizeof(double));
  if (E == NULL || H == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    return 1;
  }

  // Initialize fields
  initialize_fields(E, H);

  // Main FDTD loop
  for (int t = 0; t < NSTEPS; t++) {
    update_H(E, H);
    update_E(E, H);
  }

  // Output final snapshot of the electric field for verification
  printf("Final electric field snapshot (first 20 values):\n");
  for (int i = 0; i < 20; i++) {
    printf("%f ", E[i]);
  }
  printf("\n");

  free(E);
  free(H);
  return 0;
}