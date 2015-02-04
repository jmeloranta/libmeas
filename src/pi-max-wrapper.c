#include "config.h"

#ifdef PVCAM
/* This must be taken from the pvcam distribution and placed in /usr/include */
#include <master.h>
#include <string.h>
#include <pvcam.h>

/*
 * Wrapper for PI-MAX gated CCD camera. Requires PVCAM 2.7.1.7.
 * pvcam.h in the examples directory must be placed in /usr/include
 *
 * This provides a small subset of commands and simplified usage.
 * (gated spectroscopy mode)
 *
 * Note that timing is provided by external delay gens rather than the built-in
 * timer unit (I can't find any docs how to program it).
 *
 * TODO: support multiple cameras.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "pi-max-wrapper.h"
#include "misc.h"

/* Only one camera / machine supported (TODO) */

static char cam_name[CAM_NAME_LEN];
static int16 hCam;
static rgn_type region;

/* 
 * Return number of points in spectrum (after binning).
 *
 */

EXPORT int meas_pi_max_size() {

#if 0
  unsigned short x;

  if(!pl_ccd_get_ser_size(hCam, &x))
    meas_err("Error in pi_max_size()");

  return (int) x;
#endif

  return (int) ((region.s2 - region.s1 + 1) * (region.p2 - region.p1 + 1) / (region.sbin * region.pbin));
}

/*
 * Set CCD element temperature.
 *
 * temp = Temperature in oC.
 *
 */

EXPORT int meas_pi_max_temperature(double temp) {

  int16 value = (int16) (temp * 100.0), cvalue;
  int first = 1;

  /* Wait until temperature reached */
  while(1) {
    if(!pl_get_param(hCam, PARAM_TEMP, ATTR_CURRENT, (void *) &cvalue))
      meas_err("meas_pi_max_temperature: Error in setting temperature.");
    if(cvalue > value) fprintf(stderr, "meas_pi_max_temperature: Current T = %le C, requested T = %le C.\n", cvalue / 100.0, value / 100.0);
    else break;
    if(first) {
      if(!pl_set_param(hCam, PARAM_TEMP_SETPOINT, (void *) &value))
	meas_err("meas_pi_max_temperature: Error in setting temperature.");
      first = 0;
    }
    sleep(2);
  }
  fprintf(stderr, "meas_pi_max_temperature: Temperature set point (%le) reached.\n", temp);
  return 0;
}

/*
 * Return CCD element temperature.
 *
 */

EXPORT double meas_pi_max_get_temperature() {

  int16 cvalue;

  if(!pl_get_param(hCam, PARAM_TEMP, ATTR_CURRENT, (void *) &cvalue))
    meas_err("meas_pi_max_get_temperature: Error reading temperature.");
  return cvalue / 100.0;
}

/*
 * Set gain index.
 *
 * m = gain index (0, 1, ...). Values are camera dependent.
 *
 */

EXPORT int meas_pi_max_gain_index(int m) {

  if(!pl_set_param(hCam, PARAM_GAIN_INDEX, (void *) &m))
    meas_err("meas_pi_max_gain_index: Error setting gain index.");
  return 0;
}

/*
 * Set camera speed index.
 *
 * m = speed index (0, 1, ...). Values are camera dependent.
 *
 */

EXPORT int meas_pi_max_speed_index(int m) {

  if(!pl_set_param(hCam, PARAM_SPDTAB_INDEX, (void *) &m))
    meas_err("meas_pi_max_speed_index: Error setting camera speed index.");
  return 0;
}

/*
 * Set edge camera trigger.
 *
 * m = 0 (rising edge), 1 (falling edge).
 *
 */

EXPORT int meas_pi_max_trigger_mode(int m) {

  switch(m) {
  case 0:
    m = EDGE_TRIG_POS;
    break;
  case 1:
    m = EDGE_TRIG_NEG;
    break;
  default:
    meas_err("meas_pi_max_trigger_mode: Invalid trigger mode.\n");
  }
  if(!pl_set_param(hCam, PARAM_EDGE_TRIGGER, (void *) &m))
    meas_err("meas_pi_max_trigger_mode: Error setting camera trigger mode.");
  return 0;
}

/*
 * CCD element clear mode.
 *
 * 1 = Never clear.
 * 2 = Clear pre-exposure.
 * 3 = Clear pre-sequence.
 * 4 = Clear post-sequence.
 * 5 = Clear pre-post sequence
 * 6 = Clear pre-exposure post-sequence.
 *
 */

