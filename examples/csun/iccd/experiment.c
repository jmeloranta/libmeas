/*
 *
 * Experiment execution.
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
#include <signal.h>

double zmin_fact = 1.0, zmax_fact = 1.0;
double shg_x[8192], wl_scale = 1.0;
double shg_y[8192];
int roi_s2 = -1, roi_s1, roi_sbin, roi_p2, roi_p1, roi_pbin;
int shg_n = 0, gain, dye_noscan = 0, active_bkg = 0;
double ccd_temp;
double bkg_data[512*512];

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

static void turn_off(int x) {

  laser_stop();
  //  if(p->display == 2) meas_graphics_close();
  exit(0);
}

void exp_init() {

  meas_scanmate_pro_init(0, SCANMATE_PRO);
  meas_bnc565_init(0, 0, BNC565);
  meas_dg535_init(0, 0, DG535);
  meas_pi_max_init(ccd_temp);
  meas_pi_max_gain(gain);
  // meas_pi_max_gain_index(3);  /* 1, 2, 3 */
  meas_pi_max_speed_index(1); /* 0 = 1 MHz, 1 = 5 MHz */
  if(roi_s2 != -1)
    meas_pi_max_roi(roi_s1, roi_s2, roi_sbin, roi_p1, roi_p2, roi_pbin);
  // Turn everything off when ctrl-c hit
  signal(SIGINT, turn_off);
}


/* 
 * Graphics modes (p->display):
 * 0 = Display both the emission and excitation spectrum.
 * 1 = Full display of the CCD element.
 * 2 = Full display of the CCD element (into a PS file).
 *
 */

