/*
 * Measure absorption spectrum with Newport Matrix.
 *
 * Usage:
 *
 
 * abs2 -b                To measure lamp background to file (bkgspec1.dat).
 * abs2 -g                Disable graphics output.
 * abs2 -c                To measure diode array background (bkgspec2.dat).
 * abs2 -s <outspec>      To measure spectrum (using current background) to file
 *                       <outspec>.
 * abs2 -n <outpsec>      To measure spectrum without background correction.
 *                       <outspec>.
 *
 * In addition the following options can be given:
 * 
 * -e <exptime>          To specify exposure time in sec (default 600 ms).
 * -a <averages>         To average scans (default 1).
 * -d                    Live display (default off).
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <meas/meas.h>

#define BACKGROUND1 "abs2-bkg1.dat"
#define BACKGROUND2 "abs2-bkg2.dat"
#define DEFAULT_EXP 0.6
#define DEFAULT_AVE 1

double exp_time = DEFAULT_EXP;
int averages = DEFAULT_AVE, mode = -1;
int continuous = 0, first = 1, graph = 1;
char *output = NULL;
double spec[2048], bkg1[2048], bkg2[2048], x[2048];

void usage() {
  
  fprintf(stderr, "Usage: abs2 {-a <averages>, -b, -c, -e <exptime>, -s <specfile>}\n");
  exit(1);
}

int main(int argc, char **argv) {

  int i, size;
  FILE *fp;
  double bexp;
  int bave;
  char dummy[512];

  for (i = 1; i < argc; i++) {
    if(*argv[i] == '-') {
      switch(argv[i][1]) {
      case 'a': /* averages */
	averages = atoi(argv[i+1]);
	i++;
	break;
      case 'b':
	mode = 0; /* background */
	output = BACKGROUND1;
	break;
      case 'c':
	mode = 0; /* background */
	output = BACKGROUND2;
	break;
      case 'd':
	continuous = 1;
	break;
      case 'e':
	exp_time = atof(argv[i+1]);
	i++;
	break;
      case 'g':
	graph = 0;
	break;
      case 's':
	mode = 1; /* spectrum */
	output = argv[i+1];
	i++;
	break;
      case 'n':
	mode = 2; /* no background correction */
	output = argv[i+1];
	i++;
	break;
      default:
	fprintf(stderr, "Unknown option %s.\n", argv[i]);
	usage();
      }
    } else usage();
  }

  if(mode == -1) {
    fprintf(stderr, "No mode (-b, -s or -n) given.\n");
    exit(1);
  }
  if(mode == 1 && !access(output, F_OK)) {
    fprintf(stderr, "Warning: file exists. Do you wish to overwrite (y/n)? ");
    gets(dummy);
    if(dummy[0] == 'n') {
      fprintf(stderr, "Not overwritten.\n");
      exit(1);
    }
  }
  
  meas_matrix_init(0);
  meas_matrix_size(0, &size, NULL);

  if(mode == 1) {
    /* read in background and modify the spectrum to absorbance scale */
    if(!(fp = fopen(BACKGROUND2, "r"))) {
      fprintf(stderr, "Can't read the diode array background spectrum (%s): use -b.\n", BACKGROUND2);
      meas_matrix_close(0);
      exit(1);
    }
    fscanf(fp, " %le %d", &bexp, &bave);
    if(fabs(bexp - exp_time) > 1E-3 || bave != averages) {
      fprintf(stderr, "Inconsistent background file: %s\n", BACKGROUND2);
      meas_matrix_close(0);
      exit(1);
    }
    for (i = 0; i < size; i++)
      fscanf(fp, " %*le %le", &bkg2[i]);
    fclose(fp);
    
    if(!(fp = fopen(BACKGROUND1, "r"))) {
      fprintf(stderr, "Can't read the lamp background spectrum (%s): use -b.\n", BACKGROUND1);
      meas_matrix_close(0);
      exit(1);
    }
    fscanf(fp, " %le %d", &bexp, &bave);
    if(fabs(bexp - exp_time) > 1E-3 || bave != averages) {
      fprintf(stderr, "Inconsistent background file: %s\n", BACKGROUND1);
      meas_matrix_close(0);
      exit(1);
    }
    for (i = 0; i < size; i++) {
      fscanf(fp, " %*le %le", &bkg1[i]);
      bkg1[i] -= bkg2[i]; /* subtract the diode array background */
    }
    fclose(fp);
  } /* end if mode */

  if(graph) meas_graphics_init(0, MEAS_GRAPHICS_XY, 512, 512, 65535, "abs2");

  while (1) {
    memset(spec, 0, sizeof(double) * size);
    meas_matrix_read(0, exp_time, averages, spec);

    if(mode == 1) { /* spectrum */

      for (i = 0; i < size; i++) {
	spec[i] -= bkg2[i]; 
	spec[i] = log10(fabs(bkg1[i] / (fabs(spec[i]) + 1E-10)) + 1E-10);
      }

    } /* end if mode == 1 */
    
    if(!(fp = fopen(output, "w"))) {
      fprintf(stderr, "Can't open file for writing.\n");
      meas_matrix_close(0);
      exit(1);
    }
    if(mode == 0) fprintf(fp, "%le %d\n", exp_time, averages);
    for (i = 0; i < size; i++) {
      x[i] = meas_matrix_calib(i);
      fprintf(fp, "%le %le\n", x[i], spec[i]);
    }
    fclose(fp);
    if(graph) {
      meas_graphics_update_xy(0, x, spec, size);
      meas_graphics_autoscale(0);
      meas_graphics_update();
    }
    if(!continuous || mode == 0) break;
  }
  if(graph) {
    printf("Press any enter to stop:");
    gets(dummy);
    meas_graphics_close();
  }
  meas_matrix_close(0);
}
