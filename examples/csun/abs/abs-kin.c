#if !defined(MATRIX) && !defined(NEWPORT)
#error "Either MATRIX or NEWPORT must be defined."
#endif

/*
 * Measure absorption at a given wavelength as a function of time with Matrix 
 * or Newport diode array spectrometer.
 *
 * Usage:
 * 
 * For newport:
 * abs-kin -s <outspec> -w <wl1>-<wl2> -p <dl>
 * For matrix:
 * abs2-kin -s <outspec> -w <wl1>-<wl2> -p <dl>
 *  To measure kinetic trace to file at wavelength <wl1> - <wl2>
 *  (or use wl1/wl2 format to integrate)
 *  to file <outspec>. Delay between the easurements is <dl>.
 *
 * In addition the following options can be given:
 * 
 * -d                    Live display (default on).
 *
 * Note: Current backgrounds must exist before running this program.
 *       Use abs.c for getting these.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <meas/meas.h>

#ifdef NEWPORT
#define BACKGROUND1 "abs-bkg1.dat"
#define BACKGROUND2 "abs-bkg2.dat"
#else
#define BACKGROUND1 "abs2-bkg1.dat"
#define BACKGROUND2 "abs2-bkg2.dat"
#endif
#define DEFAULT_EXP 0.6
#define DEFAULT_AVE 1

#define MAXKIN 65535

double exp_time = DEFAULT_EXP, delay = -1.0;
int averages = DEFAULT_AVE;
int live = 1, first = 1;
char *output = NULL;
double spec[1024], bkg1[1024], bkg2[1024], x[1024], wl1 = -1.0, wl2 = -1.0;
double kinetics[MAXKIN], kinetics_t[MAXKIN];

void usage() {
  
#ifdef NEWPORT
  fprintf(stderr, "Usage: abs-kin -s <specfile> -w <wl or wl1-wl2 or wl1/wl2> -p <dl>\n");
#else
  fprintf(stderr, "Usage: abs2-kin -s <specfile> -w <wl or wl1-wl2 or wl1/wl2> -p <dl>\n");
#endif
  exit(1);
}

void sigexit() {

#ifdef NEWPORT
  meas_newport_is_close();
#else
  meas_matrix_close();
#endif
  exit(0);
}

int main(int argc, char **argv) {

  int i, j, mvv, mvv2, k, bave, size, mode;
  time_t sl_s;
  long sl_ns;
  FILE *fp;
  double mv, bexp;
  char dummy[512];

#ifdef NEWPORT
  signal(SIGINT, sigexit);
#else
  signal(SIGINT, sigexit);
#endif

  for (i = 1; i < argc; i++) {
    if(*argv[i] == '-') {
      switch(argv[i][1]) {
      case 'd':
	live = 0;
	break;
      case 'w':
	mode = 2; /* integrate */
	if(sscanf(argv[i+1], "%lf/%lf", &wl1, &wl2) != 2) {
	  /* subtract */
	  mode = 1;
	  if(sscanf(argv[i+1], "%lf-%lf", &wl1, &wl2) != 2) {
	    mode = 0; /* single wavelength */
	    wl1 = atof(argv[i+1]);
	    wl2 = wl1;
	  }
	}
	i++;
	break;
      case 'p':
	delay = atof(argv[i+1]);
	i++;
	break;
      case 's':
	output = argv[i+1];
	i++;
	break;
      default:
	fprintf(stderr, "Unknown option %s.\n", argv[i]);
	usage();
      }
    } else usage();
  }

  if(wl1 == -1.0 || wl2 == -1.0) {
    fprintf(stderr, "No wavelength specified.\n");
    usage();
    exit(1);
  }

  if(delay == -1.0) {
    fprintf(stderr, "No delay specified.\n");
    usage();
    exit(1);
  }

  if(!access(output, F_OK)) {
    fprintf(stderr, "Warning: file exists. Do you wish to overwrite (y/n)? ");
    gets(dummy);
    if(dummy[0] == 'n') {
      fprintf(stderr, "Not overwritten.\n");
      exit(1);
    }
  }

#ifdef NEWPORT
  meas_newport_is_init();
  size = meas_newport_is_size();
#else
  meas_matrix_init();
  size = meas_matrix_size();
  if(size > 1024) {
    fprintf(stderr, "CCD element size %d > 1024 - fix the program...\n", size);
    meas_matrix_close();
    exit(1);
  }
