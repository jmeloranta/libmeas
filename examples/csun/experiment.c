/*
 *
 * Experiment execution.
 *
 * TODO: implement delay scans.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "csun.h"
#include <meas/meas.h>
#include <math.h>

double shg_x[8192], wl_scale = 1.0;
double shg_y[8192];
int shg_n = 0, zeroth = 0;

static void set_shg(char *file, double wl) {

  static char been_here = 0;
  FILE *fp;
  int i, ibest;
  double best, tmp;

  if(!been_here) {
    been_here = 1;
    meas_scanmate_pro_shg_sync(0, 0); /* shg sync off */
    wl_scale = 2.0;
    if(!(fp = fopen(file, "r"))) {
      fprintf(stderr, "Can't open shg calibration.\n");
      exit(1);
    }
    for (shg_n = 0; shg_n < 8192; shg_n++)
      if(fscanf(fp, " %le %le", &shg_x[shg_n], &shg_y[shg_n]) != 2) break;
    fclose(fp);
  }
  ibest = 0;
  best = fabs(2.0*wl - shg_x[0]);
  for(i = 0; i < shg_n; i++) {
    tmp = fabs(2.0*wl - shg_x[i]);
    if(tmp < best) {
      best = tmp;
      ibest = i;
    }
  }
  printf("Move SHG to %le\n", shg_y[ibest]);
  meas_scanmate_pro_setstp(0, MEAS_SCANMATE_PRO_SHG, (unsigned int) shg_y[ibest]);
  meas_misc_nsleep(1, 0);
}

static int spec = 0;

static void err(char *txt) {

  fprintf(stderr, "%s\n", txt);
  exit(1);
}

void exp_init() {

  /*  meas_pdr2000_init(0, PDR2000);*/
  meas_sr245_init(0, 0, SR245, NULL); /* GPIB mode */
  /*  meas_itc503_init(0, ITC503); */
  meas_scanmate_pro_init(0, SCANMATE_PRO);
  meas_dk240_init(0, DK240);
  meas_bnc565_init(0, 0, BNC565);
}


/* 
 * Graphics modes:
 * 0 = no graphics
 * 1 = Live emission display at fixed wavelength.
 * 2 = Live emission display - full spectrum - one at the time.
 * 3 = Live emission display - full spectrum - pseudo 3D.
 *
 */

