/* View files and convert to ppm */

#include <stdio.h>
#include <stdlib.h>
#include <meas/meas.h>

#define HEIGHT 1280
#define WIDTH 960

unsigned char *rgb, *y16;

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
  if(!(rgb = (unsigned char *) malloc(WIDTH * HEIGHT * 3))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(y16 = (unsigned char *) malloc(WIDTH * HEIGHT * 4))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, HEIGHT, WIDTH, 0, "video");
  for(delay = t0; ; delay += tstep) {
    printf("Delay = %le ns.\n", delay*1E9);
    sprintf(filename, "%s-%le.img", filebase, delay);
    if(!(fp = fopen(filename, "r"))) {
      fprintf(stderr, "Error reading file.\n");
      exit(1);
    }
    fread((void *) y16, 4 * HEIGHT * WIDTH, 1, fp);
    fclose(fp);
    meas_image_y16_to_rgb3(y16, rgb, WIDTH, HEIGHT);    
    meas_graphics_update_image(0, rgb);
    meas_graphics_update();
    //sleep(1);
  }
}
