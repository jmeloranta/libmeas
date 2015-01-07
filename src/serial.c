/* 
 * RS232 port functions.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "serial.h"
#include "misc.h"

/*
 * Open RS232 port.
 *
 * dev   = RS232 devince name.
 * speed = line speed (see serial.h).
 *
 * Note: Returns file descriptor for the RS232 device.
 *
 */

EXPORT int meas_rs232_open(char *dev, int speed) {

  struct termios newtio;
  int fd;
  unsigned int handshake;

  if(speed >= MEAS_NOHANDSHAKE) {
    speed -= MEAS_NOHANDSHAKE;
    handshake = 0;
  } else handshake = CRTSCTS;
  meas_misc_root_on();
  if((fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY)) < 0)
    meas_err("meas_serial_open: Can't open device.");
  bzero(&newtio, sizeof(newtio));
  /*    newtio.c_cflag = B9600 | CRTSCTS | CS8 | CLOCAL | CREAD; */
  /*    newtio.c_cflag = B9600 | CS8 | CLOCAL | CRTSCTS | CREAD | CSTOPB; */
  switch (speed) {
  case MEAS_B9600:
    newtio.c_cflag = B9600 | CS8 | CLOCAL | handshake | CREAD | IGNBRK;
    break;
  case MEAS_B19200:
    newtio.c_cflag = B19200 | CS8 | CLOCAL | handshake | CREAD | IGNBRK;
    break;
  case MEAS_B57600:
    newtio.c_cflag = B57600 | CS8 | CLOCAL | handshake | CREAD | IGNBRK;
    break;
  case MEAS_B38400:
    newtio.c_cflag = B38400 | CS8 | CLOCAL | handshake | CREAD | IGNBRK;
    break;
  case MEAS_B115200:
    newtio.c_cflag = B115200 | CS8 | CLOCAL | handshake | CREAD | IGNBRK;
    break;
  }
  newtio.c_iflag = 0;
  newtio.c_oflag = 0;             /* raw output */
  newtio.c_cc[VTIME]    = 100;    /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 10;     /* blocking read until 1 character arrives */
  
  if(tcflush(fd, TCIFLUSH) < 0) meas_err("serial: TCIFLUSH failed.");
  if(tcsetattr(fd, TCSANOW, &newtio) < 0) meas_err("serial: TCSANOW failed.");
  
  fcntl(fd, F_SETFL, 0); /* FIXME: somehow the above set non-blocking I/O */
  
  meas_misc_root_off();
  return fd;
}

/*
 * Close RS232 device.
 *
 * fd = File descriptor for the device to be closed.
 *
 */

EXPORT int meas_rs232_close(int fd) {

  meas_misc_root_on();
  close(fd);
  meas_misc_root_off();
  return 0;
}

/*
 * Read line from RS232 port.
 *
 * fd  = File descriptor for the RS232 port.
 * buf = Output buffer for data.
 *
 */

EXPORT int meas_rs232_readnl(int fd, char *buf) {

  int i = 0;

  meas_misc_root_on();
  while (1) {
    meas_rs232_read(fd, buf + i, 1);
    if(buf[i] == MEAS_SERIAL_EOS) break;
    i++;
  }
  meas_misc_root_off();
  buf[i] = 0;
  return 0;
}

/*
 * Read line from RS232 port (with specified end of transmission string).
 *
 * fd  = File descriptor for the RS232 port.
 * buf = Output buffer for data.
 * eot = End of transmission string.
 *
 */

EXPORT int meas_rs232_readeot(int fd, char *buf, char *eot) {

  int i = 0, len;

  len = strlen(eot);
  meas_misc_root_on();
  while (1) {
    meas_rs232_read(fd, buf + i, 1);
    i++;
    if(i >= len && !strncmp(buf + i - len, eot, len)) break;
  }
  meas_misc_root_off();
  buf[i] = 0;
  return 0;
}

/*
 * Read line from RS232 port (with two possible end of transmission strings).
 *
 * fd  = File descriptor for the RS232 port.
 * buf = Output buffer for data.
 * eot1 = End of transmission string 1.
 * eot2 = End of transmission string 2.
 *
 * Return value: 1 = if eot1 found, 2 = if eot2 found.
 *
 */

EXPORT int meas_rs232_readeot2(int fd, char *buf, char *eot1, char *eot2) {

  int i = 0, len1, len2, which;

  len1 = strlen(eot1);
  len2 = strlen(eot2);
  meas_misc_root_on();
  while (1) {
    meas_rs232_read(fd, buf + i, 1);
    i++;
    if(i >= len1 && !strncmp(buf + i - len1, eot1, len1)) {
      which = 1;
      break;
    }
    if(i >= len2 && !strncmp(buf + i - len2, eot2, len2)) {
      which = 2;
      break;
    }
    }
  meas_misc_root_off();
  buf[i] = 0;
  return which;
}

/*
 * Read N bytes from RS232 port.
 *
 * fd  = File descriptor for the RS232 port.
 * buf = Output buffer for data.
 * len = Number of bytes (characters) to be read.
 *
 */

EXPORT int meas_rs232_read(int fd, char *buf, int len) {

  int len2 = 0;

  meas_misc_root_on();
  while (len2 < len) {
    if((len2 += read(fd, buf + len2, len - len2)) < 0)
      meas_err("meas_serial_read: Serial line read failed.");
  }
  meas_misc_root_off();
  return 0;
}

/*
 * Write data to RS232 port.
 *
 * fd  = File descriptor for the RS232 port.
 * buf = Buffer containing the data.
 * len = Number of bytes (characters) to write.
 *
 */

EXPORT int meas_rs232_write(int fd, char *buf, int len) {

  int len2 = 0;

  meas_misc_root_on();
  while (len2 < len) {
    if((len2 += write(fd, buf + len2, len - len2)) < 0)
      meas_err("meas_serial_write: Serial line write failed.");
  }
  meas_misc_root_off();
  return 0;
}

/*
 * Write single byte to RS232 port.
 *
 * fd   = File descriptor for the RS232 port.
 * byte = Byte to be written.
 *
 */

EXPORT int meas_rs232_writeb(int fd, unsigned char byte) {

  meas_misc_root_on();
  meas_rs232_write(fd, &byte, 1);
  meas_misc_root_off();
  return 0;
}