static void graph_callback(struct experiment *p) {

  int i, j, k;
  static double *tmp = NULL;
  static int cur = 0;
  double wl;

  if(!tmp) {
    switch(p->display) {
    case 0:
      break;
    case 1:
    meas_graphics_init(0, MEAS_GRAPHICS_XY, 512, 512, MAX_SAMPLES, "exp0");
    meas_graphics_init(1, MEAS_GRAPHICS_XY, 512, 512, MAX_SAMPLES, "exp1");
    break;
    case 2:
      
    if(!(tmp = (double *) malloc(sizeof(double) * MAX_SAMPLES))) {
      fprintf(stderr, "Out of memory.\n");
      exit(1);
    }
  }

  printf("Dye cur = %le nm\n", p->dye_cur);
  switch(p->display) {
  case 0:
    /* no display */
    break;
  case 1:
    /* Live emission intensity at fixed wavelength */
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
    meas_graphics_update_xy(1, p->x2data, &(p->ydata[cur * p->mono_points]), p->mono_points);
    tmp[cur] = 0.0;
    for (k = i; k <= j; k++)
      tmp[cur] += p->ydata[cur * p->mono_points + k];
    cur++;
    meas_graphics_update_xy(0, p->x1data, tmp, cur);
    break;
  case 2:
    /* Live emission spectrum display */
    i = (int) ((p->dye_cur - p->dye_begin) / p->dye_step);
    j = (int) ((p->mono_cur - p->mono_begin) / p->mono_step);
    if(!j) return;
    meas_graphics_update_xy(0, p->x2data, &(p->ydata[i * p->mono_points]), j);
    break;
  case 3:
    /* Live emission spectrum display - pseudo 3D */
    i = (int) ((p->dye_cur - p->dye_begin) / p->dye_step);
    for (j = 0; j < p->mono_points; j++)
      tmp[j] = p->ydata[i * p->mono_points + j] + 100.0 * spec;
    meas_graphics_update_image(0, spec, MEAS_GRAPHICS_WHITE, p->x2data, tmp, p->mono_points);
    spec++;
    break;
  default:
    fprintf(stderr, "Unknown display mode.\n");
    exit(1);
  }
  if(p->display_autoscale) meas_graphics_autoscale(0);
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
  p.signal_ref = -1;
  p.dye_order = 0;
  ti = time(0);
  strcpy(p.date, ctime(&ti));
  p.shg[0] = '\0';
  while(fgets(buf, sizeof(buf), fp)) {
    if(buf[0] == '#' || buf[0] == '\n') continue; /* Skip comment lines */
    if(sscanf(buf, "operator%*[ \t]=%*[ \t]%s", p.operator) == 1) continue;
    if(sscanf(buf, "sample%*[ \t]=%*[ \t]%s", p.sample) == 1) continue;
    if(sscanf(buf, "dye_begin%*[ \t]=%*[ \t]%le", &p.dye_begin) == 1) continue;
    if(sscanf(buf, "dye_end%*[ \t]=%*[ \t]%le", &p.dye_end) == 1) continue;
    if(sscanf(buf, "dye_step%*[ \t]=%*[ \t]%le", &p.dye_step) == 1) continue;
    if(sscanf(buf, "dye_order%*[ \t]=%*[ \t]%d", &p.dye_order) == 1) continue;
    if(sscanf(buf, "mono_begin%*[ \t]=%*[ \t]%le", &p.mono_begin) == 1) continue;
    if(sscanf(buf, "mono_end%*[ \t]=%*[ \t]%le", &p.mono_end) == 1) continue;
    if(sscanf(buf, "mono_step%*[ \t]=%*[ \t]%le", &p.mono_step) == 1) continue;
    if(sscanf(buf, "mono_zeroth%*[ \t]=%*[ \t]%d", &zeroth) == 1) continue;

    if(sscanf(buf, "source%*[ \t]=%*[ \t]%d", &p.signal_source) == 1) continue;
    if(sscanf(buf, "ref-source%*[ \t]=%*[ \t]%d", &p.signal_ref) == 1) continue;    
    if(sscanf(buf, "synchronous%*[ \t]=%*[ \t]%d", &p.synchronous) == 1) continue;
    if(sscanf(buf, "accum%*[ \t]=%*[ \t]%d", &p.accum) == 1) continue;
    if(sscanf(buf, "delay1%*[ \t]=%*[ \t]%le", &p.delay1) == 1) continue;
    if(sscanf(buf, "delay2%*[ \t]=%*[ \t]%le", &p.delay2) == 1) continue;
    if(sscanf(buf, "qswitch1%*[ \t]=%*[ \t]%le", &p.qswitch1) == 1) continue;
    if(sscanf(buf, "qswitch2%*[ \t]=%*[ \t]%le", &p.qswitch2) == 1) continue;
    if(sscanf(buf, "islit%*[ \t]=%*[ \t]%le", &p.islit) == 1) continue;
    if(sscanf(buf, "oslit%*[ \t]=%*[ \t]%le", &p.oslit) == 1) continue;
    if(sscanf(buf, "pmt%*[ \t]=%*[ \t]%le", &p.pmt) == 1) continue;
    if(sscanf(buf, "gate%*[ \t]=%*[ \t]%le", &p.gate) == 1) continue;
    if(sscanf(buf, "output%*[ \t]=%*[ \t]%s", p.output) == 1) continue;
    if(sscanf(buf, "display%*[ \t]=%*[ \t]%d", &p.display) == 1) continue;
    if(sscanf(buf, "display-autoscale%*[ \t]=%*[ \t]%d", &p.display_autoscale) == 1) continue;
    if(sscanf(buf, "display-arg1%*[ \t]=%*[ \t]%le", &p.display_arg1) == 1) continue;
    if(sscanf(buf, "display-arg2%*[ \t]=%*[ \t]%le", &p.display_arg2) == 1) continue;
    if(sscanf(buf, "manual_shg%*[ \t]=%*[ \t]%s", p.shg) == 1) continue;        

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
  if(p.qswitch1 < 0.0) err("Illegal qswitch1 value.\n");
  if(p.qswitch2 < 0.0) err("Illegal qswitch2 value.\n");
  if(p.islit < 0.0) err("Illegal islit value.\n");
  if(p.oslit < 0.0) err("Illegal oslit value.\n");
  if(p.pmt < 0.0) err("Illegal PMT voltage.\n");
  if(p.gate < 0.0) err("Illegal gate value.\n");
  if(!p.output[0]) err("Missing output file name.\n");
  
  p.mono_cur = 0.0; /* Not set */
  p.dye_cur = 0.0;  /* Not set */
  p.dye_points = 1 + (int) (0.5 + (p.dye_end - p.dye_begin) / p.dye_step);
  switch(p.signal_source) {
  case -1: /* Newport Matrix */
    meas_matrix_init();
    p.mono_points = meas_matrix_size();
    p.mono_begin = meas_matrix_calib(0);
    p.mono_end = meas_matrix_calib(p.mono_points - 1);
    p.mono_step = (p.mono_end - p.mono_begin) / (double) (meas_matrix_size() - 1);
    break;
  case 0: /* Newport IS */
    meas_newport_is_init();
    p.mono_points = meas_newport_is_size();
    p.mono_begin = meas_newport_is_calib(0);
    p.mono_end = meas_newport_is_calib(p.mono_points - 1);
    p.mono_step = (p.mono_end - p.mono_begin) / (double) (meas_newport_is_size() - 1);
    break;
  case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: /* SR245 */
    p.mono_points = 1 + (int) (0.5 + (p.mono_end - p.mono_begin) / p.mono_step);
    break;
  default:
    err("Illegal signal source.");
  }

  if(!(p.ydata = (double *) malloc(sizeof(double) * p.dye_points * p.mono_points))) err("Out of memory.");
  if(!(p.x1data = (double *) malloc(sizeof(double) * p.dye_points))) err("Out of memory.");
  if(!(p.x2data = (double *) malloc(sizeof(double) * p.mono_points))) err("Out of memory.");

  if(!access(p.output, R_OK)) {
    fprintf(stderr, "Output file already exists. Remove it first if you want to overwrite.\n");
    return NULL;
  }

  return &p;
}

void exp_setup(struct experiment *p) {

  /* Stop everything */
  laser_stop();

  /* Read pressure & temperature */
  /* p->pressure = meas_pdr2000_read(0, 1);*/
  /*  p->temperature = meas_itc503_read(0); */
  
  /* Setup monochromator */
  if(p->signal_source > 0) { /* Not needed for Newport IS & Matrix */
    meas_dk240_set_slits(0, p->islit, p->oslit);
    if(zeroth)
      meas_dk240_setwl(0, p->mono_begin); /* 0th order */
    else
      meas_dk240_setwl(0, meas_dk240_calib(p->mono_begin));
  }

  /* Setup lasers */
  minilite_qswitch(p->qswitch1);
  surelite_qswitch(p->qswitch2);
  minilite_delay(p->delay1);
  surelite_delay(p->delay2);
  laser_set_delays();

  /* SR245 */
  meas_sr245_reset(0);
  meas_sr245_ports(0,7);       /* all ports are for data input (A/D ports) */
  p->delay3 = meas_sr245_read(0, 7) / 1E4; /* 10 V = 0.001 sec */
  fprintf(stderr, "Boxcar delay = %le s\n", p->delay3);
  meas_sr245_ttl_mode(0, 1, 0); /* Digital I/O port #1 for trigger input */
  meas_sr245_mode(0, p->synchronous); /* Synchronous mode? - trigger at digital port #1 */

  /* Dye laser */
  if(p->shg[0]) set_shg(p->shg, p->dye_begin);
  meas_scanmate_pro_setwl(0, p->dye_begin*wl_scale);

  /* Start the experiment */
  laser_start();
}

void exp_run(struct experiment *p) {

  int i, j, k;
  double tmp;

  /* Update axes */
  for (tmp = p->dye_begin, i = 0; i < p->dye_points; tmp += p->dye_step, i++)
    p->x1data[i] = tmp;
  for (tmp = p->mono_begin, i = 0; i < p->mono_points; tmp += p->mono_step, i++)
    p->x2data[i] = tmp;
  /* Reset spectrum counter */
  spec = 0;

  if(p->signal_source > 0) { /* Data source SR245 */
    /* discard first 2xaccum points */    
    meas_scanmate_pro_grating(0, p->dye_order);
    if(p->shg[0]) set_shg(p->shg, p->dye_begin);
    meas_scanmate_pro_setwl(0, p->dye_begin * wl_scale);

    meas_sr245_enable_trigger(0);
    for (k = 0; k < 2*p->accum; k++) (void) meas_sr245_read(0, p->signal_source);
    meas_sr245_disable_trigger(0);

    for(p->dye_cur = p->dye_begin, i = 0; p->dye_cur <= p->dye_end; p->dye_cur += p->dye_step, i++) {
      meas_scanmate_pro_grating(0, p->dye_order);
      if(p->shg[0]) set_shg(p->shg, p->dye_cur);
      meas_scanmate_pro_setwl(0, p->dye_cur * wl_scale);
      for(p->mono_cur = p->mono_begin, j = 0; p->mono_cur <= p->mono_end; p->mono_cur += p->mono_step, j++)  {
	if(zeroth)
	  meas_dk240_setwl(0, p->mono_cur);	/* otherwise stay at 0th */
	else
	  meas_dk240_setwl(0, meas_dk240_calib(p->mono_cur));	/* otherwise stay at 0th */
	p->ydata[i * p->mono_points + j] = 0.0;
	/* we run under external control of the boxcar */
	for (k = 0; k < p->accum; k++) {
	  double bkg, sig;
	  /* data indexing: (dye, monochromator) - PMT invert */
	  meas_misc_disable_signals();
	  meas_sr245_enable_trigger(0);
	  if(p->signal_ref == -1) 
	    p->ydata[i * p->mono_points + j] += -meas_sr245_read(0, p->signal_source);
	  else {
	    bkg = meas_sr245_read(0, p->signal_ref);
	    sig = meas_sr245_read(0, p->signal_source);
	    p->ydata[i * p->mono_points + j] += -sig / (bkg + 1E-6);
	    /* !!! DEBUG !!! */
	    printf("%d %le %le\n", k, sig, bkg);
	  }
	  meas_sr245_disable_trigger(0);	  
	  meas_misc_enable_signals();
	}
	p->ydata[i * p->mono_points + j] /= (double) p->accum;
	graph_callback(p);
      }
    }
  } else if (p->signal_source == 0) {        /* Data source NewportIS */
    int ext_trig;
    if(p->synchronous) { /* this can be just used to adjust readout delay */
      ext_trig = (int) (p->delay3 * 1000.0);
      if(!ext_trig) ext_trig = 1;
    } else ext_trig = 0;
    sleep(2); /* To allow laser etc. to start up */
    /* Apparently the first read from newport has an odd exposure time */
    /* This could be the background spectrum ? */
    /* PROBLEM: how does this execute without trigger ? */
    meas_newport_is_read(p->gate, ext_trig, p->accum, &(p->ydata[0]));
    memset(&(p->ydata[0]), 0, sizeof(double) * meas_newport_is_size());
    for(p->dye_cur = p->dye_begin, i = 0; p->dye_cur <= p->dye_end; p->dye_cur += p->dye_step, i++) {
      meas_scanmate_pro_grating(0, p->dye_order);
      if(p->shg[0]) set_shg(p->shg, p->dye_cur);
      meas_scanmate_pro_setwl(0, p->dye_cur * wl_scale);
      /* PROBLEM: newportis_read() should execute first and another thread */
      /* should then trigger */
      meas_sr245_write(0, 8, 5.0);
      meas_misc_nsleep(0, 10000);
      meas_sr245_write(0, 8, 0.0);
      meas_misc_nsleep(0, 100000000);
      meas_newport_is_read(p->gate, ext_trig, p->accum, &(p->ydata[i * meas_newport_is_size()]));
      p->mono_cur = p->mono_end;
      graph_callback(p);
    }    
  } else if (p->signal_source == -1) { /* Newport Matrix */
    sleep(2); /* To allow laser etc. to start up */
    meas_matrix_read(p->gate, p->accum, &(p->ydata[0]));
    for(p->dye_cur = p->dye_begin, i = 0; p->dye_cur <= p->dye_end; p->dye_cur += p->dye_step, i++) {
      memset(&(p->ydata[i * meas_matrix_size()]), 0, sizeof(double) * meas_matrix_size());
      meas_scanmate_pro_grating(0, p->dye_order);
      if(p->shg[0]) set_shg(p->shg, p->dye_cur);
      meas_scanmate_pro_setwl(0, p->dye_cur * wl_scale);
      meas_matrix_read(p->gate, p->accum, &(p->ydata[i * meas_matrix_size()]));
      p->mono_cur = p->mono_end;
      graph_callback(p);
    }    
  }

  /* Stop the delay generator */
  laser_stop();
}

int exp_save_data(struct experiment *p) {

  FILE *fp;
  int i, j;

  if(!(fp = fopen(p->output, "w"))) return -1;

  fprintf(fp, "# Experiment date: %s\n", p->date);
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
  fprintf(fp, "# Q-switch delay 1: %le\n", p->qswitch1);
  fprintf(fp, "# Q-switch Delay 2: %le\n", p->qswitch2);
  fprintf(fp, "# Input slit: %lf\n", p->islit);
  fprintf(fp, "# Output slit: %lf\n", p->oslit);
  fprintf(fp, "# Pressure (torr): %le\n", p->pressure);
  fprintf(fp, "# Temperature (K): %le\n", p->temperature);
  fprintf(fp, "# PMT: %lf\n", p->pmt);
  fprintf(fp, "# Gate: %le\n", p->gate);
  fprintf(fp, "# Display mode: %d\n", p->display);
  fprintf(fp, "# Display autoscale: %d\n", p->display_autoscale);
  fprintf(fp, "# Display arg1: %le\n", p->display_arg1);
  fprintf(fp, "# Display arg2: %le\n", p->display_arg2);
  
  switch(p->display) {
  case 0:
  case 2:
  case 3: /* save only the last spectrum - if one wants to save everything,
	     do it in xmgrace */
    for (i = 0; i < p->dye_points; i++) {
      for (j = 0; j < p->mono_points; j++)
	fprintf(fp, "%le %le %le\n", p->x1data[i], p->x2data[j], p->ydata[i * p->mono_points + j]);
      fprintf(fp, "\n");
    }
    break;
  case 1:
    j = (int) ((p->display_arg1 - p->mono_begin) / p->mono_step);
    if (j < 0 || j >= p->mono_points) {
      fprintf(stderr, "Warning: Display argument outside recorded wavelength interval.\n");
      j = 0;
    }
    for (i = 0; i < p->dye_points; i++)
      fprintf(fp, "%le %le\n", p->x1data[i], p->ydata[i * p->mono_points + j]);
    break;
  defaults:
    fprintf(stderr, "Error: unknown display mode. Save failed.\n");
    exit(1);
  }
  fclose(fp);

  return 0;
}
