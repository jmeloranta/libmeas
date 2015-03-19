#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <meas/meas.h>

#define NS 3

double xdata[NS], ydata[NS];

main() {

  int i;

  meas_graphics_init(0, MEAS_GRAPHICS_XY, 512, 512, 128, "test");
  meas_graphics_xscale(0, 0.0, (double) NS);
  meas_graphics_yscale(0, 0.0, 1.0);
  meas_graphics_xtitle(0, "Time");
  meas_graphics_ytitle(0, "Random #");
  for (i = 0; i < NS; i++) {
    xdata[i] = i;
    ydata[i] = drand48();
    meas_graphics_update_xy(0, xdata, ydata, i);
    meas_graphics_update();
    sleep(1);
  }
  meas_graphics_close(-1);
}
