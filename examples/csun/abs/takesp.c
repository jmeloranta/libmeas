/*
 * Take a spectrum with Newport IS minispectrometer.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <meas/meas.h>

void quit() {

  meas_newport_is_close();
  printf("Close device\n");
  exit(1);
}

int main(int argc, char **argv) {

  double *x, *y;
  int i, xsize, mode, con = 0;
  char dummy[512];

  xsize = meas_newport_is_size();
  if(!(x = (double *) malloc(sizeof(double) * xsize))) {
    fprintf(stderr, "Memory allocation failure.");
    exit(1);
  }
  if(!(y = (double *) malloc(sizeof(double) * xsize))) {
    fprintf(stderr, "Memory allocation failure.");
    exit(1);
  }

  signal(SIGINT, quit);
  if(argc != 4) {
    fprintf(stderr, "Usage: takesp <exptime in s> <one shot:0, ext trigger:1, continuous:2> <number of averages>\n");
    exit(1);
  }
  mode = atoi(argv[2]);
  if(mode == 2) {
    con = 1;
    mode = 0;  /* could be ext trigger too (fixme) */
  }

  meas_newport_is_init();
  meas_graphics_init(0, MEAS_GRAPHICS_XY, 512, 512, xsize, "takesp");

  while(con) {
    meas_newport_is_read(atof(argv[1]), mode, atoi(argv[3]), y);
    
    /* for now (x = pixel #) */
    for (i = 0; i < xsize; i++) {
      x[i] = meas_newport_is_calib(i);
      //      printf("%le %le\n", x[i], y[i]);
    }

    meas_graphics_update_xy(0, x, y, xsize);
    meas_graphics_autoscale(0);
    meas_graphics_update();
  }
  fprintf(stderr,"Press any enter to stop:");
  gets(dummy);
  meas_graphics_close();  
  return 0;
}
