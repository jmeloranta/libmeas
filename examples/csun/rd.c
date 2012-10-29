/*
 * Read output from photodiode via boxcar at SR245 port #1.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "csun.h"
#include <meas/meas.h>

#define MD 655350

int main(int argc, char **argv) {

  int i, j, loop_amt;
  char buf[2048];
  double xval[MD], yval[MD], qd;
  char graphix;
  char dummy[512];

  printf("Q-switch delay (in microsec): ");
  scanf(" %le", &qd);
  qd *= 1E-6;

  printf("Number of averages? ");
  scanf("%d", &loop_amt);
  printf("Use graphics output? (y/n) ");
  scanf("%s", &graphix);
  if(graphix != 'y' && graphix != 'n') {
    printf("Invalid input, exiting\n");
    exit(1);
  }

  /* Setup surelite */
  meas_bnc565_init(0, 0, BNC565);
  surelite_qswitch(qd);
  minilite_qswitch(qd);
  surelite_delay(0.0);
  minilite_delay(0.0);
  laser_set_delays();
  laser_start();

  meas_sr245_init(0, 0, SR245, NULL);
  meas_sr245_reset(0);
  meas_sr245_ports(0, 8); /* All declared as inputs */
  meas_sr245_ttl_mode(0, 1, 0); /* port #1 as trigger input */
  meas_sr245_mode(0, 1); /* triggered mode */

  if(graphix == 'y')
    meas_graphics_init(0, MEAS_GRAPHICS_XY, 512, 512, 65535, "rd");
  i = 0;
  while(1) {
    meas_misc_set_time();
    xval[i] = i;
    yval[i] = 0.0;
    meas_sr245_enable_trigger(0);
    for (j = 0; j < loop_amt; j++)
      yval[i] += meas_sr245_read(0, 1); /* From port #1 */
    meas_sr245_disable_trigger(0);
    yval[i] /= (double) loop_amt;
    if(graphix == 'y')
      meas_graphics_update_xy(0, xval, yval, i+1);
    if(i && graphix == 'y')
      meas_graphics_autoscale(0);
    if(graphix == 'y') meas_graphics_update();
    printf("%le %le\n", xval[i], yval[i]);
    i++;
    if(i >= MD) break;
  }
  printf("Press any enter to stop:");
  gets(dummy);  
  meas_graphics_close();  
}

