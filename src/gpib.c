/*
 * GPIB wrapper functions.
 *
 * Note that the boards in /etc/gpib.conf must be defined and named as:
 * gpib0  = 1st board,
 * gpib1  = 2nd board, etc.
 *
 */

#ifdef GPIB

#include <stdio.h>
#include <gpib/ib.h>
#include <string.h>
#include <stdlib.h>
#include "gpib.h"
#include "misc.h"

/* Up to 5 boards supported */
static int board_fd[5] = {-1, -1, -1, -1, -1};

/*
 * Open GPIB device.
 *
 * board = GPIB board # (0, 1, ...).
 * id    = GPIB ID.
 *
 */

EXPORT int meas_gpib_open(int board, int id) {

  int fd;
  char buf[128];

  if(board >= 5) meas_err("meas_gpib_open: Illegal board number.\n");
  meas_misc_root_on();
  if((fd = ibdev(board, id, 0, MEAS_GPIB_TIMEOUT, MEAS_GPIB_SENDEOI, MEAS_GPIB_EOS)) < 0 ) {
    meas_misc_root_off();
    meas_err("meas_gpib_open: Can't open GPIB device.");
  }
  if(board_fd[board] == -1) {  
    sprintf(buf, "gpib%1d", board);
    if (board == 0) {
      if ((board_fd[board] = ibfind("violet")) < 0 && (board_fd[board] = ibfind(buf)) < 0) {
	meas_misc_root_off();
	meas_err("meas_gpib_open: Can't open GPIB board.\n");
      }
    } else {
      if ((board_fd[board] = ibfind(buf)) < 0) {
	meas_misc_root_off();
	meas_err("meas_gpib_open: Can't open GPIB board.\n");
      }
    }
    meas_gpib_clear(board);
    usleep(MEAS_GPIB_DELAY); /* TODO: are these waits still needed? */
    meas_misc_root_off();
  }
  return fd;
}

/*
 * Close GPIB device.
 *
 * boad = GPIB board # (0, 1, ...).
 * fd   = fd to close.
 *
 */ 

EXPORT int meas_gpib_close(int board, int fd) {
  
  ibonl(fd, 0); /* offline */
  return 0;
}

/*
 * Set GPIB mode.
 * 
 * fd = GPIB device descriptor.
 * mode = Set as follows:
 *        REOS = 0x400, XEOS = 0x800, BIN = 0x1000
 *        REOS = enable end-of-string on reads, XEOS = assert EOI line when
 *        EOS is sent.
 *        BIN = match EOS with full 8 bits rather than 7 bits.
 *        See GPIB documentation for further info.
 *
 */

EXPORT int meas_gpib_set(int fd, int mode) {

  (void) ibeos(fd, mode);
  return 0;
}

/*
 * Set device timeout value.
 * 
 * fd    = GPIB device descriptor.
 * value = timeout value in seconds.
 *
 */

EXPORT int meas_gpib_timeout(int fd, double value) {

  int val;
  
  if(value == 0.0) val = 0;
  else if(value <= 10.0E-6) val = 1;
  else if(value <= 30.0E-6) val = 2;
  else if(value <= 100.0E-6) val = 3;
  else if(value <= 300.0E-6) val = 4;
  else if(value <= 1.0E-3) val = 5;
  else if(value <= 3.0E-3) val = 6;
  else if(value <= 10.0E-3) val = 7;
  else if(value <= 30.0E-3) val = 8;
  else if(value <= 100.0E-3) val = 9;
  else if(value <= 300.0E-3) val = 10;
  else if(value <= 1.0) val = 11;
  else if(value <= 3.0) val = 12;
  else if(value <= 10.0) val = 13;
  else if(value <= 30.0) val = 14;
  else if(value <= 100.0) val = 15;
  else if(value <= 300.0) val = 16;
  else if(value <= 1000.0) val = 17;
  else meas_err("meas_gpib_timeout: illegal device timeout value.");
  
  (void) ibtmo(fd, val); /* see ibtmo documentation for value */
  return 0;
}

/*
 * Read string from GPIB device.
 *
 * fd  = GPIB device descriptor.
 * buf = Buffer for output.
 *
 */

EXPORT int meas_gpib_read(int fd, char *buf) {

  char *tmp;

  usleep(MEAS_GPIB_DELAY);
  if(ibrd(fd, buf, MEAS_GPIB_BUF_SIZE) < 0) 
    meas_err("gpib: read failed.");
  if((tmp = strchr(buf, '\r'))) *tmp = 0;
  return 0;
}

/*
 * Read N bytes from GPIB device.
 *
 * fd     = GPIB device descriptor.
 * buf    = Buffer for output.
 * nbytes = Number of bytes to read.
 *
 */

