/*
 * 
 * LambdaPhysik Scanmate Pro dye laser.
 *
 * Could add lots of other functions later...
 *
 * The system takes care of doubling crystals etc. automatically.
 * No need to control them here. However, for extended scans
 * (i.e., dye mixtures etc.) the built-in polynomial fit is not sufficient.
 * It is also possible to control the crystal position manually.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <setjmp.h>
#include "scanmate_pro.h"
#include "serial.h"
#include "misc.h"

static sigjmp_buf env;

static int dye_fd[5] = {-1, -1, -1, -1, -1};

/*
 * Initialize the dye laser
 *
 * unit = Unit number.
 * dev  = RS232 device name.
 *
 * Note: 57600 Baud.
 *
 */

EXPORT int meas_scanmate_pro_init(int unit, char *dev) {

  if(dye_fd[unit] == -1)
    dye_fd[unit] = meas_rs232_open(dev, MEAS_B57600);
  return 0;
}

/*
 * Set grating.
 *
 * unit = Unit to be addressed.
 * x    = Grating operating order.
 *
 */

EXPORT int meas_scanmate_pro_grating(int unit, int x) {

  char buf[512];

  if(dye_fd[unit] == -1) meas_err("meas_scanmate_pro_grating: Illegal unit.");

  sprintf(buf, "ORDER=%d\r", x);
  meas_rs232_write(dye_fd[unit], buf, strlen(buf) + 1);
  return 0;
}

static void scanmate_pro_signal(int x) {

  longjmp(env, 1); /* Activate the alarm handler code in scanmate_pro_setwl() */
}

/*
 * Set wavelength.
 *
 * unit = Unit number.
 * wl   = Wavelength in nm.
 *
 */

EXPORT int meas_scanmate_pro_setwl(int unit, double wl) {

  char buf[512];
  int i;

  if(dye_fd[unit] == -1) meas_err("meas_scanmate_pro_setwl: Illegal unit.");

  sprintf(buf, "X=%lf\r", wl);
  meas_rs232_write(dye_fd[unit], buf, strlen(buf)+1);

  (void) signal(SIGALRM, scanmate_pro_signal);
  alarm(MEAS_SCANMATE_PRO_SKIP_HANDSHAKE); /* Sometimes ack from the dyelaser is lost */
                         /* Continue after some timeout (specified by MEAS_SCANMATE_PRO_SKIP_HANDSHAKE) */
  if(setjmp(env)) {
    alarm(0); /* cancel alarm */
    signal(SIGALRM, SIG_DFL); /* clear signal handler */
    fprintf(stderr, "meas_scanmate_pro_setwl: Lost dyelaser response.\n");
    return 0; /* if returning from alarm interrupt, return */
  }

  while(1) {
    meas_rs232_write(dye_fd[unit], "S?\r", 3);
    meas_rs232_readnl(dye_fd[unit], buf);
    if(buf[0] == 'R') {
      alarm(0); /* cancel alarm */
      signal(SIGALRM, SIG_DFL); /* clear signal handler */
      break;
    }
    meas_misc_nsleep(0, 20000000); /* sleep for 20 ms before recheck */
  }
  alarm(0);
  sleep(1); /* Just be sure that things have settled down */
  return 0;
}

/*
 * Get current wavelength.
 *
 * unit = Unit to be addressed.
 *
 * Returns wavelength in nm.
 *
 */

EXPORT double meas_scanmate_pro_getwl(int unit) {

  double wl;
  char buf[512];

  if(dye_fd[unit] == -1) meas_err("meas_scanmate_pro_getwl: Illegal unit.");

  meas_rs232_write(dye_fd[unit], "X?\r", 3);
  meas_rs232_readnl(dye_fd[unit], buf);
  sscanf(buf, "%le", &wl);

  return wl;
}

/*
 * Get the stepper motor status.
 *
 * unit = Unit number.
 * what = MEAS_SCANMATE_PRO_GRATING, MEAS_SCANMATE_PRO_ETALON, 
 *        MEAS_SCANMATE_PRO_SHG, MEAS_SCANMATE_PRO_SFG.
 *
 */

