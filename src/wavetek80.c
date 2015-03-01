/*
 * Wavetek 80 control routines.
 *
 * Note that Wavetek 81 is different from 80 and has slightly different
 * commands. It will need its own driver (only very small modifications needed,
 * see the manual).
 *
 */

#ifdef GPIB

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "wavetek80.h"
#include "gpib.h"
#include "misc.h"

/* Up to 5 devices supported */
static int wavetek80_fd[5] = {-1, -1, -1, -1, -1};

EXPORT int meas_wavetek80_init(int unit, int board, int dev) {
  
  if(wavetek80_fd[unit] == -1) {
    wavetek80_fd[unit] = meas_gpib_open(board, dev);
    meas_gpib_timeout(wavetek80_fd[unit], 1.0);
  }
  meas_gpib_write(wavetek80_fd[unit], "X0", 0); /* response header off */
  meas_gpib_write(wavetek80_fd[unit], "Z0", 0); /* Newline + EOI terminator */
  return 0;
}

/*
 * Available modes:
 *
 * MEAS_WAVETEK80_MODE_NORMAL            Normal operating mode.
 * MEAS_WAVETEK80_MODE_LINEAR_SWEEP      Linear sweep.
 * MEAS_WAVETEK80_MODE_LOGARITHMIC_SWEEP Logarithmic sweep.
 * MEAS_WAVETEK80_MODE_PLL               PLL.
 *
 */

EXPORT int meas_wavetek80_operating_mode(int unit, int mode) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(mode < MEAS_WAVETEK80_MODE_NORMAL || mode > MEAS_WAVETEK80_MODE_PLL)
    meas_err("wavetek80: Invalid operating mode.");

  sprintf(buf, "F%d", mode);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Select sweep direction:
 *
 * MEAS_WAVETEK80_SWEEP_UP      Start to stop (up).
 * MEAS_WAVETEK80_SWEEP_DOWN    Stop to start (down).
 * MEAS_WAVETEK80_SWEEP_UP_DOWN Start to stop to start (up-down).
 * MEAS_WAVETEK80_SWEEP_DOWN_UP Stop to start to stop (down-up).
 *
 */

EXPORT int meas_wavetek80_sweep_direction(int unit, int dir) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(dir < MEAS_WAVETEK80_SWEEP_UP || dir > MEAS_WAVETEK80_SWEEP_DOWN_UP)
    meas_err("wavetek80: Invalid sweep mode.");

  sprintf(buf, "S%d", dir );
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Trigger mode:
 *
 * MEAS_WAVETEK80_TRIGGER_CONTINUOUS       Normal continuous mode.
 * MEAS_WAVETEK80_TRIGGER_EXTERNAL_TRIGGER External trigger mode.
 * MEAS_WAVETEK80_TRIGGER_EXTERNAL_GATE    External gate.
 * MEAS_WAVETEK80_TRIGGER_EXTERNAL_BURST   External burst.
 * MEAS_WAVETEK80_TRIGGER_INTERNAL_TRIGGER Internal trigger.
 * MEAS_WAVETEK80_TRIGGER_INTERNAL_BURST   Internal burst.
 *
 */

EXPORT int meas_wavetek80_trigger_mode(int unit, int mode) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(mode < MEAS_WAVETEK80_TRIGGER_CONTINUOUS
     || mode > MEAS_WAVETEK80_TRIGGER_INTERNAL_BURST)
    meas_err("wavetek80: Invalid trigger mode.");

  sprintf(buf, "M%d", mode);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Control modes:
 * 
 * MEAS_WAVETEK80_CONTROL_OFF    Normal mode.
 * MEAS_WAVETEK80_CONTROL_FM     Frequency modulation.
 * MEAS_WAVETEK80_CONTROL_AM     Amplitude modulation.
 * MEAS_WAVETEK80_CONTROL_VCO    VCO mode.
 *
 */

EXPORT int meas_wavetek80_control_mode(int unit, int mode) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(mode < MEAS_WAVETEK80_CONTROL_OFF || mode > MEAS_WAVETEK80_CONTROL_VCO)
    meas_err("wavetek80: Invalid control mode.");

  sprintf(buf, "CT%d", mode);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Output waveform:
 *
 * MEAS_WAVETEK80_WAVEFORM_DC              DC output.
 * MEAS_WAVETEK80_WAVEFORM_SINE            Sine waveform.
 * MEAS_WAVETEK80_WAVEFORM_TRIANGLE        Triangle waveform.
 * MEAS_WAVETEK80_WAVEFORM_SQUARE          Square waveform.
 * MEAS_WAVETEK80_WAVEFORM_SQUARE_POSITIVE Fixed baseline positive squarewave.
 * MEAS_WAVETEK80_WAVEFORM_SQUARE_NEGATIVE Fixed baseline negative squarewave.
 *
 */

