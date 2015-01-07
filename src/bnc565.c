/* filename: bnc565.c
 *
 * Berkeley Nucleonics Delay generator model 565 (4 channels).
 *
 * The outputs are 50 Ohm.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bnc565.h"
#include "gpib.h"
#include "misc.h"

/* Up to 5 devices supported */
static int bnc565_fd[5] = {-1, -1, -1, -1, -1};

/* Initialize the DAC interface */
EXPORT int meas_bnc565_init(int unit, int board, int dev) {

  /* remember to put in single shot mode initially */
  /* both the system timer and each channel */
  if(bnc565_fd[unit] == -1) {
    bnc565_fd[unit] = meas_gpib_open(board, dev);
    meas_gpib_timeout(bnc565_fd[unit], 1.0);
  }
  meas_bnc565_run(unit, 0); /* stop */
  meas_gpib_write(bnc565_fd[unit], ":PULSE0:MODE SINGLE", MEAS_BNC565_TERM);   /* force single shot mode */
  return 0;
}

/* 
 * Program channel:
 *
 * unit    = which BNC565 should be addressed
 * channel = which channel (MEAS_BNC565_CHA, MEAS_BNC565_CHB, MEAS_BNC565_CHC,
 *           MEAS_BNC565_CHD)
 * origin  = channel for relative timing (MEAS_BNC565_T0, MEAS_BNC565_CHA,
 *           MEAS_BNC565_CHB, MEAS_BNC565_CHC, MEAS_BNC565_CHD)
 * delay   = delay (s)
 * width   = output pulse width (s)
 * level   = output pulse voltage level (V)
 * polarity= output logic (MEAS_BNC565_POL_NORM = normal, MEAS_BNC565_POL_INV
 *           = inverted).
 * 
 */

EXPORT int meas_bnc565_set(int unit, int channel, int origin, double delay, double width, double level, int polarity) {

  char buf[512], *src;

  if(bnc565_fd[unit] == -1 || channel < MEAS_BNC565_CHA || channel > MEAS_BNC565_CHD) meas_err("meas_bnc565: non-existent unit.");
  switch(origin) {
  case 0: src = "T0"; break;
  case 1: src = "CHA"; break;
  case 2: src = "CHB"; break;
  case 3: src = "CHC"; break;
  case 4: src = "CHD"; break;
  default: meas_err("meas_bnc565: unknown channel.");
  }
  
  /* Sync source */
  sprintf(buf, ":PULSE%d:SYNC %s", channel, src);
  meas_gpib_write(bnc565_fd[unit], buf, MEAS_BNC565_TERM);

  /* Pulse delay */
  sprintf(buf, ":PULSE%d:DELAY %le", channel, delay);
  meas_gpib_write(bnc565_fd[unit], buf, MEAS_BNC565_TERM);

  /* Pulse width */
  sprintf(buf, ":PULSE%d:WIDTH %le", channel, width);
  meas_gpib_write(bnc565_fd[unit], buf, MEAS_BNC565_TERM);
  
  /* Polarity */
  sprintf(buf, ":PULSE%d:POLARITY %s", channel, 
	  (polarity==MEAS_BNC565_POL_INV)?"INVERTED":"NORMAL");
  meas_gpib_write(bnc565_fd[unit], buf, MEAS_BNC565_TERM);

  /* Level (remove if necessary - just for safety) */
  /* TODO: add a global variable that can be used to override this */
  if(level > 5.1)
    meas_err("meas_bnc565: Output level greater than 5 V requested! Won't do.");
  sprintf(buf, ":PULSE%d:OUTPUT:MODE ADJUSTABLE", channel);
  meas_gpib_write(bnc565_fd[unit], buf, MEAS_BNC565_TERM);
  sprintf(buf, ":PULSE%d:OUTPUT:AMPLITUDE %lf", channel, level);
  meas_gpib_write(bnc565_fd[unit], buf, MEAS_BNC565_TERM);
  return 0;
}

/*
 * Set trigger source.
 *
 * unit   = unit to be addressed
 * source = Trigger source: MEAS_BNC565_TRIG_EXT = external trigger,
 *          MEAS_BNC565_TRIG_INT = internal trigger mode.
 * data   = Trigger level (V) or repetition rate (Hz) for internal trigger.
 * edge   = Trigger position: MEAS_BNC565_TRIG_RISE = rising edge,
 *          MEAS_BNC565_TRIG_FALL = falling edge.
 *
 */

