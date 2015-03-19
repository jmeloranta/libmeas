/*
 * Driver for Lambda Physik FL3001/3002 dye laser.
 *
 * NOTE: Uses old style communication. Does not appear to work if there
 * are any new devices on the same bus...
 *
 */

#ifdef GPIB

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "fl3000.h"
#include "gpib.h"
#include "misc.h"

/* Up to 5 units supported */
static int fl3000_board[5] = {-1, -1, -1, -1, -1};
static int fl3000_dev[5] = {-1, -1, -1, -1, -1};

unsigned int meas_etalon_zero = 0;
double meas_etalon_zero_wl = 0.0;

double meas_etalon_wl_scale1 = 0.0;
double meas_etalon_wl_scale2 = 0.0;
double meas_etalon_wl_p1 = 0.0;
double meas_etalon_wl_p2 = 0.0;
double meas_etalon_wl_p3 = 0.0;

static double lambda_0;
static int grating_order = 5;

static void unparse(unsigned int val, char *buf, int size) {

  int i;
  unsigned int pwr;

  pwr = 1;
  for (i = 1; i < size; i++)
    pwr *= 16;

  for (i = 0; i < size; i++) {
    buf[size-i-1] = ((char) (val / pwr)) + 'A';
    val = val % pwr;
    pwr /= 16;
  }
}

static unsigned int parse(char *str, int len) {

  int i;
  unsigned int val, pwr;
  char tmp;
  char fmt[128];

  for (i = 0; i < len; i++)
    str[i] -= 'A';

  val = 0;
  pwr = 1;
  for (i = 0; i < len; i++) {
    val += str[i] * pwr;
    pwr *= 16;
  }

  return val;
}

/*
 * Initialize FL3000.
 * 
 * unit  = Unit number.
 * board = GPIB board number.
 * dev   = GPIB number.
 *
 */

EXPORT int meas_fl3000_open(int unit, int board, int dev) {

  char buf[512];
  
  if(fl3000_board[unit] == -1) {
    (void) meas_gpib_open(board, dev); /* device fd not needed - hence void... */
    fl3000_board[unit] = board;
    fl3000_dev[unit] = dev;
    meas_gpib_old_write(board, dev, "?G\r", 3);
    meas_gpib_old_read(board, dev, buf, 7);
    
    lambda_0 = ((double) parse(buf, 6)) / 1000.0;
    fprintf(stderr, "meas_fl3000: Wavelength calibration constant = %lf nm\n", lambda_0);
  }
  return 0;
}

/* Given the real wavelength, this returns the required wl for the dye laser */
static double calib(double realwl) {

  return MEAS_FL3000_K * realwl + MEAS_FL3000_B;
}

/* 
 * Set wavelength.
 *
 * unit = Unit to be addressed.
 * wl   = Wavelength in nm.
 *
 * Note: With etalon, one should scan down
 *
 */

