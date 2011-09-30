/*
 * Experiment execution.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "uci.h"
#include <meas/meas.h>

static int spec = 0;

int roi_s2 = -1, roi_s1, roi_sbin, roi_p2, roi_p1, roi_pbin;
double zmin_fact = 1.0, zmax_fact = 1.0;
int dye_noscan = 0;

static void err(char *txt) {

  fprintf(stderr, "%s\n", txt);
  exit(1);
}

void exp_init(struct experiment *p) {

  meas_sr245_init(0, 0, 0, SR245_DEV); /* serial mode */
  /* NOTE: sr245 already uses the RS port! */
  /*itc503_init(0, ITC503); */
  meas_fl3000_init(0, FL3000_BOARD, FL3000_ID);
  meas_dg535_init(0, DG535_1_BOARD, DG535_1_ID);
  meas_dg535_init(1, DG535_2_BOARD, DG535_2_ID);
  if(!p->signal_source) {
    meas_pi_max_init(-20.0); /* -20 oC */
    meas_pi_max_gain(p->gain);
    if(roi_s2 != -1)
      meas_pi_max_roi(roi_s1, roi_s2, roi_sbin, roi_p1, roi_p2, roi_pbin);
    p->mono_points = meas_pi_max_size();
    printf("mono_points = %d\n", p->mono_points);
    /* p.mono_begin and p.mono_end must be defined in the input file */
    /* This is only meaningful if standard roi params are used */
    p->mono_step = (p->mono_end - p->mono_begin) / (double) (p->mono_points - 1);
  } else {
    p->mono_points = 1 + (int) (0.5 + (p->mono_end - p->mono_begin) / p->mono_step);
  }
  p->mono_cur = 0.0; /* Not set */
  p->dye_cur = 0.0;  /* Not set */
  p->dye_points = 1 + (int) (0.5 + (p->dye_end - p->dye_begin) / p->dye_step);
  if(!(p->ydata = (double *) malloc(sizeof(double) * p->dye_points * p->mono_points))) err("Out of memory.");
  if(!(p->x1data = (double *) malloc(sizeof(double) * p->dye_points))) err("Out of memory.");
  if(!(p->x2data = (double *) malloc(sizeof(double) * p->mono_points))) err("Out of memory.");
}

/* 
 * Graphics modes:
 * 0 = no graphics
 * 1 = Live emission display at fixed wavelength.
 * 2 = Live emission display - full spectrum - one at the time.
 * 3 = Live emission display - full spectrum - pseudo 3D.
 * 4 = Live image display from camera.
 *
 */

