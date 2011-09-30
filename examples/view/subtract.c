/*
 * Subtract two 3d data files.
 *
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

int main(int argc, char **argv) {

  int col, row, ncol, nrow;
  double *data, *data2;
  FILE *fp, *fp2;
  char asd[128];

  if (argc != 7) {
    fprintf(stderr, "Usage: subtract [-a|-b] ncol nrow file bkg_file outfile\n");
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
  if(!(data2 = (double *) malloc(sizeof(double) * nrow * ncol))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(fp2 = fopen(argv[5], "r"))) {
    fprintf(stderr, "Can't open %s.\n", argv[5]);
    exit(1);
  }

  switch(argv[1][1]) {
  case 'a':
    /* two first columns are dummy variables */
    for (col = 0; col < ncol; col++) {
      for (row = 0; row < nrow; row++) {
	fscanf(fp, " %*le %*le %le", &data[nrow * col + row]);
      }
    }
    /* two first columns are dummy variables */
    for (col = 0; col < ncol; col++) {
      for (row = 0; row < nrow; row++) {
	fscanf(fp2, " %*le %*le %le", &data2[nrow * col + row]);
      }
    }
    break;
  case 'b':
    fread(data, sizeof(double), nrow * ncol, fp);
    fread(data2, sizeof(double), nrow * ncol, fp2);
    break;
  default:
    fprintf(stderr, "Usage: view [-a|-b] ncol nrow file\n");
    fclose(fp);
    exit(1);
  }
  fclose(fp);
  fclose(fp2);

  for (col = 0; col < ncol; col++)
    for (row = 0; row < nrow; row++)
      data[nrow * col + row] -= data2[nrow * col + row];
  
  if(!(fp = fopen(argv[6], "w"))) {
    fprintf(stderr, "Can't open %s for writing.\n", argv[6]);
    exit(1);
  }
  fwrite(data, sizeof(double), nrow * ncol, fp);
  fclose(fp);
  return 0;
}