EXPORT int meas_pi_max_clear_mode(int m) {

  enum {CLEAR_NEVER, CLEAR_PRE_EXPOSURE, CLEAR_PRE_SEQUENCE, 
	CLEAR_POST_SEQUENCE, CLEAR_PRE_POST_SEQUENCE, 
	CLEAR_PRE_EXPOSURE_POST_SEQ} mode;
  
  switch(m) {
  case 1:
    mode = CLEAR_NEVER;
    break;
  case 2:
    mode = CLEAR_PRE_EXPOSURE;
    break;
  case 3:
    mode = CLEAR_PRE_SEQUENCE;
    break;
  case 4:
    mode = CLEAR_POST_SEQUENCE;
    break;
  case 5:
    mode = CLEAR_PRE_POST_SEQUENCE;
    break;
  case 6:
    mode = CLEAR_PRE_EXPOSURE_POST_SEQ;
    break;
  default:
    meas_err("meas_pi_max_clear_mode: Unknonwn CCD clear mode.");
  }
  
  if(!pl_set_param(hCam, PARAM_CLEAR_MODE, (void *) &mode))
    meas_err("meas_pi_max_shutter_open_mode: Error setting shutter mode.");
  return 0;
}

/*
 * CCD Camera shutter mode.
 *
 * m = Shutter mode as:
 *     1 = Never open;
 *     2 = Open pre-exposure;
 *     3 = Open pre-sequence;
 *     4 = Open pre-trigger;
 *     5 = No change.
 *
 */

EXPORT int meas_pi_max_shutter_open_mode(int m) {

  enum { OPEN_NEVER, OPEN_PRE_EXPOSURE, OPEN_PRE_SEQUENCE, OPEN_PRE_TRIGGER, OPEN_NO_CHANGE} mode;
  
  switch(m) {
  case 1:
    mode = OPEN_NEVER;
    break;
  case 2:
    mode = OPEN_PRE_EXPOSURE;
    break;
  case 3:
    mode = OPEN_PRE_SEQUENCE;
    break;
  case 4:
    mode = OPEN_PRE_TRIGGER;
    break;
  case 5:
    mode = OPEN_NO_CHANGE;
    break;
  default:
    meas_err("meas_pi_max_shutter_open_mode: Unknonwn shutter mode.");
  }
  
  if(!pl_set_param(hCam, PARAM_SHTR_OPEN_MODE, (void *) &mode))
    meas_err("meas_pi_max_shutter_open_mode: Error setting shutter mode.");
  return 0;
}

/*
 * Set CCD gate mode.
 *
 * m = Set as follows:
 *     1 = safe mode, 2 = regular gate mode, 3 = shutter mode (*).
 *
 * (*) Warning: Intensifier on all the time!!!
 *
 */

EXPORT int meas_pi_max_gate_mode(int m) {

  enum {INTENSIFIER_SAFE = 0, INTENSIFIER_GATING, INTENSIFIER_SHUTTER } mode;

  switch(m) {
  case 1:
    mode = INTENSIFIER_SAFE;
    break;
  case 2:
    mode = INTENSIFIER_GATING;
    break;
  case 3:
    mode = INTENSIFIER_SHUTTER; /* Warning: Photocathode biased on for the whole exposure time!!! */
    break;
  default:
    meas_err("meas_pi_max_gate_mode: Invalid gate mode.");
  }
  if(!pl_set_param(hCam, PARAM_SHTR_GATE_MODE, (void *) &mode))
    meas_err("meas_pi_max_gate_mode: Error setting gate mode.");
  return 0;
}

/*
 * CCD exposure mode.
 *
 * mode = Set as follows:
 *        1 = Timed mode;
 *        2 = Strobed mode;
 *        3 = Bulb mode;
 *        4 = Trigger first mode;
 *        5 = Variable timed mode;
 *        6 = Int. Strobe mode.
 *
 * See pvcam documentation for info on these modes.
 *
 */

EXPORT int meas_pi_max_exposure_mode(int mode) {

  enum {
    TIMED_MODE,
    STROBED_MODE,
    BULB_MODE,
    TRIGGER_FIRST_MODE,
    FLASH_MODE,
    VARIABLE_TIMED_MODE,
    INT_STROBE_MODE
  } value;

  switch (mode) {
  case 1:
    value = TIMED_MODE;
    break;
  case 2:
    value = STROBED_MODE;
    break;
  case 3:
    value = BULB_MODE;
    break;
  case 4:
    value = TRIGGER_FIRST_MODE;
    break;
  case 5:
    value = VARIABLE_TIMED_MODE;
    break;
  case 6:
    value = INT_STROBE_MODE;
    break;
  defalt:
    meas_err("meas_pi_max_exposure_mode: Invalid exposure mode.");
  }
  if(!pl_set_param(hCam, PARAM_EXPOSURE_MODE, (void *) &value))
    meas_err("meas_pi_max_exposure_mode: Error setting exposure mode.");
  return 0;
}

