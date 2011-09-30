/* Newport IS (diode array) calibration; pixel -> nm */
#define MEAS_NEWPORT_IS_A 162.0991
#define MEAS_NEWPORT_IS_B 0.6680432

/* Electronics delay in Newport IS (ms) */
#define MEAS_NEWPORT_IS_DELAY 18

/* Newport diode array spectrometer */
int meas_newport_is_init(), meas_newport_is_read(double, int, int, double *);
double meas_newport_is_calib(int);

