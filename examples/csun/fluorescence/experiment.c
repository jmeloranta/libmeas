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

static void err(char *txt) {

  fprintf(stderr, "%s\n", txt);
  exit(1);
}

void exp_init() {

  meas_hp34401a_init(0, 0, HP34401);
  meas_bnc565_init(0, 0, BNC565);
  meas_dk240_init(0, DK240);
}

/* 
 * Live spectrum display.
 *
 */

static void graph_callback(struct experiment *p) {

  static int cur = 0;

  if(!cur) meas_graphics_init(1, MEAS_GRAPHICS_XY, 512, 512, 1024, "graph"); /* TODO: are these sensible values ? */
  /* fluorescence as a function of emission wavelength */
  meas_graphics_update2d(0, p->xdata, p->ydata, cur);
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
  ti = time(0);
  strcpy(p.date, ctime(&ti));
  while(fgets(buf, sizeof(buf), fp)) {
    if(buf[0] == '#' || buf[0] == '\n') continue; /* Skip comment lines */
    if(sscanf(buf, "operator%*[ \t]=%*[ \t]%s", p.operator) == 1) continue;
    if(sscanf(buf, "sample%*[ \t]=%*[ \t]%s", p.sample) == 1) continue;

    if(sscanf(buf, "mono_begin%*[ \t]=%*[ \t]%le", &p.mono_begin) == 1) continue;
    if(sscanf(buf, "mono_end%*[ \t]=%*[ \t]%le", &p.mono_end) == 1) continue;
    if(sscanf(buf, "mono_step%*[ \t]=%*[ \t]%le", &p.mono_step) == 1) continue;
    if(sscanf(buf, "mono_islit%*[ \t]=%*[ \t]%le", &p.islit) == 1) continue;
    if(sscanf(buf, "mono_oslit%*[ \t]=%*[ \t]%le", &p.oslit) == 1) continue;
    if(sscanf(buf, "accum%*[ \t]=%*[ \t]%d", &p.accum) == 1) continue;
    if(sscanf(buf, "delay1%*[ \t]=%*[ \t]%le", &p.delay1) == 1) continue;
    p.delay2 = 0.0;
    if(sscanf(buf, "delay3%*[ \t]=%*[ \t]%le", &p.delay3) == 1) continue;
    if(sscanf(buf, "gain%*[ \t]=%*[ \t]%d", &p.gain) == 1) continue;    
    if(sscanf(buf, "output%*[ \t]=%*[ \t]%s", p.output) == 1) continue;

    fprintf(stderr, "Unknown command: %s\n", buf);
    puts(buf);
    return NULL;
  }
  fclose(fp);

  /* Sanity checks */
  if(p.mono_begin > p.mono_end) err("mono_begin > mono_end.\n");
  if(p.mono_begin == 0.0) err("Illegal mono_begin value.\n");
  if(p.mono_end == 0.0) err("Illegal mono_end value.\n");
  if(p.mono_step < 0.0) err("Illegal mono_step value.\n");
  if(p.accum < 1) err("Illegal accum value.\n");
  if(p.delay3 < 0.0) err("Illegal delay3 value.\n");
  if(p.gain < 1 || p.gain > 4) err("Illegal gain setting.\n");
  if(!p.output[0]) err("Missing output file name.\n");
  
  p.mono_cur = 0.0;  /* Not set */
  p.mono_points = 1 + (int) (0.5 + (p.mono_end - p.mono_begin) / p.mono_step);

  if(!(p.ydata = (double *) malloc(sizeof(double) * p.mono_points))) err("Out of memory.");
  if(!(p.xdata = (double *) malloc(sizeof(double) * p.mono_points))) err("Out of memory.");

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
  minilite_qswitch(p->delay1); /* not in use */

  /* setup PMT */
  pmt_delay(p->delay3);

  /* program the delay generator */
  laser_set_delays();

  /* Dye laser */
  meas_dk240_setwl(0, p->mono_begin);

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
  for (p->mono_cur = p->mono_begin, i = 0; i < p->mono_points; p->mono_cur += p->mono_step, i++)
    p->xdata[i] = p->mono_cur;

  /* set slits */
  meas_dk240_set_slits(0, p->islit, p->oslit);

  /* set the initial wavelength */     
  meas_dk240_setwl(0, p->mono_begin);

  for(p->mono_cur = p->mono_begin, i = 0; p->mono_cur <= p->mono_end; p->mono_cur += p->mono_step, i++) {
    meas_dk240_setwl(0, p->mono_cur);
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
  fprintf(fp, "# Monochromator laser begin: %lf\n", p->mono_begin);
  fprintf(fp, "# Monochromator laser end: %lf\n", p->mono_end);
  fprintf(fp, "# Monochromator laser step: %lf\n", p->mono_step);
  fprintf(fp, "# Monochromator input slit: %lf\n", p->islit);
  fprintf(fp, "# Monochromator output slit: %lf\n", p->oslit);

  fprintf(fp, "# Accumulations: %d\n", p->accum);
  fprintf(fp, "# Delay 1: %le\n", p->delay1);
  fprintf(fp, "# Delay 3: %le\n", p->delay3);
  fprintf(fp, "# Gain: %d\n", p->gain);
  
  for (i = 0; i < p->mono_points-1; i++)
    fprintf(fp, "%le %le\n", p->xdata[i], p->ydata[i]);
  fclose(fp);

  return 0;
}
