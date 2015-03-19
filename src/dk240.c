/* filename: monochromator.c
 * 
 * SP DK240 monochromator control. For monochromator calibration, see the
 * manual.
 *
 * TODO: Add slit #3 support + slit calibration support.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "dk240.h"
#include "serial.h"
#include "misc.h"

static int dk240_fd[5] = {-1, -1, -1, -1, -1};

/*
 * Initialize monochromator & perform handshake with the instrument.
 *
 * unit = Unit to be addressed.
 * dev  = Device to be used.
 * 
 */

EXPORT int meas_dk240_open(int unit, char *dev) {

  char resp;

  if(dk240_fd[unit] == -1) {
    dk240_fd[unit] = meas_rs232_open(dev, 0);
    /* could do also a reset here ? - might be slow... */
  }
  meas_misc_disable_signals();
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_ECHO);
  meas_rs232_read(dk240_fd[unit], &resp, 1);
  if(resp != MEAS_DK240_ECHO) meas_err("meas_dk240: monochromator handshake failed.");
  meas_misc_enable_signals();

  return 0;
}

/*
 * Set the current wavelength.
 * 
 * unit  = Unit to be addressed.
 * wl    = Wavelength in nm.
 *
 */

EXPORT int meas_dk240_setwl(int unit, double wl) {

  unsigned char buf[3], tmp;
  unsigned int wll;

  if(dk240_fd[unit] == -1)
    meas_err("dk240: non-existent unit.");

  meas_misc_disable_signals();
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_GOTO);
  meas_rs232_read(dk240_fd[unit], buf, 1);
  if(buf[0] != MEAS_DK240_GOTO) 
    meas_err("meas_dk240: Unexpected response (GOTO).");

  wll = wl * 100;
  buf[0] = wll / 65536;
  wll = wll % 65535;
  buf[1] = wll / 256;
  buf[2] = wll % 256;
  meas_rs232_write(dk240_fd[unit], buf, 3);
  meas_rs232_read(dk240_fd[unit], &tmp, 1);
  /* status reply ignored for now */
  meas_rs232_read(dk240_fd[unit], &tmp, 1);
  if(tmp != MEAS_DK240_EOT)
    meas_err("dk240: Unexpected response (EOT).");
  meas_misc_enable_signals();

  /* TODO: how do we ensure that the monochromator has moved before we return? */
  return 0;
}

/*
 * Get current wavelength.
 *
 * unit  = Unit to be addressed.
 *
 * Returns wavelength in nm.
 *
 */

EXPORT double meas_dk240_getwl(int unit) {

  unsigned char buf[3];
  double wl;

  if(dk240_fd[unit] == -1)
    meas_err("meas_dk240: non-existent monochromator.");

  meas_misc_disable_signals();
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_WAVEQ);
  meas_rs232_read(dk240_fd[unit], buf, 1);
  if(buf[0] != MEAS_DK240_WAVEQ)
    meas_err("meas_dk240: error getting wavelength.");

  meas_rs232_read(dk240_fd[unit], buf, 3);  
  wl = ((double) (65536 * buf[0] + 256 * buf[1] + buf[2])) / 100.0;
  
  meas_rs232_read(dk240_fd[unit], buf, 1);
  /* ignore status for now */
  meas_rs232_read(dk240_fd[unit], buf, 1);
  if(buf[0] != MEAS_DK240_EOT)
    meas_err("meas_dk240: error getting wavelength.");
  meas_misc_enable_signals();

  return wl;
}

/* 
 * Set current input and output slits.
 * 
 * unit    = Unit to be addressed.
 * input   = Input slit setting (micrometers).
 * output  = Output slit setting (micrometers).
 *
 */

EXPORT int meas_dk240_set_slits(int unit, double input, double output) {

  unsigned char buf[2];

  if(dk240_fd[unit] == -1)
    meas_err("meas_dk240: non-existing monochromator.");
  
  meas_misc_disable_signals();
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_S1ADJ);
  meas_rs232_read(dk240_fd[unit], buf, 1);
  if(buf[0] != MEAS_DK240_S1ADJ)
    meas_err("meas_dk240: error setting slits.");
  buf[0] = ((int) input) / 256;
  buf[1] = ((int) input) % 256;
  meas_rs232_write(dk240_fd[unit], buf, 2);
  meas_rs232_read(dk240_fd[unit], buf, 1); /* status byte - ignored for now */
  meas_rs232_read(dk240_fd[unit], buf, 1); /* EOT */
  if(buf[0] != MEAS_DK240_EOT)
    meas_err("meas_dk240: error setting slits.");

  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_S2ADJ);
  meas_rs232_read(dk240_fd[unit], buf, 1);
  if(buf[0] != MEAS_DK240_S2ADJ)
    meas_err("meas_dk240: error setting slits.");
  buf[0] = ((int) output) / 256;
  buf[1] = ((int) output) % 256;
  meas_rs232_write(dk240_fd[unit], buf, 2);
  meas_rs232_read(dk240_fd[unit], buf, 1); /* status byte - ignored for now */
  meas_rs232_read(dk240_fd[unit], buf, 1); /* EOT */
  if(buf[0] != MEAS_DK240_EOT)
    meas_err("meas_dk240: error setting slits.");
  meas_misc_enable_signals();

  return 0;
}

