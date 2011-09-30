#include <time.h>

/* Prototypes */
void meas_misc_nsleep(time_t, long), meas_misc_set_time(), meas_misc_sleep_rest(time_t, long);
void meas_misc_root_on(), meas_misc_root_off(), meas_misc_enable_signals();
void meas_misc_disable_signals();
void meas_misc_set_reftime();
double meas_misc_get_reftime();

/* Error handler macro */
#define meas_err(x) {fprintf(stderr, "%s\n", x); return -1;}
