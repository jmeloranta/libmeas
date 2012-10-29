#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <meas/meas.h>
#include "csun.h"

unsigned char r[640 * 480], g[640 * 480], b[640 * 480];

int main(int argc, char **argv) {

  double tstep, delay;
  char filebase[512], filename[512];
  int fd;
  FILE *fp;

  printf("Enter file basename: ");
  scanf("%s", filebase);
  printf("Enter time step (ns): ");
  scanf(" %le", &tstep);
  tstep *= 1E-9;
  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, 640, 480, 0, "video");
  for(delay = 0.0; ; delay += tstep) {
    printf("Delay = %le ns.\n", delay*1E9);
    sprintf(filename, "%s-%le.img", filebase, delay);
    if(filename[0] != '0') {
      if(!(fp = fopen(filename, "r"))) {
	fprintf(stderr, "Error reading file.\n");
	exit(1);
      }
      fread((void *) r, sizeof(unsigned char) * 640 * 480, 1, fp);
      fread((void *) g, sizeof(unsigned char) * 640 * 480, 1, fp);
      fread((void *) b, sizeof(unsigned char) * 640 * 480, 1, fp);
      fclose(fp);
      meas_graphics_update_image(0, r, g, b);
      meas_graphics_update();
      sleep(1);
    }
  }
}
