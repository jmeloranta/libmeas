/*
 * Oxford ITC 503 temperature controller. Using RS232 (not GPIB!).
 *
 * For now we are just reading the temperature - we could do much more later...
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "itc503.h"
#include "serial.h"
#include "misc.h"

/* Up to 5 units supported */
static int temp_fd[5] = {-1, -1, -1, -1, -1};

/* 
 * Initialize temperature reading.
 *
 * unit = Unit number.
 * dev  = Device name for the RS232 port.
 *
 * Note: 9600 Baud.
 *
 */

int meas_itc503_init(int unit, char *dev) {

  if(temp_fd[unit] == -1)
    temp_fd[unit] = meas_rs232_open(dev, MEAS_B9600);
  return 0;
}

/*
 * Read temperature.
 *
 * unit = Unit for the operation.
 *
 * Returns temperature in Kelvin.
 *
 */

double meas_itc503_read(int unit) {

  char buf[512];
  double t;

  if(temp_fd[unit] == -1)
    meas_err("meas_itc503_read: Temperature read error.");
  meas_rs232_write(temp_fd[unit], "R1\r", 3);
  meas_rs232_readnl(temp_fd[unit], buf);
  sscanf(buf+1, "%le", &t);
  return t;
}
