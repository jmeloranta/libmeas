/* CCD temperature */
#define MEAS_PI_MAX_TEMPERATURE (-20.0)

/* Sleep time for readout completion (in microsec) */
#define MEAS_PI_MAX_SLEEP 200

/*
 * Prototypes. If HAVE_PVCAM_H is not defined, reference to these would
 * result in linker error.
 *
 */

int meas_pi_max_init(double), meas_pi_max_close(), meas_pi_max_read(int, double *);
int meas_pi_max_temperature(double);
int meas_pi_max_size();
int meas_pi_max_gate_mode(int), meas_pi_max_shutter_mode(int);
int meas_pi_max_exposure_mode(int);
int meas_pi_max_pmode(int);
int meas_pi_max_gain(int);
int meas_pi_max_roi(int, int, int, int, int, int);
int meas_pi_max_clear_mode(int);
int meas_pi_max_gain_index(int);
int meas_pi_max_speed_index(int);
int meas_pi_max_trigger_mode(int);
double meas_pi_max_get_temperature();