#endif
  
  /* read in background and modify the spectrum to absorbance scale */
  if(!(fp = fopen(BACKGROUND2, "r"))) {
    fprintf(stderr, "Can't read the background spectrum (%s): use -b.\n", BACKGROUND2);
#ifdef NEWPORT
    meas_newport_is_close();
#else
    meas_matrix_close();
#endif
    exit(1);
  }
  fscanf(fp, " %le %d", &exp_time, &averages);
  for (i = 0; i < size; i++)
    fscanf(fp, " %*le %le", &bkg2[i]);
  fclose(fp);
  
  if(!(fp = fopen(BACKGROUND1, "r"))) {
    fprintf(stderr, "Can't read the lamp background spectrum (%s): use -b.\n", BACKGROUND1);
#ifdef NEWPORT
    meas_newport_is_close();
#else
    meas_matrix_close();
#endif
    exit(1);
  }
  fscanf(fp, " %le %d", &exp_time, &averages);
  for (i = 0; i < size; i++) {
    fscanf(fp, " %*le %le", &bkg1[i]);
    bkg1[i] -= bkg2[i]; /* subtract the diode array background */
  }
  fclose(fp);
  
  if(live) meas_graphics_init(0, MEAS_GRAPHICS_XY, 512, 512, 1024, "abs-kin");
  
  if(!(fp = fopen(output, "w"))) {
    fprintf(stderr, "Can't open file for writing.\n");
#ifdef NEWPORT
    meas_newport_is_close();
#else
    meas_matrix_close();
#endif
    exit(1);
  }

  k = 0;
  meas_misc_set_reftime(); /* use the wall clock... */
  while (1) {
    meas_misc_set_time(); /* set timer */
    memset(spec, 0, sizeof(double) * 1024);
#ifdef NEWPORT
    meas_newport_is_read(exp_time, 0, averages, spec);    
#else
    meas_matrix_read(exp_time, averages, spec);
#endif    
    for (i = 0; i < 1024; i++) {
      spec[i] -= bkg2[i]; 
      spec[i] = log10(fabs(bkg1[i] / (fabs(spec[i]) + 1E-10)) + 1E-10);
    }
    
    /* locate the requested pixels */
    mv = 99999.0;
    mvv = 0;
    for (i = 0; i < 1024; i++) {
      double ttmp;
#ifdef NEWPORT
      x[i] = meas_newport_is_calib(i);
#else
      x[i] = meas_matrix_calib(i);
#endif
      ttmp = fabs(x[i] - wl1);
      if(ttmp < mv) {
	mv = ttmp;
	mvv = i;
      }
    }
    mv = 99999.0;
    mvv2 = 0;
    for (i = 0; i < 1024; i++) {
      double ttmp;
      ttmp = fabs(x[i] - wl2);
      if(ttmp < mv) {
	mv = ttmp;
	mvv2 = i;
      }
    }
    if(mvv > mvv2 && mode == 2) {
      fprintf(stderr, "Error in wavelength integration range: %le %le\n", wl1, wl2);
#ifdef NEWPORT
      meas_newport_is_close();
#else
      meas_matrix_close();
#endif
      exit(1);
    }

    kinetics[k] = 0.0;

    if(mode == 2 || mode == 0) {
      for (i = mvv, j = 0; i <= mvv2; i++, j++)
	kinetics[k] += spec[i];    
      kinetics[k] /= (double) j;
    } else { /* subtract */
      kinetics[k] = spec[mvv] - spec[mvv2];
    }

    /*    kinetics_t[k] = delay * (double) k; */
    kinetics_t[k] = meas_misc_get_reftime();
    fprintf(fp, "%le %le\n", kinetics_t[k], kinetics[k]);
    fflush(fp);

    if(live) {
      meas_graphics_update_xy(0, kinetics_t, kinetics, k);
      if(k > 1) meas_graphics_autoscale(0);
      meas_graphics_update();
    }
    k++;
    if(k == MAXKIN) {
      fprintf(stderr, "Maximum samples reached. Increase MAXKIN.\n");
      fclose(fp);
#ifdef NEWPORT
      meas_newport_is_close();
#else
      meas_matrix_close();
#endif
      exit(1);
    }
    
    sl_s = (time_t) delay;
    sl_ns = (long) ((delay - (double) sl_s) * 1E9);
    meas_misc_sleep_rest(sl_s, sl_ns); /* sleep between samples */
  } /* end while */

  fclose(fp);
  printf("Press any enter to stop:");
  gets(dummy);  
  meas_graphics_close();
#ifdef NEWPORT
  meas_newport_is_close();
#else
  meas_matrix_close();
#endif
}
