#define MEAS_ER023_CRLF 0    /* Line end */

int meas_er023_init(int, int, int), meas_er023_calibrate(int, double, double, double),
  meas_er023_modulation_amp(int, double), meas_er023_harmonic(int, int), meas_er023_gain(int, double), meas_er023_timeconstant(int, double), meas_er023_conversiontime(int, double), meas_er023_resonator(int, int), meas_er023_phase(int, double), meas_er023_reset(int);
int meas_er023_reclevel(int), meas_er023_status(int), meas_er023_calibrated(int);
unsigned int meas_er023_read(int);
