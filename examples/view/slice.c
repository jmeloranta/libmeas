/*
 * Slice 3d data into 2D (ascii or binary files). View by xmgrace.
 *
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define AVEWIN 50

int main(int argc, char **argv) {

  int col, row, ncol, nrow, val, ll, ul;
  double *data, *data2, tmp;
  FILE *fp, *fp2;
  char asd[128];

  if (argc < 7 || argc > 8) {
    fprintf(stderr, "Usage: slice [-a|-b] [-x|-y] val ncol nrow file [bkg_file]\n");
    exit(1);
  }
  
  val = atoi(argv[3]);
  ncol = atoi(argv[4]);
  nrow = atoi(argv[5]);

  if(!(fp = fopen(argv[6], "r"))) {
    fprintf(stderr, "Can't open %s.\n", argv[6]);
    exit(1);
  }

  if(!(data = (double *) malloc(sizeof(double) * nrow * ncol))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(argc == 8) {
    if(!(data2 = (double *) malloc(sizeof(double) * nrow * ncol))) {
      fprintf(stderr, "Out of memory.\n");
      exit(1);
    }
    if(!(fp2 = fopen(argv[7], "r"))) {
      fprintf(stderr, "Can't open %s.\n", argv[7]);
      exit(1);
    }
  }

  switch(argv[1][1]) {
  case 'a':
    /* two first columns are dummy variables */
    for (col = 0; col < ncol; col++) {
      for (row = 0; row < nrow; row++) {
	fscanf(fp, " %*le %*le %le", &data[nrow * col + row]);
      }
    }
    if(argc == 8) {
      /* two first columns are dummy variables */
      for (col = 0; col < ncol; col++) {
	for (row = 0; row < nrow; row++) {
	  fscanf(fp2, " %*le %*le %le", &data2[nrow * col + row]);
	}
      }
    }
    break;
  case 'b':
    fread(data, sizeof(double), nrow * ncol, fp);
    if(argc == 8) fread(data2, sizeof(double), nrow * ncol, fp2);
    break;
  default:
    fprintf(stderr, "Usage: view [-a|-b] ncol nrow file\n");
    fclose(fp);
    exit(1);
  }

  if(argc == 8) {
    for (col = 0; col < ncol; col++)
      for (row = 0; row < nrow; row++)
	data[nrow * col + row] -= data2[nrow * col + row];
  }

  switch(argv[2][1]) {
  case 'x':
    for (col = 0; col < ncol; col++) {
      tmp = 0.0;
      ll = val - AVEWIN;
      if(ll < 0) ll = 0;
      ul = val + AVEWIN;
      if(ul > 512) ul = 512;
      for (row = ll; row < ul; row++)	
	tmp += data[nrow * col + row];
      printf("%le %le\n", (double) col, tmp);
    }
    break;
  case 'y':
    for (row = 0; row < nrow; row++) {
      tmp = 0.0;
      ll = val - AVEWIN;
      if(ll < 0) ll = 0;
      ul = val + AVEWIN;
      if(ul > 512) ul = 512;
      for (col = ll; col < ul; col++)	
	tmp += data[nrow * col + row];
      printf("%le %le\n", (double) row, tmp);
    }
    break;
  default:
    fprintf(stderr, "Specify either -x or -y to choose binning direction.\n");
    exit(1);
  }

  free(data);
  fclose(fp);
  return 0;
}
