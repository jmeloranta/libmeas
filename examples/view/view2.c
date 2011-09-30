/*
 * Contour plot 3d data (binary files).
 *
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <meas/meas.h>

int main(int argc, char **argv) {

  int col, row, ncol, nrow;
  double *data, *data2;
  FILE *fp, *fp2;
  char asd[64];

  if (argc < 5 || argc > 6) {
    fprintf(stderr, "Usage: view ncol nrow file [bkg_file]\n");
    exit(1);
  }
  
  ncol = atoi(argv[2]);
  nrow = atoi(argv[3]);

  if(!(fp = fopen(argv[4], "r"))) {
    fprintf(stderr, "Can't open %s.\n", argv[4]);
    exit(1);
  }

  if(!(data = (double *) malloc(sizeof(double) * nrow * ncol))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(argc == 6) {
    if(!(data2 = (double *) malloc(sizeof(double) * nrow * ncol))) {
      fprintf(stderr, "Out of memory.\n");
      exit(1);
    }
    if(!(fp2 = fopen(argv[5], "r"))) {
      fprintf(stderr, "Can't open %s.\n", argv[5]);
      exit(1);
    }
  }

  meas_graphics_init(1, NULL, NULL);
  fread(data, sizeof(double), nrow * ncol, fp);
  if(argc == 5) {
    fread(data2, sizeof(double), nrow * ncol, fp2);
    for (col = 0; col < ncol; col++)
      for (row = 0; row < nrow; row++)
	data[nrow * col + row] -= data2[nrow * col + row];
  }
  meas_graphics_update3d(0, data, nrow, ncol, 0.0, 0.0, 1.0, 1.0);
  meas_graphics_update();
  printf("Press reutrn: ");
  gets(asd);
  free(data);
  fclose(fp);
  return 0;
}
