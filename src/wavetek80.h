/*
 * Wavetek 80 defs.
 *
 */

/* Operating modes */
#define MEAS_WAVETEK80_MODE_NORMAL            0
#define MEAS_WAVETEK80_MODE_LINEAR_SWEEP      1
#define MEAS_WAVETEK80_MODE_LOGARITHMIC_SWEEP 2
#define MEAS_WAVETEK80_MODE_PLL               3

/* Sweep direction */
#define MEAS_WAVETEK80_SWEEP_UP      1
#define MEAS_WAVETEK80_SWEEP_DOWN    2
#define MEAS_WAVETEK80_SWEEP_UP_DOWN 3
#define MEAS_WAVETEK80_SWEEP_DOWN_UP 4

/* Trigger modes */
#define MEAS_WAVETEK80_TRIGGER_CONTINUOUS       1
#define MEAS_WAVETEK80_TRIGGER_EXTERNAL_TRIGGER 2
#define MEAS_WAVETEK80_TRIGGER_EXTERNAL_GATE    3
#define MEAS_WAVETEK80_TRIGGER_EXTERNAL_BURST   4
#define MEAS_WAVETEK80_TRIGGER_INTERNAL_TRIGGER 5
#define MEAS_WAVETEK80_TRIGGER_INTERNAL_BURST   6

/* Control modes */
#define MEAS_WAVETEK80_CONTROL_OFF 0
#define MEAS_WAVETEK80_CONTROL_FM  1
#define MEAS_WAVETEK80_CONTROL_AM  2
#define MEAS_WAVETEK80_CONTROL_VCO 4

/* Output waveforms */
#define MEAS_WAVETEK80_WAVEFORM_DC              0
#define MEAS_WAVETEK80_WAVEFORM_SINE            1
#define MEAS_WAVETEK80_WAVEFORM_TRIANGLE        2
#define MEAS_WAVETEK80_WAVEFORM_SQUARE          3
#define MEAS_WAVETEK80_WAVEFORM_SQUARE_POSITIVE 4
#define MEAS_WAVETEK80_WAVEFORM_SQUARE_NEGATIVE 5

/* Output modes */
#define MEAS_WAVETEK80_OUTPUT_NORMAL   0
#define MEAS_WAVETEK80_OUTPUT_DISABLED 1

/* Prototypes */

int meas_wavetek80_init(int, int, int), meas_wavetek80_operating_mode(int, int);
int meas_wavetek80_sweep_direction(int, int), meas_wavetek80_trigger_mode(int, int);
int meas_wavetek80_control_mode(int, int), meas_wavetek80_output_waveform(int, int);
int meas_wavetek80_output_mode(int, int), meas_wavetek80_set_frequency(int, double);
double meas_wavetek80_get_frequency(int), meas_wavetek80_get_amplitude(int);
int meas_wavetek80_set_amplitude(int, double), meas_wavetek80_set_offset(int, double);
double meas_wavetek80_get_offset(int), meas_wavetek80_get_phase_lock_offset(int);
int meas_wavetek80_set_phase_lock_offset(int, double), meas_wavetek80_set_internal_trigger_interval(int, double);
double meas_wavetek80_get_internal_trigger_interval(int), meas_wavetek80_get_counted_burst(int);
int meas_wavetek80_set_counted_burst(int, int), meas_wavetek80_set_trigger_level(int, double);
double meas_wavetek80_get_trigger_level(int), meas_wavetek80_get_trigger_phase_offset(int);
int meas_wavetek80_set_trigger_phase_offset(int, double), meas_wavetek80_set_output_level(int, double);
double meas_wavetek80_get_output_level(int), meas_wavetek80_get_log_sweep_stop(int);
int meas_wavetek80_set_log_sweep_stop(int, double), meas_wavetek80_set_sweep_time(int, double);
double meas_wavetek80_get_sweep_time(int), meas_wavetek80_get_log_sweep_marker(int);
int meas_wavetek80_set_log_sweep_marker(int, double), meas_wavetek80_set_sweep_stop(int, int);
int meas_wavetek80_get_sweep_stop(int);
int meas_wavetek80_get_sweep_marker(int);
int meas_wavetek80_set_sweep_marker(int, int);


