/* 
 * CSUN Configuration.
 *
 */

/*
 * RS232 Device configuration.
 */

#define SR245_DEV       "/dev/ttyS0"

/*
 * GPIB configuration.
 *
 */

/* Board 0 */
#define FL3000_BOARD 0
#define FL3000_ID    6

/* YAG */
#define DG535_1_BOARD 1
#define DG535_1_ID    15
/* Excimer + CCD */
#define DG535_2_BOARD 1
#define DG535_2_ID    16

/* Laser parameters */
/* measured at 16 Hz */
#define SURELITE_DELAY 0.240E-6
/* measured at 16 Hz */
#define EXCIMER_DELAY 1.37E-6

/* CCD trigger delay */
#define CCD_DELAY 20.0E-9

/* Standard buffer size for reads */
#define BUF_SIZE 4096

/* Maximum number of samples */
#define MAX_SAMPLES 65535

/* GPIB parameters */
#define TIMEOUT 20
#define SENDEOI 1
#define EOS '\r'

/* Proto types etc. */
extern struct experiment *exp_read(char *);
void exp_setup(struct experiment *), exp_run(struct experiment *);
int exp_save_data(struct experiment *);
void exp_init();

void surelite_qswitch(double), laser_stop(), laser_start();
double surelite_int_delay(double), excimer_int_delay();
void surelite_delay(double), excimer_delay(double), laser_set_delays();

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
  double etalon_zero_wl;   /* Wavelength at etalon normal position*/
  unsigned etalon_zero;   /* Etalon normal position */
  double etalon_wl_scale1; /* etalon scan params */
  double etalon_wl_scale2;
  double etalon_wl_p1;
  double etalon_wl_p2;
  double etalon_wl_p3;
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
  double qswitch;   /* Q-switch delay for Surelite */
  double gain;      /* CCD gain */
  double gate;      /* Boxcar gate width or diode array exposure time (s) */
  int display;      /* Graphical display mode */
  int display_autoscale;/* Autoscale display? (1 = yes, 0 = no) */
  double display_arg1;/* Display mode argument (begin integration) */
  double display_arg2;/* Display mode argument (end integration) */
  char output[4096];/* Output file name */
  double pressure1;  /* Pressure (read only) */
  double pressure2;  /* Pressure (read only) */
  double temperature1;/* Temperature (read only) */
  double temperature2;/* Temperature (read only) */
  double *ydata;     /* y-data (intensity) */
  double *x1data;    /* x-data (wavelength of the dye laser) */
  double *x2data;    /* x-data (wavelength of the monochromator / diode array) */
};
