/* View files and convert to ppm */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <meas/meas.h>

#define HEIGHT 640
#define WIDTH 480

unsigned char r[HEIGHT * WIDTH], g[HEIGHT * WIDTH], b[HEIGHT * WIDTH], rgb[3 * HEIGHT * WIDTH];

int main(int argc, char **argv) {

  double tstep, delay, t0;
  char filebase[512], filename[512];
  int fd, i, j;
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
  meas_graphics_open(0, MEAS_GRAPHICS_IMAGE, HEIGHT, WIDTH, 0, "video");
  meas_tty_raw();
  for(delay = t0; ; delay += tstep) {
    char chr;
    if((chr = meas_tty_read()) != 0) {
      switch(chr) {
      case ' ':
	while(meas_tty_read() != ' ') usleep(200);
	break;
      case '-':
	tstep = -fabs(tstep);
	break;
      case '+':
	tstep = fabs(tstep);
	break;
      case 'q':
	meas_tty_normal();
	exit(1);
      }
    }
    if(delay < 0.0) delay = 0.0;
    printf("Delay = %le ns.\n\r", delay*1E9);
    sprintf(filename, "%s-%le.img", filebase, delay);
    if(!(fp = fopen(filename, "r"))) {
      fprintf(stderr, "Error reading file.\n");
      meas_tty_normal();
      exit(1);
    }
    fread((void *) r, sizeof(unsigned char) * HEIGHT * WIDTH, 1, fp);
    fread((void *) g, sizeof(unsigned char) * HEIGHT * WIDTH, 1, fp);
    fread((void *) b, sizeof(unsigned char) * HEIGHT * WIDTH, 1, fp);
    fclose(fp);
    for(i = 0, j = 0; i < HEIGHT * WIDTH; i++, j += 3) {
      rgb[j] = r[i];
      rgb[j+1] = g[i];
      rgb[j+2] = b[i];
    }

    meas_graphics_update_image(0, rgb);
    meas_graphics_update();
    usleep(100000);
  }
}
