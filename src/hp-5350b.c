/*
 * Driver for HP 5350B microwave frequency counter.
 *
 */

#ifdef GPIB

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hp-5350b.h"
#include "gpib.h"
#include "misc.h"

/* Up to 5 devices supported */
static int hp5350_fd[5] = {-1, -1, -1, -1, -1};

/* Initialize instrument (unit = GPIB id) */

EXPORT int meas_hp5350_init(int unit, int board, int dev) {
  
  if(hp5350_fd[unit] == -1) {
    hp5350_fd[unit] = meas_gpib_open(board, dev);
    meas_gpib_timeout(hp5350_fd[unit], 1.0);
  }
  /* from the old code - needed? */
  meas_gpib_set(hp5350_fd[unit], REOS); /* XEOS and BIN disabled */  

  meas_gpib_write(hp5350_fd[unit], "reset", MEAS_HP5350_CRLF);
  meas_gpib_write(hp5350_fd[unit], "clr", MEAS_HP5350_CRLF);
  meas_gpib_write(hp5350_fd[unit], "init", MEAS_HP5350_CRLF);
  return 0;
}

EXPORT double meas_hp5350_read(int unit) {

  char buf[30];

  if(hp5350_fd[unit] == -1) meas_err("hp5350b: non-existent unit.");
  meas_gpib_write(hp5350_fd[unit], "sample,fast", MEAS_HP5350_CRLF);
  meas_gpib_read_n(hp5350_fd[unit], buf, 24);
  return atof(buf);
}

#endif /* GPIB */
