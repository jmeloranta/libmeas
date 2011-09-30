/*
 * Take a spectrum with Newport IS minispectrometer. (but store to out.dat)
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "csun.h"
#include <meas/meas.h>

void quit() {

  meas_newport_is_close();
  printf("Close device\n");
  exit(1);
}

int main(int argc, char **argv) {

  double *x, *y;
  int i, size;
  FILE *fp;

  size = meas_newport_is_size();
  if(!(x = (double *) malloc(sizeof(double) * size))) {
    fprintf(stderr,"Memory allocation failure.");
    exit(1);
  }
  if(!(y = (double *) malloc(sizeof(double) * size))) {
    fprintf(stderr,"Memory allocation failure.");
    exit(1);
  }

  signal(SIGINT, quit);
  if(argc != 4) {
    fprintf(stderr, "Usage: takesp <exptime in s> <ext trigger; 1/0> <number of averages>\n");
    exit(1);
  }

  meas_newport_is_init();
  meas_newport_is_read(atof(argv[1]), atoi(argv[2]), atoi(argv[3]), y);

  /* for now (x = pixel #) */
  for (i = 0; i < size; i++) x[i] = meas_newport_is_calib(i);

  fp = fopen("out.dat", "w");
  for (i = 0; i < size; i++)
    fprintf(fp, "%le %le\n", x[i], y[i]);
  fclose(fp);
  return 0;
}