static void graph_callback(struct experiment *p) {

  int i, j, k;
  static int cur = 0;
  static double *tmp = NULL;
  double wl, zmin, zmax;
  static FILE *fp = NULL;

  if(!tmp) {
    if(p->display == 1) meas_graphics_init(2, NULL, NULL); /* both LIF and emission */
    else meas_graphics_init(1, NULL, NULL);
    if(!(tmp = (double *) malloc(sizeof(double) * MAX_SAMPLES))) {
      fprintf(stderr, "Out of memory.\n");
      exit(1);
    }
  }

  switch(p->display) {
  case 0:
    if(!fp) {
      if(!(fp = fopen("dump.dat", "w"))) {
	fprintf(stderr, "Can't open dump.dat\n");
	exit(1);
      }
    }
    i = (int) ((p->dye_end - p->dye_cur) / p->dye_step);
    for (j = 0; j < p->mono_points; j++)
      fprintf(fp, "%le\n", p->ydata[i * p->mono_points + j]);
    fprintf(fp, "\n");
    fflush(fp);
    break;
  case 1:
    /*a Live emission intensity at fixed wavelength */
    for (i = 0; i < p->mono_points; i++)
      if (p->x2data[i] >= p->display_arg1) break;
    if(i >= p->mono_points) {
      printf("%le %le %le\n", p->display_arg1, p->x2data[0], p->x2data[p->mono_points-1]);
      fprintf(stderr, "Error: monitor wavelength not within spectrum.\n");
      exit(1);
    }
    for (j = 0; j < p->mono_points; j++)
      if (p->x2data[j] >= p->display_arg2) break;
    if(j >= p->mono_points) {
      printf("%le %le %le\n", p->display_arg2, p->x2data[0], p->x2data[p->mono_points-1]);
      fprintf(stderr, "Error: monitor wavelength not within spectrum.\n");
      exit(1);
    }

    if(j < i) {
      fprintf(stderr, "display-arg1 > display-arg2. Setting the equal.\n");
      j = i;
    }
    meas_graphics_update2d(1, 0, MEAS_GRAPHICS_WHITE, p->x2data, &(p->ydata[cur * p->mono_points]), p->mono_points);
    tmp[cur] = 0.0;
    for (k = i; k <= j; k++)
      tmp[cur] += p->ydata[cur * p->mono_points + k];
    cur++;
    meas_graphics_update2d(0, 0, MEAS_GRAPHICS_WHITE, p->x1data, tmp, cur);
    break;
  case 2:
    /* Live emission spectrum display */
    i = (int) ((p->dye_end - p->dye_cur) / p->dye_step);
    j = (int) ((p->mono_cur - p->mono_begin) / p->mono_step);
    if(!j) return;
    meas_graphics_update2d(0, 0, MEAS_GRAPHICS_WHITE, p->x2data, &(p->ydata[i * p->mono_points]), p->mono_points);
    break;
  case 3:
    /* Live emission spectrum display - pseudo 3D */
    i = (int) ((p->dye_end - p->dye_cur) / p->dye_step);
    for (j = 0; j < p->mono_points; j++)
      tmp[j] = p->ydata[i * p->mono_points + j] + 100.0 * spec;
    meas_graphics_update2d(0, spec, MEAS_GRAPHICS_WHITE, p->x2data, tmp, p->mono_points);
    spec++;
    break;
  case 4:
    /* Live camera image display */
    i = (int) ((p->dye_end - p->dye_cur) / p->dye_step);
    j = (int) (sqrt(p->mono_points) + 0.5);
    meas_graphics_update3d(0, &p->ydata[i * p->mono_points], j, j, 0.0, 0.0, 1.0, 1.0);
    zmin = 1E99; zmax = -zmin;
    for (k = 0; k < j*j; k++) {
      if(p->ydata[i * p->mono_points + k] < zmin) zmin = p->ydata[i * p->mono_points + k];
      if(p->ydata[i * p->mono_points + k] > zmax) zmax = p->ydata[i * p->mono_points + k];
    }
    zmin *= zmin_fact;
    zmax *= zmax_fact;
    meas_graphics_zscale(0, zmin, zmax);
    break;
  default:
    fprintf(stderr, "Unknown display mode.\n");
    exit(1);
  }
  if(p->display_autoscale) {
    meas_graphics_autoscale(0);
    if(p->display == 1) meas_graphics_autoscale(1);
  }
  meas_graphics_update();
}

struct experiment *exp_read(char *file) {

  static struct experiment p;
  char buf[4096];
  FILE *fp;
  time_t ti;

  if(!(fp = fopen(file, "r"))) return NULL;

