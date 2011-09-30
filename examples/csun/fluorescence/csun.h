/* 
 * CSUN Configuration.
 *
 */

/*
 * RS232 Device configuration.

USB0 = DK240 monochromator.

 */

#define DK240 "/dev/ttyUSB0"

/*
 * GPIB configuration.
 *
 */

#define HP34401  12
#define BNC565   15

/* Standard buffer size for reads */
#define BUF_SIZE 4096

/* Maximum number of samples */
#define MAX_SAMPLES 8192

/* GPIB parameters */
#define BOARD_INDEX 0
#define TIMEOUT 20
#define SENDEOI 1
#define EOS '\r'

/* Internal electronics delays for Surelite-II and Minilite-II (excl. Q-switch) */

#define MINILITE_DELAY 0.140E-6
#define SURELITE_DELAY 0.380E-6

/* Prototypes etc. */
extern struct experiment *exp_read(char *);
void exp_setup(struct experiment *), exp_run(struct experiment *);
int exp_save_data(struct experiment *);
void exp_init();

void surelite_qswitch(double), minilite_qswitch(double), laser_stop(), laser_start();
double surelite_int_delay(double), minilite_int_delay();
void surelite_delay(double), minilite_delay(double), laser_set_delays(), pmt_delay(double), pmt_gate(double);

/* Structures. Wavelengths in nm and times in sec. */

struct experiment {
  char date[256];   /* Date of experiment */
  char operator[256];/* Operator */
  char sample[256]; /* Sample info etc. */
  double mono_begin; /* Dye laser scan start wavelength */
  double mono_end;   /* Dye laser scan end wavelength */
  double mono_step;  /* Dye laser scan step */
  double mono_cur;   /* Current wavelength */
  int mono_points;   /* # of data points for dye laser */
  double islit, oslit; /* input & output slits */
  int accum;        /* Number of accumulations / data point */
  double delay1;    /* Delay for Surelite-II (laser 1) */
  double delay2;    /* Delay for Minilite-II (laser 2) */
  double delay3;    /* Delay for Boxcar */
  int gain;         /* Voltmeter scale for PMT */
  char output[4096];/* Output file name */
  char shg[4096];   /* Optional shg drive data */
  double *ydata;     /* y-data (intensity) */
  double *xdata;    /* x-data (wavelength of the dye laser) */
};
