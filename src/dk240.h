/* DK240 commands */
#define MEAS_DK240_STEPDOWN 1
#define MEAS_DK240_SLEWDOWN 2
#define MEAS_DK240_STEPUP   7
#define MEAS_DK240_SLEWUP   8
#define MEAS_DK240_GOTO     16
#define MEAS_DK240_GCAL     18
#define MEAS_DK240_GRTID    19
#define MEAS_DK240_CLEAR    25
#define MEAS_DK240_GRTSEL   26
#define MEAS_DK240_ECHO     27
#define MEAS_DK240_WAVEQ    29
#define MEAS_DK240_SLITQ    30
#define MEAS_DK240_S1ADJ    31
#define MEAS_DK240_S2ADJ    32
#define MEAS_DK240_SERIAL   33
#define MEAS_DK240_ZERO     52
#define MEAS_DK240_RESET    255

/* end of text */
#define MEAS_DK240_EOT 24

/* # of calibration points */
#define MEAS_DK240_NCAL 17

/* Calibration data - NOTE: Monochromator dependent!!! */
static double calib_x[] = {190.0, 253.6521, 265.369, 275.278, 289.3601, 296.7278, 302.1504, 312.5674, 334.1484, 365.0158, 365.4840, 404.6565,407.7837, 435.8335, 546.0750, 576.9610, 579.0670}; 
/* mono = real - offset */
static double calib_y[] = {0.6521, 0.6521, 0.369, 0.278, 0.8601, 0.7278, 0.6504, 0.0674, 0.6484, 1.0158, -0.0160, 0.6565, 0.7837, 0.3335, 0.5750, 1.461, 1.0670};