EXPORT int meas_scanmate_pro_getstp(int unit, int what) {

  char m, buf[512];
  int val;

  if(dye_fd[unit] == -1) 
    meas_err("meas_scanmate_pro_getstp: Illegal unit.");

  switch(what) {
  case MEAS_SCANMATE_PRO_GRATING:
    m = 'G';
    break;
  case MEAS_SCANMATE_PRO_ETALON:
    m = 'E';
    break;
  case MEAS_SCANMATE_PRO_SHG:
    m = 'S';
    break;
  case MEAS_SCANMATE_PRO_SFG:
    m = 'F';
    break;
  default:
    meas_err("meas_scanmate_pro_getstp: Unknown stepper motor.");
  }
  sprintf(buf, "POS,%c\r", m);
  meas_rs232_write(dye_fd[unit], buf, 6);
  meas_rs232_readnl(dye_fd[unit], buf);
  return atoi(buf);
}

/*
 * Set the stepper motors.
 *
 * unit  = Unit to be addressed.
 * what  = MEAS_SCANMATE_PRO_GRATING, MEAS_SCANMATE_PRO_ETALON, 
 *         MEAS_SCANMATE_PRO_SHG, MEAS_SCANMATE_PRO_SFG.
 * value = Stepper motor value.
 *
 * Note: With shg and sfg, remember to turn sync off.
 *
 */

EXPORT int meas_scanmate_pro_setstp(int unit, int what, unsigned int value) {

  char m, buf[512];
  int val;

  if(dye_fd[unit] == -1) 
    meas_err("meas_scanmate_pro_getstp: Illegal unit.");

  switch(what) {
  case MEAS_SCANMATE_PRO_GRATING:
    if(value > 1257146) meas_err("meas_scanmate_pro_getstp: Grating value larger than 1257146.");
    m = 'G';
    break;
  case MEAS_SCANMATE_PRO_ETALON:
    if(value > 60000) meas_err("meas_scanmate_pro_getstp: Etalon value larger than 60000.");
    m = 'E';
    break;
  case MEAS_SCANMATE_PRO_SHG:
    if(value > 60000) meas_err("meas_scanmate_pro_getstp: SHG value larger than 60000.");
    m = 'S';
    break;
  case MEAS_SCANMATE_PRO_SFG:
    if(value > 60000) meas_err("meas_scanmate_pro_getstp: SFG value larger than 60000.");
    m = 'F';
    meas_rs232_write(dye_fd[unit], "SFG=OFF\r", 8);
    break;
  default:
    meas_err("meas_scanmate_pro_getstp: Unknown stepper motor.");
  }
  sprintf(buf, "POS,%c=%u\r", m, value);
  meas_rs232_write(dye_fd[unit], buf, strlen(buf));
  return 0;
}

/*
 * Control autotracking for SFG.
 *
 * unit  = Unit to be addressed.
 * onoff = 1 = ON, 0 = OFF.
 *
 */

EXPORT int meas_scanmate_pro_sfg_sync(int unit, int onoff) {

  char *buf;

  if(dye_fd[unit] == -1) 
    meas_err("meas_scanmate_pro_sfg_sync: Illegal unit.");

  if(onoff) 
    buf = "SFG=ON\r";
  else
    buf = "SFG=OFF\r";

  meas_rs232_write(dye_fd[unit], buf, strlen(buf));
  return 0;
}

/* 
 * Control autotracking for SHG.
 * 
 * unit  = Unit to be addressed.
 * onoff = 1 = ON, 0 = OFF.
 *
 */

EXPORT int meas_scanmate_pro_shg_sync(int unit, int onoff) {

  char *buf;

  if(dye_fd[unit] == -1) 
    meas_err("meas_scanmate_pro_shg_sync: Illegal unit.");

  if(onoff) 
    buf = "SHG=ON\r";
  else
    buf = "SHG=OFF\r";

  meas_rs232_write(dye_fd[unit], buf, strlen(buf));
  return 0;
}
