/*
 * 
 * Stanford Research Instruments SR245 (with SR250 behind it).
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "misc.h"
#include "sr245.h"
#include "serial.h"
#include "gpib.h"

/* Up to 5 units supported */
static int sr245_fd[5] = {-1, -1, -1, -1, -1};
static int trig_mode[5] = {0, 0, 0, 0, 0};
static int srmode[5] = {0, 0, 0, 0, 0};

/* 
 * Initialize the DAC interface.
 *
 * unit   = unit number (0 - 4)
 * board  = GPIB board number
 * dev    = GPIB id
 * serial = NULL for GPIB operation or path to RS port for serial operation.
 *          (e.g. /dev/ttyS0)
 *
 * Returns zero on success.
 *
 */

int meas_sr245_init(int unit, int board, int dev, char *serial) {

  if(unit < 0 || unit > 4)
    meas_err("meas_sr245_init: Illegal units number.");
  if(serial) {
    if(sr245_fd[unit] == -1)
      sr245_fd[unit] = meas_rs232_open(serial, 1);
    srmode[unit] = 1;
  } else {
    if(sr245_fd[unit] == -1) {
      sr245_fd[unit] = meas_gpib_open(board, dev); 
      meas_gpib_timeout(sr245_fd[unit], 1.0);
    }
    srmode[unit] = 0;
  }
  return 0;
}

/*
 * Disable triggers.
 *
 * unit = SR245 unit number.
 *
 * Returns zero on success.
 *
 */

int meas_sr245_disable_trigger(int unit) {

  if(sr245_fd[unit] == -1) 
    meas_err("meas_sr245_disable_trigger: Non-existent unit.");
  if(srmode[unit]) 
    meas_rs232_write(sr245_fd[unit], "DT\r", 3);
  else
    meas_gpib_write(sr245_fd[unit], "DT", MEAS_SR245_TERM);
  return 0;
}

/*
 * Enable triggers.
 *
 * unit = SR245 unit number.
 *
 * Returns zero on success.
 *
 */

int meas_sr245_enable_trigger(int unit) {

  if(sr245_fd[unit] == -1)
    meas_err("meas_sr245_enable_trigger: Non-existent unit.");
  if(srmode[unit]) 
    meas_rs232_write(sr245_fd[unit], "ET\r", 3);
  else
    meas_gpib_write(sr245_fd[unit], "ET", MEAS_SR245_TERM);
  return 0;
}

/*
 * SR245 operation mode.
 * 
 * unit = SR245 unit number.
 * mode = 1 for synchronous, 0 for asynchronous.
 *
 * Return zero on success.
 *
 */

int meas_sr245_mode(int unit, int mode) {

  if(sr245_fd[unit] == -1)
    meas_err("meas_sr245_mode: Non-existent unit.");
  if(srmode[unit]) {
    if(mode) {
      meas_rs232_write(sr245_fd[unit], "MS\r", 3);
      meas_rs232_write(sr245_fd[unit], "T1\r", 3);
      meas_rs232_write(sr245_fd[unit], "ET\r", 3);
      meas_rs232_write(sr245_fd[unit], "DT\r", 3);
      trig_mode[unit] = 1;
    } else {
      meas_rs232_write(sr245_fd[unit], "MA\r", 3);
      trig_mode[unit] = 0;
    }
  } else {
    if(mode) {
      meas_gpib_write(sr245_fd[unit], "MS", MEAS_SR245_TERM);
      meas_gpib_write(sr245_fd[unit], "T1", MEAS_SR245_TERM);
      meas_gpib_write(sr245_fd[unit], "ET", MEAS_SR245_TERM);
      meas_gpib_write(sr245_fd[unit], "DT", MEAS_SR245_TERM);
      trig_mode[unit] = 1;
    } else {
      meas_gpib_write(sr245_fd[unit], "MA", MEAS_SR245_TERM);
      trig_mode[unit] = 0;
    }
  }
  return 0;
}

/*
 * Declare input/output analog ports.
 *
 * unit = SR245 unit number.
 * n    = n first ports as inputs (A/D) and the rest outputs (D/A).
 *
 * Returns zero on success.
 *
 */

int meas_sr245_ports(int unit, int n) {

  char buf[512];

  if(sr245_fd[unit] == -1)
    meas_err("meas_sr245_ports: Non-existent unit.");

  sprintf(buf, "I%d", n);
  if(srmode[unit]) {
    strcat (buf, "\r");
    meas_rs232_write(sr245_fd[unit], buf, 3);
  } else
    meas_gpib_write(sr245_fd[unit], buf, MEAS_SR245_TERM);
  return 0;
}


