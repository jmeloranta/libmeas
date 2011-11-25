/*
 *
 * MKS PDR2000 capacitance manometer controller.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "pdr2000.h"
#include "serial.h"
#include "misc.h"

/* Up to 5 units supported */
static int pdr2000_fd[5] = {-1, -1, -1, -1, -1};

/* 
 * Initialize pressure reading.
 *
 * unit = Unit number.
 * dev  = RS232 device for the controller.
 *
 */

int meas_pdr2000_init(int unit, char *dev) {

  if(unit < 0 || unit > 4)
    meas_err("meas_pdr2000_init: Non-existing unit.");
  if(pdr2000_fd[unit] == -1)
    pdr2000_fd[unit] = meas_rs232_open(dev, 0);
  return 0;
}

/*
 * Read pressure.
 *
 * unit = Unit to be read.
 * chan = channel number (1 or 2).
 *
 * Returns pressure in torr.
 *
 */

double meas_pdr2000_read(int unit, int chan) {

  char buf[512];
  double p1, p2;

  if(pdr2000_fd[unit] == -1) meas_err("meas_pdr2000_read: Non-existing unit.");
  meas_rs232_writeb(pdr2000_fd[unit], 'p');
  meas_rs232_readnl(pdr2000_fd[unit], buf);
  if(sscanf(buf, " Off %le", &p2) == 1) {
    if(chan == 1) meas_err("meas_pdr2000_read: Channel 1 not connected.\n");
    return p2;
  }
  if(sscanf(buf, " %le Off", &p1) == 1) {
    if(chan == 2) meas_err("meas_pdr2000_read: Channel 2 not connected.\n");
    return p1;
  }
  meas_err("meas_pdr2000_read: Both channels off.\n");
}

/*
 * Close the instrument.
 *
 * unit = Unit number to be closed.
 *
 */

int meas_pdr2000_close(int unit) {

  if(pdr2000_fd[unit] == -1) return;
  meas_rs232_close(pdr2000_fd[unit]);
  pdr2000_fd[unit] = -1;
  return 0;
}
