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
  double *data, x, y, min_x, max_x, min_y, max_y, prev_x, prev_y;
  FILE *fp;
  char asd[512];

  if (argc != 2) {
    fprintf(stderr, "Usage: view file\n");
    exit(1);
  }
  if(!(fp = fopen(argv[1], "r"))) {
    fprintf(stderr, "Can't open %s.\n", argv[1]);
    exit(1);
  }

  meas_graphics_init(1, NULL, NULL);
  min_x = 1E99; max_x = -min_x;
  min_y = 1E99; max_y = -min_y;
  nx = ny = 0;
  for (i = 0; fgets(asd, sizeof(asd), fp) != NULL; i++) {
    prev_x = x;
    prev_y = y;
    fscanf(fp, " %le %le %*le", &x, &y);
    if(i > 0) {
      if(prev_x < x) nx++;
      if(prev_y < y && nx == 0) ny++;
    }
    if(x < min_x) min_x = x;
    if(x > max_x) max_x = x;
    if(y < min_y) min_y = y;
    if(y > max_y) max_y = y;
  }
  nx++; ny++; ny++;
  rewind(fp);
  if(!(data = (double *) malloc(sizeof(double) * nx * ny))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }

  for (ix = 0; ix < nx; ix++) { /* x */
    for (iy = 0; iy < ny; iy++) { /* y */
      fscanf(fp, " %*le %*le %le", &data[ix * ny + iy]);
    }
  }
  meas_graphics_update3d(0, data, ny, nx, min_y, min_x, (max_y - min_y) / ny, (max_x - min_x) / nx);

  meas_graphics_update();
  printf("Press reutrn: ");
  gets(asd);
  free(data);
  fclose(fp);
  return 0;
}
