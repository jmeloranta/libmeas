/*
 *
 * MKS PDR900 pressure controller.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "pdr900.h"
#include "serial.h"
#include "misc.h"

/* Up to 5 units supported */
static int pdr900_fd[5] = {-1, -1, -1, -1, -1};

/* 
 * Initialize pressure reading.
 *
 * unit = Unit number.
 * dev  = RS232 device for the controller.
 *
 */

int meas_pdr900_init(int unit, char *dev) {

  if(unit < 0 || unit > 4)
    meas_err("meas_pdr900_init: Non-existing unit.");
  if(pdr900_fd[unit] == -1)
    pdr900_fd[unit] = meas_rs232_open(dev, MEAS_B9600 + MEAS_NOHANDSHAKE);
  return 0;
}

/*
 * Read pressure.
 *
 * unit = Unit to be read.
 * chan = channel number (1, 2, 3).
 *
 * Returns pressure in torr.
 *
 * Turn on data logging, accumulate one point and read it.
 * The dumb instrument does not allow to read the pressure directly.
 *
 * The default addr seems to be 253 and default baud rate 9600.
 * The default unit seemed to be Pascal (in the response) but it
 * still is in the units that are specified in the cotroller (see the
 * front panel).
 *
 */

double meas_pdr900_read(int unit, int chan) {

  char buf[512];
  double pressure;

  if(pdr900_fd[unit] == -1) meas_err("meas_pdr900_read: Non-existing unit.");
  if(chan < 1 || chan > 3) meas_err("meas_pdr900_read: Invalid channel.");
  sprintf(buf, "@253DLS!PR%d;FF", chan);
  meas_rs232_write(pdr900_fd[unit], buf, 14);
  meas_rs232_readeot(pdr900_fd[unit], buf, ";FF");

  meas_rs232_write(pdr900_fd[unit], "@253DLC!STOP;FF", 15);
  meas_rs232_readeot(pdr900_fd[unit], buf, ";FF");

  /* flush the log */
  meas_rs232_write(pdr900_fd[unit], "@253DL?;FF", 10);
  meas_rs232_readeot(pdr900_fd[unit], buf, ";FF");
  meas_misc_nsleep(0, 100000000);  // avoid controller dropping the command; 100 ms seems to work

  meas_rs232_write(pdr900_fd[unit], "@253DLT!00:00:01;FF", 19);
  meas_rs232_readeot(pdr900_fd[unit], buf, ";FF");

  meas_rs232_write(pdr900_fd[unit], "@253DLC!START;FF", 16);
  meas_rs232_readeot(pdr900_fd[unit], buf, ";FF");
  meas_misc_nsleep(0, 600000000);
  meas_rs232_write(pdr900_fd[unit], "@253DLC!STOP;FF", 15);
  meas_rs232_readeot(pdr900_fd[unit], buf, ";FF");

  meas_rs232_write(pdr900_fd[unit], "@253DL?;FF", 10);
  meas_rs232_readnl(pdr900_fd[unit], buf); /* header */
  meas_rs232_readnl(pdr900_fd[unit], buf); /* 1st data point */
  sscanf(buf, "%*[^;];%le", &pressure);
  while(1) /* skip until the end */
    if(meas_rs232_readeot2(pdr900_fd[unit], buf, "\r", ";FF") == 2) break;
  return pressure;
}

/*
 * Close the instrument.
 *
 * unit = Unit number to be closed.
 *
 */

int meas_pdr900_close(int unit) {

  if(pdr900_fd[unit] == -1) return;
  meas_rs232_close(pdr900_fd[unit]);
  pdr900_fd[unit] = -1;
  return 0;
}
