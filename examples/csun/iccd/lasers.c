/*
 * File: lasers.c
 *
 * CSUN: Laser timings and triggering.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "csun.h"
#include <meas/meas.h>

static double surelite_qswitch_val = 300.0E-6; /* sec */
static double surelite_delay_val = 0.0; /* sec */
static double minilite_qswitch_val = 300.0E-6; /* sec */
static double minilite_delay_val = 0.0; /* sec */

void surelite_qswitch(double x) {

  surelite_qswitch_val = x;
}

void minilite_qswitch(double x) {

  minilite_qswitch_val = x;
}

double surelite_int_delay(double x) { /* internal electronics excl. q-switch */

  /*  return SURELITE_DELAY_A0 * exp(SURELITE_DELAY_A1 * x) + SURELITE_DELAY_A2; */
  return SURELITE_DELAY;
}

double minilite_int_delay(double x) { /* internal electronics excl. q-switch */

  return MINILITE_DELAY;
}

void surelite_delay(double x) {

  surelite_delay_val = x;
}

void minilite_delay(double x) {

  minilite_delay_val = x;
}

void laser_stop() {

  meas_bnc565_run(0, 0);
  meas_dg535_run(0, 0);
}

void laser_start() {

  /* Program the external trigger */
  meas_bnc565_trigger(0, MEAS_BNC565_TRIG_INT, 10.0, 0); /* 10 Hz */
  meas_bnc565_run(0, 1);
  meas_dg535_trigger(0, MEAS_DG535_TRIG_EXT, 1.0, MEAS_DG535_TRIG_FALL, MEAS_DG535_IMP_50); /* Trigger at 1.5 V and falling edge (surelite is inverted) */
  meas_dg535_run(0, 1);
}

void laser_set_delays() {

  double tot_minilite, tot_surelite, diff;

  tot_minilite = minilite_int_delay(minilite_qswitch_val) + minilite_qswitch_val - minilite_delay_val;
  tot_surelite = surelite_int_delay(surelite_qswitch_val) + surelite_qswitch_val - surelite_delay_val;

  diff = fabs(tot_minilite - tot_surelite);

  if(tot_minilite > tot_surelite) { /* minilite goes first */
    /* Surelite */
    meas_bnc565_set(0, 1, 0, diff, 10.0E-6, 5.0, 1); /* negative logic / TTL */
    meas_bnc565_set(0, 2, 1, surelite_qswitch_val, 10.0E-6, 5.0, 1);
    /* Minilite */
    meas_bnc565_set(0, 3, 0, 0.0, 10.0E-6, 5.0, 0); /* positive logic / TTL */
    meas_bnc565_set(0, 4, 3, minilite_qswitch_val, 10.0E-6, 5.0, 1); /* negative logic */
  } else { /* Surelite goes first */
    /* Surelite */
    meas_bnc565_set(0, 1, 0, 0.0, 10.0E-6, 5.0, 1); /* negative logic / TTL */
    meas_bnc565_set(0, 2, 1, surelite_qswitch_val, 10.0E-6, 5.0, 1);
    /* Minilite */
    meas_bnc565_set(0, 3, 0, diff, 10.0E-6, 5.0, 0); /* positive logic / TTL */
    meas_bnc565_set(0, 4, 3, minilite_qswitch_val, 10.0E-6, 5.0, 1); /* negative logic */    
  }
  /* Enable channels */
  meas_bnc565_enable(0, 1, 1);
  meas_bnc565_enable(0, 2, 1);
  meas_bnc565_enable(0, 3, 1);
  meas_bnc565_enable(0, 4, 1);
  /* modes */
  meas_bnc565_mode(0, 1, MEAS_BNC565_MODE_SINGLE_SHOT, 0, 0, 0); /* regular single shot */
  meas_bnc565_mode(0, 2, MEAS_BNC565_MODE_SINGLE_SHOT, 0, 0, 0); /* regular single shot */
  meas_bnc565_mode(0, 3, MEAS_BNC565_MODE_SINGLE_SHOT, 0, 0, 0); /* regular single shot */
  meas_bnc565_mode(0, 4, MEAS_BNC565_MODE_SINGLE_SHOT, 0, 0, 0); /* regular single shot */
}

/* Note: CCD delay is given relative to surelite! */

void ccd_set_delays(double delay, double gate) {

  printf("delay = %le\n", delay);
  delay += CCD_DELAY; /* internal triggering delay & 100 ns from surelite */
  delay += surelite_qswitch_val;   /* was surelite delay */
  printf("delay = %le\n", delay);
  if(delay < 0.0) {
    fprintf(stderr, "Error: CCD delay too small.\n");
    exit(1);
  }
  /* A = Start, B = End, AB = intensifier - all TTL */
  meas_dg535_set(0, MEAS_DG535_CHA, MEAS_DG535_T0, delay, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHB, MEAS_DG535_CHA, gate, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHAB, 0, 0.0, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
}
