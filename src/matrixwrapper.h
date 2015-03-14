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

/* Maximum number of devices */
#define MEAS_MATRIX_MAXDEV 3
