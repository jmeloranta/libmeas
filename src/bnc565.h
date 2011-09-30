/* line terminator (1 = CR LF, 0 = CR) */
#define MEAS_BNC565_TERM 1

/* BNC565 channels */
#define MEAS_BNC565_T0  0
#define MEAS_BNC565_CHA 1
#define MEAS_BNC565_CHB 2
#define MEAS_BNC565_CHC 3
#define MEAS_BNC565_CHD 4

/* Trigger mode */
#define MEAS_BNC565_TRIG_EXT 0
#define MEAS_BNC565_TRIG_INT 1

/* Polarity */
#define MEAS_BNC565_POL_INV  0
#define MEAS_BNC565_POL_NORM 1

/* leading or falling edge trigger */
#define MEAS_BNC565_TRIG_FALL 0
#define MEAS_BNC565_TRIG_RISE 1

/* operating mode */
#define MEAS_BNC565_MODE_SS   0  /* single shot */
#define MEAS_BNC565_MODE_DC   1  /* duty cycle */

/* Prototypes */
int meas_bnc565_init(int, int, int), meas_bnc565_set(int, int, int, double, double, double, int);
int meas_bnc565_trigger(int, int, double, int), meas_bnc565_run(int, int);
int meas_bnc565_mode(int, int, int, int, int), meas_bnc565_enable(int, int, int);
int meas_bnc565_do_trigger(int);
