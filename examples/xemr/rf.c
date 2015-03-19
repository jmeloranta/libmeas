#include <stdio.h>
#include <meas/meas.h>

main() {

  double f = 10.0;

  meas_hp5384_open(0, 0, 8);
  meas_sl_open();
  while(1) {    
    meas_sl_pts(f);
    printf("%le %le\n", f, meas_hp5384_read(0, HP5384_CHA));
    sleep(1);
    f += 0.1;
  }
  meas_sl_close();
}