  memset(&p, 0, sizeof(p));
  p.sample[0] = p.operator[0] = 0;
  p.dye_order = 0;
  p.signal_ref = -1;
  ti = time(0);
  strcpy(p.date, ctime(&ti));
  while(fgets(buf, sizeof(buf), fp)) {
    if(buf[0] == '#' || buf[0] == '\n') continue; /* Skip comment lines */
    if(sscanf(buf, "operator%*[ \t]=%*[ \t]%s", p.operator) == 1) continue;
    if(sscanf(buf, "sample%*[ \t]=%*[ \t]%s", p.sample) == 1) continue;
    if(sscanf(buf, "dye_begin%*[ \t]=%*[ \t]%le", &p.dye_begin) == 1) continue;
    if(sscanf(buf, "dye_end%*[ \t]=%*[ \t]%le", &p.dye_end) == 1) continue;
    if(sscanf(buf, "dye_step%*[ \t]=%*[ \t]%le", &p.dye_step) == 1) continue;
    if(sscanf(buf, "dye_order%*[ \t]=%*[ \t]%d", &p.dye_order) == 1) continue;
    if(sscanf(buf, "dye_etalon_zero_wl%*[ \t]=%*[ \t]%le", &p.etalon_zero_wl) == 1) continue;
    if(sscanf(buf, "dye_etalon_zero%*[ \t]=%*[ \t]%u", &p.etalon_zero) == 1) continue;
    if(sscanf(buf, "dye_etalon_wl_scale1%*[ \t]=%*[ \t]%le", &p.etalon_wl_scale1) == 1) continue;
    if(sscanf(buf, "dye_etalon_wl_scale2%*[ \t]=%*[ \t]%le", &p.etalon_wl_scale2) == 1) continue;
    if(sscanf(buf, "dye_etalon_wl_p1%*[ \t]=%*[ \t]%le", &p.etalon_wl_p1) == 1)	continue;
    if(sscanf(buf, "dye_etalon_wl_p2%*[ \t]=%*[ \t]%le", &p.etalon_wl_p2) == 1) continue;
    if(sscanf(buf, "dye_etalon_wl_p3%*[ \t]=%*[ \t]%le", &p.etalon_wl_p3) == 1) continue;
    if(sscanf(buf, "mono_begin%*[ \t]=%*[ \t]%le", &p.mono_begin) == 1) continue;
    if(sscanf(buf, "mono_end%*[ \t]=%*[ \t]%le", &p.mono_end) == 1) continue;
    if(sscanf(buf, "mono_step%*[ \t]=%*[ \t]%le", &p.mono_step) == 1) continue;
    if(sscanf(buf, "source%*[ \t]=%*[ \t]%d", &p.signal_source) == 1) continue;
    if(sscanf(buf, "ref-source%*[ \t]=%*[ \t]%d", &p.signal_ref) == 1) continue;    
    if(sscanf(buf, "synchronous%*[ \t]=%*[ \t]%d", &p.synchronous) == 1) continue;
    if(sscanf(buf, "accum%*[ \t]=%*[ \t]%d", &p.accum) == 1) continue;
    if(sscanf(buf, "delay1%*[ \t]=%*[ \t]%le", &p.delay1) == 1) continue;
    if(sscanf(buf, "delay2%*[ \t]=%*[ \t]%le", &p.delay2) == 1) continue;
    if(sscanf(buf, "delay3%*[ \t]=%*[ \t]%le", &p.delay3) == 1) continue;    
    if(sscanf(buf, "qswitch%*[ \t]=%*[ \t]%le", &p.qswitch) == 1) continue;
    if(sscanf(buf, "gain%*[ \t]=%*[ \t]%le", &p.gain) == 1) continue;
    if(sscanf(buf, "gate%*[ \t]=%*[ \t]%le", &p.gate) == 1) continue;
    if(sscanf(buf, "roi%*[ \t]=%*[ \t]%d %d %d %d %d %d",
	      &roi_s1, &roi_s2, &roi_sbin, &roi_p1, &roi_p2, &roi_pbin) == 6)
      continue;
    if(sscanf(buf, "output%*[ \t]=%*[ \t]%s", p.output) == 1) continue;
    if(sscanf(buf, "display%*[ \t]=%*[ \t]%d", &p.display) == 1) continue;
    if(sscanf(buf, "display-autoscale%*[ \t]=%*[ \t]%d", &p.display_autoscale) == 1) continue;
    if(sscanf(buf, "display-arg1%*[ \t]=%*[ \t]%le", &p.display_arg1) == 1) continue;
    if(sscanf(buf, "display-arg2%*[ \t]=%*[ \t]%le", &p.display_arg2) == 1) continue;
    if(sscanf(buf, "display-zmin%*[ \t]=%*[ \t]%le", &zmin_fact) == 1) continue;
    if(sscanf(buf, "display-zmax%*[ \t]=%*[ \t]%le", &zmax_fact) == 1) continue;
    if(sscanf(buf, "dye_noscan%*[ \t]=%*[ \t]%d", &dye_noscan) == 1) continue;

    fprintf(stderr, "Unknown command: %s\n", buf);
    return NULL;
  }
  fclose(fp);

  /* Sanity checks */
  if(p.dye_begin > p.dye_end) err("dye_begin > dye_end.\n");
  if(p.mono_begin > p.mono_end) err("mono_begin > mono_end.\n");
  if(p.dye_begin == 0.0) err("Illegal dye_begin value.\n");
  if(p.dye_end == 0.0) err("Illegal dye_end value.\n");
  if(p.dye_step < 0.0) err("Illegal dye_step value.\n");
  if(p.dye_order < 0 || p.dye_order > 8) err("Illegal grating order (dye).\n");
  if(p.signal_source > 0 && p.mono_begin < 0.0) err("Illegal mono_begin value.\n");
  if(p.signal_source > 0 && p.mono_end < 0.0) err("Illegal mono_end value.\n");
  if(p.signal_source > 0 && p.mono_step < 0.0) err("Illegal mono_step value.\n");
  if(p.accum < 1) err("Illegal accum value.\n");
  if(p.delay1 < 0.0) err("Illegal delay1 value.\n");
  if(p.delay2 < 0.0) err("Illegal delay2 value.\n");
  if(p.delay3 < 0.0) err("Illegal delay3 value.\n");
  if(p.qswitch < 0.0) err("Illegal qswitch value.\n");
  if(p.gain < 0.0) err("Illegal gain value.\n");
  if(p.gate < 0.0) err("Illegal gate value.\n");
  if(!p.output[0]) err("Missing output file name.\n");
  
