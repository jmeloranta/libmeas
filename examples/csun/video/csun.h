/* 
 * CSUN Configuration.
 *
 */

/*
 * GPIB configuration.
 *
 */

#define DG535  16
#define BNC565 15

/* Standard buffer size for reads */
#define BUF_SIZE 4096

/* Internal electronics delays for Surelite-II, Minilite-II (excl. Q-switch) and the ICCD in sec */

#define MINILITE_DELAY (0.140E-6 - 10E-6)
#define SURELITE_DELAY 0.380E-6
#define CCD_DELAY      120E-9

/* Prototypes etc. */
void surelite_qswitch(double), minilite_qswitch(double), laser_stop(), laser_start();
double surelite_int_delay(double), minilite_int_delay(double);
void surelite_delay(double), minilite_delay(double), laser_set_delays();
