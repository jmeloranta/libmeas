/* View files and convert to ppm */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <meas/meas.h>
//#include "csun.h"

#define NX 512
#define NY 512

unsigned char rgb[3 * NX * NY];
unsigned char y16[2 * NX * NY];

int main(int argc, char **argv) {

  double tstep, delay, t0;
  char filebase[512], filename[512];
  int fd, i;
  FILE *fp;

  printf("Enter file basename: ");
  scanf("%s", filebase);
  sprintf(filename, "%s.info", filebase);
  if(!(fp = fopen(filename, "r"))) {
    fprintf(stderr, "Can't open info file.\n");
    exit(1);
  }
  fscanf(fp , " %*d %le %le", &t0, &tstep);
  fclose(fp);
  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, NX, NY, 0, "video");
  for(delay = t0; ; delay += tstep) {
    printf("Delay = %le ns.\n", delay*1E9);
    sprintf(filename, "%s-%le.img", filebase, delay);
    if(!(fp = fopen(filename, "r"))) {
      fprintf(stderr, "Error reading file.\n");
      exit(1);
    }
    meas_image_pgm_to_y16(fp, y16, NX, NY);
    meas_image_y16_to_rgb3(y16, rgb, NX, NY);
    fclose(fp);
    meas_graphics_update_image(0, rgb);
    meas_graphics_update();
#if 0
    sprintf(filename, "%s-%le.ppm", filebase, delay);
    if(!(fp = fopen(filename, "w"))) {
      fprintf(stderr, "Error writing file.\n");
      exit(1);
    }
    meas_video_rgb_to_ppm(fp, r, g, b);
    fclose(fp);
    //sleep(1);
#endif
  }
}