EXPORT int meas_fl3000_setwl(int unit, double wl) {

  char buf[128];
  unsigned int grating, etalon, crystal, reserve = 0, cur_etalon;
  double cur_wl, dl, orig_wl, tmp, prev_wl;
  double meas_fl3000_getwl(int);
  unsigned meas_fl3000_getetalon(int);

  if(fl3000_board[unit] == -1)
    meas_err("meas_fl3000_setwl: non-existing unit.");

  /* disable ctrl-c */
  meas_misc_disable_signals();

  /* Calibration */
  orig_wl = wl;
  wl = calib(wl);

  /* Set grating */
  grating = (unsigned int) ((wl * (double) grating_order - lambda_0) / MEAS_FL3000_GRATING_CONST);
  /*  printf("order = %d, stepper = %u\n", grating_order, grating); */
  if(grating < 0 || grating > 285715)
    meas_err("Grating stepper out of range.");
  
  /* Set Etalon (still a bit messy) */
#if 0
  dl = orig_wl - meas_etalon_zero_wl;
  etalon = (unsigned) (0.5 + sqrt(-2.0 * dl * MEAS_FL3000_REFIND * MEAS_FL3000_REFIND / (meas_etalon_zero_wl * MEAS_FL3000_ETALON_STEP * MEAS_FL3000_ETALON_STEP)));
  printf("change = %u, %le\n", etalon, (0.5 + sqrt(-2.0 * dl * MEAS_FL3000_REFIND * MEAS_FL3000_REFIND / (meas_etalon_zero_wl * MEAS_FL3000_ETALON_STEP * MEAS_FL3000_ETALON_STEP))));
  etalon = meas_etalon_zero + etalon;
  if(etalon > MEAS_FL3000_ETALON_MAX) {
    fprintf(stderr, "meas_fl3000_setwl: Etalon stepper too large.\n");
    etalon = meas_etalon_zero;
  }
  etalon = MEAS_FL3000_ETALON_MAX - etalon;
#else
  /* hack attack */
  tmp = wl - meas_etalon_wl_scale1;
  tmp = (meas_etalon_wl_p1 + meas_etalon_wl_p2 * tmp + meas_etalon_wl_p3 * tmp * tmp) 
    * meas_etalon_wl_scale2;
  /*   tmp = wl - 452.21;
       tmp = (3.3061 - 55.827 * tmp + 110.72 * tmp * tmp) * 1000.0; */
  etalon = (unsigned) tmp;
  fprintf(stderr,"meas_fl3000_setwl: wl = %le, etalon = %u\n", wl, etalon);
#endif
  
  fprintf(stderr, "meas_fl3000_setwl: Etalon stepper = %u\n", etalon);

  /* Set crystal drive */
  /* TODO: Not implemented yet (see around p. 150 in the manual) */
  crystal = 0;

  buf[0] = 'S';
  buf[1] = 'A';
  unparse(grating, buf + 2, 6);
  unparse(etalon, buf + 8, 4);
  unparse(crystal, buf + 12, 4);
  unparse(reserve, buf + 16, 4);
  buf[20] = '\r';
  meas_gpib_old_write(fl3000_board[unit], fl3000_dev[unit], buf, 21);
  /* wait for wavelength to stabilize */
  while (1) {
    double xx;
    prev_wl = cur_wl;
    cur_wl = meas_fl3000_getwl(unit);
    cur_etalon = meas_fl3000_getetalon(unit);
    fprintf(stderr, "meas_fl3000_setwl: cur = %.8le nm, dest = %.8le nm\n", cur_wl, wl);
    fprintf(stderr, "meas_fl3000_setwl: cur_etalon = %u, dest_etalon = %u\n", cur_etalon, etalon);
    if(fabs(cur_wl - prev_wl) <= 1E-8 && etalon == cur_etalon) {
      fprintf(stderr, "meas_fl3000_setwl: Difference = %le nm\n", fabs(cur_wl - wl));
      break;
    }
    sleep(1);
    meas_gpib_old_write(fl3000_board[unit], fl3000_dev[unit], buf, 21);
  }
  /* enable ctrl-c */
  meas_misc_enable_signals();
  return 0;
}

/*
 * Get current wavelength.
 *
 * unit = Unit to be addressed.
 *
 * Returns current wavelength in nm.
 *
 */

EXPORT double meas_fl3000_getwl(int unit) {

  char buf[128];
  double wl;
  int grating, etalon, crystal, reserve;

  if(fl3000_board[unit] == -1)
    meas_err("meas_fl3000_getwl: non-existing unit.");

  /* disable ctrl-c */
  meas_misc_disable_signals();

  meas_gpib_old_write(fl3000_board[unit], fl3000_dev[unit], "?A\r", 3);
  meas_gpib_old_read(fl3000_board[unit], fl3000_dev[unit], buf, 19);

  grating = parse(buf, 6);
  etalon = parse(buf+6, 4) - meas_etalon_zero;
  crystal = parse(buf+6+4, 4);
  reserve = parse(buf+6+4+4, 4);

  wl = (((double) grating) * MEAS_FL3000_GRATING_CONST + lambda_0) / (double) grating_order;

  /* enable ctrl-c */
  meas_misc_enable_signals();

  return wl;
}

/*
 * Return absolute position of etalon.
 *
 * unit = Unit to be addressed.
 *
 */

EXPORT unsigned meas_fl3000_getetalon(int unit) {

  char buf[128];
  double wl;
  unsigned grating, etalon, crystal, reserve;

  /* disable ctrl-c */
  meas_misc_disable_signals();

  meas_gpib_old_write(fl3000_board[unit], fl3000_dev[unit], "?A\r", 3);
  meas_gpib_old_read(fl3000_board[unit], fl3000_dev[unit], buf, 19);

  grating = parse(buf, 6);
  etalon = parse(buf+6, 4);
  crystal = parse(buf+6+4, 4);
  reserve = parse(buf+6+4+4, 4);

  /* enable ctrl-c */
  meas_misc_enable_signals();

  return etalon;
}

/*
 * Set grating order.
 *
 * unit = Unit to be addressed.
 * x    = Order.
 *
 */

EXPORT int meas_fl3000_grating(int unit, int x) {

  if(x < 0 || x > 8) meas_err("meas_fl3000_grating: Invalid grating order.");
  grating_order = x;
  return 0;
}

#endif /* GPIB */