EXPORT int meas_bnc565_trigger(int unit, int source, double data, int edge) {

  char buf[512];
  
  if(bnc565_fd[unit] == -1) meas_err("mea_bnc565: non-existent unit.");
  if(source == MEAS_BNC565_TRIG_INT) { /* internal trigger */
    meas_gpib_write(bnc565_fd[unit], ":PULSE0:EXT:MODE DISABLED", MEAS_BNC565_TERM); /* disable ext trigger */
    sprintf(buf, ":PULSE0:PERIOD %lf", 1.0 / data); /* %lf? */
    meas_gpib_write(bnc565_fd[unit], buf, MEAS_BNC565_TERM);
    meas_gpib_write(bnc565_fd[unit], ":PULSE0:MODE NORM", MEAS_BNC565_TERM);
  } else { /* external triggering */
    meas_gpib_write(bnc565_fd[unit], ":PULSE0:EXT:MODE TRIGGER", MEAS_BNC565_TERM); /* external triggering */
    sprintf(buf, ":PULSE0:EXT:LEVEL %lf", data); /* trigger level (V) */
    meas_gpib_write(bnc565_fd[unit], buf, MEAS_BNC565_TERM);
    sprintf(buf, ":PULSE0:EXT:EDGE %s", edge?"FALLING":"RISING");
    meas_gpib_write(bnc565_fd[unit], buf, MEAS_BNC565_TERM);
    meas_gpib_write(bnc565_fd[unit], ":PULSE0:EXT:POLARITY HIGH", MEAS_BNC565_TERM);
  }
  return 0;
}

/*
 * Start/Stop the delay generator.
 *
 * unit =  Unit to be addressed.
 * mode =  0 = stop or 1 = run.
 *
 */

EXPORT int meas_bnc565_run(int unit, int mode) {

  char buf[512];

  if(bnc565_fd[unit] == -1) meas_err("meas_bnc565: non-existent unit.");
  sprintf(buf, ":PULSE0:STATE %s", mode?"ON":"OFF");
  meas_gpib_write(bnc565_fd[unit], buf, MEAS_BNC565_TERM);
  return 0;
}

/*
 * Set number of triggers the channel is on & off (i.e., the duty cycle mode).
 *
 * 1 = A, 2 = B, 3 = C, 4 = D.
 * 
 * unit    = Unit to be addressed.
 * channel = Channel (MEAS_BNC565_CHA, MEAS_BNC565_CHB, MEAS_BNC565_CHC,
 *           MEAS_BNC565_CHD).
 * mode    = operation mode; MEAS_BNC565_SS = single shot,
 *           MEAS_BNC565_DC = duty cycle on/off.
 * on      = # of cycles on.
 * off     = # of cycles off.
 *
 * Note that this behaves differently from DG535 function.
 * TODO: Add software trigger
 *
 */

EXPORT int meas_bnc565_mode(int unit, int channel, int mode, int on, int off) {

  char buf[512];

  if(bnc565_fd[unit] == -1) meas_err("meas_bnc565: non-existent unit.");
  if(mode == MEAS_BNC565_MODE_SS) { /* regular single shot */
    sprintf(buf, ":PULSE%d:CMODE NORMAL", channel);
    meas_gpib_write(bnc565_fd[unit], buf, MEAS_BNC565_TERM);
  } else { /* duty cycle with on/off */
    sprintf(buf, ":PULSE%d:CMODE DCYCLE", channel);
    meas_gpib_write(bnc565_fd[unit], buf, MEAS_BNC565_TERM);
    sprintf(buf, ":PULSE%d:PCO %d", channel, on);
    meas_gpib_write(bnc565_fd[unit], buf, MEAS_BNC565_TERM);
    sprintf(buf, ":PULSE%d:OCO %d", channel, off);
    meas_gpib_write(bnc565_fd[unit], buf, MEAS_BNC565_TERM);
  }
  return 0;
}

/*
 * Enable or disable a given channel.
 *
 * unit    = Unit to be addressed.
 * channel = Channel (MEAS_BNC565_CHA, MEAS_BNC565_CHB, MEAS_BNC565_CHC,
 *           MEAS_BNC565_CHD).
 * status  = 1 = enable, 0 = disable.
 *
 */

EXPORT int meas_bnc565_enable(int unit, int channel, int status) {

  char buf[512];

  if(bnc565_fd[unit] == -1) meas_err("meas_bnc565: non-existent unit.");
  if(channel < MEAS_BNC565_CHA || channel > MEAS_BNC565_CHD)
    meas_err("meas_bnc565: unknown channel.");
  sprintf(buf, ":PULSE%d:STATE %s", channel, status?"ON":"OFF");
  meas_gpib_write(bnc565_fd[unit], buf, MEAS_BNC565_TERM);
  return 0;
}

/*
 * Software trigger (TODO).
 *
 * unit  = Unit to be triggered.
 *
 */

EXPORT int meas_bnc565_do_trigger(int unit) {

  /* TODO */
  return -1;
}
