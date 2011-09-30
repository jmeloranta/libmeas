/*
 * Mettler-Toledo B-S/FACT line balances using RS232. 
 * TODO: use MT-SICS format rather than the simple PM.
 *
 * The balance must have the following settings (manual section 4.3.12 ->):
 *
 * Peripheral unit = Host
 * Send format = PM 
 * Send mode = S. Cont.
 * Baud rate = 19200
 * Bit/Parity = 8b-no     (8 bits, no parity)
 * Handshake = HS HArd    (hardware RTS/CTS handshake)
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include "mettler.h"
#include "serial.h"
#include "misc.h"

/* Up to 5 units supported */
static int mettler_fd[5] = {-1, -1, -1, -1, -1};

/* Initialize the balance.
 *
 * unit = Unit number.
 * dev  = RS232 device for the balance.
 *
 * Note: 19200 Baud.
 *
 */

int meas_mettler_init(int unit, char *dev) {

  if(mettler_fd[unit] == -1)
    mettler_fd[unit] = meas_rs232_open(dev, MEAS_B19200);
  return 0;
}

/* 
 * Read current weight.
 *
 * unit = Unit to be read.
 * 
 * Returns weight in grams.
 *
 */

double meas_mettler_read(int unit) {

  char buf[512], un[5];
  double t;

  if(mettler_fd[unit] == -1) 
    meas_err("meas_mettler_read: Balance read error (1).");
  meas_rs232_write(mettler_fd[unit], "S\r\n", 3);
  meas_rs232_readnl(mettler_fd[unit], buf);
  if(sscanf(buf+1, "S S %lf %s", &t, un) != 2) 
    meas_err("meas_mettler_read: Balance read error (2).");
  if(un[0] == 'm' && un[1] == 'g') t *= 1E-3;
  if(un[0] == 'k' && un[1] == 'g') t *= 1E3;
  return t;
}
