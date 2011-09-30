/*
 * Configuration.
 *
 */

/* CCD temperature in Celcius (should go down to -30 oC) */
#define MEAS_MATRIX_TEMPERATURE (-10.0)

/* Wait for CCD ready (in microsec) */
#define MEAS_MATRIX_SLEEP 10

/* Wavelength calibration (spectrometer dependent!) */
#define MEAS_MATRIX_A 194.7
#define MEAS_MATRIX_B 0.59408

/* Prototypes */
extern int meas_matrix_init();
extern int meas_matrix_size();
extern int meas_matrix_read(double exp, int ave, double *dst);
extern int meas_matrix_close();
extern int meas_matrix_status();
extern double meas_matrix_calib(int);
extern int meas_matrix_temperature(double);
