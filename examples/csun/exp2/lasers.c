/*
 * File: lasers.c
 *
 * CSUN: Laser timings and triggering.
 *
 */

#include <meas/meas.h>
#include <math.h>
#include "csun.h"

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

  meas_bnc565_run(0, MEAS_BNC565_STOP);
}

void laser_start() {

  /* Program the external trigger */
  meas_bnc565_trigger(0, MEAS_BNC565_TRIG_EXT, 1.0, MEAS_BNC565_TRIG_RISE); /* Trigger at 1.0 V and rising edge */
  meas_bnc565_run(0, MEAS_BNC565_RUN);
}

void laser_set_delays() {

  double tot_minilite, tot_surelite, diff;

  tot_minilite = minilite_int_delay(minilite_qswitch_val) + minilite_qswitch_val - minilite_delay_val;
  tot_surelite = surelite_int_delay(surelite_qswitch_val) + surelite_qswitch_val - surelite_delay_val;

  diff = fabs(tot_minilite - tot_surelite);

  if(tot_minilite > tot_surelite) { /* minilite goes first */
    /* Surelite */
    meas_bnc565_set(0, MEAS_BNC565_CHA, MEAS_BNC565_T0, diff, 10.0E-6, 5.0, MEAS_BNC565_POL_INV); /* negative logic / TTL */
    meas_bnc565_set(0, MEAS_BNC565_CHB, MEAS_BNC565_CHA, surelite_qswitch_val, 10.0E-6, 5.0, MEAS_BNC565_POL_INV);
    /* Minilite */
    meas_bnc565_set(0, MEAS_BNC565_CHC, MEAS_BNC565_T0, 0.0, 10.0E-6, 5.0, MEAS_BNC565_POL_NORM); /* positive logic / TTL */
    meas_bnc565_set(0, MEAS_BNC565_CHD, MEAS_BNC565_CHC, minilite_qswitch_val, 10.0E-6, 5.0, MEAS_BNC565_POL_NORM);
  } else { /* Surelite goes first */
    /* Surelite */
    meas_bnc565_set(0, MEAS_BNC565_CHA, MEAS_BNC565_T0, 0.0, 10.0E-6, 5.0, MEAS_BNC565_POL_INV); /* negative logic / TTL */
    meas_bnc565_set(0, MEAS_BNC565_CHB, MEAS_BNC565_CHA, surelite_qswitch_val, 10.0E-6, 5.0, MEAS_BNC565_POL_INV);
    /* Minilite */
    meas_bnc565_set(0, MEAS_BNC565_CHC, MEAS_BNC565_T0, diff, 10.0E-6, 5.0, MEAS_BNC565_POL_NORM); /* positive logic / TTL */
    meas_bnc565_set(0, MEAS_BNC565_CHD, MEAS_BNC565_CHC, minilite_qswitch_val, 10.0E-6, 5.0, MEAS_BNC565_POL_NORM);
  }
  /* Enable channels */
  meas_bnc565_enable(0, MEAS_BNC565_CHA, MEAS_BNC565_CHANNEL_ENABLE);
  meas_bnc565_enable(0, MEAS_BNC565_CHB, MEAS_BNC565_CHANNEL_ENABLE);
  meas_bnc565_enable(0, MEAS_BNC565_CHC, MEAS_BNC565_CHANNEL_ENABLE);
  meas_bnc565_enable(0, MEAS_BNC565_CHD, MEAS_BNC565_CHANNEL_ENABLE);
  /* modes */
  meas_bnc565_mode(0, MEAS_BNC565_CHA, MEAS_BNC565_MODE_SINGLE_SHOT, 0, 0, 0); /* regular single shot */
  meas_bnc565_mode(0, MEAS_BNC565_CHB, MEAS_BNC565_MODE_SINGLE_SHOT, 0, 0, 0); /* regular single shot */
  meas_bnc565_mode(0, MEAS_BNC565_CHC, MEAS_BNC565_MODE_SINGLE_SHOT, 0, 0, 0); /* regular single shot */
  meas_bnc565_mode(0, MEAS_BNC565_CHD, MEAS_BNC565_MODE_SINGLE_SHOT, 0, 0, 0); /* regular single shot */
}