EXPORT int meas_wavetek80_output_waveform(int unit, int mode) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(mode < MEAS_WAVETEK80_WAVEFORM_DC || mode > MEAS_WAVETEK80_WAVEFORM_SQUARE_NEGATIVE)
    meas_err("wavetek80: Invalid waveform.");

  sprintf(buf, "W%d", mode);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Output mode:
 *
 * MEAS_WAVETEK80_OUTPUT_NORMAL   Normal output.
 * MEAS_WAVETEK80_OUTPUT_DISABLED Output disabled.
 *
 */

EXPORT int meas_wavetek80_output_mode(int unit, int mode) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(mode < MEAS_WAVETEK80_OUTPUT_NORMAL || mode > MEAS_WAVETEK80_OUTPUT_DISABLED)
    meas_err("wavetek80: Invalid output mode.");

  sprintf(buf, "D%d", mode);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Set output frequency (in Hz).
 *
 */

EXPORT int meas_wavetek80_set_frequency(int unit, double freq) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(freq < 10E-3 || freq > 50E6) meas_err("wavetek80: Illegal frequency setting.");

  sprintf(buf, "FRQ %leHZ", freq);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Get output frequency (in Hz).
 *
 */

EXPORT double meas_wavetek80_get_frequency(int unit) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");

  meas_gpib_write(wavetek80_fd[unit], "FRQ?", 0);
  meas_gpib_read(wavetek80_fd[unit], buf);
  return atof(buf);
}

/*
 * Set output amplitude (in V).
 *
 */

EXPORT int meas_wavetek80_set_amplitude(int unit, double ampl) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(ampl < 10E-3 || ampl > 16.0) meas_err("wavetek80: Illegal amplitude setting.");

  sprintf(buf, "AMP %leV", ampl);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Get output amplitude (in V).
 *
 */

EXPORT double meas_wavetek80_get_amplitude(int unit) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");

  meas_gpib_write(wavetek80_fd[unit], "AMP?", 0);
  meas_gpib_read(wavetek80_fd[unit], buf);
  return atof(buf);
}

/*
 * Set output offset (in V).
 *
 */

EXPORT int meas_wavetek80_set_offset(int unit, double offset) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(offset < -8.0 || offset > 8.0) meas_err("wavetek80: Illegal offset setting.");

  sprintf(buf, "OFS %leV", offset);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Get output offset (in V).
 *
 */

EXPORT double meas_wavetek80_get_offset(int unit) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");

  meas_gpib_write(wavetek80_fd[unit], "OFS?", 0);
  meas_gpib_read(wavetek80_fd[unit], buf);
  return atof(buf);
}

/*
 * Set phase lock offset (in degrees).
 *
 */

EXPORT int meas_wavetek80_set_phase_lock_offset(int unit, double offset) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(offset < -180.0 || offset > 180.0) meas_err("wavetek80: Illegal phase lock offset setting.");

  sprintf(buf, "PLL %le", offset);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Get phase lock offset (in degrees).
 *
 */

EXPORT double meas_wavetek80_get_phase_lock_offset(int unit) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");

  meas_gpib_write(wavetek80_fd[unit], "PLL?", 0);
  meas_gpib_read(wavetek80_fd[unit], buf);
  return atof(buf);
}

/*
 * Set internal trigger generator repetition interval (in sec).
 *
 */

EXPORT int meas_wavetek80_set_internal_trigger_interval(int unit, double ival) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(ival < 20E-6 || ival > 999.0) meas_err("wavetek80: Illegal trigger interval setting.");

  sprintf(buf, "RPT %le", ival);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Get internal trigger generator repetition interval (in sec).
 *
 */

EXPORT double meas_wavetek80_get_internal_trigger_interval(int unit) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");

  meas_gpib_write(wavetek80_fd[unit], "RPT?", 0);
  meas_gpib_read(wavetek80_fd[unit], buf);
  return atof(buf);
}

/*
 * Set counted burst (in #).
 *
 */

EXPORT int meas_wavetek80_set_counted_burst(int unit, int nburst) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(nburst < 1 || nburst > 4000) meas_err("wavetek80: Illegal counted burst setting.");

  sprintf(buf, "BUR %le", nburst);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Get counted burst (in #).
 *
 */

EXPORT double meas_wavetek80_get_counted_burst(int unit) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");

  meas_gpib_write(wavetek80_fd[unit], "BUR?", 0);
  meas_gpib_read(wavetek80_fd[unit], buf);
  return atof(buf);
}


/*
 * Set trigger level (in V).
 *
 */

EXPORT int meas_wavetek80_set_trigger_level(int unit, double level) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(level < -10.0 || level > 10.0) meas_err("wavetek80: Illegal trigger level setting.");

  sprintf(buf, "TLV %le", level);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Get trigger level (in V).
 *
 */

