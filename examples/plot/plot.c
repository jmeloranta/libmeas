#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <meas/meas.h>

double xdata[128], ydata[128];

main() {

  int i;

  meas_graphics_init(0, MEAS_GRAPHICS_2D, 512, 512, 128, "testi");
  while(1) {
    for (i = 0; i < 128; i++) {
      xdata[i] = i;
      ydata[i] = drand48();
    }
    meas_graphics_update_xy(0, xdata, ydata, 128);
    meas_graphics_update();
  }
}
