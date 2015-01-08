/* View files and convert to ppm */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <meas/meas.h>

#define HEIGHT 640
#define WIDTH 480

unsigned char r[HEIGHT * WIDTH], g[HEIGHT * WIDTH], b[HEIGHT * WIDTH];

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
  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, HEIGHT, WIDTH, 0, "video");
  for(delay = t0; ; delay += tstep) {
    printf("Delay = %le ns.\n", delay*1E9);
    sprintf(filename, "%s-%le.img", filebase, delay);
    if(!(fp = fopen(filename, "r"))) {
      fprintf(stderr, "Error reading file.\n");
      exit(1);
    }
    fread((void *) r, sizeof(unsigned char) * HEIGHT * WIDTH, 1, fp);
    fread((void *) g, sizeof(unsigned char) * HEIGHT * WIDTH, 1, fp);
    fread((void *) b, sizeof(unsigned char) * HEIGHT * WIDTH, 1, fp);
    fclose(fp);
    meas_graphics_update_image(0, r, g, b);
    meas_graphics_update();
    sprintf(filename, "%s-%le.ppm", filebase, delay);
    if(!(fp = fopen(filename, "w"))) {
      fprintf(stderr, "Error writing file.\n");
      exit(1);
    }
    meas_video_rgb_to_ppm(fp, r, g, b, WIDTH, HEIGHT);
    fclose(fp);
    //sleep(1);
  }
}