static void graph_callback(struct experiment *p) {

  int i, j, k;
  static double *tmp = NULL;
  static int cur = 0;
  double wl, zmin, zmax;

  if(!tmp) {
    if(p->display == 0)
      meas_graphics_init(2, NULL, NULL); /* two windows: emission and excitation */
    else {
      if(p->display == 1) 
	meas_graphics_init(1, NULL, NULL); /* one image */
      else
	meas_graphics_init(1, "pngcairo", "/tmp/image.png"); /* slow... */
      meas_graphics_colormap(0, MEAS_GRAPHICS_GRAY); /* or MEAS_GRAPHICS_RGB */
    }
    if(!(tmp = (double *) malloc(sizeof(double) * p->dye_points))) {
      fprintf(stderr, "Out of memory.\n");
      exit(1);
    }
  }

  /* display both emission and exciation */
  if(p->display == 0) {
    
    /* Current emission spectrum */
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
      fprintf(stderr, "display-arg1 > display-arg2. Setting them equal.\n");
      j = i;
    }
    meas_graphics_update2d(1, 0, MEAS_GRAPHICS_WHITE, p->x2data, &(p->ydata[cur * p->mono_points]), p->mono_points);
    /* Current excitation spectrum */
    tmp[cur] = 0.0;
    for (k = i; k <= j; k++)
      tmp[cur] += p->ydata[cur * p->mono_points + k];
    cur++;
    meas_graphics_update2d(0, 0, MEAS_GRAPHICS_WHITE, p->x1data, tmp, cur);
    if(p->display_autoscale) {
      meas_graphics_autoscale(0);
      meas_graphics_autoscale(1);
    }
  }
    
  /* Full display of the CCD element. */
  if(p->display == 1 || p->display == 2) {
    if(p->zscale == 1) meas_graphics_zlog(0, MEAS_GRAPHICS_LOG3D);
    else meas_graphics_zlog(0, MEAS_GRAPHICS_LINEAR3D);
    i = (int) ((p->dye_cur - p->dye_begin) / p->dye_step);
    j = (int) (sqrt(p->mono_points) + 0.5);
    zmin = 1E99; zmax = -zmin;
    for (k = 0; k < j * j; k++) {
      if(p->ydata[i * p->mono_points + k] < zmin) zmin = p->ydata[i * p->mono_points + k];
      if(p->ydata[i * p->mono_points + k] > zmax) zmax = p->ydata[i * p->mono_points + k];
    }
      
    zmin *= zmin_fact;
    zmax *= zmax_fact;
    fprintf(stderr, "zmin = %le, zmax = %le\n", zmin, zmax);
    if(p->display_autoscale) meas_graphics_zscale(0, zmin, zmax);
    /* invert image */
    meas_graphics_xaxis3d(MEAS_GRAPHICS_REV3D);
    meas_graphics_yaxis3d(MEAS_GRAPHICS_REV3D);
    meas_graphics_pixel3d(MEAS_GRAPHICS_SMOOTH3D);
    meas_graphics_update3d(0, &p->ydata[i * p->mono_points], j, j, 0.0, 0.0, 1.0, 1.0);
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
    if(sscanf(buf, "dye_noscan%*[ \t]=%*[ \t]%d", &dye_noscan) == 1) continue;
    if(sscanf(buf, "mono_begin%*[ \t]=%*[ \t]%le", &p.mono_begin) == 1) continue;
    if(sscanf(buf, "mono_end%*[ \t]=%*[ \t]%le", &p.mono_end) == 1) continue;
    if(sscanf(buf, "accum%*[ \t]=%*[ \t]%d", &p.accum) == 1) continue;
    if(sscanf(buf, "delay1%*[ \t]=%*[ \t]%le", &p.delay1) == 1) continue;
    if(sscanf(buf, "delay2%*[ \t]=%*[ \t]%le%*[ \t]%le", &p.delay2, &p.delay2_inc) == 2) continue;
    if(sscanf(buf, "delay3%*[ \t]=%*[ \t]%le", &p.delay3) == 1) continue;
    if(sscanf(buf, "gate%*[ \t]=%*[ \t]%le", &p.gate) == 1) continue;
    if(sscanf(buf, "gain%*[ \t]=%*[ \t]%le", &p.gain) == 1) continue;
    if(sscanf(buf, "roi%*[ \t]=%*[ \t]%d %d %d %d %d %d",
	      &roi_s1, &roi_s2, &roi_sbin, &roi_p1, &roi_p2, &roi_pbin) == 6)
      continue;
    if(sscanf(buf, "qswitch1%*[ \t]=%*[ \t]%le", &p.qswitch1) == 1) continue;
    if(sscanf(buf, "qswitch2%*[ \t]=%*[ \t]%le", &p.qswitch2) == 1) continue;
    if(sscanf(buf, "temp%*[ \t]=%*[ \t]%le", &ccd_temp) == 1) continue;
    if(sscanf(buf, "output%*[ \t]=%*[ \t]%s", p.output) == 1) continue;
    if(sscanf(buf, "display%*[ \t]=%*[ \t]%d", &p.display) == 1) continue;
    if(sscanf(buf, "display-autoscale%*[ \t]=%*[ \t]%d", &p.display_autoscale) == 1) continue;
    if(sscanf(buf, "display-arg1%*[ \t]=%*[ \t]%le", &p.display_arg1) == 1) continue;
    if(sscanf(buf, "display-arg2%*[ \t]=%*[ \t]%le", &p.display_arg2) == 1) continue;
    if(sscanf(buf, "display-zmin%*[ \t]=%*[ \t]%le", &zmin_fact) == 1) continue;
    if(sscanf(buf, "display-zmax%*[ \t]=%*[ \t]%le", &zmax_fact) == 1) continue;
    if(sscanf(buf, "manual_shg%*[ \t]=%*[ \t]%s", p.shg) == 1) continue;
    if(sscanf(buf, "background%*[ \t]=%*[ \t]%d", &p.bkg_sub)== 1) continue;
    if(sscanf(buf, "display-scale%*[ \t]=%*[ \t]%d", &p.zscale)== 1) continue;
    if(sscanf(buf, "active_bkg%*[ \t]=%*[ \t]%d", &active_bkg) == 1) continue;

    fprintf(stderr, "Unknown command: %s\n", buf);
    return NULL;
  }
  fclose(fp);

  /* Sanity checks */
  if(p.dye_begin > p.dye_end) err("dye_begin > dye_end.\n");
  if(p.mono_begin > p.mono_end) err("mono_begin > mono_end.\n");
  if(p.dye_begin == 0.0) err("Illegal dye_begin value.\n");
  if(p.dye_end == 0.0) err("Illegal dye_end value.\n");
  if(p.dye_order < 0 || p.dye_order > 8) err("Illegal grating order (dye).\n");
  if(p.accum < 1) err("Illegal accum value.\n");
  if(p.delay1 < 0.0) err("Illegal delay1 value.\n");
  if(p.delay2 < 0.0) err("Illegal delay2 value.\n");
  if(p.delay3 < 0.0) err("Illegal delay3 value.\n");
  if(p.gate < 0.0) err("Illegal gate value.\n");
  if(p.qswitch1 < 0.0) err("Illegal qswitch1 value.\n");
  if(p.qswitch2 < 0.0) err("Illegal qswitch2 value.\n");
  if(!p.output[0]) err("Missing output file name.\n");
  
  gain = (int) p.gain;
  p.mono_cur = 0.0; /* Not set */
  p.dye_cur = 0.0;  /* Not set */
  p.dye_points = 1 + (int) (0.5 + (p.dye_end - p.dye_begin) / p.dye_step);

  if(!access(p.output, R_OK)) {
    fprintf(stderr, "Output file already exists. Remove it first if you want to overwrite.\n");
    return NULL;
  }

  return &p;
}

