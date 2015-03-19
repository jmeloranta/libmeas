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

double shg_x[8192], wl_scale = 1.0;
double shg_y[8192];
int shg_n = 0;

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
}

static void err(char *txt) {

  fprintf(stderr, "%s\n", txt);
  exit(1);
}

void exp_init() {

  meas_hp34401a_open(0, 0, HP34401);
  meas_scanmate_pro_open(0, SCANMATE_PRO);
  meas_bnc565_open(0, 0, BNC565);
}

/* 
 * Live spectrum display.
 *
 */

static void graph_callback(struct experiment *p) {

  static int cur = 0;

  // TODO: Check that this is reasonable
  if(!cur) meas_graphics_open(1, MEAS_GRAPHICS_XY, 512, 512, 1024, "spectrum");
  /* LIF as a function of extitation wavelength */
  meas_graphics_update_xy(0, p->xdata, p->ydata, cur);
  meas_graphics_autoscale(0);
  meas_graphics_update();
  cur++;
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
    if(sscanf(buf, "accum%*[ \t]=%*[ \t]%d", &p.accum) == 1) continue;
    if(sscanf(buf, "delay1%*[ \t]=%*[ \t]%le", &p.delay1) == 1) continue;
    if(sscanf(buf, "delay2%*[ \t]=%*[ \t]%le", &p.delay2) == 1) continue;
    if(sscanf(buf, "delay3%*[ \t]=%*[ \t]%le", &p.delay3) == 1) continue;
    if(sscanf(buf, "qswitch2%*[ \t]=%*[ \t]%le", &p.qswitch2) == 1) continue;
    if(sscanf(buf, "gate%*[ \t]=%*[ \t]%le", &p.gate) == 1) continue;
    if(sscanf(buf, "gain%*[ \t]=%*[ \t]%d", &p.gain) == 1) continue;    
    if(sscanf(buf, "output%*[ \t]=%*[ \t]%s", p.output) == 1) continue;
    if(sscanf(buf, "manual_shg%*[ \t]=%*[ \t]%s", p.shg) == 1) continue;

    fprintf(stderr, "Unknown command: %s\n", buf);
    puts(buf);
    return NULL;
  }
  fclose(fp);

  /* Sanity checks */
  if(p.dye_begin > p.dye_end) err("dye_begin > dye_end.\n");
  if(p.dye_begin == 0.0) err("Illegal dye_begin value.\n");
  if(p.dye_end == 0.0) err("Illegal dye_end value.\n");
  if(p.dye_step < 0.0) err("Illegal dye_step value.\n");
  if(p.dye_order < 0 || p.dye_order > 8) err("Illegal grating order (dye).\n");
  if(p.accum < 1) err("Illegal accum value.\n");
  if(p.delay1 < 0.0) err("Illegal delay1 value.\n");
  if(p.delay2 < 0.0) err("Illegal delay2 value.\n");
  if(p.delay3 < 0.0) err("Illegal delay3 value.\n");
  if(p.qswitch2 < 0.0) err("Illegal qswitch2 value.\n");
  if(p.gate < 0.0) err("Illegal gate value.\n");
  if(p.gain < 1 || p.gain > 4) err("Illegal gain setting.\n");
  if(!p.output[0]) err("Missing output file name.\n");
  
  p.dye_cur = 0.0;  /* Not set */
  p.dye_points = 1 + (int) (0.5 + (p.dye_end - p.dye_begin) / p.dye_step);

  if(!(p.ydata = (double *) malloc(sizeof(double) * p.dye_points))) err("Out of memory.");
  if(!(p.xdata = (double *) malloc(sizeof(double) * p.dye_points))) err("Out of memory.");

  if(!access(p.output, R_OK)) {
    fprintf(stderr, "Output file already exists. Remove it first if you want to overwrite.\n");
    return NULL;
  }

  return &p;
}