/*
 * Read data from a given port (in Volts).
 *
 * unit = SR245 unit number.
 * port = port number to read data from (0 - 7).
 *
 * Returns the value in Volts.
 *
 */

double meas_sr245_read(int unit, int port) {

  char buf[512];
  double val;

  if(sr245_fd[unit] == -1)
    meas_err("meas_sr245_read: Non-existent unit.");

  meas_misc_disable_signals();
  if(srmode[unit]) {
    if(trig_mode[unit])
      meas_rs232_write(sr245_fd[unit], "ET\r", 3); /* now we accept triggers */
    sprintf(buf, "?%d\r", port);
    meas_rs232_write(sr245_fd[unit], buf, 3);
    meas_rs232_readnl(sr245_fd[unit], buf);
    if(trig_mode[unit]) 
      meas_rs232_write(sr245_fd[unit], "DT\r", 3); /* disable triggers */
  } else {
    if(trig_mode[unit])
      meas_gpib_write(sr245_fd[unit], "ET", MEAS_SR245_TERM); /* now we accept triggers */
    sprintf(buf, "?%d", port);
    meas_gpib_write(sr245_fd[unit], buf, MEAS_SR245_TERM);
    meas_gpib_read(sr245_fd[unit], buf);
    if(trig_mode[unit]) 
      meas_gpib_write(sr245_fd[unit], "DT", MEAS_SR245_TERM); /* disable triggers */
  }
  meas_misc_enable_signals();

  sscanf(buf, "%lf", &val);
  return val;
}

/*
 * Write data to a given port.
 *
 * unit = SR245 unit number
 * port = output port number (0 - 7)
 * val  = value to be output (in Volts)
 *
 * Returns zero on success.
 *
 */

int meas_sr245_write(int unit, int port, double val) {

  char buf[512];

  if(sr245_fd[unit] == -1) 
    meas_err("meas_sr245_write: Non-existent unit.");
  sprintf(buf, "S%d=%lf", port, val);

  if(srmode[unit]) {
    strcat(buf, "\r");
    meas_rs232_write(sr245_fd[unit], buf, strlen(buf));
  } else
    meas_gpib_write(sr245_fd[unit], buf, MEAS_SR245_TERM);
  return 0;
}

/*
 * Specify a given TTL port as input or output 
 *
 * unit = SR245 unit number
 * port = TTL port number (1 or 2)
 * mode = 0 for input, 1 for output
 *
 * Returns zero on success.
 *
 */

int meas_sr245_ttl_mode(int unit, int port, int mode) {

  char buf[512];

  if(sr245_fd[unit] == -1) 
    meas_err("meas_sr245_ttl_mode: Non-existent unit.");

  if(mode)  /* output */
    sprintf(buf, "SB%d=0", port);
  else /* input */
    sprintf(buf, "SB%d=I", port);

  if(srmode[unit]) {
    strcat(buf, "\r");
    meas_rs232_write(sr245_fd[unit], buf, strlen(buf));
  } else 
    meas_gpib_write(sr245_fd[unit], buf, MEAS_SR245_TERM);
  return 0;
}

/*
 * Read a given TTL port (must be declared as input)
 *
 * unit = SR245 unit number
 * port = port number to read (1 or 2)
 *
 * Returns the port reading.
 *
 */

int meas_sr245_ttl_read(int unit, int port) {

  char buf[512];
  int val;

  if(sr245_fd[unit] == -1) 
    meas_err("meas_sr245_ttl_read: Non-existent unit.");

  meas_misc_disable_signals();
  sprintf(buf, "?B%d", port);
  if(srmode[unit]) {
    strcat(buf, "\r");
    meas_rs232_write(sr245_fd[unit], buf, strlen(buf));
    meas_rs232_readnl(sr245_fd[unit], buf);  
  } else {
    meas_gpib_write(sr245_fd[unit], buf, MEAS_SR245_TERM);
    meas_gpib_read(sr245_fd[unit], buf);
  }
  meas_misc_enable_signals();

  sscanf(buf, "%d", &val);
  return val;
}

/*
 * Write to a given TTL port (must be declared as output)
 *
 * unit = SR245 unit number
 * port = output port (1 or 2)
 * val  = value to output
 *
 * Returns zero on success.
 *
 */