  if(!access(p.output, R_OK)) {
    fprintf(stderr, "Output file already exists. Remove it first if you want to overwrite.\n");
    return NULL;
  }

  return &p;
}

void exp_setup(struct experiment *p) {

  meas_etalon_zero = p->etalon_zero;
  meas_etalon_zero_wl = p->etalon_zero_wl;
  meas_etalon_wl_scale1 = p->etalon_wl_scale1;
  meas_etalon_wl_scale2 = p->etalon_wl_scale2;
  meas_etalon_wl_p1 = p->etalon_wl_p1;
  meas_etalon_wl_p2 = p->etalon_wl_p2;
  meas_etalon_wl_p3 = p->etalon_wl_p3;

  /* Stop everything */
  laser_stop();

  /* Setup lasers */
  surelite_qswitch(p->qswitch);
  excimer_delay(p->delay1);
  surelite_delay(p->delay2);
  laser_set_delays();
  /* CCD delay & gate */
  if(!p->signal_source) {
    ccd_set_delays(p->delay3, p->gate);
  }

  /* SR245 */
  meas_sr245_ports(0,8);       /* all ports are for data input (A/D ports) */
  meas_sr245_ttl_mode(0, 1, 0); /* Digital I/O port #1 for trigger input */
  
  /* Read pressure & temperature */
  meas_sr245_mode(0, 0);
  meas_sr245_enable_trigger(0);
  p->pressure1 = (meas_sr245_read(0, 8) - 0.695) * 96.2878;
  /*  printf("%le\n", meas_sr245_read(0, 8));*/
  printf("P1 = %le\n", p->pressure1);
  meas_sr245_disable_trigger(0);
  /*p->temperature1 = itc503_read(0); */
  /* printf("T1 = %le\n", p->temperature1); */

  meas_sr245_mode(0, p->synchronous); /* Synchronous mode? - trigger at digital port #1 */

  /* Dye laser */
  meas_fl3000_grating(0, p->dye_order);
  meas_fl3000_setwl(0, p->dye_end);

  /* Start the experiment */
  laser_start();
}

