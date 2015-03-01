/*
 * Take a spectrum with Newport Matrix minispectrometer.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <strings.h>
#include <meas/meas.h>

void quit() {

  meas_matrix_close();
  printf("Close device\n");
  exit(1);
}


int main(int argc, char **argv) {

  double *x, *y;
  int i, size;
  char dummy[512], cont;

  if(argc != 4) {
    fprintf(stderr, "Usage: takesp2 <exptime in s> <number of averages> <0/1 continuous mode>\n");
    exit(1);
  }
  if(atoi(argv[3]) != 0) cont = 1;
  else cont = 0;

  meas_matrix_init();
  size = meas_matrix_size();
  if(!(x = (double *) malloc(sizeof(double) * size))) {
    fprintf(stderr, "Memory allocation failure.");
    exit(1);
  }
  if(!(y = (double *) malloc(sizeof(double) * size))) {
    fprintf(stderr, "Memory allocation failure.");
    exit(1);
  }
  signal(SIGINT, quit);

  meas_graphics_init(0, MEAS_GRAPHICS_XY, 512, 512, 1024, "takesp2");
  while (1) {
    bzero(y, sizeof(double) * size);
    meas_matrix_read(atof(argv[1]), atoi(argv[2]), y);
    
    /* for now (x = pixel #) */
    for (i = 0; i < size; i++) x[i] = meas_matrix_calib(i);
    
    meas_graphics_update_xy(0, x, y, size);
    meas_graphics_autoscale(0);
    meas_graphics_update();
    if(!cont) break;
  }

  meas_matrix_close();
  fprintf(stderr,"Press any enter to stop:");
  gets(dummy);
  meas_graphics_close();
  for (i = 0; i < size; i++) printf("%le %le\n", x[i], y[i]);  
  return 0;
}

