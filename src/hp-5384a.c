/*
 * Driver for HP 5384A and 5385A radio frequency counter.
 * It has two channels for RF input.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hp-5384a.h"
#include "gpib.h"
#include "misc.h"

/* Up to 5 devices supported */
static int hp5384_fd[5] = {-1, -1, -1, -1, -1};

/* Initialize instrument.
 *
 * unit  = Unit to be initialized.
 * board = GPIB board # (0, 1, ...).
 * dev   = GPIB ID.
 *
 */

EXPORT int meas_hp5384_init(int unit, int board, int dev) {
  
  if(hp5384_fd[unit] == -1) {
    hp5384_fd[unit] = meas_gpib_open(board, dev);
    /* FIXME: we should not rely on time outs! */
    meas_gpib_timeout(hp5384_fd[unit], 1.0);
    meas_gpib_write(hp5384_fd[unit], "DN", MEAS_HP5384_CRLF); /* normal # of digits */
  }
  return 0;
}

/*
 * Read frequency.
 *
 * unit    = Unit number to be read.
 * channel = Channel number (MEAS_HP5384_CHA or MEAS_HP5384_CHB)
 *
 */

EXPORT double meas_hp5384_read(int unit, int channel) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(hp5384_fd[unit] == -1)
    meas_err("meas_hp5384_read: Non-existent unit.");
  switch(channel) {
  case MEAS_HP5384_CHA:
    meas_gpib_write(hp5384_fd[unit], "FU1", MEAS_HP5384_CRLF);
    break;
  case MEAS_HP5384_CHB:
    meas_gpib_write(hp5384_fd[unit], "FU3", MEAS_HP5384_CRLF);
    break;
  default:
    meas_err("meas_hp5384_read: Invalid channel.");
  }
  meas_gpib_read(hp5384_fd[unit], buf);
  buf[0] = ' ';   /* the first character is F - ignore */
  return atof(buf);  
}

/*
 * Control the builtin attenuator for channel A.
 *
 * unit = Unit number to be addressed.
 * attn = Attenuation (MEAS_HP5384_ATTN_0 or MEAS_HP5384_ATTN_20)
 *
 */

EXPORT int meas_hp5384_attn(int unit, int attn) {

  if(hp5384_fd[unit] == -1)
    meas_err("meas_hp5384_attn: Non-existent unit.");
  switch(attn) {
  case MEAS_HP5384_ATTN_0: 
    meas_gpib_write(hp5384_fd[unit], "AT0", MEAS_HP5384_CRLF);
    break;
  case MEAS_HP5384_ATTN_20:
    meas_gpib_write(hp5384_fd[unit], "AT1", MEAS_HP5384_CRLF);
    break;
  default:
    meas_err("meas_hp5384a_attn: Invalid attenuator setting.");
  }
  return 0;
}

/* 
 * Control the builtin 100 kHz long pass filter for channel A.
 *
 * unit = Unit to be addressed.
 * filt = Filter setting (MEAS_HP5384_FILT_ON or MEAS_HP5384_ATTN_OFF)
 *
 */

EXPORT int meas_hp5384_filt(int unit, int filt) {

  if(hp5384_fd[unit] == -1) meas_err("meas_hp5384_filt: Non-existent unit.");
  switch(filt) {
  case MEAS_HP5384_FILT_ON: 
    meas_gpib_write(hp5384_fd[unit], "FI1", MEAS_HP5384_CRLF);
    break;
  case MEAS_HP5384_FILT_OFF:
    meas_gpib_write(hp5384_fd[unit], "FI0", MEAS_HP5384_CRLF);
    break;
  default:
    meas_err("meas_hp5384_filt: Invalid 100 kHz filter setting.");
  }
  return 0;
}

/*
 *
 * Control the manual level control.
 *
 * unit = Unit to be addressed.
 * leve = Level setting (MEAS_HP5384_LEVEL_MAN or MEAS_HP5384_LEVEL_AUTO)
 *
 */

EXPORT int meas_hp5384_level(int unit, int level) {

  if(hp5384_fd[unit] == -1) 
    meas_err("meas_hp5384_level: Non-existent unit.");
  switch(level) {
  case MEAS_HP5384_LEVEL_MAN: 
    meas_gpib_write(hp5384_fd[unit], "ML1", MEAS_HP5384_CRLF);
    break;
  case MEAS_HP5384_LEVEL_AUTO:
    meas_gpib_write(hp5384_fd[unit], "ML0", MEAS_HP5384_CRLF);
    break;
  default:
    meas_err("meas_hp5384_level: Invalid level setting.");
  }
  return 0;
}

/*
 * Control the gate time.
 *
 * unit = Unit to be addressed.
 * gate = Gate time (MEAS_HP5384_GATE_100MS, MEAS_HP5384_GATE_1S, or
 *        MEAS_HP5384_GATE_10S).
 *
 */

EXPORT int meas_hp5384_gate(int unit, int gate) {

  if(hp5384_fd[unit] == -1) 
    meas_err("meas_hp5384_gate: Non-existent unit.");
  switch(gate) {
  case MEAS_HP5384_GATE_100MS:
    meas_gpib_write(hp5384_fd[unit], "GA1", MEAS_HP5384_CRLF);
    break;
  case MEAS_HP5384_GATE_1S:
    meas_gpib_write(hp5384_fd[unit], "GA2", MEAS_HP5384_CRLF);
    break;
  case MEAS_HP5384_GATE_10S:
    meas_gpib_write(hp5384_fd[unit], "GA3", MEAS_HP5384_CRLF);
    break;
  default:
    meas_err("meas_hp5384_gate: Invalid gate time.");
  }
  return 0;
}

/*
 * Reset instrument.
 *
 * unit = Unit to be reset.
 *
 */

EXPORT int meas_hp5384_reset(int unit) {

  if(hp5384_fd[unit] == -1)
    meas_err("meas_hp5384_reset: Non-existent unit.");
  meas_gpib_write(hp5384_fd[unit], "RE", MEAS_HP5384_CRLF);
  return 0;
}

/*
 * Retrieve the instrument error code.
 *
 * unit = Unit for the operation.
 *
 */

EXPORT int meas_hp5384_error(int unit) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(hp5384_fd[unit] == -1) 
    meas_err("meas_hp5384_error: Non-existent unit.");
  meas_gpib_write(hp5384_fd[unit], "SE", MEAS_HP5384_CRLF);
  meas_gpib_read(hp5384_fd[unit], buf);
  return atoi(buf);
}
