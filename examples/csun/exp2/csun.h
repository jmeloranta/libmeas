/* 
 * CSUN Configuration.
 *
 */

/*
 * RS232 Device configuration.

USB0 = monochromator
USB1 = PDR2000
USB2 = ITC503
USB0 = SCANMATE

 */

#define PDR2000      "/dev/ttyUSB2"
#define DK240        "/dev/ttyUSB1"
#define ITC503       "/dev/ttyUSB1"
#define SCANMATE_PRO "/dev/ttyUSB0"

/*
 * GPIB configuration.
 *
 */

#define SR245  16
#define BNC565 15

/* Standard buffer size for reads */
#define BUF_SIZE 4096

/* Maximum number of samples */
#define MAX_SAMPLES 8192

/* GPIB parameters */
#define BOARD_INDEX 0
#define TIMEOUT 20

/* Internal electronics delays for Surelite-II and Minilite-II (excl. Q-switch) */

#define MINILITE_DELAY 0.140E-6
#define SURELITE_DELAY 0.380E-6

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
  double mono_cur;  /* Current wavelength */
  int mono_points;  /* # of data points for dye laser */
  int signal_source;/* Signal source: 0 = diode array, other = SR245 channel */
  int signal_ref;   /* Reference signal source (-1 = not in use) */
  int synchronous;  /* Synchronous(1) or asynchronous mode(0) of SR245 ? */
  int accum;        /* Number of accumulations / data point */
  double delay1;    /* Delay for Surelite-II (laser 1) */
  double delay2;    /* Delay for Minilite-II (laser 2) */
  double delay3;    /* Delay for Boxcar */
  double qswitch1;  /* Q-switch delay for Surelite-II (laser 1) */
  double qswitch2;  /* Q-switch delay for Minilite-II (laser 2) */
  double islit;     /* Input slit (micron) for DK240 monochromator */
  double oslit;     /* Output slit (micron) for DK240 monochromator */
  double pmt;       /* Photomultiplier tube voltage level (V) */
  double gate;      /* Boxcar gate width or diode array exposure time (s) */
  int display;      /* Graphical display mode */
  int display_autoscale;/* Autoscale display? (1 = yes, 0 = no) */
  double display_arg1;/* Display mode argument */
  double display_arg2;/* Display mode argument */
  char output[4096];/* Output file name */
  double pressure;  /* Pressure (read only) */
  double temperature;/* Temperature (read only) */
  double *ydata;     /* y-data (intensity) */
  char shg[4096];    /* Optional shg drive data */
  double *x1data;    /* x-data (wavelength of the dye laser) */
  double *x2data;    /* x-data (wavelength of the monochromator / diode array) */
};
