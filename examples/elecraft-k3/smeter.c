#include <stdio.h>
#include <math.h>
#include <meas/meas.h>

// #define PLOT_NOISE_VARIANCE

#define N 8192
#define AVE 10

double xdata[N], ydata[N];

int main(int argc, char **argv) {

  int fd, value, i, j;
  char buf[512];
  double maxval = -1E99, minval = 1E99, val, ma, mi;
  
  fd = meas_rs232_open("/dev/ttyS0", MEAS_B38400 | MEAS_NOHANDSHAKE);
  meas_graphics_open(0, MEAS_GRAPHICS_XY, 512, 512, 128, "test");
  meas_graphics_xtitle(0, "Time");
  meas_graphics_ytitle(0, "S-meter");

  for(i = 0; i < N; i++) {
    meas_graphics_xscale(0, 0.0, (double) i);
    val = 0.0;
#ifdef PLOT_NOISE_VARIANCE
    mi = 1E99; ma = -mi;
    for (j = 0; j < AVE; j++) {
      meas_rs232_write(fd, "smh;", 4);
      meas_rs232_readeot(fd, buf, ";");
      sscanf(buf, "SMH%d;", &value);
      if(value < mi) mi = value;
      if(value > ma) ma = value;
    }
    val = (ma - mi) / ma;
#else
    for (j = 0; j < AVE; j++) {
      meas_rs232_write(fd, "smh;", 4);
      meas_rs232_readeot(fd, buf, ";");
      sscanf(buf, "SMH%d;", &value);
      val += value;
    }
    val /= (double) AVE;
#endif
    if(val > maxval)
      maxval = val;
    if(val < minval)
      minval = val;
    meas_graphics_yscale(0, minval, maxval);
    xdata[i] = i;
    ydata[i] = val;
    meas_graphics_update_xy(0, xdata, ydata, i);
    meas_graphics_update();
  }

  meas_rs232_close(fd);
  meas_graphics_close(-1);
}
