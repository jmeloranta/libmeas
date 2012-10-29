/*
 * Measure absorption spectrum with NewportIS.
 *
 * Usage:
 *
 
 * abs -b                To measure lamp background to file (bkgspec1.dat).
 * abs -g                Disable graphics output.
 * abs -c                To measure diode array background (bkgspec2.dat).
 * abs -s <outspec>      To measure spectrum (using current background) to file
 *                       <outspec>.
 * abs -n <outpsec>      To measure spectrum without background correction.
 *                       <outspec>.
 *
 * In addition the following options can be given:
 * 
 * -e <exptime>          To specify exposure time in sec (default 600 ms).
 * -a <averages>         To average scans (default 1).
 * -d                    Live display (default off).
 *
 * Each background file consists of exposure time, # of averages and spectrum.
 *
 */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <meas/meas.h>

#define BACKGROUND1 "abs-bkg1.dat"
#define BACKGROUND2 "abs-bkg2.dat"
#define DEFAULT_EXP 0.6
#define DEFAULT_AVE 1

double exp_time = DEFAULT_EXP;
int averages = DEFAULT_AVE, mode = -1, graph = 1;
int continuous = 0;
char *output = NULL;
double spec[1024], bkg1[1024], bkg2[1024], x[1024];

void usage() {
  
  fprintf(stderr, "Usage: abs {-a <averages>, -b, -c, -e <exptime>, -s <specfile>}\n");
  exit(1);
}

int main(int argc, char **argv) {

  int i;
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
  
  if(mode == 1) {
    /* read in background and modify the spectrum to absorbance scale */
    if(!(fp = fopen(BACKGROUND2, "r"))) {
      fprintf(stderr, "Can't read the diode array background spectrum (%s): use -b.\n", BACKGROUND2);
      exit(1);
    }
    fscanf(fp, " %le %d", &bexp, &bave);
    if(fabs(bexp - exp_time) > 1E-3 || bave != averages) {
      fprintf(stderr, "Inconsistent background file: %s\n", BACKGROUND2);
      exit(1);
    }
    for (i = 0; i < 1024; i++)
      fscanf(fp, " %*le %le", &bkg2[i]);
    fclose(fp);
    
    if(!(fp = fopen(BACKGROUND1, "r"))) {
      fprintf(stderr, "Can't read the lamp background spectrum (%s): use -b.\n", BACKGROUND1);
      exit(1);
    }
    fscanf(fp, " %le %d", &bexp, &bave);
    if(fabs(bexp - exp_time) > 1E-3 || bave != averages) {
      fprintf(stderr, "Inconsistent background file: %s\n", BACKGROUND1);
      exit(1);
    }
    for (i = 0; i < 1024; i++) {
      fscanf(fp, " %*le %le", &bkg1[i]);
      bkg1[i] -= bkg2[i]; /* subtract the diode array background */
    }
    fclose(fp);
  } /* end if mode */
    
  if(graph) meas_graphics_init(0, MEAS_GRAPHICS_2D, 512, 512, 65535);
  meas_newport_is_init();
  
  while (1) {
    memset(spec, 0, sizeof(double) * 1024);
    meas_newport_is_read(exp_time, 0, averages, spec);

    if(mode == 1) { /* spectrum */

      for (i = 0; i < 1024; i++) {
	spec[i] -= bkg2[i]; /* subtract diode array background */
	spec[i] = log10(fabs(bkg1[i] / (fabs(spec[i]) + 1E-10)) + 1E-10);
      }

    } /* end if mode == 1 */

    if(!(fp = fopen(output, "w"))) {
      fprintf(stderr, "Can't open file for writing.\n");
      meas_newport_is_close();  
      exit(1);
    }
    if(mode == 0) fprintf(fp, "%le %d\n", exp_time, averages);
    for (i = 0; i < 1024; i++) {
      x[i] = meas_newport_is_calib(i);
      fprintf(fp, "%le %le\n", x[i], spec[i]);
    }
    fclose(fp);
    if(graph) {
      meas_graphics_update2d(0, 0, MEAS_GRAPHICS_WHITE, x, spec, 1024);
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
  meas_newport_is_close();  
}
