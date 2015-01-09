/* 
 * Ablation laser on A/B @ BNC565.
 * Diode on C.
 *
 * BNC565 driven by external trigger (pulse generator)
 * so that pulse bursts can be generated on C.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <meas/meas.h>
#include "conf.h"

#define SURELITE_QSWITCH 290E-6 

#define HEIGHT 640
#define WIDTH 480

unsigned char r[HEIGHT * WIDTH], g[HEIGHT * WIDTH], b[HEIGHT * WIDTH];

int main(int argc, char **argv) {

  double tstep, cur_time, t0, diode_drive, diode_length, diode_delay;
  char filebase[512], filename[512];
  int fd, diode_npulses;
  FILE *fp;

  printf("Enter output file name (0 = no save): ");
  scanf("%s", filebase);
  printf("Enter T0 (microsec): ");
  scanf("%le", &t0);
  t0 *= 1E-6;
  printf("Enter time step (microsec): ");
  scanf(" %le", &tstep);
  tstep *= 1E-6;
  printf("Diode drive voltage (V): ");
  scanf(" %le", &diode_drive);
  printf("Diode pulse length (microsec.): ");
  scanf(" %le", &diode_length);
  diode_length *= 1E-6;
  printf("Number of pulses: ");
  scanf(" %d", &diode_npulses);
  printf("Delay between diode pulses (microsec.): ");
  scanf(" %le", &diode_delay);

  printf("Running... press ctrl-c to stop.\n");

  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, HEIGHT, WIDTH, 0, "video");
  meas_bnc565_init(0, 0, BNC565);
  meas_bnc565_run(0, MEAS_BNC565_STOP); /* stop unit */

  meas_bnc565_trigger(0, MEAS_BNC565_TRIG_EXT, 2.0, MEAS_BNC565_TRIG_RISE); /* external trigger (2 V) - 10 Hz from wavetek */

  meas_bnc565_set(0, MEAS_BNC565_CHA, MEAS_BNC565_T0, 0.0, 10.0E-6, 5.0, MEAS_BNC565_POL_INV); /* negative logic / TTL */
  meas_bnc565_set(0, MEAS_BNC565_CHB, MEAS_BNC565_CHB, SURELITE_QSWITCH, 10.0E-6, 5.0, MEAS_BNC565_POL_INV);
  meas_bnc565_set(0, MEAS_BNC565_CHC, MEAS_BNC565_CHB, SURELITE_QSWITCH + SURELITE_DELAY, 1.0E-6, 5.0, MEAS_BNC565_POL_NORM);

  meas_bnc565_mode(0, MEAS_BNC565_CHA, MEAS_BNC565_MODE_BURST, 1, diode_npulses, diode_delay);
  meas_bnc565_mode(0, MEAS_BNC565_CHB, MEAS_BNC565_MODE_BURST, 1, diode_npulses, diode_delay);
  meas_bnc565_mode(0, MEAS_BNC565_CHC, MEAS_BNC565_MODE_BURST, diode_npulses, diode_npulses, diode_delay);

  meas_bnc565_run(0, MEAS_BNC565_RUN); /* start unit */

  fd = meas_video_open("/dev/video0", WIDTH, HEIGHT);
  if(filename[0] != '0') {
    sprintf(filename, "%s.info", filebase);
    if(!(fp = fopen(filename, "w"))) {
      fprintf(stderr, "Can't open file for writing.\n");
      exit(1);
    }
    fprintf(fp, "%.15le %.15le %lf %.15le %d %.15le\n", t0, tstep, diode_drive, diode_length, diode_npulses, diode_delay);
    fclose(fp);
  }
  for(cur_time = t0; ; cur_time += tstep) {
    printf("Diode delay = %le s.\n", cur_time);
    meas_bnc565_set(0, MEAS_BNC565_CHC, MEAS_BNC565_CHB, SURELITE_QSWITCH + SURELITE_DELAY + cur_time, 1.0E-6, 5.0, MEAS_BNC565_POL_NORM);
    meas_video_start(fd);
    meas_video_read_rgb(fd, r, g, b, 1);
    meas_video_stop(fd);
    meas_graphics_update_image(0, r, g, b);
    meas_graphics_update();
    sprintf(filename, "%s-%le.img", filebase, diode_delay);
    if(filename[0] != '0') {
      if(!(fp = fopen(filename, "w"))) {
	fprintf(stderr, "Error writing file.\n");
	exit(1);
      }
      fwrite((void *) r, sizeof(unsigned char) * HEIGHT * WIDTH, 1, fp);
      fwrite((void *) g, sizeof(unsigned char) * HEIGHT * WIDTH, 1, fp);
      fwrite((void *) b, sizeof(unsigned char) * HEIGHT * WIDTH, 1, fp);
      fclose(fp);
    }
  }
}
