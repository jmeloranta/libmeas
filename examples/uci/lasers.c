/*
 * File: lasers.c
 *
 * UCI: Laser timings and triggering.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <math.h>
#include "uci.h"
#include <meas/meas.h>

static double surelite_qswitch_val = 300.0E-6; /* sec */
static double surelite_delay_val = 0.0; /* sec */
static double excimer_delay_val = 300.0E-6; /* sec */

void surelite_qswitch(double x) {

  surelite_qswitch_val = x;
}

double surelite_int_delay(double x) { /* internal electronics excl. q-switch */

  return SURELITE_DELAY;
}

double excimer_int_delay() { /* firing delay */

  return EXCIMER_DELAY;
}

void surelite_delay(double x) {

  surelite_delay_val = x;
}

void excimer_delay(double x) {

  excimer_delay_val = x;
}

void laser_stop() {

  meas_dg535_run(0, 0);
}

void laser_start() {

  /* Delay generator 1 (master clock) */
  meas_dg535_trigger(0, MEAS_DG535_TRIG_INT, 16.0, 0,0);

  /* Delay generator 2 (external trigger) */
  meas_dg535_trigger(1, MEAS_DG535_TRIG_EXT, 2.0, MEAS_DG535_TRIG_RISE, MEAS_DG535_IMP_50);

  meas_dg535_run(0, 1);
  meas_dg535_run(1, 1);
}

void laser_set_delays() {

  double tot_excimer, tot_surelite, diff;

  tot_excimer = excimer_int_delay() - excimer_delay_val;
  tot_surelite = surelite_int_delay(surelite_qswitch_val) + surelite_qswitch_val - surelite_delay_val;

  diff = fabs(tot_excimer - tot_surelite);

  printf("exc = %le, sure = %le, diff = %le\n", tot_excimer, tot_surelite, diff);

  meas_dg535_set(0, MEAS_DG535_T0, 0, 0.0, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  if(tot_excimer > tot_surelite) { /* excimer goes first */
    /* Surelite */
    meas_dg535_set(0, MEAS_DG535_CHA, MEAS_DG535_T0, diff, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
    meas_dg535_set(0, MEAS_DG535_CHB, MEAS_DG535_CHA, 10E-6, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
    meas_dg535_set(0, MEAS_DG535_CHC, MEAS_DG535_CHA, surelite_qswitch_val, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
    meas_dg535_set(0, MEAS_DG535_CHD, MEAS_DG535_CHC, 10E-6, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
    meas_dg535_set(0, MEAS_DG535_CHAB, 0, 0.0, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_HIGH);
    meas_dg535_set(0, MEAS_DG535_CHCD, 0, 0.0, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_HIGH);
    /* excimer, output voltage: 10 x 1.5 V */
    meas_dg535_set(1, MEAS_DG535_CHA, MEAS_DG535_T0, 0.0, 1.5, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_HIGH);
  } else { /* Surelite goes first */
    /* Surelite */
    meas_dg535_set(0, MEAS_DG535_CHA, MEAS_DG535_T0, 0.0, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
    meas_dg535_set(0, MEAS_DG535_CHB, MEAS_DG535_CHA, 10E-6, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
    meas_dg535_set(0, MEAS_DG535_CHC, MEAS_DG535_CHA, surelite_qswitch_val, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
    meas_dg535_set(0, MEAS_DG535_CHD, MEAS_DG535_CHC, 10E-6, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
    meas_dg535_set(0, MEAS_DG535_CHAB, 0, 0.0, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_HIGH);
    meas_dg535_set(0, MEAS_DG535_CHCD, 0, 0.0, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_HIGH);
    /* excimer, output voltage: 10 x 1.5 V */
    meas_dg535_set(1, MEAS_DG535_CHA, MEAS_DG535_T0, diff, 1.5, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_HIGH);
  }
}

/*
 * In addition to lasers, we need to setup the PI-MAX2 CCD.
 *
 * (i.e. p.delay3 and p.gate)
 *
 */

void ccd_set_delays(double delay, double gate) {

  double tot_excimer, tot_surelite, diff, tot_delay;

  if(delay < excimer_delay_val || delay < surelite_delay_val) {
    fprintf(stderr, "Error: CCD delay smaller than either laser delay.\n");
    exit(1);
  }

  tot_excimer = excimer_int_delay() - excimer_delay_val;
  tot_surelite = surelite_int_delay(surelite_qswitch_val) + surelite_qswitch_val - surelite_delay_val;
  diff = fabs(tot_excimer - tot_surelite);

  if(tot_excimer > tot_surelite) { /* excimer goes first */
    delay += tot_excimer;
  } else { /* surelite goes first */
    delay += tot_surelite;
  }
  delay -= CCD_DELAY; /* internal triggering delay */
  if(delay < 0.0) {
    fprintf(stderr, "Error: CCD delay too small.\n");
    exit(1);
  }
  /* C = Start, D = End, CD = intensifier - all TTL */
  meas_dg535_set(1, MEAS_DG535_CHC, MEAS_DG535_T0, delay, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(1, MEAS_DG535_CHD, MEAS_DG535_CHC, gate, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(1, MEAS_DG535_CHCD, 0, 0.0, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
}
