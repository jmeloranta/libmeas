/*
 * Driver for Bruker ER023M signal channel.
 * 
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bruker-er023.h"
#include "gpib.h"
#include "misc.h"

/* Up to 5 devices supported */
static int er023_fd[5] = {-1, -1, -1, -1, -1};

static int nb[5] = {0, 0, 0, 0, 0}; /* Number of bytes in data */
static double current_ma[5] = {-1, -1, -1, -1, -1}; /* Current max modulation */

/*
 * Initialize instrument.
 *
 * unit  = device handle for accessing the device.
 * board = GPIB board # (first is 0, 2nd 1, etc.).
 * dev   = GPIB ID.
 *
 */

int meas_er023_init(int unit, int board, int dev) {
  
  char buf[MEAS_GPIB_BUF_SIZE];

  if(er023_fd[unit] == -1) {
    er023_fd[unit] = meas_gpib_open(board, dev);
    meas_gpib_timeout(er023_fd[unit], 1.0);
  }

  /* these were taken from the old code.. check if we need these */
  meas_gpib_set(er023_fd[unit], BIN); /* XEOS and REOS disabled */

  /* setup reasonable defaults */
  meas_gpib_write(er023_fd[unit], "HA1", MEAS_ER023_CRLF);
  meas_gpib_write(er023_fd[unit], "RE1", MEAS_ER023_CRLF);
  meas_gpib_write(er023_fd[unit], "SR0", MEAS_ER023_CRLF);
  meas_gpib_write(er023_fd[unit], "NB", MEAS_ER023_CRLF);
  meas_gpib_read(er023_fd[unit], buf);
  sscanf(buf, "NB%d", &nb[unit]);
  return 0;
}

/*
 * Calibrate signal channel.
 *
 * unit  = device handle for accessing the device.
 * freq  = modulation frequency (100 KHz, 50 KHz, 25 KHz, 12.5 KHz, 6.25 KHz, 3.13 KHz, 1.56 KHz).
 * ma    = maximum modulation amplitude in the cavity.
 * phmax = phase setting for signal maximum.
 *
 */

int meas_er023_calibrate(int unit, double freq, double ma, double phmax) {

  int i;
  char buf[MEAS_GPIB_BUF_SIZE];

  i = -1;
  if(freq == 100.0) i = 0;
  if(freq == 50.0) i = 1;
  if(freq == 25.0) i = 2;
  if(freq == 12.5) i = 3;
  if(freq == 6.25) i = 4;
  if(freq == 3.13) i = 5;
  if(freq == 1.56) i = 6;
  if(i == -1) meas_err("meas_er023: Illegal modulation frequency.");

  sprintf(buf, "MF%d", i);
  meas_gpib_write(er023_fd[unit], buf, MEAS_ER023_CRLF);
  sprintf(buf, "MC%2.2f.", ma);
  meas_gpib_write(er023_fd[unit], buf, MEAS_ER023_CRLF);
  current_ma[unit] = ma;
  sprintf(buf, "PC%f.", phmax);
  meas_gpib_write(er023_fd[unit], buf, MEAS_ER023_CRLF);
  return 0;
}

/*
 * Read signal channel.
 *
 * unit = device handle for accessing the device.
 *
 * Return value: signal amplitude.
 *
 */

unsigned int meas_er023_read(int unit) {

  unsigned char buf[3];

  if(er023_fd[unit] == -1) meas_err("meas_er023: non-existing signal channel.");
  if(current_ma[unit] == -1) meas_err("meas_er023: uncalibrated signal channel.");
  if(nb[unit] == 0) meas_err("meas_er023: data word size zero.");
  meas_gpib_write(er023_fd[unit], "SM", MEAS_ER023_CRLF);
  switch (nb[unit]) {
  case 1:
    meas_gpib_read_n(er023_fd[unit], buf, 1);
    return (unsigned int) buf[0];
  case 2:
    meas_gpib_read_n(er023_fd[unit], buf, 2);
    return (unsigned int) (buf[0] + buf[1] * 256);
  case 3:
    meas_gpib_read_n(er023_fd[unit], buf, 3);
    return (unsigned int) (buf[0] + buf[1] * 256 + buf[2] * 65536);     
  case 4: /* ??? according to manual, this should not be possible */
    meas_gpib_read_n(er023_fd[unit], buf, 4);
    return (unsigned int) (buf[0] + buf[1] * 256 + buf[2] * 65536 + buf[3] * 16777216);     
  default:
    meas_err("meas_er023: Illegal data word size.");
  }
}

/*
 * Set modulation amplitude.
 *
 * unit = device handle for accessing the device.
 * amp  = modulation amplitude (Gauss).
 *
 */

int meas_er023_modulation_amp(int unit, double amp) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(er023_fd[unit] == -1) meas_err("meas_er023: non-existing signal channel.");
  if(current_ma[unit] == -1) meas_err("meas_er023: uncalibrated signal channel.");  
  if(amp > current_ma[unit]) amp = current_ma[unit];
  amp /= current_ma[unit];
  sprintf(buf, "MA%d", (int) (-20.0 * log10(amp)));
  meas_gpib_write(er023_fd[unit], buf, MEAS_ER023_CRLF);
  return 0;
}

/*
 * Set harmonic.
 *
 * unit     = device handle for accessing the device.
 * harmonic = 1 (first; 1st derivative) or 2 (second; 2nd derivative).
 *
 */

int meas_er023_harmonic(int unit, int harm) {

  switch (harm) {
  case 1:
    meas_gpib_write(er023_fd[unit], "HA1", MEAS_ER023_CRLF);
    break;
  case 2:
    meas_gpib_write(er023_fd[unit], "HA2", MEAS_ER023_CRLF);
    break;
  default:
    meas_err("meas_er023: Illegal harmonic setting.");
  }
  return 0;
}