void exp_setup(struct experiment *p, int init) {

  /* Stop everything */
  if(init) laser_stop();

  /* Setup lasers */
  minilite_qswitch(p->qswitch1);
  surelite_qswitch(p->qswitch2);
  minilite_delay(p->delay1);
  surelite_delay(p->delay2);
  laser_set_delays();
  ccd_set_delays(p->delay3, p->gate);

  /* Dye laser */
  if(init) {
    if(p->shg[0]) set_shg(p->shg, p->dye_begin);
    meas_scanmate_pro_setwl(0, p->dye_begin*wl_scale);
  }
    
  /* Start the experiment */
  if(init) laser_start();
}

void exp_run(struct experiment *p) {

  int i, j, k;
  double tmp;

  p->mono_points =  meas_pi_max_size();
  p->mono_step = (p->mono_end - p->mono_begin) / (double) (p->mono_points - 1);
  if(!(p->ydata = (double *) malloc(sizeof(double) * p->dye_points * p->mono_points))) err("Out of memory.");
  if(!(p->x1data = (double *) malloc(sizeof(double) * p->dye_points))) err("Out of memory.");
  if(!(p->x2data = (double *) malloc(sizeof(double) * p->mono_points))) err("Out of memory.");

  /* Update axes */
  for (tmp = p->dye_begin, i = 0; i < p->dye_points; tmp += p->dye_step, i++)
    p->x1data[i] = tmp;
  for (tmp = p->mono_begin, i = 0; i < p->mono_points; tmp += p->mono_step, i++)
    p->x2data[i] = tmp;
  /* Reset spectrum counter */
  spec = 0;
  meas_scanmate_pro_grating(0, p->dye_order);
  if(p->shg[0]) set_shg(p->shg, p->dye_begin);
  meas_scanmate_pro_setwl(0, p->dye_begin * wl_scale);

  /* Discard few first samples */
  for (i = 0; i < 10; i++)
    meas_pi_max_read(1, &(p->ydata[0]));

  for(p->dye_cur = p->dye_begin, i = 0; p->dye_cur < p->dye_end; p->dye_cur += p->dye_step, i++) {
    p->delay2 += p->delay2_inc;
    fprintf(stderr, "Current delay between lasers = %le s.\n", p->delay2);
    exp_setup(p, 0);
    if(dye_noscan == 0) {
      if(p->shg[0]) set_shg(p->shg, p->dye_cur);
      meas_scanmate_pro_setwl(0, p->dye_cur * wl_scale);
      printf("dye laser wl = %le nm.\n", p->dye_cur);
    } else {
      if(p->display > 0) {
	i = 0; /* keep going for ever */
	p->dye_cur = p->dye_begin;
      }
    }
    printf("Current CCD temperature = %le\n", meas_pi_max_get_temperature());
    memset(&(p->ydata[i * p->mono_points]), 0, sizeof(double) * p->mono_points);
    meas_pi_max_read(p->accum, &(p->ydata[i * p->mono_points]));
    if(active_bkg) {
      double ave = 0.0;
      for(j = 0; j < p->mono_points; j++)
	ave += p->ydata[i * p->mono_points + j];
      ave /= (double) p->mono_points;
      for(j = 0; j < p->mono_points; j++)
	p->ydata[i * p->mono_points + j] -= ave;
    }

    switch (p->bkg_sub) {
    case 0: /* No background sub */
      break;
    case 1: /* Subtract ablation */
      /* Turn off dye laser */
      meas_bnc565_enable(0, 2, 0);  /* disable Q switch */
      memset(&bkg_data, 0, sizeof(double) * p->mono_points);
      meas_pi_max_read(p->accum, bkg_data);
      for (j = 0; j < p->mono_points; j++)
	p->ydata[i * p->mono_points + j] -= bkg_data[j];
      /* Turn on dye laser */
      meas_bnc565_enable(0, 2, 1);  /* enable Q switch */
      break;
    case 2: /* Subtract dye off */
      /* Turn off ablation laser */
      meas_bnc565_enable(0, 3, 0);
      memset(&bkg_data, 0, sizeof(double) * p->mono_points);
      meas_pi_max_read(p->accum, bkg_data);
      for (j = 0; j < p->mono_points; j++)
	p->ydata[i * p->mono_points + j] -= bkg_data[j];
      /* Turn on ablation laser */
      meas_bnc565_enable(0, 3, 1);
      break;
    case 3: /* Subtract both ablation and dye off */
      /* Dye laser off, ablation on */
      meas_bnc565_enable(0, 2, 0);
      memset(&bkg_data, 0, sizeof(double) * p->mono_points);
      meas_pi_max_read(p->accum, bkg_data);
      for (j = 0; j < p->mono_points; j++)
	p->ydata[i * p->mono_points + j] -= bkg_data[j];
      meas_bnc565_enable(0, 2, 1);
      /*  dye laser on and ablation off */
      meas_bnc565_enable(0, 3, 0);
      memset(&bkg_data, 0, sizeof(double) * p->mono_points);
      meas_pi_max_read(p->accum, bkg_data);
      for (j = 0; j < p->mono_points; j++)
	p->ydata[i * p->mono_points + j] -= bkg_data[j];
      meas_bnc565_enable(0, 3, 1);
      break;
    }
    p->mono_cur = p->mono_end;
    graph_callback(p);
  }

  /* Stop the delay generator */
  laser_stop();
  meas_pi_max_close();

  /* save window data */
  meas_graphics_save("window.dat");
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
  fprintf(fp, "# Accumulations: %d\n", p->accum);
  fprintf(fp, "# Delay 1: %le\n", p->delay1);
  fprintf(fp, "# Delay 2: %le\n", p->delay2);
  fprintf(fp, "# Delay 3: %le\n", p->delay3);
  fprintf(fp, "# Gate: %le\n", p->gate);
  fprintf(fp, "# Q-switch delay 1: %le\n", p->qswitch1);
  fprintf(fp, "# Q-switch Delay 2: %le\n", p->qswitch2);
  fprintf(fp, "# Display mode: %d\n", p->display);
  fprintf(fp, "# Display autoscale: %d\n", p->display_autoscale);
  fprintf(fp, "# Display arg1: %le\n", p->display_arg1);
  fprintf(fp, "# Display arg2: %le\n", p->display_arg2);
  
  switch(p->display) {
  case 1:
  case 2:
    for (i = 0; i < p->dye_points; i++) {
      for (j = 0; j < p->mono_points; j++)
	fprintf(fp, "%le %le %le\n", p->x1data[i], p->x2data[j], p->ydata[i * p->mono_points + j]);
      fprintf(fp, "\n");
    }
    break;
  case 0:
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
