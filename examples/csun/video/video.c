#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <meas/meas.h>
#include "csun.h"

unsigned char r[640 * 480], g[640 * 480], b[640 * 480];

int main(int argc, char **argv) {

  double tstep, delay, t0;
  char filebase[512], filename[512];
  int fd, aves;
  FILE *fp;

  printf("Enter output file name (0 = no save): ");
  scanf("%s", filebase);
  printf("Enter number of averages: ");
  scanf("%d", &aves);
  printf("Enter T0 (ns): ");
  scanf("%le", &t0);
  printf("Enter time step (ns): ");
  scanf(" %le", &tstep);
  printf("Running... press ctrl-c to stop.\n");
  tstep *= 1E-9;
  t0 *= 1E-9;
  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, 640, 480, 0, "video");
  meas_bnc565_init(0, 0, BNC565);
  meas_dg535_init(0, 0, DG535);
  surelite_qswitch(290E-6);
  minilite_qswitch(157E-6);
  fd = meas_video_open("/dev/video0");
  surelite_delay(0.0);
  minilite_delay(delay);
  laser_set_delays();
  laser_start();
  if(filename[0] != '0') {
    sprintf(filename, "%s.info", filebase);
    if(!(fp = fopen(filename, "w"))) {
      fprintf(stderr, "Can't open file for writing.\n");
      exit(1);
    }
    fprintf(fp, "%d %.15le %.15le\n", aves, t0, tstep);
    fclose(fp);
  }
  for(delay = t0; ; delay += tstep) {
    printf("Delay = %le ns.\n", delay*1E9);
    // need to stop lasers for this?
    surelite_delay(0.0);  // ablation
    minilite_delay(delay);// flash
    laser_set_delays();
    meas_video_start(fd);
    meas_video_read_rgb(fd, r, g, b, aves);
    meas_video_stop(fd);
    meas_graphics_update_image(0, r, g, b);
    meas_graphics_update();
    sprintf(filename, "%s-%le.img", filebase, delay);
    if(filename[0] != '0') {
      if(!(fp = fopen(filename, "w"))) {
	fprintf(stderr, "Error writing file.\n");
	exit(1);
      }
      fwrite((void *) r, sizeof(unsigned char) * 640 * 480, 1, fp);
      fwrite((void *) g, sizeof(unsigned char) * 640 * 480, 1, fp);
      fwrite((void *) b, sizeof(unsigned char) * 640 * 480, 1, fp);
      fclose(fp);
    }
  }
}
