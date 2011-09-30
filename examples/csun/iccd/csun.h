/* 
 * CSUN Configuration.
 *
 */

/*
 * RS232 Device configuration.

USB0 = SCANMATE

 */

#define SCANMATE_PRO "/dev/ttyUSB0"

/*
 * GPIB configuration.
 *
 */

#define DG535  16
#define BNC565 15

/* Standard buffer size for reads */
#define BUF_SIZE 4096

/* Internal electronics delays for Surelite-II, Minilite-II (excl. Q-switch) and the ICCD in sec */

// #define MINILITE_DELAY 0.140E-6
// #define SURELITE_DELAY 0.380E-6
#define MINILITE_DELAY 0.140E-6
#define SURELITE_DELAY 0.380E-6
#define CCD_DELAY      0.0E-9

/* Prototypes etc. */
extern struct experiment *exp_read(char *);
void exp_setup(struct experiment *), exp_run(struct experiment *);
int exp_save_data(struct experiment *);
void exp_init();

void surelite_qswitch(double), minilite_qswitch(double), laser_stop(), laser_start();
double surelite_int_delay(double), minilite_int_delay(double);
void surelite_delay(double), minilite_delay(double), laser_set_delays();

/* Structures. Wavelengths in nm and times in sec. */

struct experiment {
  char date[256];   /* Date of experiment */
  char operator[256];/* Operator */
  char sample[256]; /* Sample info etc. */
  double dye_begin; /* Dye laser scan start wavelength */
  double dye_end;   /* Dye laser scan end wavelength */
  double dye_step;  /* Dye laser scan step */
  double dye_cur;   /* Current wavelength */
  int dye_points;   /* # of data points for dye laser */
  int dye_order;    /* Dye laser grating order (3...8; 0 = auto) */
  double mono_begin;/* Monochromator scan start wavelength */
  double mono_end;  /* Monochromator scan end wavelength */
  double mono_step; /* Monochromator scan step */
  double mono_cur;  /* Monochromator current position (not really used here) */
  int mono_points;  /* # of data points for monochromator */
  int accum;        /* Number of accumulations / data point */
  double delay1;    /* Delay for Surelite-II (laser 1) (s) */
  double delay2;    /* Delay for Minilite-II (laser 2) (s) */
  double delay3;    /* Delay for ICCD (s) */
  double gain;      /* ICCD gain (0 - 255) */
  double gate;      /* ICCD gate width (s) */
  double qswitch1;  /* Q-switch delay for Surelite-II (laser 1) (s) */
  double qswitch2;  /* Q-switch delay for Minilite-II (laser 2) (s) */
  int display;      /* Graphical display mode */
  int display_autoscale;/* Autoscale display? (1 = yes, 0 = no) */
  double display_arg1;/* Display mode argument */
  double display_arg2;/* Display mode argument */
  char output[4096];/* Output file name */
  char shg[4096];    /* Optional shg drive data */
  double *ydata;     /* y-data (intensity) */
  double *x1data;    /* x-data (wavelength of the dye laser) */
  double *x2data;    /* x-data (wavelength of the monochromator / diode array) */
  int bkg_sub;       /* Background subtraction */
  int zscale;        /* z-axis scale (0 = linear, 1 = log) */
};