/*
 * Get current input and output slit settings.
 *
 * unit    = Unit to be addressed.
 * input   = Input slit setting (to be returned; micrometers).
 * output  = Output slit setting (to be returned; micrometers).
 *
 */

EXPORT int meas_dk240_get_slits(int unit, double *input, double *output) {

  unsigned char buf[4];

  if(dk240_fd[unit] == -1)
    meas_err("meas_dk240: non-existent monochromator.");

  meas_misc_disable_signals();
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_SLITQ);
  meas_rs232_read(dk240_fd[unit], buf, 1);
  if(buf[0] != MEAS_DK240_SLITQ)
    meas_err("meas_dk240: error getting slits.");

  meas_rs232_read(dk240_fd[unit], buf, 4);
  *input = buf[0] * 256 + buf[1];
  *output = buf[2] * 256 + buf[3];

  meas_rs232_read(dk240_fd[unit], buf, 1); /* ignore status byte */

  meas_rs232_read(dk240_fd[unit], buf, 1);
  if(buf[0] != MEAS_DK240_EOT) 
    meas_err("meas_dk240: error getting slits.");;
  meas_misc_enable_signals();

  return 0;
}

/*
 * Polynomial wavelength calibration routine (linear interpolation).
 * This can be used in addition to the built-in monochromator wl calibration.
 *
 * real_wl  = Desired real wavelength (nm).
 *
 * Returns wavelength to be entered in the monochromator (nm).
 *
 * Note: this is obviously monochromator dependent and hardwired in the code.
 * TODO: modify interface so that the calibration params can be specified.
 * 
 */

EXPORT double meas_dk240_calib(double real_wl) {

  int i;
  double offset, x;

  if(real_wl < calib_x[0] || real_wl > calib_x[MEAS_DK240_NCAL-1])
    meas_err("meas_dk240: Wavelength outside the calibration range.");

  for (i = 0; i < MEAS_DK240_NCAL-1; i++)
    if(real_wl >= calib_x[i] && real_wl < calib_x[i+1]) break;

  x = (real_wl - calib_x[i]) / (calib_x[i+1] - calib_x[i]);
  offset = (1.0 - x) * calib_y[i] + x * calib_y[i+1];

  return real_wl - offset;
}

/* 
 * Return grating ID.
 *
 * ngratings   = number of gratings installed.
 * cur_grating = current grating #.
 * ruling      = current grating ruling (g/mm)
 * blaze       = blaze wavelength (nm)
 *
 */
 
EXPORT int meas_dk240_grating_info(int unit, int *ngratings, int *cur_grating, int *ruling, int *blaze) {

  unsigned char buf[5];

  if(dk240_fd[unit] == -1) meas_err("meas_dk240: non-existent unit.");

  meas_misc_disable_signals();
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_GRTID);
  meas_rs232_read(dk240_fd[unit], buf, 1);
  if(buf[0] != MEAS_DK240_GRTID) 
    meas_err("meas_dk240: Unexpected response (GRTID).");
  meas_rs232_read(dk240_fd[unit], buf, 1);
  *ngratings = (int) buf[0];
  meas_rs232_read(dk240_fd[unit], buf, 1);
  *cur_grating = (int) buf[0];
  meas_rs232_read(dk240_fd[unit], buf, 2);
  *ruling = (int) (256 * buf[0] + buf[1]);
  meas_rs232_read(dk240_fd[unit], buf, 2);
  *blaze = (int) (256 * buf[0] + buf[1]);
  meas_rs232_read(dk240_fd[unit], buf, 2);
  meas_rs232_read(dk240_fd[unit], buf, 1); /* ignore status byte */
  meas_rs232_read(dk240_fd[unit], buf, 1); /* EOT */
  if(buf[0] != MEAS_DK240_EOT)
    meas_err("meas_dk240: error reading grating info.");
  meas_misc_enable_signals();

  return 0;
}

/*
 * Select grating.
 *
 * unit    = Unit to be addressed.
 * grating = Grating to be selected (1 - 3).
 *
 */