int meas_sr245_ttl_write(int unit, int port, int val) {

  char buf[512];

  if(sr245_fd[unit] == -1) 
    meas_err("meas_sr245_ttl_write: Non-existent unit.");

  sprintf(buf, "SB%d=%d", port, (val?1:0));
  if(srmode[unit]) {
    strcat(buf, "\r");
    meas_rs232_write(sr245_fd[unit], buf, strlen(buf));
  } else meas_gpib_write(sr245_fd[unit], buf, MEAS_SR245_TERM);
  return 0;
}

/*
 * (positive) Trigger a given TTL port (must be declared as output)
 *
 * unit  = SR245 unit number
 * port  = TTL port number (1 or 2)
 * ttime = time to keep the signal up in nanoseconds
 *         (verify what is the time resolution of your system)
 *
 * Returns zero on success.
 *
 */

int meas_sr245_ttl_trigger(int unit, int port, long ttime) {

  if(sr245_fd[unit] == -1) 
    meas_err("meas_sr245_ttl_trigger: Non-existent unit.");

  meas_misc_disable_signals();
  meas_sr245_ttl_write(unit, port, 0);
  meas_sr245_ttl_write(unit, port, 1);
  meas_misc_nsleep(0, ttime);
  meas_sr245_ttl_write(unit, port, 0);
  meas_misc_enable_signals();

  return 0;
}

/*
 * Master reset.
 *
 * unit = SR245 unit to be reset
 *
 * Returns zero on success.
 *
 */

int meas_sr245_reset(int unit) {

  if(sr245_fd[unit] == -1)
    meas_err("meas_sr245_reset: Non-existent unit.");
  if(srmode[unit])
    meas_rs232_write(sr245_fd[unit], "MR\r", 3);
  else
    meas_gpib_write(sr245_fd[unit], "MR", MEAS_SR245_TERM);
  sleep(2);
  return 0;
}

/*
 * Scan mode.
 *
 * unit    = SR245 unit number
 * ports   = an array of port numbers to be scanned
 * nports  = number of ports in ports array
 * points  = an array of data points read
 * npoints = number of points in points array
 *
 * Returns zero on success.
 *
 * (NOTE: NOT TESTED) 
 *
 */

int meas_sr245_scan_read(int unit, int *ports, int nports, double *points, int npoints) {

  char buf[512], buf3[128];
  unsigned char buf2[65535];
  int i, j, sign;

  if(sr245_fd[unit] == -1)
    meas_err("meas_sr245_scan_read: Non-existent unit.");

  meas_misc_disable_signals();
  /* Alternative way would be to use SC - would allow for higher throughput */
  if(!trig_mode[unit]) meas_err("meas_sr245_scan_read: Only triggered mode allowed.");
  if(npoints * 2 > 65535) meas_err("meas_sr245_scan_read: Too many samples.");
  if(srmode[unit]) 
    meas_rs232_write(sr245_fd[unit], "ES\r", 3);
  else
    meas_gpib_write(sr245_fd[unit], "ES", MEAS_SR245_TERM);
  sprintf(buf, "SS");
  for (i = 0; ; i++) {
    sprintf(buf3, "%d", ports[i]);
    strcat(buf, buf3);
    if (i == nports) break;
    strcat(buf, ",");
  }
  sprintf(buf3, ":%d", npoints);
  strcat(buf, buf3);

  if(srmode[unit]) {
    strcat(buf, "\r");
    meas_rs232_write(sr245_fd[unit], buf, strlen(buf));
    meas_rs232_write(sr245_fd[unit], "ET\r", 3);
    meas_rs232_read(sr245_fd[unit], (char *) buf2, 2 * npoints);
    meas_rs232_write(sr245_fd[unit], "ES\r", 3);
  } else {
    meas_gpib_write(sr245_fd[unit], buf, MEAS_SR245_TERM);
    meas_gpib_write(sr245_fd[unit], "ET", MEAS_SR245_TERM);
    meas_gpib_read_n(sr245_fd[unit], (char *) buf2, 2 * npoints);
    meas_gpib_write(sr245_fd[unit], "ES", MEAS_SR245_TERM);
  }
  meas_misc_enable_signals();

  /* reconstruct data */
  for (i = j = 0; i < 2*npoints; i += 2, j++) {
    sign = buf2[i] & 0x10;
    if(sign) sign = -1;
    else sign = 0;
    points[j] = sign * ((buf2[i] & 0x0f) * 256 + buf2[i+1]) * 0.0025; /* to volts */
  }
  return 0;
}