EXPORT int meas_gpib_read_n(int fd, char *buf, int nbytes) {

  char *tmp;
  int len, tmp2, err;

  tmp = buf;
  len = 0;
  while (1) {
    usleep(MEAS_GPIB_DELAY);
    ibrd(fd, tmp, nbytes);
    if(iberr) {
      if(iberr != EABO) {
	fprintf(stderr, "meas_gpib_read_n: read failed (err = %d).\n", iberr);
	continue;
      }
      /* Hopefully the device trasferred everything and we can read the data */
      tmp2 = nbytes;
    } else tmp2 = ThreadIbcnt();
    tmp += tmp2;
    len += tmp2;
    if(len >= nbytes) break;
  }
  return 0;
}

/*
 * Write string to GPIB device.
 *
 * fd   = GPIB device descriptor.
 * buf  = Data to write.
 * crlf = 0: use CR, 1: use CR LF as line end.
 *
 */

EXPORT int meas_gpib_write(int fd, char *buf, int crlf) {
  
  char tmp[4096];
  int len;

  if(crlf) { /* CR LF */
    strcpy(tmp, buf);
    len = strlen(buf);
    tmp[len] = '\r';
    tmp[len+1] = '\n';
    
    usleep(MEAS_GPIB_DELAY);
    if(ibwrt(fd, tmp, len+2) < 0)
      meas_err("meas_gpib_write: write failed.");
  } else {
    strcpy(tmp, buf);
    len = strlen(buf);
    tmp[len] = MEAS_GPIB_EOS;
    
    usleep(MEAS_GPIB_DELAY);
    if(ibwrt(fd, tmp, len+1) < 0)
      meas_err("meas_gpib_write: write failed.");
  }
  return 0;
}

/*
 * Issue GPIB command.
 * 
 * fd  = GPIB device descriptor.
 * cmd = Command byte.
 *
 */

EXPORT int meas_gpib_cmd(int fd, char cmd) {

  ibcmd(fd, &cmd, 1);
  return 0;
}

/*
 * Issue interface clear.
 *
 * Board = GPIB board to clear.
 *
 */

EXPORT int meas_gpib_clear(int board) {

  if(board_fd[board] == -1)
    meas_err("meas_gpib_clear: non-existent board.");
  ibsic(board_fd[board]);   /* Inteface clear */
  meas_gpib_cmd(board, DCL);     /* Device clear */
  ibsre(board_fd[board],1); /* Everyone to remote */
  usleep(MEAS_GPIB_DELAY);
  return 0;
}

/*
 * For some old instruments (such as FL3001/2). Note that there
 * may be odd problems when other devices are connected to the same bus..
 *
 */

EXPORT int meas_gpib_old_read(int board, int id, char *buf, int len) {

  if(board_fd[board] == -1)
    meas_err("meas_gpib_old_read: non-existent board.");
  meas_gpib_cmd(board_fd[board], 0x20);     /* board (id = 0) as listener */
  meas_gpib_cmd(board_fd[board], 0x40 + id);/* instrument (id) as talker */
  ibrd(board_fd[board], buf, len);
  meas_gpib_cmd(board_fd[board], UNT);
  return 0;
}

/*
 * For some old instrumnents (such as FL3001/2). Note that there may
 * be problems when other devices are connected to the same bus..
 *
 */

EXPORT int meas_gpib_old_write(int board, int id, char *buf, int len) {

  if(board_fd[board] == -1)
    meas_err("meas_gpib_old_write: non-existent board.");
  meas_gpib_cmd(board_fd[board], UNL);
  meas_gpib_cmd(board_fd[board], 0x20 + id); /* listener = device with id */
  meas_gpib_cmd(board_fd[board], 0x40);      /* talker = board (id 0) */
  ibwrt(board_fd[board], buf, len);
  return 0;
}

/*
 * Read N bytes from GPIB device.
 *
 * fd     = GPIB device descriptor.
 * buf    = Buffer for output.
 * nbytes = Number of bytes to read.
 *
 */

EXPORT int meas_gpib_async_read_n(int fd, char *buf, int nbytes) {

  ibrda(fd, buf, nbytes);
  return 0;
}

/*
 * Write string to GPIB device (async).
 *
 * fd   = GPIB device descriptor.
 * buf  = Data to write.
 * crlf = 0: use CR, 1: use CR LF as line end.
 *
 */

EXPORT int meas_gpib_async_write(int fd, char *buf, int crlf) {
  
  char tmp[4096];
  int len;

  if(crlf) { /* CR LF */
    strcpy(tmp, buf);
    len = strlen(buf);
    tmp[len] = '\r';
    tmp[len+1] = '\n';
    ibwrta(fd, tmp, len+2);
  } else {
    strcpy(tmp, buf);
    len = strlen(buf);
    tmp[len] = MEAS_GPIB_EOS;    
    ibwrta(fd, tmp, len+1);
  }
  return 0;
}

#endif /* GPIB */