EXPORT int meas_dk240_grating_select(int unit, int grating) {

  unsigned char buf[5];

  if(dk240_fd[unit] == -1) 
    meas_err("meas_dk240: non-existent unit.");
  if(grating < 1 || grating > 3)
    meas_err("meas_dk240: invalid grating number.");
  
  meas_misc_disable_signals();
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_GRTSEL);
  meas_rs232_read(dk240_fd[unit], buf, 1);
  if(buf[0] != MEAS_DK240_GRTSEL)
    meas_err("meas_dk240: Unexpected response (GRTSEL).");
  meas_rs232_writeb(dk240_fd[unit], (unsigned char) grating);
  meas_rs232_read(dk240_fd[unit], buf, 1); /* ignore status byte */
  meas_rs232_read(dk240_fd[unit], buf, 1); /* EOT */
  if(buf[0] != MEAS_DK240_EOT)
    meas_err("meas_dk240: error selecting grating.");
  meas_misc_enable_signals();

  return 0;
}

/*
 * Reset monochromator.
 *
 * unit = Unit to be reset.
 *
 */

EXPORT int meas_dk240_reset(int unit) {

  if(dk240_fd[unit] == -1)
    meas_err("meas_dk240: non-existent unit.");
  
  meas_misc_disable_signals();
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_RESET);
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_RESET);
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_RESET);
  meas_misc_enable_signals();
  return 0;
}

/*
 * Get monochromator serial.
 *
 * unit  = Unit for the operation.
 *
 * Returns the serial number.
 *
 */

EXPORT int meas_dk240_serial(int unit) {

  unsigned char buf[5];

  if(dk240_fd[unit] == -1)
    meas_err("meas_dk240: non-existent unit.");
  
  meas_misc_disable_signals();
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_SERIAL);
  meas_rs232_read(dk240_fd[unit], buf, 1);
  if(buf[0] != MEAS_DK240_SERIAL)
    meas_err("meas_dk240: Unexpected response (SERIAL).");
  meas_rs232_read(dk240_fd[unit], buf, 5);
  meas_misc_enable_signals();

  buf[5] = 0;
  return atoi(buf);
}

/*
 * Clear the calibration values and restore the factory settings for
 * *BOTH* grating and slits.
 *
 * Don't use this unless you know what you are doing.
 * 
 * unit  = Unit to be addressed.
 * 
 */

EXPORT int meas_dk240_nvram_clear(int unit) {

  unsigned char buf[5];

  if(dk240_fd[unit] == -1)
    meas_err("meas_dk240: non-existent unit.");

  meas_misc_disable_signals();
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_CLEAR);
  meas_rs232_read(dk240_fd[unit], buf, 1);
  if(buf[0] != MEAS_DK240_CLEAR)
    meas_err("meas_dk240: Unexpected response (CLEAR).");
  meas_rs232_read(dk240_fd[unit], buf, 1); /* ignore status byte */
  meas_rs232_read(dk240_fd[unit], buf, 1); /* EOT */
  if(buf[0] != MEAS_DK240_EOT)
    meas_err("meas_dk240: error issuing clear.");
  meas_misc_enable_signals();

  return 0;
}

/*
 * Perform the 1st step in calibration: set to 0th order and call this 
 * routine. The next step is to proceed with meas_dk240_grating_calibrate().
 *
 * unit   = Unit to be addressed.
 *
 */

EXPORT int meas_dk240_grating_zero(int unit) {

  unsigned char buf[5];

  if(dk240_fd[unit] == -1)
    meas_err("meas_dk240: non-existent unit.");

  meas_misc_disable_signals();
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_ZERO);
  meas_rs232_read(dk240_fd[unit], buf, 1);
  if(buf[0] != MEAS_DK240_ZERO)
    meas_err("meas_dk240: Unexpected response (ZERO).");
  meas_rs232_writeb(dk240_fd[unit], 1); /* TODO: for DK242 1 or 2 */
  meas_rs232_read(dk240_fd[unit], buf, 1); /* ignore status byte */
  meas_rs232_read(dk240_fd[unit], buf, 1); /* EOT */
  if(buf[0] != MEAS_DK240_EOT)
    meas_err("meas_dk240: error issuing zero.");
  meas_misc_enable_signals();
  return 0;


}

/*
 * Perform the 2nd step in calibration: Park the monochromator at the position of
 * a known spectral line and call this routine with the known wavelength value.
 * (note meas_dk240_zero() must be done first). This will change the nvram
 * settings of the monochromator.
 *
 * Don't use this unless you know what you are doing.
 *
 * unit  = Unit to be addressed.
 * wl    = Wavelength.
 * 
 */

