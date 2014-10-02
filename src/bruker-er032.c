/*
 * Driver for Bruker ER-032 magnetic field controller.
 *
 * We set the field explicitly (ie. SW = 0 and CF then gives the field).
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bruker-er032.h"
#include "gpib.h"
#include "misc.h"

/* Up to 5 devices supported */
static int er032_fd[5] = {-1, -1, -1, -1, -1};

/*
 * Initialize instrument.
 *
 * unit = GPIB id.
 *
 */

int meas_er032_init(int unit, int board, int dev) {
  
  if(er032_fd[unit] == -1) {
    er032_fd[unit] = meas_gpib_open(board, dev);
    meas_gpib_timeout(er032_fd[unit], 1.0);
  }
  /* TODO: from the old code - needed? */
  meas_gpib_set(er032_fd[unit], REOS); /* XEOS and BIN disabled */  

  meas_gpib_write(er032_fd[unit], "CO", MEAS_ER032_CRLF);
  meas_gpib_write(er032_fd[unit], "SW0.0", MEAS_ER032_CRLF);
  return 0;
}

/*
 * Read current magnetic field value.
 *
 * unit  = Unit to be addressed.
 * 
 * Returns the field in Gauss.
 *
 */

double meas_er032_read(int unit) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(er032_fd[unit] == -1)
    meas_err("meas_er032: non-existent field controller.");
  meas_gpib_write(er032_fd[unit], "FC", MEAS_ER032_CRLF);
  meas_gpib_read(er032_fd[unit], buf);
  return atof(buf);
}

/*
 * Set the current magnetic field value.
 *
 * unit  = Unit to be addressed.
 * field = Field value in Gauss.
 * 
 */

int meas_er032_write(int unit, double field) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(er032_fd[unit] == -1)
    meas_err("meas_er032: non-existent field controller.");
  sprintf(buf, "CF%.4lf", field);
  meas_gpib_write(er032_fd[unit], buf, MEAS_ER032_CRLF);
  return 0;
}