/*
 * Set CCD "pmode".
 *
 * mode  = Pmode as:
 *         1 = Normal.
 * Other modes are not currently in use.
 *
 */

EXPORT int meas_pi_max_pmode(int mode) {

  enum {PMODE_NORMAL,
	      PMODE_FT,
	      PMODE_MPP,
	      PMODE_FT_MPP,
	      PMODE_ALT_NORMAL,
	      PMODE_ALT_FT,
	      PMODE_ALT_MPP,
	      PMODE_ALT_FT_MPP,
	      PMODE_INTERLINE,
	      PMODE_KINETICS,
	      PMODE_DIF} value;
    
  switch (mode) {
  case 1:
    value = PMODE_NORMAL;
    break;
  default:
    meas_err("meas_pi_max_pmode: Unsupported pmode.");
  }
  if(!pl_set_param(hCam, PARAM_PMODE, (void *) &value))
    meas_err("meas_pi_max_pmode: Error setting pmode.");    
  return 0;
}

/*
 * Set intensifier gain.
 *
 * gain = Gain setting (0 - 255).
 *
 */

EXPORT int meas_pi_max_gain(int gain) {

  int16 value = gain;

  if(!pl_set_param(hCam, PARAM_INTENSIFIER_GAIN, (void *) &value))
    meas_err("meas_pi_max_gain: Error setting gain.");
  return 0;
}

/*
 * Specify region of interest (ROI).
 *
 * s1   = Serial begin.
 * s2   = Serial end.
 * sbin = Serial bin.
 * p1   = Parallel begin.
 * p2   = Parallel end.
 * pbin = Parallel bin.
 *
 */

EXPORT int meas_pi_max_roi(int s1, int s2, int sbin, int p1, int p2, int pbin) {

  uns16 x, y;

  pl_get_param(hCam, PARAM_SER_SIZE, ATTR_DEFAULT, (void *) &x);
  pl_get_param(hCam, PARAM_PAR_SIZE, ATTR_DEFAULT, (void *) &y);
  if(s1 < 0 || s1 > x || s2 < 0 || s2 > x || sbin < 0 || sbin > x ||
     p1 < 0 || p1 > y || p2 < 0 || p2 > y || pbin < 0 || pbin > y)
    meas_err("meas_pi_max_roi: Invalid ROI or binning");
  region.s1 = s1;
  region.s2 = s2;
  region.sbin = sbin;
  region.p1 = p1;
  region.p2 = p2;
  region.pbin = pbin;
  return 0;
}

/*
 * Initialize the camera.
 *
 * temperature = Operating temperature for the CCD (oC).
 *
 */

EXPORT int meas_pi_max_init(double temperature) {

  uns16 x, y;

  pl_pvcam_init();
  pl_cam_get_name(0, cam_name);
  pl_cam_open(cam_name, &hCam, OPEN_EXCLUSIVE);
  if(pl_error_code() != 0)
    meas_err("meas_pi_max_init: Camera not found.");
  pl_get_param(hCam, PARAM_SER_SIZE, ATTR_DEFAULT, (void *) &x);
  pl_get_param(hCam, PARAM_PAR_SIZE, ATTR_DEFAULT, (void *) &y);
  fprintf(stderr, "meas_pi_max_init: CCD element size = %dx%d\n", x, y);
  region.s1 = 0; /* Start from pixel 0 (serial = x) */
  region.s2 = x-1; /* End to max pixel */
  region.sbin = 1; /* x binning (none) */
  region.p1 = 0; /* Start from pixel 0 (parallel = y) */
  region.p2 = y-1; /* End to max pixel */
  region.pbin = y; /* Bin everything along y */
  /* Wait for external trigger before grabbing a frame(4) */
  /* TODO: Does not work */
  /* meas_pi_max_exposure_mode(6); */

  /* Set default gain to intensifier (default off) */
  if(meas_pi_max_gain(0)) return -1;
  /* normal mode(1) */
  if(meas_pi_max_pmode(1)) return -1;
  /* Shutter mode - defaults to OPEN_PRE_EXPOSURE */
  if(meas_pi_max_shutter_open_mode(2)) return -1;

  /* Gated operation(2) */
  if(meas_pi_max_gate_mode(2)) return -1;
  /* Cool the CCD element */
  if(meas_pi_max_temperature(temperature)) return -1;

  return 0;
}

