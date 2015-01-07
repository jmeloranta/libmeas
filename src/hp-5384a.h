/* line termination mode (1 = CR LF, 0 = CR) */
#define MEAS_HP5384_CRLF 1

/* RF input channels */
#define MEAS_HP5384_CHA 1
#define MEAS_HP5384_CHB 2

/* Channel A attenuator settings */
#define MEAS_HP5384_ATTN_0  0    /* no attn for channel A */
#define MEAS_HP5384_ATTN_20 1    /* x20 attn for channel A */

/* Channel A builtin long pass filter (100 kHz) */
#define MEAS_HP5384_FILT_ON  0   /* filter on */
#define MEAS_HP5384_FILT_OFF 1   /* filter off */

/* Channel A manual level control setting */
#define MEAS_HP5384_LEVEL_MAN  0   /* manual level control */
#define MEAS_HP5384_LEVEL_AUTO 1   /* disable manual level control */

/* Gate time settings */
#define MEAS_HP5384_GATE_100MS  0   /* 0.1 s gate time */
#define MEAS_HP5384_GATE_1S     1   /* 1 s gate time */
#define MEAS_HP5384_GATE_10S    2   /* 10 s gate time */
