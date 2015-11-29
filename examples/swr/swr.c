/*
 * Read SWR data from MFJ-226 as a function of frequency.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <meas/meas.h>

#define Z0 50.0    /* 50 Ohm transmission line */

int main(int argc, char **argv) {

  double freq, mag, phase;
  double complex refl;
  
  meas_mfj226_open(0, "/dev/ttyUSB0");
  if(argc != 4) {
    fprintf(stderr, "Usage: swr begin end step      (frequencies in Hz)\n");
    exit(0);
  }
  for(freq = atof(argv[1]); freq <= atof(argv[2]); freq += atof(argv[3])) {
    meas_mfj226_write(0, (int) freq);
    meas_mfj226_read(0, &mag, &phase);
    refl = mag * (cos(M_PI * phase / 180.0) + I * sin(M_PI * phase / 180.0));
    printf("%le %le\n", freq, (1.0 + cabs(refl)) / (1.0 - cabs(refl)));
  }
  meas_mfj226_close(0);
}