EXPORT double meas_wavetek80_get_trigger_level(int unit) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");

  meas_gpib_write(wavetek80_fd[unit], "TLV?", 0);
  meas_gpib_read(wavetek80_fd[unit], buf);
  return atof(buf);
}

/*
 * Set trigger phase offset (in degrees).
 *
 */

EXPORT int meas_wavetek80_set_trigger_phase_offset(int unit, double offset) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(offset < -90.0 || offset > 90.0) meas_err("wavetek80: Illegal trigger phase offset setting.");

  sprintf(buf, "TPH %le", offset);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Get trigger phase offset (in degrees).
 *
 */

EXPORT double meas_wavetek80_get_trigger_phase_offset(int unit) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");

  meas_gpib_write(wavetek80_fd[unit], "TPH?", 0);
  meas_gpib_read(wavetek80_fd[unit], buf);
  return atof(buf);

}

/*
 * Set DC output level (in Volts).
 *
 */

EXPORT int meas_wavetek80_set_output_level(int unit, double level) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(level < -8.0 || level > 8.0) meas_err("wavetek80: Illegal output level setting.");

  sprintf(buf, "DCO %leV", level);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Get DC output level (in Volts).
 *
 */

EXPORT double meas_wavetek80_get_output_level(int unit) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");

  meas_gpib_write(wavetek80_fd[unit], "DCO?", 0);
  meas_gpib_read(wavetek80_fd[unit], buf);
  return atof(buf);

}

/*
 * Set logarithmic sweep stop (in Hz).
 *
 */

EXPORT int meas_wavetek80_set_log_sweep_stop(int unit, double value) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(value < 10.0E-3 || value > 50.0E6) meas_err("wavetek80: Illegal log sweep stop setting.");

  sprintf(buf, "STP %leHZ", value);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Get logarithmic sweep stop (in Hz).
 *
 */

EXPORT double meas_wavetek80_get_log_sweep_stop(int unit) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");

  meas_gpib_write(wavetek80_fd[unit], "STP?", 0);
  meas_gpib_read(wavetek80_fd[unit], buf);
  return atof(buf);
}

/*
 * Set sweep time (in sec).
 *
 */

EXPORT int meas_wavetek80_set_sweep_time(int unit, double value) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(value < 10.0E-3 || value > 999.0) meas_err("wavetek80: Illegal sweep time setting.");

  sprintf(buf, "SWT %leS", value);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Get sweep time (in sec).
 *
 */

EXPORT double meas_wavetek80_get_sweep_time(int unit) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");

  meas_gpib_write(wavetek80_fd[unit], "SWT?", 0);
  meas_gpib_read(wavetek80_fd[unit], buf);
  return atof(buf);
}

/*
 * Set logarithmic sweep marker (in Hz)
 *
 */

EXPORT int meas_wavetek80_set_log_sweep_marker(int unit, double value) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(value < 10E-3 || value > 50.0E6) meas_err("wavetek80: Illegal log sweep marker setting.");

  sprintf(buf, "MRK %leHZ", value);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Get logarithmic sweep marker (in Hz).
 *
 */

EXPORT double meas_wavetek80_get_log_sweep_marker(int unit) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");

  meas_gpib_write(wavetek80_fd[unit], "MRK?", 0);
  meas_gpib_read(wavetek80_fd[unit], buf);
  return atof(buf);
}

/*
 * Set linear sweep stop (in Hz).
 *
 */

EXPORT int meas_wavetek80_set_sweep_stop(int unit, int value) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(value < 10 || value > 5000) meas_err("wavetek80: Illegal sweep stop setting.");

  sprintf(buf, "SSN %d", value);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Get linear sweep stop (in Hz).
 *
 */

EXPORT int meas_wavetek80_get_sweep_stop(int unit) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");

  meas_gpib_write(wavetek80_fd[unit], "SSN?", 0);
  meas_gpib_read(wavetek80_fd[unit], buf);
  return atoi(buf);
}

/*
 * Set linear sweep marker (in Hz).
 *
 */

EXPORT int meas_wavetek80_set_sweep_marker(int unit, int value) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");
  if(value < 10 || value > 5000) meas_err("wavetek80: Illegal sweep marker setting.");

  sprintf(buf, "MKN %leHZ", value);
  meas_gpib_write(wavetek80_fd[unit], buf, 0);
  return 0;
}

/*
 * Get linear sweep marker (in Hz).
 *
 */

EXPORT int meas_wavetek80_get_sweep_marker(int unit) {

  char buf[256];

  if(wavetek80_fd[unit] == -1) meas_err("wavetek80: non-existent unit.");

  meas_gpib_write(wavetek80_fd[unit], "MKN?", 0);
  meas_gpib_read(wavetek80_fd[unit], buf);
  return atoi(buf);
}

#endif /* GPIB */
