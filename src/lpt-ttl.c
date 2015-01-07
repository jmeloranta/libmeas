/*
 * Driver for using a parallel port for TTL in/out.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/ioctl.h>
#include <linux/ppdev.h>
#include <linux/parport.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "lpt-ttl.h"
#include "misc.h"

static int fds[5] = {-1, -1, -1, -1, -1};

/* TODO: allow changing this at run time */
#ifdef MEAS_LPT_DIRECTIO

static unsigned ports[] = {0x378, 0x278}; /* lpt1 and lpt2 */

/*
 * Initialize LPT.
 *
 * unit = Unit number to be initialized (not used at the moment).
 *
 */

EXPORT int meas_lpt_init(int unit) {

  /* could add here detection for parallel port presence */
  return 0;
}

/*
 * Read from LPT.
 *
 * unit = Unit to be read (MEAS_LPT_LPT1 or MEAS_LPT_LPT2).
 *
 * Returns LPT data.
 *
 */

EXPORT int meas_lpt_read(int unit) {

  unsigned char a;

  if(unit < MEAS_LPT_LPT1 || unit > MEAS_LPT_LPT2)
    meas_err("meas_lpt: Invalid parallel port.\n");
  meas_misc_root_on();
  ioperm(ports[unit], 3, 1);
  a = inb(ports[unit]);
  ioperm(ports[unit], 3, 0);
  meas_misc_root_off();
  return (int) a;
}

/*
 * Write to parallel port.
 *
 * unit = Unit to be written (MEAS_LPT_LPT1 or MEAS_LPT_LPT2).
 * a    = Value.
 *
 */

EXPORT int meas_lpt_write(int unit, unsigned char a) {

  if(unit < MEAS_LPT_LPT1 || unit > MEAS_LPT_LPT2)
    meas_err("meas_lpt_write: Invalid parallel port.\n");
  meas_misc_root_on();
  ioperm(ports[unit], 3, 1);
  /* setup data */
  outb(a, ports[unit]);
  ioperm(ports[unit], 3, 0);
  meas_misc_root_off();
  return 0;
}

/*
 * Strobe LPT.
 *
 * unit = Unit for the operation (MEAS_LPT_LPT1 or MEAS_LPT_LPT2).
 * value = 1 (0 V) or 0 (+5 V).
 *
 */

EXPORT int meas_lpt_strobe(int unit, unsigned char value) {

  meas_misc_root_on();
  ioperm(ports[unit], 3, 1);
  /* strobe */
  outb(value?1:0, ports[unit]+2);
  ioperm(ports[unit], 3, 0);
  meas_misc_root_off();
  return 0;
}

/*
 * Close LPT.
 *
 * unit = Unit to be closed.
 *
 */

EXPORT int meas_lpt_close(int unit) {

  return 0;
}
#else

/*
 * Initialize LPT.
 *
 * unit = Unit number to be initialized.
 *
 */

EXPORT int meas_lpt_init(int unit) {

  char buf[MEAS_LPT_BUF_SIZE];
  unsigned int mode = IEEE1284_MODE_BYTE;

  if(fds[unit] == -1) {
    sprintf(buf, "/dev/parport%d", unit);
    meas_misc_root_on();
    if((fds[unit] = open(buf, O_RDWR)) == -1)
      meas_err("meas_lpt_init: Non-existent parallel port.");
    ioctl(fds[unit], PPCLAIM, NULL);
    ioctl(fds[unit], PPEXCL, NULL);
    ioctl(fds[unit], PPSETMODE, &mode);
    meas_misc_root_off();
  }
  return 0;
}

/*
 * Close LPT.
 *
 * unit = Unit to be closed.
 *
 */

EXPORT int meas_lpt_close(int unit) {

  if(fds[unit] == -1)
    meas_err("meas_lpt_close: Unit already closed.");
  close(fds[unit]);
  fds[unit] = -1;
  return 0;
}

/*
 * Read from LPT.
 *
 * unit = Unit to be read (MEAS_LPT_LPT1 or MEAS_LPT_LPT2).
 *
 * Returns LPT data.
 *
 */

EXPORT int meas_lpt_read(int unit) {

  unsigned char a;
  unsigned int dir = 1;

  if(fds[unit] == -1)
    meas_err("meas_lpt_read: Non-existent parallel port.");
  meas_misc_root_on();
  ioctl(fds[unit], PPRDATA, &a);
  meas_misc_root_off();
  return (int) a;
}

/*
 * Write to parallel port.
 *
 * unit = Unit to be written (MEAS_LPT_LPT1 or MEAS_LPT_LPT2).
 * a    = Value.
 *
 */

EXPORT int meas_lpt_write(int unit, unsigned char a) {

  unsigned int dir = 0;
  struct timespec ts;
  struct ppdev_frob_struct frob;  

  if(fds[unit] == -1)
    meas_err("meas_lpt_write: Non-existent parallel port.");
  meas_misc_root_on();
  ioctl(fds[unit], PPWDATA, &a);
  meas_misc_root_off();
  return 0;
}

/*
 * Strobe LPT.
 *
 * unit = Unit for the operation (MEAS_LPT_LPT1 or MEAS_LPT_LPT2).
 * value = 1 (0 V) or 0 (+5 V).
 *
 */

EXPORT int meas_lpt_strobe(int unit, unsigned char value) {

  unsigned int dir = 0;
  struct timespec ts;
  struct ppdev_frob_struct frob;  

  if(fds[unit] == -1)
    meas_err("meas_lpt_strobe: Non-existent parallel port.");
  meas_misc_root_on();
  /* Strobe */
  frob.mask = PARPORT_CONTROL_STROBE;
  frob.val = value?PARPORT_CONTROL_STROBE:0;
  ioctl(fds[unit], PPFCONTROL, &frob);
  meas_misc_root_off();
  return 0;
}

#endif /* MEAS_LPT_DIRECTIO */
