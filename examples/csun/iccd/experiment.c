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

/* CCD dimensions */
#define NX 512
#define NY 512

double zmin_fact = 1.0, zmax_fact = 1.0;
double shg_x[8192], wl_scale = 1.0;
double shg_y[8192], gate;
int roi_s2 = -1, roi_s1, roi_sbin, roi_p2, roi_p1, roi_pbin;
int shg_n = 0, gain, dye_noscan = 0, active_bkg = 0, diode_bkg = 0;
double ccd_temp, diode_bkg_wl1 = -1.0, diode_bkg_wl2, diode_bkg_val1, diode_bkg_val2;
double bkg_data[NX*NY];

unsigned char rr[512*512], gg[512*512], bb[512*512];

double diode_corr(double val) {

  double tmp;

  if(diode_bkg_wl1 == -1.0) return 1.0;
  if(val < diode_bkg_wl1 || val > diode_bkg_wl2) {
    fprintf(stderr, "Diode background correction wavelength out of range.\n");
    exit(1);
  }
  tmp = (val - diode_bkg_wl1) / (diode_bkg_wl2 - diode_bkg_wl1);
  return tmp * diode_bkg_val2 + (1.0 - tmp) * diode_bkg_val1;
}

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
  meas_graphics_close();
  exit(0);
}

void exp_init() {

  meas_scanmate_pro_init(0, SCANMATE_PRO);
  meas_bnc565_init(0, 0, BNC565);
  meas_dg535_init(0, 0, DG535);
  meas_pdr900_init(0, PDR900);
  meas_itc503_init(0, ITC503);
  meas_pdr2000_init(0, PDR2000);

  if(diode_bkg) {
    meas_hp34401a_init(0, 0, HP34401A);
    meas_hp34401a_set_autoscale(0, MEAS_HP34401A_AUTOSCALE_VOLT_DC, 1);
    meas_hp34401a_set_resolution(0, MEAS_HP34401A_MODE_VOLT_DC, MEAS_HP34401A_SCALE_VOLT_10V, MEAS_HP34401A_RESOL_MAX);
    meas_hp34401a_set_integration_time(0, MEAS_HP34401A_MODE_VOLT_DC, MEAS_HP34401A_INTEGRATION_TIME_100NLPC);
    meas_hp34401a_autozero(0, MEAS_HP34401A_AUTOZERO_ONCE);
    meas_hp34401a_set_mode(0, MEAS_HP34401A_MODE_VOLT_DC);
    meas_hp34401a_set_trigger_source(0, MEAS_HP34401A_TRIGGER_IMMEDIATE);
  }
  if(gate >= 0.0) {
    meas_pi_max_init(ccd_temp);
    meas_pi_max_gain(gain);
    // meas_pi_max_gain_index(3);  /* 1, 2, 3 */
    meas_pi_max_speed_index(1); /* 0 = 1 MHz, 1 = 5 MHz */
    if(roi_s2 != -1)
      meas_pi_max_roi(roi_s1, roi_s2, roi_sbin, roi_p1, roi_p2, roi_pbin);
  }
  // Turn everything off when ctrl-c hit
  signal(SIGINT, turn_off);
}


/* 
 * Graphics modes (p->display):
 * 0 = Display both the emission and excitation spectrum.
 * 1 = Full display of the CCD element.
 * 2 = Full display of the CCD element (into a PS file).
 * 3 = No display.
 *
 */

