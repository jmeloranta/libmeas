#include <stdio.h>
#include <meas/meas.h>

main() {

  meas_er023_open(0, 0, 1);
  meas_er023_calibrate(0, 100.0, 10.0, 180.0);
  meas_er023_resonator(0, 2);
  meas_er023_modulation_amp(0, 10.0);
  meas_er023_gain(0, 1e4);
  meas_er023_timeconstant(0, 0.1);
  meas_er023_conversiontime(0, 0.1);
  meas_er023_phase(0, 180);
  while(1) {
    printf("%d\n", meas_er023_read(0));
    sleep(1);
  }
}

