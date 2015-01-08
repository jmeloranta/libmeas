/*
 * File: lasers.c
 *
 * CSUN: Laser timings and triggering.
 *
 */

#include <stdio.h>
#include <math.h>
#include "csun.h"
#include <meas/meas.h>

static double surelite_qswitch_val = 300.0E-6; /* sec */
static double surelite_delay_val = 0.0; /* sec */
static double minilite_delay_val = 0.0; /* sec */
static double pmt_delay_val = 0.0; /* sec */
static double pmt_gate_val = 0.0; /* sec */

void pmt_delay(double x) {

  pmt_delay_val = x;
}

void pmt_gate(double x) {

  pmt_gate_val = x;
}

void surelite_qswitch(double x) {

  surelite_qswitch_val = x;
}

double surelite_int_delay(double x) { /* internal electronics excl. q-switch */

  /*  return SURELITE_DELAY_A0 * exp(SURELITE_DELAY_A1 * x) + SURELITE_DELAY_A2; */
  return SURELITE_DELAY;
}

double minilite_int_delay() { /* internal electronics excl. q-switch */

  return 157.8E-6; /* delay between trigger in and laser pulse out */
  /*  return MINILITE_DELAY; */
}

void surelite_delay(double x) {

  surelite_delay_val = x;
}

void minilite_delay(double x) {

  minilite_delay_val = x;
}

void laser_stop() {

  meas_bnc565_run(0, 0);
}

void laser_start() {

  /* Program the external trigger */
  /* internal trigger */
  meas_bnc565_trigger(0, 1, 10.0, 0); /* internal trigger at 10 Hz */
  meas_bnc565_run(0, 1);
}

/* setup lasers and PMT on ch4 */
void laser_set_delays() {

  double tot_minilite, tot_surelite, diff;

  /*  tot_minilite = minilite_int_delay(minilite_qswitch_val) + minilite_qswitch_val - minilite_delay_val; */
  tot_minilite = minilite_int_delay() - minilite_delay_val;
  tot_surelite = surelite_int_delay(surelite_qswitch_val) + surelite_qswitch_val - surelite_delay_val;

  diff = fabs(tot_minilite - tot_surelite);

  if(tot_minilite > tot_surelite) { /* minilite goes first */
    /* Surelite */
    meas_bnc565_set(0, 1, 0, diff, 10.0E-6, 5.0, 1); /* negative logic / TTL */
    meas_bnc565_set(0, 2, 1, surelite_qswitch_val, 10.0E-6, 5.0, 1);
    /* Minilite */
    meas_bnc565_set(0, 3, 0, 0.0, 10.0E-6, 5.0, 0); /* positive logic / TTL */
    /* meas_bnc565_set(0, 4, 3, minilite_qswitch_val, 10.0E-6, 5.0, 0);  */
    /* PMT delay */
    meas_bnc565_set(0, 4, 3, pmt_delay_val, pmt_gate_val, 5.0, 0);
  } else { /* Surelite goes first */
    /* Surelite */
    meas_bnc565_set(0, 1, 0, 0.0, 10.0E-6, 5.0, 1); /* negative logic / TTL */
    meas_bnc565_set(0, 2, 1, surelite_qswitch_val, 10.0E-6, 5.0, 1);
    /* Minilite */
    meas_bnc565_set(0, 3, 0, diff, 10.0E-6, 5.0, 0); /* positive logic / TTL */
    /* meas_bnc565_set(0, 4, 3, minilite_qswitch_val, 10.0E-6, 5.0, 0); */
    /* PMT delay */
    meas_bnc565_set(0, 4, 3, pmt_delay_val, pmt_gate_val, 5.0, 0);
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
