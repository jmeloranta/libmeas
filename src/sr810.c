/*
 * Stanford Research Instruments SR810 (Lock-in-Amplifier).
 *
 * !!!!!NOT COMPLETE!!!!
 *
 */

#ifdef GPIB

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "misc.h"
#include "sr810.h"
#include "serial.h"
#include "gpib.h"

/* Up to 5 units supported */
static int sr810_fd[5] = {-1, -1, -1, -1, -1};
static int srmode[5] = {0, 0, 0, 0, 0};

/* 
 * Initialize the instrument.
 *
 * unit   = unit number (0 - 4)
 * board  = GPIB board number
 * dev    = GPIB id
 * serial = NULL for GPIB operation or path to RS port for serial operation.
 *          (e.g. /dev/ttyS0)
 *
 * Returns zero on success.
 *
 */

EXPORT int meas_sr810_open(int unit, int board, int dev, char *serial) {

  if(unit < 0 || unit > 4)
    meas_err("meas_sr810_init: Illegal units number.");
  if(serial) {
    if(sr810_fd[unit] == -1)
      sr810_fd[unit] = meas_rs232_open(serial, 1);
    srmode[unit] = 1;
  } else {
    if(sr810_fd[unit] == -1) {
      sr810_fd[unit] = meas_gpib_open(board, dev); 
      meas_gpib_timeout(sr810_fd[unit], 1.0);
    }
    srmode[unit] = 0;
  }
  return 0;
}

/*
 * Set the reference phase shift.
 *
 * unit  = SR810 unit number.
 * phase = reference phase shift.
 *
 * Returns zero on success.
 *
 */

EXPORT int meas_sr810_set_refphase(int unit, double phase) {

  char buf[512];

  if(sr810_fd[unit] == -1)
    meas_err("meas_sr810_set_refphase: Non-existent unit.");
  sprintf(buf, "PHAS %le", phase);
  if(srmode[unit]) { /* RS232 */
    strcat(buf, "\r");
    meas_rs232_write(sr810_fd[unit], buf, strlen(buf));
  } else /* GPIB */
    meas_gpib_write(sr810_fd[unit], buf, MEAS_SR810_TERM);

  return 0;
}

/*
 * Get the reference phase shift.
 *
 * unit  = SR810 unit number.
 *
 * Returns the phase.
 *
 */

EXPORT double meas_sr810_get_refphase(int unit) {

  char buf[512];

  if(sr810_fd[unit] == -1)
    meas_err("meas_sr810_get_refphase: Non-existent unit.");
  sprintf(buf, "PHAS?");
  if(srmode[unit]) { /* RS232 */
    strcat(buf, "\r");
    meas_rs232_write(sr810_fd[unit], buf, strlen(buf));
    meas_rs232_readnl(sr810_fd[unit], buf);
  } else { /* GPIB */
    meas_gpib_write(sr810_fd[unit], buf, MEAS_SR810_TERM);
    meas_gpib_read(sr810_fd[unit], buf);
  }

  return atof(buf);
}

/*
 * Set the reference source.
 *
 * unit   = SR810 unit number.
 * source = reference source (MEAS_SR810_REF_SOURCE_INT = internal;
 *          MEAS_SR810_REF_SOURCE_EXT = external).
 *
 * Returns zero on success.
 *
 */

EXPORT int meas_sr810_set_refsource(int unit, int source) {

  char buf[512];

  if(sr810_fd[unit] == -1)
    meas_err("meas_sr810_set_refsource: Non-existent unit.");
  if(source < MEAS_SR810_REF_SOURCE_EXT || MEAS_SR810_REF_SOURCE_INT > 1)
    meas_err("meas_sr810_set_refsource: Illegal reference source.");
  sprintf(buf, "FMOD %d", (source==MEAS_SR810_REF_SOURCE_INT)?1:0);
  if(srmode[unit]) { /* RS232 */
    strcat(buf, "\r");
    meas_rs232_write(sr810_fd[unit], buf, strlen(buf));
  } else /* GPIB */
    meas_gpib_write(sr810_fd[unit], buf, MEAS_SR810_TERM);

  return 0;
}

/*
 * Get the reference source.
 *
 * unit  = SR810 unit number.
 *
 * Returns source. Note that zero is a valid return value
 * (MEAS_SR810_REF_SOURCE_EXT = external, MEAS_SR810_REF_SOURCE_INT = internal).
 *
 */