/*
 * Set receiver gain.
 *
 * unit = device handle for accessing the device.
 * gain = receiver gain setting (between 2.0E2 and 1.0E7).
 *
 */

int meas_er023_gain(int unit, double gain) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(er023_fd[unit] == -1) meas_err("meas_er023: non-existing signal channel.");
  /* the signal channel manual is very erratic... */
  /* the lowest rg is 2.0E1 (not 2.0E2) and the step is 1db not 2db ! */
  gain = 10.0 * log10(gain / 2.0E1) + 1;
  if(gain < 0.0) gain = 0.0;
  if(gain > 57.0) gain = 57.0;
  sprintf(buf, "RG%d", (int) gain);
  meas_gpib_write(er023_fd[unit], buf, MEAS_ER023_CRLF);
  return 0;
}

/*
 * Set time constant.
 *
 * unit = device handle for accessing the device.
 * tc   = time constant (between 10 microsec. and 5 sec.)
 *
 */

int meas_er023_timeconstant(int unit, double tc) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(er023_fd[unit] == -1) meas_err("meas_er023: non-existing signal channel.");
  if(tc > 0.0) tc = log(tc * 1.0E5) / M_LN2;
  else tc = 0.0;
  if(tc < 0.0) tc = 0.0;
  if(tc > 19.0) tc = 19.0;
  sprintf(buf, "TC%d", (int) tc);
  meas_gpib_write(er023_fd[unit], buf, MEAS_ER023_CRLF);
  meas_gpib_write(er023_fd[unit], "NB", MEAS_ER023_CRLF);
  meas_gpib_read(er023_fd[unit], buf);
  sscanf(buf,"NB%d", &nb[unit]);
  return 0;
}

/*
 * Set A/D conversion time.
 *
 * unit = device handle for accessing the device.
 * ct   = A/D channel conversion time (between 320 microsec. and 3.2 sec.).
 *
 */ 

int meas_er023_conversiontime(int unit, double ct) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(er023_fd[unit] == -1) meas_err("meas_er023: non-existing signal channel.");
  ct /= 320.0E-6;
  if(ct < 1.0) ct = 1.0;
  if(ct > 9999.0) ct = 9999.0;
  sprintf(buf, "CT%d", (int) ct);
  meas_gpib_write(er023_fd[unit], buf, MEAS_ER023_CRLF);
  meas_gpib_write(er023_fd[unit], "NB", MEAS_ER023_CRLF);
  meas_gpib_read(er023_fd[unit], buf);
  sscanf(buf, "NB%d", &nb[unit]);
  return 0;
}

/* 
 * Select resonator input.
 *
 * unit = device handle for accessing the device.
 * resonator = resonator input (1 or 2).
 *
 */

int meas_er023_resonator(int unit, int resonator) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(er023_fd[unit] == -1) meas_err("meas_er023: non-existing signal channel.");
  switch(resonator) {
  case 1:
    sprintf(buf, "RE1");
    break;
  case 2:
    sprintf(buf, "RE2");
    break;
  default:
    meas_err("er023: Unknown resonator setting.");
  }
  meas_gpib_write(er023_fd[unit], buf, MEAS_ER023_CRLF);
  return 0;
}

/*
 * Set signal phase offset.
 *
 * unit  = device handle for accessing the device.
 * phase = phase offset between 0 and 360.
 *
 */

int meas_er023_phase(int unit, double phase) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(er023_fd[unit] == -1)
    meas_err("meas_er023: non-existing signal channel.");
  if(phase < 0.0) phase = 360 + phase;
  if(phase >= 360.0) phase = phase - 360.0;
  sprintf(buf, "PH%d", (int) phase);
  meas_gpib_write(er023_fd[unit], buf, MEAS_ER023_CRLF);
  return 0;
}

/*
 * Read receiver level.
 *
 * unit = device handle for accessing the device.
 * 
 * Return value: receiver level between 0 (lowest) and 128 (highest).
 *
 */

int meas_er023_reclevel(int unit) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(er023_fd[unit] == -1) meas_err("meas_er023: non-existing signal channel.");
  meas_gpib_write(er023_fd[unit], "RL", MEAS_ER023_CRLF);
  meas_gpib_read(er023_fd[unit], buf);
  return atoi(buf);
}

/*
 * Reset ER023.
 *
 * unit = device handle for accessing the device.
 *
 */

int meas_er023_reset(int unit) {

  if(er023_fd[unit] == -1) meas_err("meas_er023: non-existing signal channel.");
  meas_gpib_write(er023_fd[unit], "INIT", MEAS_ER023_CRLF);
  return 0;
}

/*
 * Read ER023 status word.
 *
 * unit = device handle for accessing the device.
 * 
 * Return value: status word (0 = OK, 1 = overload).
 *
 */

int meas_er023_status(int unit) {
  
  char buf[MEAS_GPIB_BUF_SIZE];
  int st;

  if(er023_fd[unit] == -1) meas_err("meas_er023: non-existing signal channel.");
  meas_gpib_write(er023_fd[unit], "ST", MEAS_ER023_CRLF);
  meas_gpib_read(er023_fd[unit], buf);
  sscanf(buf, "ST%d", &st);
  return st;
}

/*
 * Is the signal channel calibrated?
 *
 * unit = device handle for accessing the device.
 *
 * Return value: 0 = not calibrated, 1 = calibrated.
 *
 */

int meas_er023_calibrated(int unit) {

  if(current_ma[unit] == -1) return 0;
  else return 1;
}
