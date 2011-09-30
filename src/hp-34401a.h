/* line termination mode (1 = CR LF, 0 = CR) */
#define MEAS_HP34401A_CRLF 1

/* AC filters */
#define MEAS_HP34401A_ACFILTER_3HZ   0
#define MEAS_HP34401A_ACFILTER_20HZ  1
#define MEAS_HP34401A_ACFILTER_200HZ 2

/* Impedance mode */
#define MEAS_HP34401A_IMPEDANCE_AUTO_ON  0
#define MEAS_HP34401A_IMPEDANCE_AUTO_OFF 1

/* Measurement modes */
#define MEAS_HP34401A_MODE_VOLT_AC     0
#define MEAS_HP34401A_MODE_VOLT_DC     1
#define MEAS_HP34401A_MODE_RESISTANCE  2
#define MEAS_HP34401A_MODE_CURRENT_AC  3
#define MEAS_HP34401A_MODE_CURRENT_DC  4
#define MEAS_HP34401A_MODE_FREQUENCY   5
#define MEAS_HP34401A_MODE_PERIODIC    6

/* Scales */
#define MEAS_HP34401A_SCALE_VOLT_100MV 0
#define MEAS_HP34401A_SCALE_VOLT_1V    1
#define MEAS_HP34401A_SCALE_VOLT_10V   2
#define MEAS_HP34401A_SCALE_VOLT_100V  3
#define MEAS_HP34401A_SCALE_VOLT_1000V 4

#define MEAS_HP34401A_SCALE_RESISTANCE_100OHM  5
#define MEAS_HP34401A_SCALE_RESISTANCE_1KOHM   6
#define MEAS_HP34401A_SCALE_RESISTANCE_10KHM   7
#define MEAS_HP34401A_SCALE_RESISTANCE_100KOHM 8
#define MEAS_HP34401A_SCALE_RESISTANCE_1MOHM   9
#define MEAS_HP34401A_SCALE_RESISTANCE_10MOHM  10
#define MEAS_HP34401A_SCALE_RESISTANCE_100MOHM 11

#define MEAS_HP34401A_SCALE_CURRENT_10MA  12
#define MEAS_HP34401A_SCALE_CURRENT_100MA 13
#define MEAS_HP34401A_SCALE_CURRENT_1A    14
#define MEAS_HP34401A_SCALE_CURRENT_3A    15

#define MEAS_HP34401A_SCALE_MIN              16
#define MEAS_HP34401A_SCALE_MAX              17
#define MEAS_HP34401A_SCALE_DEF              18

/* Special resoultions */
#define MEAS_HP34401A_RESOL_MIN              (-1.0)
#define MEAS_HP34401A_RESOL_MAX              (-2.0)
#define MEAS_HP34401A_RESOL_DEF              (-3.0)

/* Autoscaling */
#define MEAS_HP34401A_AUTOSCALE_VOLT_AC  0
#define MEAS_HP34401A_AUTOSCALE_VOLT_DC  1
#define MEAS_HP34401A_AUTOSCALE_RES      2
#define MEAS_HP34401A_AUTOSCALE_CURR_AC  3
#define MEAS_HP34401A_AUTOSCALE_CURR_DC  4
#define MEAS_HP34401A_AUTOSCALE_FREQ     5
#define MEAS_HP34401A_AUTOSCALE_PERIODIC 6

/* Integration times */
#define MEAS_HP34401A_INTEGRATION_TIME_20MNLPC  0    /* 0.02 NLPC */
#define MEAS_HP34401A_INTEGRATION_TIME_200MNLPC 1    /* 0.2 NLPC */
#define MEAS_HP34401A_INTEGRATION_TIME_1NLPC    2    /* 1 NLPC */
#define MEAS_HP34401A_INTEGRATION_TIME_10NLPC   3    /* 10 NLPC */
#define MEAS_HP34401A_INTEGRATION_TIME_100NLPC  4    /* 100 NLPC */

/* analysis time windows (for frequency & periodic analysis) */
#define MEAS_HP34401A_TIME_WINDOW_10MS  5
#define MEAS_HP34401A_TIME_WINDOW_100MS 6
#define MEAS_HP34401A_TIME_WINDOW_1S    7

/* auto-zero settings */
#define MEAS_HP34401A_AUTOZERO_OFF  0
#define MEAS_HP34401A_AUTOZERO_ONCE 1
#define MEAS_HP34401A_AUTOZERO_ON   2

/* trigger sources */
#define MEAS_HP34401A_TRIGGER_BUS       0
#define MEAS_HP34401A_TRIGGER_IMMEDIATE 1
#define MEAS_HP34401A_TRIGGER_EXTERNAL  2

/* trigger delay */
#define MEAS_HP34401A_TRIGGER_AUTO (-1.0)

/* prototypes */
int meas_hp34401a_init(int, int, int), meas_hp34401a_set_ac_filter(int, int), meas_hp34401a_set_autoscale(int, int, int), 
  meas_hp34401a_set_impedance(int, int), meas_hp34401a_set_resolution(int, int, int, double), meas_hp34401a_set_integration_time(int, int, int),
  meas_hp34401a_autozero(int, int), meas_hp34401a_set_mode(int, int), meas_hp34401a_set_trigger_source(int, int), 
  meas_hp34401a_set_trigger_dealy(int, double), meas_hp34401a_set_trigger_sample_count(int, int), meas_hp34401a_set_trigger_count(int, int);
double meas_hp34401a_complete_read(int), meas_hp34401a_read_auto(int);
int meas_hp34401a_initiate_read(int);