EXPORT double meas_sr810_get_refsource(int unit) {

  char buf[512];

  if(sr810_fd[unit] == -1)
    meas_err("meas_sr810_get_refsource: Non-existent unit.");
  sprintf(buf, "FMOD?");
  if(srmode[unit]) { /* RS232 */
    strcat(buf, "\r");
    meas_rs232_write(sr810_fd[unit], buf, strlen(buf));
    meas_rs232_readnl(sr810_fd[unit], buf);
  } else { /* GPIB */
    meas_gpib_write(sr810_fd[unit], buf, MEAS_SR810_TERM);
    meas_gpib_read(sr810_fd[unit], buf);
  }

  return (atoi(buf)==1)?MEAS_SR810_REF_SOURCE_INT:MEAS_SR810_REF_SOURCE_EXT;
}

/*
 * Set the reference frequency.
 *
 * unit   = SR810 unit number.
 * freq   = reference frequency (0.001 <= freq <= 102000).
 *
 * Returns zero on success.
 *
 */

EXPORT int meas_sr810_set_reffreq(int unit, double freq) {

  char buf[512];

  if(sr810_fd[unit] == -1)
    meas_err("meas_sr810_set_reffreq: Non-existent unit.");
  if(freq < 0.001 || freq > 102000.0)
    meas_err("meas_sr810_set_reffreq: Illegal reference frequency.");
  if(meas_sr810_get_refsource(unit) == MEAS_SR810_REF_SOURCE_EXT)
    meas_err("meas_sr810_set_reffreq: Frequency set when in external reference mode.");
  sprintf(buf, "FREQ %le", freq);
  if(srmode[unit]) { /* RS232 */
    strcat(buf, "\r");
    meas_rs232_write(sr810_fd[unit], buf, strlen(buf));
  } else /* GPIB */
    meas_gpib_write(sr810_fd[unit], buf, MEAS_SR810_TERM);

  return 0;
}

/*
 * Get the reference frequency.
 *
 * unit  = SR810 unit number.
 *
 * Returns the reference frequency.
 *
 */

EXPORT double meas_sr810_get_reffreq(int unit) {

  char buf[512];

  if(sr810_fd[unit] == -1)
    meas_err("meas_sr810_get_reffreq: Non-existent unit.");
  sprintf(buf, "FREQ?");
  if(srmode[unit]) { /* RS232 */
    strcat(buf, "\r");
    meas_rs232_write(sr810_fd[unit], buf, strlen(buf));
    meas_rs232_readnl(sr810_fd[unit], buf);
  } else { /* GPIB */
    meas_gpib_write(sr810_fd[unit], buf, MEAS_SR810_TERM);
    meas_gpib_read(sr810_fd[unit], buf);
  }

  return atof(buf);
}

/*
 * Set the reference trigger mode.
 *
 * unit   = SR810 unit number.
 * trig   = trigger mode (MEAS_SR810_REF_MODE_SZC = sine zero crossing,
 *          MEAS_SR810_REF_MODE_RE = rising edge (TTL),
 *          MEAS_SR810_REF_MODE_FE = falling edge (TTL)).
 *
 * Returns zero on success.
 *
 */

EXPORT int meas_sr810_set_reftrig(int unit, int trig) {

  char buf[512];

  if(sr810_fd[unit] == -1)
    meas_err("meas_sr810_set_reftrig: Non-existent unit.");
  if(trig < MEAS_SR810_REF_MODE_SCZ || trig > MEAS_SR810_REF_MODE_FE) 
    meas_err("meas_sr810_set_reftrig: Illegal reference trigger setting.");
  if(meas_sr810_get_refsource(unit) == MEAS_SR810_REF_SOURCE_EXT)
    meas_err("meas_sr810_set_reftrig: Trigger set when in external reference mode.");
  switch(trig) {
  case MEAS_SR810_REF_MODE_SZC:
    sprintf(buf, "RSLP 0");
    break;
  case MEAS_SR810_REF_MODE_RE:
    sprintf(buf, "RSLP 1");
    break;
  case MEAS_SR810_REF_MODE_FE:
    sprintf(buf, "RSLP 2");
    break;
  }
  if(srmode[unit]) { /* RS232 */
    strcat(buf, "\r");
    meas_rs232_write(sr810_fd[unit], buf, strlen(buf));
  } else /* GPIB */
    meas_gpib_write(sr810_fd[unit], buf, MEAS_SR810_TERM);

  return 0;
}

/*
 * Get the reference trigger mode.
 *
 * unit  = SR810 unit number.
 *
 * Returns the reference trigger mode:
 * MEAS_SR810_REF_MODE_SZC = sine zero crossing,
 * MEAS_SR810_REF_MODE_RE = rising edge (TTL), 
 * MEAS_SR810_REF_MODE_FE = falling edge (TTL).
 *
 */

