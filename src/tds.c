/*
 * Support tektronix TDS series oscilloscopes.
 *
 * This contains only a routine to read data off the oscillscope.
 * There are tons of GPIB commands that it accepts, use those directly
 * to change the scope paramters.
 *
 */

#ifdef GPIB

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "tds.h"
#include "gpib.h"
#include "misc.h"

static int tds_fd[5] = {-1, -1, -1, -1, -1};
static int data_width = -1;

/*
 * Initialize instrument.
 *
 * fd      = GPIB device file descriptor (int).
 * src     = Data source on the scope: "CH1", "CH2", etc., "MATH1", ... (char *)
 * length  = Data length (int). (number of points to acquire)
 * start   = Data start location (int).
 * end     = Data stop location (int).
 * width   = 1: one byte per point, 2: two bytes per point. (int)
 *
 * This must be called first: it sets data transfer parameters.
 *
 * Return 0 for OK, -1 for error.
 *
 */

EXPORT int meas_tds_init(int fd, char *src, int length, int start, int end, int width) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(end < start) return -1;
  if(width != 1 && width != 2) return -1;
  if(length*width >= MEAS_GPIB_BUF_SIZE) return -1; // Increase MEAS_GPIB_BUF_SIZE
  usleep(10000); // device gets stuck if communicated to quickly after open
  meas_gpib_set(fd, BIN); /* XEOS and REOS disabled (binary data transfer) */
  if(meas_gpib_write(fd, "DATA:ENCDG RIBINARY", MEAS_TDS_CRLF) < 0) return -1;
  sprintf(buf, "DATA:SOURCE %s", src);
  if(meas_gpib_write(fd, buf, MEAS_TDS_CRLF) < 0) return -1;
  sprintf(buf, "DATa:STARt %d", start);
  if(meas_gpib_write(fd, buf, MEAS_TDS_CRLF) < 0) return -1;
  sprintf(buf, "DATa:STOP %d", end);
  if(meas_gpib_write(fd, buf, MEAS_TDS_CRLF) < 0) return -1;
  if(width == 1) { 
    if(meas_gpib_write(fd, "DATa:WIDth 1", MEAS_TDS_CRLF) < 0) return -1;
    data_width = 1;
  } else {
    if(meas_gpib_write(fd, "DATa:WIDth 2", MEAS_TDS_CRLF) < 0) return -1;
    data_width = 2;
  }
  sprintf(buf, "HORIZONTAL:RECORDLENGTH %d", length);
  if(meas_gpib_write(fd, buf, MEAS_TDS_CRLF) < 0) return -1;
  return 0;
}

/*
 * gpib_fd = GPIB device file descriptor (int).
 * data    = Storage space for data (double *).
 *
 * Return number of data points or -1 for error.
 *
 */

EXPORT int meas_tds_transfer(int fd, double *data, int len) {

  char buf[MEAS_GPIB_BUF_SIZE];
  short *buf2 = (short *) buf;
  int x, yyy;

  if(meas_gpib_write(fd, "CURVE?", MEAS_TDS_CRLF) < 0) return -1;
  if(meas_gpib_read_n(fd, buf, 9) < 0) return -1;

  if(buf[7] != '#') return -1;
  x = buf[8] - '1' + 1;
  if(meas_gpib_read_n(fd, buf, x) < 0) return -1;
  buf[x] = '\0';
  yyy = atoi(buf);
  if(meas_gpib_read_n(fd, buf, yyy) < 0) return -1;

  if(data_width == 2) {
    if(meas_endian() == 1)
      for(x = 0; x < yyy; x += 2) data[x/2] = (double) (buf[x] + 256 * buf[x+1]);
    else
      for(x = 0; x < yyy; x += 2) data[x/2] = (double) (buf[x+1] + 256 * buf[x]);
    yyy /= 2;
  } else {
    for(x = 0; x < yyy; x++)
      data[x] = (double) buf[x];
  }
  return yyy;
}

#endif /* GPIB */
