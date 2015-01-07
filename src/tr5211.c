/*
 * Driver for TakedaRiken TR 5211D Microwave Frequency Counter.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tr5211.h"
#include "gpib.h"
#include "misc.h"

/* Up to 5 devices supported */
static int tr5211_fd[5] = {-1, -1, -1, -1, -1};

static int tr5211_dev[5];
static int tr5211_board[5];

/*
 * Initialize instrument.
 *
 * unit  = Unit number.
 * board = GPIB board number (0, 1, ...)
 * dev   = GPIB ID.
 *
 */

EXPORT int meas_tr5211_init(int unit, int board, int dev) {
  
  if(tr5211_fd[unit] == -1) {
    tr5211_fd[unit] = meas_gpib_open(board, dev);
    tr5211_dev[unit] = dev;
    tr5211_board[unit] = board;
  }
  /* taken from the old code - needed? */
  meas_gpib_set(tr5211_fd[unit], REOS);
  return 0;
}

/*
 * Read microwave frequency.
 *
 * unit = Unit to be addressed.
 *
 */

EXPORT double meas_tr5211_read(int unit) {

  char buf[MEAS_GPIB_BUF_SIZE];
  double freq;
  int i;

  if(tr5211_fd[unit] == -1)
    meas_err("meas_tr5211_read: Non-existent unit.");
  meas_gpib_cmd(tr5211_board[unit], 0 + LAD);
  meas_gpib_cmd(tr5211_board[unit], tr5211_dev[unit] + TAD);
  for (freq = 0.0, i = 0; freq < 0.1; i++) {
    bzero(buf, MEAS_GPIB_BUF_SIZE);
    meas_gpib_read(tr5211_fd[unit], buf);
    freq = (double) atof(buf);
    if(i > MEAS_TR5211_RETRY) break;
  }
  meas_gpib_cmd(tr5211_board[unit], UNL);
  meas_gpib_cmd(tr5211_board[unit], UNT);
  if(i == MEAS_TR5211_RETRY)
    meas_err("meas_tr5211_read: Time out.");
  return (double) atof(buf) * 1.0E-9;  /* to Hz */
}