/*
 * Read CCD element.
 *
 * ave = Number of averages to take (if ave < 0; use abs(ave) with internal
 *       CCD accumulation).
 * dst = Destination buffer for data (unsigned short *).
 *
 */

EXPORT int meas_pi_max_read(int ave, unsigned short *dst) {

/* Binning: wavelength along x, average along y (one pixel width) */
/* TODO: Additional params required: gate gain, gate mode */

  static uns16 *data = NULL;
  int16 exp_time = 5; /* ms (seems to work...) */
  uns32 size;
  int16 status;
  uns32 dummy;
  int i, j;

  pl_exp_init_seq();
  fprintf(stderr, "meas_pi_max_read: ROI = %d %d %d / %d %d %d\n", region.s1, region.s2, region.sbin, region.p1, region.p2, region.pbin);

  if(pl_exp_setup_seq(hCam, (ave<0)?abs(ave):1, 1, &region, STROBED_MODE, exp_time, &size)) {
    fprintf(stderr, "meas_pi_max_read: Frame size = %d\n", size);
  } else meas_err("meas_pi_max_read: Experiment failed.");

  if(!data) {
    if(!(data = (uns16 *) malloc(2*size)))
      meas_err("meas_pi_max_read: Memory allocation failure.");
  }

  if(dst) {
    for (i = 0; i < size/2; i++)
      dst[i] = 0;
  }

  if (ave < 0) ave = 1;
  for (i = 0; i < ave; i++) {
    pl_exp_start_seq(hCam, data);

    while(pl_exp_check_status(hCam, &status, &dummy) &&
	  (status != READOUT_COMPLETE && status != READOUT_FAILED));
    if(status == READOUT_FAILED)
      meas_err("meas_pi_max_read: Error reading CCD.");
    pl_exp_finish_seq(hCam, data, 0);

    if(dst) {
      for (j = 0; j < size/2; j++)
	dst[j] += data[j];
    }
  }
  pl_exp_uninit_seq();
  return 0;
}

/*
 * Read CCD element.
 *
 * ave = Number of averages to take (if ave < 0; use abs(ave) with internal
 *       CCD accumulation).
 * dst = Destination buffer for data (double *).
 *
 * Just another version for double arrays (backwards compatibility).
 *
 */

EXPORT int meas_pi_max_read2(int ave, double *dst) {

/* Binning: wavelength along x, average along y (one pixel width) */
/* TODO: Additional params required: gate gain, gate mode */

  static uns16 *data = NULL;
  int16 exp_time = 100; /* ms (seems to work...) */
  uns32 size;
  int16 status;
  uns32 dummy;
  int i, j;

  pl_exp_init_seq();
  fprintf(stderr, "meas_pi_max_read: ROI = %d %d %d / %d %d %d\n", region.s1, region.s2, region.sbin, region.p1, region.p2, region.pbin);

  if(pl_exp_setup_seq(hCam, (ave<0)?abs(ave):1, 1, &region, STROBED_MODE, exp_time, &size)) {
    fprintf(stderr, "meas_pi_max_read: Frame size = %d\n", size);
  } else meas_err("meas_pi_max_read: Experiment failed.");

  if(!data) {
    if(!(data = (uns16 *) malloc(2*size)))
      meas_err("meas_pi_max_read: Memory allocation failure.");
  }

  if(dst) {
    for (i = 0; i < size/2; i++)
      dst[i] = 0.0;
  }

  if (ave < 0) ave = 1;
  for (i = 0; i < ave; i++) {
    pl_exp_start_seq(hCam, data);

    while(pl_exp_check_status(hCam, &status, &dummy) &&
	  (status != READOUT_COMPLETE && status != READOUT_FAILED));
    if(status == READOUT_FAILED)
      meas_err("meas_pi_max_read: Error reading CCD.");
    pl_exp_finish_seq(hCam, data, 0);

    if(dst) {
      for (j = 0; j < size/2; j++)
	dst[j] += data[j];
    }
  }
  pl_exp_uninit_seq();
  return 0;
}

/*
 * Close CCD.
 *
 */

EXPORT int meas_pi_max_close() {

  pl_cam_close(hCam);
  pl_pvcam_uninit();
  return 0;
}

#endif /* PVCAM */
