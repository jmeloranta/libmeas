/*
 * HP 53131A RF frequency counter (two channels).
 *
 */

#ifdef GPIB

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gpib.h"
#include "misc.h"

/* Up to 5 devices supported */
static int hp53131_fd[5] = {-1, -1, -1, -1, -1};

/*
 * Initialize instrument.
 *
 * unit  = Unit number.
 * board = GPIB board # (0, 1, ...).
 * dev   = GPIB ID.
 *
 */
 
EXPORT int meas_hp53131_init(int unit, int board, int dev) {
  
  if(hp53131_fd[unit] == -1) {
    hp53131_fd[unit] = meas_gpib_open(board, dev);
    meas_gpib_timeout(hp53131_fd[unit], 1.0);
  }
  return 0;
}

/*
 * Read current frequency.
 * 
 * unit    = Unit number to be addressed.
 * channel = Channel number (1 or 2).
 *
 */

EXPORT double meas_hp53131_read(int unit, int channel) {

  double freq;
  char buf[MEAS_GPIB_BUF_SIZE];

  if(hp53131_fd[unit] == -1)
    meas_err("meas_hp53131a_read: Non-existent unit.");
  switch(channel) {
  case 1:
    meas_gpib_write(hp53131_fd[unit], ":MEASURE:FREQ? 10MHz,10Hz,(@1)", 0);
    break;
  case 2:
    meas_gpib_write(hp53131_fd[unit], ":MEASURE:FREQ? 10MHz,10Hz,(@2)", 0);
    break;
  default:
    meas_err("meas_hp53131a_read: Invalid channel.");
    break;
  }
  for (freq = 0.0; freq < 0.1; ) {
    bzero(buf, sizeof(buf));
    meas_gpib_read(hp53131_fd[unit], buf);
    freq = (double) atof(buf);
  }
  return freq;
}

#endif /* GPIB */