static void graph_callback(struct experiment *p) {

  int i, j, k;
  static double *tmp = NULL;
  static int cur = 0;
  double wl, zmin, zmax;

  if(p->display == 3) return;

  if(!tmp) {
    if(p->display == 0) {
      meas_graphics_init(0, MEAS_GRAPHICS_2D, 512, 512, 0, "Fluorescence");
      meas_graphics_init(1, MEAS_GRAPHICS_2D, 512, 512, 0, "Excitation");
    } else
      if(p->display == 1) 
	meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, NX, NY, 0, "CCD image"); /* one image */
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
    meas_graphics_update_xy(1, p->x2data, &(p->ydata[cur * p->mono_points]), p->mono_points);
    /* Current excitation spectrum */
    tmp[cur] = 0.0;
    for (k = i; k <= j; k++)
      tmp[cur] += p->ydata[cur * p->mono_points + k];
    cur++;
    meas_graphics_update2d(0, p->x1data, tmp, cur);
    if(p->display_autoscale) {
      meas_graphics_autoscale(0);
      meas_graphics_autoscale(1);
    }
  }
    
  /* Full display of the CCD element. */
  if(p->display == 1) {
    i = (int) ((p->dye_cur - p->dye_begin) / p->dye_step); /* image position in memory */
    zmin = 1E99; zmax = -zmin;
    for (k = 0; k < NX * NY; k++) {
      if(p->ydata[i * p->mono_points + k] < zmin) zmin = p->ydata[i * p->mono_points + k];
      if(p->ydata[i * p->mono_points + k] > zmax) zmax = p->ydata[i * p->mono_points + k];
    }
    for(k = 0; k < NX * NY; k++) /* scale between 0 and 1 */
      p->ydata[i * p->mono_points + k] = (p->ydata[i * p->mono_points + k] - zmin) / (zmax - zmin); 
    for (k = 0; k < NX * NY; k++) /* invert axes & convert to rgb */
      meas_graphics_rgb(p->ydata[i * p->mono_points + k], &rr[NX * NY - k - 1], &gg[NX * NY - k - 1], &bb[NX * NY - k - 1]);
    meas_graphics_update_image(0, rr, gg, bb);
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
    if(sscanf(buf, "delay1%*[ \t]=%*[ \t]%le%*[ \t]%le", &p.delay1, &p.delay1_inc) == 2) continue;
    if(sscanf(buf, "delay2%*[ \t]=%*[ \t]%le%*[ \t]%le", &p.delay2, &p.delay2_inc) == 2) continue;
    if(sscanf(buf, "delay3%*[ \t]=%*[ \t]%le %le", &p.delay3, &p.delay3_inc) == 2) continue;
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
    if(sscanf(buf, "dye_bkg%*[ \t]=%*[ \t]%d", &diode_bkg) == 1) continue;
    if(sscanf(buf, "dye_bkg_wl1%*[ \t]=%*[ \t]%le", &diode_bkg_wl1) == 1) continue;
    if(sscanf(buf, "dye_bkg_wl2%*[ \t]=%*[ \t]%le", &diode_bkg_wl2) == 1) continue;
    if(sscanf(buf, "dye_bkg_val1%*[ \t]=%*[ \t]%le", &diode_bkg_val1) == 1) continue;
    if(sscanf(buf, "dye_bkg_val2%*[ \t]=%*[ \t]%le", &diode_bkg_val2) == 1) continue;

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
  if(p.gate < 0.0)
    fprintf(stderr,"Negative gate: ICCD disabled.\n");
  gate = p.gate;
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
  if(p->gate >= 0.0) ccd_set_delays(p->delay3, p->gate);

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
  double tmp, diode;

  if(p->gate >= 0.0)
    p->mono_points =  meas_pi_max_size();
  else
    p->mono_points = 512;
  p->mono_step = (p->mono_end - p->mono_begin) / (double) (p->mono_points - 1);
  if(!(p->ydata = (double *) malloc(sizeof(double) * p->dye_points * p->mono_points))) err("Out of memory.");
  if(!(p->x1data = (double *) malloc(sizeof(double) * p->dye_points))) err("Out of memory.");
  if(!(p->x2data = (double *) malloc(sizeof(double) * p->mono_points))) err("Out of memory.");

  /* Read: vacuum shroud pressure, cryostat temperature, cryostat pressure */
  p->vacP = meas_pdr900_read(0, 1);
  p->temp = meas_itc503_read(0);
  p->pres = meas_pdr2000_read(0, 1);
  fprintf(stderr, "P_shroud = %le torr, T = %le K, P_cryo = %le torr.\n",
	  p->vacP, p->temp, p->pres);

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
  if(diode_bkg) meas_hp34401a_initiate_read(0);

  if(p->gate >= 0.0) {
    /* Discard few first samples */
    for (i = 0; i < 10; i++) 
      meas_pi_max_read(1, &(p->ydata[0]));
  }
  if(diode_bkg) (void) meas_hp34401a_complete_read(0);

  /* Main scan loop */
  for(p->dye_cur = p->dye_begin, i = 0; p->dye_cur < p->dye_end; p->dye_cur += p->dye_step, i++) {

    if(p->delay2_inc > 0.0 || p->delay1_inc > 0.0) {
      p->delay1 += p->delay1_inc;
      p->delay2 += p->delay2_inc;
      fprintf(stderr, "Current delay between lasers = %le s.\n", fabs(p->delay1 - p->delay2));
      laser_stop();
      exp_setup(p, 0);
      laser_start();
    }
    if(p->delay3_inc > 0.0) {
      p->delay3 += p->delay3_inc;
      fprintf(stderr, "Current delay between laser and CCD = %le s.\n", p->delay3);
      ccd_set_delays(p->delay3, p->gate);
    }
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
    if(diode_bkg) meas_hp34401a_initiate_read(0);
    if(p->gate >= 0.0)
      printf("Current CCD temperature = %le\n", meas_pi_max_get_temperature());
    memset(&(p->ydata[i * p->mono_points]), 0, sizeof(double) * p->mono_points);
    if(p->gate >= 0.0)
      meas_pi_max_read(p->accum, &(p->ydata[i * p->mono_points]));
    if(active_bkg) { /* 10 last points from the long wavelength side */
      double ave = 0.0;
      for(j = p->mono_points-10; j < p->mono_points; j++)
	ave += p->ydata[i * p->mono_points + j];
      ave /= 10.0;
      for(j = 0; j < p->mono_points; j++)
	p->ydata[i * p->mono_points + j] -= ave;
    }
    if(diode_bkg) {
      diode = meas_hp34401a_complete_read(0);
      fprintf(stderr, "Diode background level = %le V.\n", diode);
      diode /= diode_corr(p->dye_cur);
      fprintf(stderr, "Diode background level after correction = %le V.\n", diode);
      for (j = 0; j < p->mono_points; j++)
	p->ydata[i * p->mono_points + j] /= diode + 1E-6;
    }

    switch (p->bkg_sub) {
    case 0: /* No background sub */
      break;
    case 1: /* Subtract ablation */
      /* Turn off dye laser */
      meas_bnc565_enable(0, 2, 0);  /* disable Q switch */
      memset(&bkg_data, 0, sizeof(double) * p->mono_points);
      if(p->gate >= 0.0)
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
      if(p->gate >= 0.0)
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
      if(p->gate >= 0.0)
	meas_pi_max_read(p->accum, bkg_data);
      for (j = 0; j < p->mono_points; j++)
	p->ydata[i * p->mono_points + j] -= 0.5*bkg_data[j];
      meas_bnc565_enable(0, 2, 1);
      /*  dye laser on and ablation off */
      meas_bnc565_enable(0, 3, 0);
      memset(&bkg_data, 0, sizeof(double) * p->mono_points);
      if(p->gate >= 0.0)
	meas_pi_max_read(p->accum, bkg_data);
      for (j = 0; j < p->mono_points; j++)
	p->ydata[i * p->mono_points + j] -= 0.5*bkg_data[j];
      meas_bnc565_enable(0, 3, 1);
      break;
    }
    p->mono_cur = p->mono_end;
    graph_callback(p);
  }

  /* Stop the delay generator */
  laser_stop();
  if(p->gate >= 0.0)
    meas_pi_max_close();

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
  fprintf(fp, "# Vacuum shroud pressure (torr): %le\n", p->vacP);
  fprintf(fp, "# Cryostat temperature (K): %le\n", p->temp);
  fprintf(fp, "# Cryostat pressure (torr): %le\n", p->pres);
  fprintf(fp, "# Data points follow (loop 1. over dye and 2. mono).\n");
  
  for (i = 0; i < p->dye_points-1; i++) {
    for (j = 0; j < p->mono_points; j++)
      fprintf(fp, "%le %le %le\n", p->x1data[i], p->x2data[j], p->ydata[i * p->mono_points + j]);
    fprintf(fp, "\n");
  }
  fclose(fp);

  return 0;
}
