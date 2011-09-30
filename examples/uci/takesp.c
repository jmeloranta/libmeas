/*
 * Take a spectrum with PI-MAX gated CCD.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "uci.h"
#include <meas/meas.h>

#define GAIN 100

void quit() {

  meas_pi_max_close();
  printf("Close device\n");
  exit(1);
}

int main(int argc, char **argv) {

  double *x, *y;
  int i, size;

  size = meas_pi_max_size();
  if(!(x = (double *) malloc(sizeof(double) * size)))
    err("Memory allocation failure.");
  if(!(y = (double *) malloc(sizeof(double) * size)))
    err("Memory allocation failure.");

  signal(SIGINT, quit);
  if(argc != 4) {
    fprintf(stderr, "Usage: takesp <exptime in s> <ext trigger; 1/0> <number of averages>\n");
    exit(1);
  }

  meas_pi_max_init();
  meas_pi_max_gain(GAIN);
  meas_pi_max_gate_mode(2); /* Intensifier mode */
  meas_pi_max_exposure_mode(4); /* Wait for ext trigger */
  meas_pi_max_mode(1); /* normal mode */


  meas_pi_max_read(atof(argv[1]), atoi(argv[2]), atoi(argv[3]), y);

  /* for now (x = pixel #) */
  for (i = 0; i < size; i++) x[i] = newport_is_calib(i);

  meas_graphics_init(1, NULL, NULL);
  meas_graphics_xscale(0, size, 100, 50);
  meas_graphics_yscale(0, 60000, 5000, 1000);
  meas_graphics_update(0, 1, x, y, size);
  return 0;
}

