/* filename: dg535.c
 *
 * Stanford Research Systems DG535 delay generator.
 *
 * TODO: error checking.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dg535.h"
#include "gpib.h"
#include "misc.h"

/* Up to 5 devices supported */
static int dg535_fd[5] = {-1, -1, -1, -1, -1};
static double vals[5][8];
static int trigger_source[5] = {MEAS_DG535_TRIG_EXT, MEAS_DG535_TRIG_EXT, MEAS_DG535_TRIG_EXT, MEAS_DG535_TRIG_EXT, MEAS_DG535_TRIG_EXT};
static double reprate[5] = {1.0, 1.0, 1.0, 1.0, 1.0};

/* Initialize the DAC interface */
int meas_dg535_init(int unit, int board, int dev) {

  char buf[512];
  int i;

  /* remember to put in single shot mode initially */
  /* both the system timer and each channel */
  if(dg535_fd[unit] == -1) {
    dg535_fd[unit] = meas_gpib_open(board, dev);
    meas_gpib_timeout(dg535_fd[unit], 1.0);
    sprintf(buf, "CL"); /* Clear device */
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
    for (i = 0; i < 8; i++) vals[unit][i] = 0.1;
  }
  return 0;
}

/* 
 * Program a given channel.
 *
 * unit    = which DG535 should be addressed
 * channel = which channel (DG_T0, DG_CHA, DG_CHB, DG_CHAB, DG_CHC, DG_CHD,
 *           DG_CHCD). Note that origin & delay are ignored for: DG_T0,
 *           DG_CHAB, DG_CHCD.
 * origin  = channel for relative timing (notation as above)
 * delay   = delay (s)
 * level   = output pulse voltage level (V)
 * polarity= output polarity (POL_NORM = normal, POL_INV = iverted).
 * imp     = impedance (IMP_50 = 50 ohm or IMP_HIGH = high Z)
 *
 */

int meas_dg535_set(int unit, int channel, int origin, double delay, double level, int polarity, int imp) {

  char buf[512];

  if(dg535_fd[unit] == -1 || channel < MEAS_DG535_T0 || channel > MEAS_DG535_CHCD) meas_err("meas_dg535_set: invalid channel.");
  
  /* Set channel delay */
  if(channel != MEAS_DG535_T0 && channel != MEAS_DG535_CHAB && channel != MEAS_DG535_CHCD) {
    sprintf(buf, "DT %d,%d,%le", channel, origin, delay);
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
  }

  /* Output level */
  /* Level (remove if necessary - just for safety) */
  /* TODO: add a function that can override this */
#if 0
  if(level > 5.1)
    meas_err("meas_dg535_set: Output level greater than 5 V requested! Won't do.");
#endif
  sprintf(buf, "OA %d,+%.1lf", channel, level);
  meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
  vals[unit][channel] = level;
  sprintf(buf, "OO %d,0", channel);
  meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
  sprintf(buf, "OM %d,3", channel);
  meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
  
  /* Polarity */
  if(channel != MEAS_DG535_T0 && channel != MEAS_DG535_CHAB && channel != MEAS_DG535_CHCD) {
    sprintf(buf, "OP %d,%d", channel, polarity);
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);  
  }

  /* Impedance */
  sprintf(buf, "TZ %d,%d", channel, imp);
  meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
  return 0;
}

/* Enable/disable given channel (set output voltage to zero) */
int meas_dg535_enable(int unit, int channel, int status) {

  char buf[512];

  if(status == 1) { /* Enable */
    sprintf(buf, "OA %d,%le", channel, vals[unit][channel]);
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
    sprintf(buf, "OM %d,3", channel);
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
  } else { /* Disable */
    sprintf(buf, "OA %d", channel);
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
    meas_gpib_read(dg535_fd[unit], buf);
    vals[unit][channel] = atof(buf);
    sprintf(buf, "OA %d,0.1", channel);
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
    sprintf(buf, "OM %d,3", channel);
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
  } 
  return 0;
}

/* Set trigger source.
 *
 * unit   = unit to be addressed
 * source = Trigger source: MEAS_DG535_TRIG_EXT = external trigger, MEAS_DG535_TRIG_INT 
 *          = internal trigger mode.
 * data   = Trigger level (V) or repetition rate (Hz) for internal trigger.
 * edge   = Trigger position: MEAS_DG535_TRIG_RISE = rising edge, MEAS_DG535_TRIG_FALL = falling edge.
 * imp    = Trigger input impedance (MEAS_DG535_IMP_50 = 50 Ohm, MEAS_DG535_IMP_HIGH = high Z).
 *
 */

int meas_dg535_trigger(int unit, int source, double data, int edge, int imp) {

  char buf[512];

  if(source == MEAS_DG535_TRIG_EXT) { /* external trigger */
    trigger_source[unit] = MEAS_DG535_TRIG_EXT;
    sprintf(buf, "TM 1");
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
    sprintf(buf, "TL %lf", data);
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
    sprintf(buf,"TZ 0,%d", imp);
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
    sprintf(buf,"TS %d", edge);
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
  } else { /* Internal trigger */
    trigger_source[unit] = MEAS_DG535_TRIG_INT;
    meas_dg535_run(unit, 0); /* stop for safety first */
    sprintf(buf, "TR 0,%lf", data);
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
    reprate[unit] = data;
    sprintf(buf, "TM 0");
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
    sleep(2);
    meas_dg535_run(unit, 1); /* stop for safety first */
  }
  return 0;
}

/*
 * Start/stop the delay generator.
 *
 * mode = 1: run and 0: stop.
 *
 */

int meas_dg535_run(int unit, int mode) {

  if(mode == 1) /* run */
    meas_dg535_mode(unit, MEAS_DG535_MODE_SS, 0, 0);
  else
    /* Stop by first recording all output voltages and then turning them
       to zero. (there is not "stop" on this system) */
    meas_dg535_mode(unit, MEAS_DG535_MODE_SOFT, 0, 0);
  return 0;
}

/*
 * Set DG535 mode.
 *
 * unit = unit to be addressed.
 * mode = MODE_SS is regular single shot mode (int/ext), MODE_BURST = burst mode, MODE_SOFT = single-shot (software trigger).
 * bc   = Number of pulses / trigger
 * bp   = period of pulses in the burst
 *
 * Note that this behaves differently from the corresponding BNC565 function.
 *
 */

int meas_dg535_mode(int unit, int mode, int bc, int bp) {

  char buf[512];

  if(mode == MEAS_DG535_MODE_SS) {
    /* Restore previous regular triggering mode (either int or ext) */
    sprintf(buf, "TM %1d", trigger_source[unit]);
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);      
  } else if (mode == MEAS_DG535_MODE_BURST) {
    sprintf(buf, "TM 3");
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);      
    sprintf(buf, "TR 1,%lf", reprate[unit]);
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
    sprintf(buf, "BC %d", bc);
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);      
    sprintf(buf, "BP %d", bp);
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);      
  } else { /* software trigger (MODE_SOFT) */
    sprintf(buf, "TM 2");
    meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);
  }
  return 0;
}

/* Cause software trigger */
int meas_dg535_do_trigger(int unit) {

  char buf[512];

  sprintf(buf, "SS");
  meas_gpib_write(dg535_fd[unit], buf, MEAS_DG535_TERM);  
  return 0;
}