EXPORT int meas_sr810_get_reftrig(int unit) {

  char buf[512];

  if(sr810_fd[unit] == -1)
    meas_err("meas_sr810_get_reftrig: Non-existent unit.");
  sprintf(buf, "RSLP?");
  if(srmode[unit]) { /* RS232 */
    strcat(buf, "\r");
    meas_rs232_write(sr810_fd[unit], buf, strlen(buf));
    meas_rs232_readnl(sr810_fd[unit], buf);
  } else { /* GPIB */
    meas_gpib_write(sr810_fd[unit], buf, MEAS_SR810_TERM);
    meas_gpib_read(sr810_fd[unit], buf);
  }

  switch (atoi(buf)) {
  case 0:
    return MEAS_SR810_REF_MODE_SZC;
  case 1:
    return MEAS_SR810_REF_MODE_RE;
  case 2:
    return MEAS_SR810_REF_MODE_FE;
  default:
    meas_err("meas_sr810_get_reftrig: Illegal response.\n");
  }
}

/*
 * Set the harmonic to be recorded.
 *
 * unit   = SR810 unit number.
 * harm   = harmonic to be recorded (1 <= harm <= 19999). 
 *
 * Returns zero on success.
 *
 */

EXPORT int meas_sr810_set_harmonic(int unit, int harm) {

  char buf[512];

  if(sr810_fd[unit] == -1) 
    meas_err("meas_sr810_set_harmonic: Non-existent unit.");
  if(harm < 1 || harm > 19999) 
    meas_err("meas_sr810_set_harmonic: Illegal harmonic setting.");
  sprintf(buf, "HARM %d", harm);
  if(srmode[unit]) { /* RS232 */
    strcat(buf, "\r");
    meas_rs232_write(sr810_fd[unit], buf, strlen(buf));
  } else /* GPIB */
    meas_gpib_write(sr810_fd[unit], buf, MEAS_SR810_TERM);

  return 0;
}

/*
 * Get the harmonic number used.
 *
 * unit  = SR810 unit number.
 *
 * Returns the harmonic setting (1 <= harm <= 19999).
 *
 */

EXPORT int meas_sr810_get_harmonic(int unit) {

  char buf[512];

  if(sr810_fd[unit] == -1)
    meas_err("meas_sr810_get_harmonic: Non-existent unit.");
  sprintf(buf, "HARM?");
  if(srmode[unit]) { /* RS232 */
    strcat(buf, "\r");
    meas_rs232_write(sr810_fd[unit], buf, strlen(buf));
    meas_rs232_readnl(sr810_fd[unit], buf);
  } else { /* GPIB */
    meas_gpib_write(sr810_fd[unit], buf, MEAS_SR810_TERM);
    meas_gpib_read(sr810_fd[unit], buf);
  }

  return atoi(buf);
}

/*
 * Set the sine otput level.
 *
 * unit   = SR810 unit number.
 * level  = harmonic to be recorded (0.004 <= level <= 5.000).
 *
 * Returns zero on success.
 *
 */

EXPORT int meas_sr810_set_sinelevel(int unit, double level) {

  char buf[512];

  if(sr810_fd[unit] == -1)
    meas_err("meas_sr810_set_sinelevel: Non-existent unit.");
  if(level < 0.004 || level > 5.000) 
    meas_err("meas_sr810_set_sinelevel: Illegal sine output voltage setting.");
  sprintf(buf, "SLVL %le", level);
  if(srmode[unit]) { /* RS232 */
    strcat(buf, "\r");
    meas_rs232_write(sr810_fd[unit], buf, strlen(buf));
  } else /* GPIB */
    meas_gpib_write(sr810_fd[unit], buf, MEAS_SR810_TERM);

  return 0;
}

/*
 * Get the sine output level.
 *
 * unit  = SR810 unit number.
 *
 * Returns the sine output level.
 *
 */

EXPORT double meas_sr810_get_sinelevel(int unit) {

  char buf[512];

  if(sr810_fd[unit] == -1) 
    meas_err("meas_sr810_get_sinelevel: Non-existent unit.");
  sprintf(buf, "SLVL?");
  if(srmode[unit]) { /* RS232 */
    strcat(buf, "\r");
    meas_rs232_write(sr810_fd[unit], buf, strlen(buf));
    meas_rs232_readnl(sr810_fd[unit], buf);
  } else { /* GPIB */
    meas_gpib_write(sr810_fd[unit], buf, MEAS_SR810_TERM);
    meas_gpib_read(sr810_fd[unit], buf);
  }

  return atof(buf);
}

#endif /* GPIB */