void exp_setup(struct experiment *p) {

  /* Stop everything */
  laser_stop();

  /* Setup lasers */
  surelite_qswitch(p->qswitch2);
  minilite_delay(p->delay1);
  surelite_delay(p->delay2);

  /* setup PMT */
  pmt_delay(p->delay3);
  pmt_gate(p->gate);

  /* program the delay generator */
  laser_set_delays();

  /* Dye laser */
  if(p->shg[0]) set_shg(p->shg, p->dye_begin);
  meas_scanmate_pro_setwl(0, p->dye_begin*wl_scale);

  /* Setup HP34401a */
  meas_hp34401a_set_mode(0, MEAS_HP34401A_MODE_VOLT_DC);
  /* integration time 1s */
  switch(p->gain) {
  case 1: /* 100 mV */
    meas_hp34401a_set_resolution(0, MEAS_HP34401A_MODE_VOLT_DC, MEAS_HP34401A_SCALE_VOLT_100MV, MEAS_HP34401A_RESOL_MAX);
    break;
  case 2: /* 1 V */
    meas_hp34401a_set_resolution(0, MEAS_HP34401A_MODE_VOLT_DC, MEAS_HP34401A_SCALE_VOLT_1V, MEAS_HP34401A_RESOL_MAX);
    break;
  case 3: /* 10 V */
    meas_hp34401a_set_resolution(0, MEAS_HP34401A_MODE_VOLT_DC, MEAS_HP34401A_SCALE_VOLT_10V, MEAS_HP34401A_RESOL_MAX);
    break;
  case 4: /* 100 V */
    meas_hp34401a_set_resolution(0, MEAS_HP34401A_MODE_VOLT_DC, MEAS_HP34401A_SCALE_VOLT_100V, MEAS_HP34401A_RESOL_MAX);
    break;
  }
  fprintf(stderr,"Gain = %d\n", p->gain);

  meas_hp34401a_set_integration_time(0, MEAS_HP34401A_MODE_VOLT_DC, MEAS_HP34401A_INTEGRATION_TIME_20MNLPC);
  meas_hp34401a_set_impedance(0, MEAS_HP34401A_IMPEDANCE_AUTO_ON);    /* 10 GOhm */
  meas_hp34401a_autozero(0, MEAS_HP34401A_AUTOZERO_ON);           /* auto 0 V level */

  /* Start the experiment */
  laser_start();
}

void exp_run(struct experiment *p) {

  int i, j;

  /* Update axes */
  for (p->dye_cur = p->dye_begin, i = 0; i < p->dye_points; p->dye_cur += p->dye_step, i++)
    p->xdata[i] = p->dye_cur;

  /* set the initial wavelength */     
  meas_scanmate_pro_grating(0, p->dye_order);
  if(p->shg[0]) set_shg(p->shg, p->dye_begin);
  meas_scanmate_pro_setwl(0, p->dye_begin * wl_scale);

  for(p->dye_cur = p->dye_begin, i = 0; p->dye_cur <= p->dye_end; p->dye_cur += p->dye_step, i++) {
    meas_scanmate_pro_setwl(0, p->dye_cur * wl_scale);
    if(p->shg[0]) set_shg(p->shg, p->dye_cur);
    p->ydata[i] = 0.0;
    sleep((int) (1.0 + p->accum / 10.0));
    meas_hp34401a_initiate_read(0);
    /*    sleep(3); */
    p->ydata[i] = -meas_hp34401a_complete_read(0); /* invert for PMT */
    graph_callback(p);
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
  fprintf(fp, "# Accumulations: %d\n", p->accum);
  fprintf(fp, "# Delay 1: %le\n", p->delay1);
  fprintf(fp, "# Delay 2: %le\n", p->delay2);
  fprintf(fp, "# Delay 3: %le\n", p->delay3);
  fprintf(fp, "# Q-switch Delay 2: %le\n", p->qswitch2);
  fprintf(fp, "# Gate: %le\n", p->gate);
  
  for (i = 0; i < p->dye_points-1; i++)
    fprintf(fp, "%le %le\n", p->xdata[i], p->ydata[i]);
  fclose(fp);

  return 0;
}
