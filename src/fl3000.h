/* line terminator (1 = CR LF, 0 = CR) */
#define MEAS_FL3000_TERM 0

/* Calibration constants */
#define MEAS_FL3000_GRATING_CONST 3.15E-3

/* Etalon constants */
/*#define MEAS_FL3000_ETALON_STEP 3.125E-6 */
#define MEAS_FL3000_ETALON_STEP 9.15E-7
#define MEAS_FL3000_REFIND      1.0003     /* refractive index of N2 (air) */
                               /* depends on the gas in the tuning unit */
#define MEAS_FL3000_ETALON_MAX  15000      /* max # of steps */
/* Site dependent (and dye) calibration constants (linear) */
#define MEAS_FL3000_K 0.99801
#define MEAS_FL3000_B 0.92535

extern unsigned int meas_etalon_zero;
extern double meas_etalon_zero_wl;

extern double meas_etalon_wl_scale1;
extern double meas_etalon_wl_scale2;
extern double meas_etalon_wl_p1;
extern double meas_etalon_wl_p2;
extern double meas_etalon_wl_p3;

/* Prototypes */
int meas_fl3000_init(int, int, int), meas_fl3000_setwl(int, double), meas_fl3000_grating(int, int);
unsigned int meas_fl3000_getetalon(int);
double meas_fl3000_getwl(int);
