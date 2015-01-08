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

/* operating modes */
#define MEAS_BNC565_MODE_CONTINUOUS   0  /* continuous */
#define MEAS_BNC565_MODE_DUTY_CYCLE   1  /* duty cycle */
#define MEAS_BNC565_MODE_BURST        2  /* burst */
#define MEAS_BNC565_MODE_SINGLE_SHOT  3  /* single shot */

/* Run/Stop state */
#define MEAS_BNC565_STOP 0
#define MEAS_BNC565_RUN  1

/* Channel enable/disable */
#define MEAS_BNC565_CHANNEL_DISABLE 0
#define MEAS_BNC565_CHANNEL_ENABLE  1
