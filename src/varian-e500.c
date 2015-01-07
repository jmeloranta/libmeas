/*
 * Driver for Varian E-500 Gaussmeter (GPIB).
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gpib.h"
#include "misc.h"

/* Up to 5 devices supported */
static int e500_fd[5] = {-1, -1, -1, -1, -1};

static int e500_board[5];
static int e500_dev[5];

/*
 * Initialize instrument.
 * 
 * unit  = Unit to be addressed.
 * board = GPIB board number (0, 1, ...).
 * dev   = GPIB ID.
 *
 */

EXPORT int meas_e500_init(int unit, int board, int dev) {
  
  if(e500_fd[unit] == -1) {
    e500_fd[unit] = meas_gpib_open(board, dev);
    e500_board[unit] = board;
    e500_dev[unit] = dev;
  }
  /* these were taken from the old code.. check if we need these */
  meas_gpib_set(e500_fd[unit], REOS); /* XEOS and BIN disabled */
  return 0;
}

/*
 * Read magnetic field value.
 *
 * unit = Unit to be addressed.
 *
 * Returns the value in Gauss.
 *
 */

EXPORT double meas_e500_read(int unit) {

  char buf[MEAS_GPIB_BUF_SIZE];
  double field = 0.0, last_field;

  if(e500_dev[unit] == -1)
    meas_err("meas_e500_read: Device has not been initialized.");
  meas_gpib_cmd(e500_board[unit], 0 + LAD); /* listener address = interface */
  meas_gpib_cmd(e500_board[unit], e500_dev[unit] + TAD); /* talker = gaussmeter */
  bzero(buf, MEAS_GPIB_BUF_SIZE);
  meas_gpib_read(e500_fd[unit], buf);
  meas_gpib_cmd(e500_board[unit], UNL); /* unlisten */
  meas_gpib_cmd(e500_board[unit], UNT); /* untalk */
  meas_gpib_clear(e500_board[unit]); /* Is this needed? (was just ibsic()) */
  return atof(buf+1);
}