EXPORT int meas_dk240_grating_calibrate(int unit, double wl) {

  unsigned char buf[5];
  unsigned int wll;

  if(dk240_fd[unit] == -1)
    meas_err("meas_dk240: non-existent unit.");

  meas_misc_disable_signals();
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_GCAL);
  meas_rs232_read(dk240_fd[unit], buf, 1);
  if(buf[0] != MEAS_DK240_GCAL)
    meas_err("meas_dk240: Unexpected response (GCAL).");
  wll = wl * 100;
  buf[0] = wll / 65536;
  wll = wll % 65535;
  buf[1] = wll / 256;
  buf[2] = wll % 256;
  meas_rs232_write(dk240_fd[unit], buf, 3);
  meas_rs232_read(dk240_fd[unit], buf, 1); /* ignore status byte */
  meas_rs232_read(dk240_fd[unit], buf, 1); /* EOT */
  if(buf[0] != MEAS_DK240_EOT)
    meas_err("meas_dk240: error issuing grating calibrate.");
  meas_misc_enable_signals();

  return 0;  
}

/*
 * Start slew up/down until grating limit reached or the interrupt is received.
 * Initiate by calling this routine and stop by calling meas_dk240_slew_stop().
 * Note that the stop routine MUST be called after this and that the stop rotuine
 * cannot be called before start.
 *
 * unit      = Unit to be addressed.
 * direction = 1 = up, 0 = down.
 *
 */

static int slew_active[5] = {0, 0, 0, 0, 0};

EXPORT int meas_dk240_slew_start(int unit, int direction) {

  unsigned char buf[5];

  if(dk240_fd[unit] == -1) 
    meas_err("meas_dk240: non-existent unit.");

  meas_misc_disable_signals();
  if(slew_active[unit])
    meas_err("meas_dk240: Can't initiate slew when already active.");

  meas_rs232_writeb(dk240_fd[unit], direction?MEAS_DK240_SLEWUP:MEAS_DK240_SLEWDOWN);
  meas_rs232_read(dk240_fd[unit], buf, 1);
  if(direction == 1 && buf[0] != MEAS_DK240_SLEWUP)
    meas_err("meas_dk240: Unexpected response (SLEWUP).");
  if(direction == 0 && buf[0] != MEAS_DK240_SLEWDOWN)
    meas_err("meas_dk240: Unexpected response (SLEWDOWN).");
  slew_active[unit] = 1;
  meas_misc_enable_signals();

  return 0;
}

/*
 * Stop slew up/down. This must only be used in combination with meas_dk240_slew_start().
 *
 * unit = Unit to be addressed.
 *
 */

EXPORT int meas_dk240_slew_stop(int unit) {
  
  unsigned char buf[5];

  if(dk240_fd[unit] == -1)
    meas_err("meas_dk240: non-existent unit.");

  meas_misc_disable_signals();
  if(!slew_active[unit])
    meas_err("meas_dk240: Can't stop slew when not active.");
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_EOT);
  meas_rs232_read(dk240_fd[unit], buf, 1); /* ignore status byte */
  meas_rs232_read(dk240_fd[unit], buf, 1); /* EOT */
  if(buf[0] != MEAS_DK240_EOT)
    meas_err("meas_dk240: error issuing slew stop.");
  slew_active[unit] = 0;
  meas_misc_enable_signals();

  return 0;
}

/*
 * Move grating down (toward UV) by one step.
 *
 * unit = Unit to be addressed.
 *
 */

EXPORT int meas_dk240_step_down(int unit) {

  unsigned char buf[5];
  
  if(dk240_fd[unit] == -1)
    meas_err("meas_dk240: non-existent unit.");

  meas_misc_disable_signals();
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_STEPDOWN);
  meas_rs232_read(dk240_fd[unit], buf, 1);
  if(buf[0] != MEAS_DK240_STEPDOWN)
    meas_err("meas_dk240: Unexpected response (STEPDOWN).");
  meas_rs232_read(dk240_fd[unit], buf, 1); /* ignore status byte */
  meas_rs232_read(dk240_fd[unit], buf, 1); /* EOT */
  if(buf[0] != MEAS_DK240_EOT)
    meas_err("meas_dk240: error issuing slew stop.");
  meas_misc_enable_signals();

  return 0;
}

/*
 * Move grating up (toward IR) by one step.
 *
 * unit = Unit to be addressed.
 *
 */

EXPORT int meas_dk240_step_up(int unit) {

  unsigned char buf[5];
  
  if(dk240_fd[unit] == -1)
    meas_err("meas_dk240: non-existent unit.");

  meas_misc_disable_signals();
  meas_rs232_writeb(dk240_fd[unit], MEAS_DK240_STEPUP);
  meas_rs232_read(dk240_fd[unit], buf, 1);
  if(buf[0] != MEAS_DK240_STEPUP) 
    meas_err("meas_dk240: Unexpected response (STEPUP).");
  meas_rs232_read(dk240_fd[unit], buf, 1); /* ignore status byte */
  meas_rs232_read(dk240_fd[unit], buf, 1); /* EOT */
  if(buf[0] != MEAS_DK240_EOT) 
    meas_err("meas_dk240: error issuing slew stop.");
  meas_misc_enable_signals();

  return 0;
}
