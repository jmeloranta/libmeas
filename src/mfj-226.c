/*
 *
 * MFJ-226 antenna analyzer.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serial.h"
#include "misc.h"

/* Up to 5 units supported */
static int mfj226_fd[5] = {-1, -1, -1, -1, -1};

/* 
 * Initialize the connection.
 *
 * unit = Unit number.
 * dev  = RS232 device for the controller (/dev/ttyUSBX).
 *
 */

EXPORT int meas_mfj226_open(int unit, char *dev) {

  if(unit < 0 || unit > 4)
    meas_err("meas_mfj226_init: Non-existing unit.");
  if(mfj226_fd[unit] == -1)
    mfj226_fd[unit] = meas_rs232_open(dev, MEAS_B115200);
  // TODO: needs sleep before this? -- not really needed anyway
  //meas_rs232_writeb(mfj226_fd[unit], 'D');
  return 0;
}

/*
 * Read device.
 *
 * unit = Unit to be read.
 * mag  = Magnitude (double *).
 * deg  = Phase angle in degrees (double *).
 *
 * Returns 0 for OK, -1 for error.
 *
 */

EXPORT int meas_mfj226_read(int unit, double *mag, double *ang) {

  char buf[512];

  if(mfj226_fd[unit] == -1) meas_err("meas_mfj226_read: Non-existing unit.");
  meas_rs232_writeb(mfj226_fd[unit], 'S');
  meas_rs232_readeoc(mfj226_fd[unit], buf, '\r');
  if(sscanf(buf, " %le, %le", mag, ang) != 2) {
    meas_err("meas_mfj226_read: Non-standard response from instrument.\n");
    return -1;
  }
  return 0;
}

/*
 * Write to device.
 *
 * unit = Unit to write to.
 * freq = Frequency to set (Hz, int). 
 *
 * Return value 0 for success, -1 for error.
 *
 */

EXPORT int meas_mfj226_write(int unit, int freq) {

  char buf[512];
  
  if(freq < 1000000 || freq > 230000000) return -1;
  if(mfj226_fd[unit] == -1) meas_err("meas_mfj226_write: Non-existing unit.");
  sprintf(buf, "%09d", freq);  // 9 digit frequency in Hz (the manual is wrong in saying 6 ...)
  meas_rs232_write(mfj226_fd[unit], buf, strlen(buf));
  return 0;
}

/*
 * Close the instrument.
 *
 * unit = Unit number to be closed.
 *
 */

EXPORT int meas_mfj226_close(int unit) {

  if(mfj226_fd[unit] == -1) return;
  meas_rs232_close(mfj226_fd[unit]);
  mfj226_fd[unit] = -1;
  return 0;
}
