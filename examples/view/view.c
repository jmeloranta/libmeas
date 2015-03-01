/*
 * Contour plot 3d data (ascii).
 *
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <meas/meas.h>

int main(int argc, char **argv) {

  int ix, iy, nx, ny, i;
  double x, y, z, min_z, max_z, prev_x, prev_y;
  FILE *fp;
  char asd[512];
  unsigned char *rgb;

  if (argc != 2) {
    fprintf(stderr, "Usage: view file\n");
    exit(1);
  }
  if(!(fp = fopen(argv[1], "r"))) {
    fprintf(stderr, "Can't open %s.\n", argv[1]);
    exit(1);
  }

  min_z = 1E99; max_z = -min_z;
  nx = ny = 0;
  for (i = 0; fgets(asd, sizeof(asd), fp) != NULL; i++) {
    prev_x = x;
    prev_y = y;
    fscanf(fp, " %le %le %le", &x, &y, &z);
    if(i > 0) {
      if(prev_x < x) nx++;
      if(prev_y < y && nx == 0) ny++;
    }
    if(z > max_z) max_z = z;
    if(z < min_z) min_z = z;
  }
  nx++; ny++; ny++;
  rewind(fp);
  if(!(rgb = (unsigned char *) malloc(3 * nx * ny))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }

  i = 0;
  for (ix = 0; ix < nx; ix++) { /* x */
    for (iy = 0; iy < ny; iy++) { /* y */
      fscanf(fp, " %*le %*le %le", &z);
      meas_graphics_rgb((z - min_z) / (max_z - min_z), &rgb[i], &rgb[i+1], &rgb[i+2]);
      i += 3;
    }
  }
  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, nx, ny, 0, "view");
  meas_graphics_update_image(0, rgb);
  meas_graphics_update();
  printf("Press reutrn: ");
  gets(asd);
  free(rgb);
  fclose(fp);
  return 0;
}

