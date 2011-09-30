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

static double minilite_delay_val = 0.0; /* sec */
static double pmt_delay_val = 0.0; /* sec */

void pmt_delay(double x) {

  pmt_delay_val = x;
}

void minilite_qswitch(double x) {

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

  /* Minilite */
  /* flash lamp */
  meas_bnc565_set(0, 1, 0, 0.0, 10.0E-6, 5.0, 0); /* positive logic / TTL */
  /* Q-switch */
  meas_bnc565_set(0, 2, 0, minilite_delay_val, 10.0E-6, 5.0, 0);
  /* PMT delay */
  meas_bnc565_set(0, 3, 0, pmt_delay_val, 10.0E-6, 5.0, 0);

  /* Enable channels */
  meas_bnc565_enable(0, 1, 1);
  meas_bnc565_enable(0, 2, 1);
  meas_bnc565_enable(0, 3, 1);
  /* modes */
  meas_bnc565_mode(0, 1, 0, 0, 0); /* regular single shot */
  meas_bnc565_mode(0, 2, 0, 0, 0); /* regular single shot */
  meas_bnc565_mode(0, 3, 0, 0, 0); /* regular single shot */
}