void exp_run(struct experiment *p) {

  int i, j, k;
  double tmp;

  /* Update axes */
  for (tmp = p->dye_end, i = 0; i < p->dye_points; tmp -= p->dye_step, i++)
    p->x1data[i] = tmp;
  for (tmp = p->mono_begin, i = 0; i < p->mono_points; tmp += p->mono_step, i++)
    p->x2data[i] = tmp;
  /* Reset spectrum counter */
  spec = 0;
  
  if(p->signal_source > 0) { /* Data source SR245 */
    /* discard first 2xaccum points */    
    for (k = 0; k < 2*p->accum; k++)
      (void) meas_sr245_read(0, p->signal_source);
    for(p->dye_cur = p->dye_end, i = 0; p->dye_cur > p->dye_begin; p->dye_cur -= p->dye_step, i++) {
      if(dye_noscan == 0) {
	meas_fl3000_setwl(0, p->dye_cur);
	printf("dye laser wl = %le nm.\n", p->dye_cur);
      }
      p->ydata[i * p->mono_points] = 0.0;
      p->mono_begin = p->mono_end;
      for (k = 0; k < p->accum; k++) {
	double xx, norm;
	xx = meas_sr245_read(0, p->signal_source);
	if(p->signal_ref >= 0) {
	  norm = meas_sr245_read(0, p->signal_ref);
	  xx /= norm;
	}
	p->ydata[i * p->mono_points] += xx;
      }
      p->ydata[i * p->mono_points] /= (double) p->accum;
      graph_callback(p);
    }
  } else if (p->signal_source == 0) {        /* Data source CCD */
    for(p->dye_cur = p->dye_end, i = 0; p->dye_cur > p->dye_begin; p->dye_cur -= p->dye_step, i++) {
      double norm;
      /* TODO: the weirdest thing: if you remove the following print, the program crashes!!! */
      printf("%d\n", i); fflush(stdout); // Still an issue?
      if(dye_noscan == 0) {
	meas_fl3000_setwl(0, p->dye_cur);
	printf("dye laser wl = %le nm.\n", p->dye_cur);
      }
      memset(&(p->ydata[i * p->mono_points]), 0, sizeof(double) * p->mono_points);
      if(p->signal_ref >= 0) meas_sr245_enable_trigger(0);
      printf("Current CCD temperature = %le\n", meas_pi_max_get_temperature());
      meas_pi_max_read(p->accum, &(p->ydata[i * p->mono_points]));
      if(p->signal_ref >= 0) {
	norm = meas_sr245_read(0, p->signal_ref);
	meas_sr245_disable_trigger(0);
	for (k = 0; k < p->mono_points; k++)
	  p->ydata[i * p->mono_points + k] /= norm + 1E-12;
      }
      p->mono_cur = p->mono_end;
      graph_callback(p);
    }    
  }

  /* Read pressure & temperature */
  meas_sr245_enable_trigger(0);
  p->pressure2 = (meas_sr245_read(0, 8) - 0.695) * 96.2878;
  printf("P2 = %le\n", p->pressure2);
  meas_sr245_disable_trigger(0);
  /*p->temperature2 = itc503_read(0); */
  /* printf("T2 = %le\n", p->temperature2); */

  /* Stop the delay generator */
  laser_stop();
  /* Stop CCD */
  if(!p->signal_source) {
    meas_pi_max_close();
  }
  /* window data */
  meas_graphics_save("window.dat");
}

int exp_save_data(struct experiment *p) {

  FILE *fp;
  int i, j;

  if(!(fp = fopen(p->output, "w"))) return -1;

  fprintf(fp, "# Experiment date: %s", p->date); /* p-> data has newline? */
  fprintf(fp, "# Sample: %s\n", p->sample);
  fprintf(fp, "# Operator: %s\n", p->operator);
  fprintf(fp, "# Dye laser begin: %lf\n", p->dye_begin);
  fprintf(fp, "# Dye laser end: %lf\n", p->dye_end);
  fprintf(fp, "# Dye laser step: %lf\n", p->dye_step);
  fprintf(fp, "# Monochromator begin: %lf\n", p->mono_begin);
  fprintf(fp, "# Monochromator end: %lf\n", p->mono_end);
  fprintf(fp, "# Monochromator step: %lf\n", p->mono_step);
  fprintf(fp, "# Signal source: %d\n", p->signal_source);
  fprintf(fp, "# DAQ mode: %s\n", (p->synchronous)?"Synchronous":"Asynchronous");
  fprintf(fp, "# Accumulations: %d\n", p->accum);
  fprintf(fp, "# Delay 1: %le\n", p->delay1);
  fprintf(fp, "# Delay 2: %le\n", p->delay2);
  fprintf(fp, "# Delay 3: %le\n", p->delay3);
  fprintf(fp, "# Q-switch delay: %le\n", p->qswitch);
  fprintf(fp, "# Pressure1 (torr): %le\n", p->pressure1);
  fprintf(fp, "# Pressure2 (torr): %le\n", p->pressure2);
  fprintf(fp, "# Temperature1 (K): %le\n", p->temperature1);
  fprintf(fp, "# Temperature2 (K): %le\n", p->temperature2);
  fprintf(fp, "# Gain: %lf\n", p->gain);
  fprintf(fp, "# Gate: %le\n", p->gate);
  fprintf(fp, "# Display mode: %d\n", p->display);
  fprintf(fp, "# Display autoscale: %d\n", p->display_autoscale);
  fprintf(fp, "# Display arg1: %le\n", p->display_arg1);
  fprintf(fp, "# Display arg2: %le\n", p->display_arg2);
  
  for (i = 0; i < p->dye_points; i++) {
    for (j = 0; j < p->mono_points; j++)
      fprintf(fp, "%le %le %le\n", p->x1data[i], p->x2data[j], p->ydata[i * p->mono_points + j]);
    fprintf(fp, "\n");
  }
  fclose(fp);
  
  return 0;
}
