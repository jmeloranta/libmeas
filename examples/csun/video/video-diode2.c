/* 
 * Ablation laser on A/B @ BNC565.
 * Burst trigger output on C (-> DG535 burst generation).
 * BNC565 driven by external trigger (pulse generator).
 *
 * DG535 trigger in from BNC565 channel C. Generates a burst
 * of flash pulses on A+B.
 *
 * Note: Surelite q-switch is set automagically by the laser.
 *       This means that it will output at full power!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <meas/meas.h>
#include "conf.h"

#define SURELITE_FIRE_DELAY 180.0E-6

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
  diode_delay *= 1E-6;

  printf("Running... press ctrl-c to stop.\n");

  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, HEIGHT, WIDTH, 0, "video");

  meas_bnc565_init(0, 0, BNC565);
  meas_dg535_init(0, 0, DG535);

  meas_bnc565_run(0, MEAS_BNC565_STOP);
  meas_dg535_run(0, MEAS_DG535_STOP);

  /* DG535 is the master clock at 10 Hz */
  meas_dg535_trigger(0, MEAS_DG535_TRIG_INT, 10.0, 0, MEAS_DG535_IMP_50);

  /* Surelite triggering */
  meas_dg535_set(0, MEAS_DG535_CHA, MEAS_DG535_T0, 0.0, 4.0, MEAS_DG535_POL_INV, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHB, MEAS_DG535_CHA, 10E-6, 4.0, MEAS_DG535_POL_INV, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHAB, 0, 0.0, 4.0, MEAS_DG535_POL_INV, MEAS_DG535_IMP_50);

  /* BNC565 triggering */
  meas_dg535_set(0, MEAS_DG535_CHC, MEAS_DG535_T0, SURELITE_FIRE_DELAY + t0, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHD, MEAS_DG535_CHC, 10E-6, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHCD, 0, 0.0, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);

  /* BNC565 to external trigger */
  meas_bnc565_trigger(0, MEAS_BNC565_TRIG_EXT, 2.0, MEAS_BNC565_TRIG_RISE);

  meas_bnc565_set(0, MEAS_BNC565_CHA, MEAS_BNC565_T0, 0.0, diode_length, diode_drive, MEAS_BNC565_POL_NORM);
  meas_bnc565_mode(0, MEAS_BNC565_CHA, MEAS_BNC565_MODE_BURST, diode_npulses, diode_npulses, diode_delay);

  meas_bnc565_run(0, MEAS_BNC565_RUN); /* start unit */
  meas_dg535_run(0, MEAS_DG535_RUN); /* start unit */

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
    meas_dg535_set(0, MEAS_DG535_CHC, MEAS_DG535_T0, SURELITE_FIRE_DELAY + cur_time, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
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
