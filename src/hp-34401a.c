/*
 * Support for HP (Agilent) 34401A multimeter.
 *
 */

#ifdef GPIB

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hp-34401a.h"
#include "gpib.h"
#include "misc.h"

/* Up to 5 devices supported */
static int hp34401a_fd[5] = {-1, -1, -1, -1, -1};

static char *modes[] = {"CONF:VOLT:AC", "CONF:VOLT:DC", "CONF:RES", "CONF:CURR:AC", "CONF:CURR:DC", "CONF:FREQ", "CONF:PER", 0};
static char *scales[] = {"0.1", "1.0", "10.0", "100.0", "1000.0", /* VOLT:AC, VOLT:DC, FREQ and PERIOD */
                         "100.0", "1000.0", "10000.0", "100000.0", "1000000.0", "10000000.0", "100000000.0", /* CONF:RES */
                         "0.01", "0.1", "1.0", "3.0", /* CURR:AC and CURR:DC */
                         "MIN", "MAX", "DEF",         /* applies to all modes */
                         0};
static char *autoscale[] = {"SENS:VOLT:AC:RANG:AUTO", "SENS:VOLT:DC:RANG:AUTO", "SENS:RES:RANG:AUTO", "SENS:CURR:AC:RANG:AUTO", "SENS:CURR:DC:RANG:AUTO", "SENS:FREQ:VOLT:RANG:AUTO", "SENS:PER:VOLT:RANG:AUTO", 0};
static char *itime[] = {0, "SENS:VOLT:DC:NPLC", "SENS:RES:NPLC", 0, "SENS:CURR:DC:NPLC", "SENS:FREQ:APER", "SENS:PER:APER", 0};
static char *itime_opts[] = {/* int. times */ "0.02", "0.2", "1.0", "10.0", "100.0", /* freq & period */ "0.010", "0.1", "1.0", 0};
static char *setmodes[] = {"SENS:FUNC \"VOLT:AC\"", "SENS:FUNC \"VOLT:DC\"", "SENS:FUNC \"RES\"", "SENS:FUNC \"CURR:AC\"", "SENS:FUNC \"CURR:DC\"", "SENS:FUNC \"FREQ\"", "SENS:FUNC \"PER\"", 0};
static char *trigger_sources[] = {"BUS", "IMM", "EXT", 0};

/* Initialize instrument (dev = GPIB id) */
EXPORT int meas_hp34401a_open(int unit, int board, int dev) {

  if(hp34401a_fd[unit] == -1) {
    hp34401a_fd[unit] = meas_gpib_open(board, dev);
    meas_gpib_timeout(hp34401a_fd[unit], 1.0);
  }
  /* we don't need the unit to do data processing since the computer can do this... */
  meas_gpib_write(hp34401a_fd[unit], "CALC:STAT OFF", MEAS_HP34401A_CRLF); /* turn off math processing */
  return 0;
}

/*
 * AC signal filter.
 * Set the lowest frequency expected in the input.
 *
 * freq =
 *   MEAS_HP34401A_ACFILTER_3HZ:   Lowest frequency 3 Hz
 *   MEAS_HP34401A_ACFILTER_20HZ:  Lowest frequency 20 Hz
 *   MEAS_HP34401A_ACFILTER_200HZ: Lowest frequency 200 Hz
 *
 */

EXPORT int meas_hp34401a_set_ac_filter(int unit, int freq) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(hp34401a_fd[unit] == -1)
    meas_err("meas_hp34401a_set_ac_filter: Non-existent unit.");
  switch (freq) {
  case MEAS_HP34401A_ACFILTER_3HZ:
    sprintf(buf, "SENS:DET:BAND 3");
    break;
  case MEAS_HP34401A_ACFILTER_20HZ:
    sprintf(buf, "SENS:DET:BAND 20");
    break;
  case MEAS_HP34401A_ACFILTER_200HZ:
    sprintf(buf, "SENS:DET:BAND 200");
    break;
  default:
    meas_err("meas_hp34401a_set_ac_filter: Invalid AC signal filter.");
  }
  meas_gpib_write(hp34401a_fd[unit], buf, MEAS_HP34401A_CRLF);
  return 0;
}

/*
 * Set autoscale on/off.
 *
 * autos =
 *   MEAS_HP34401A_AUTOSCALE_VOLT_AC:    autoscale AC voltage
 *   MEAS_HP34401A_AUTOSCALE_VOLT_DC:    autoscale DC voltage
 *   MEAS_HP34401A_AUTOSCALE_RES:        autoscale resistance
 *   MEAS_HP34401A_AUTOSCALE_CURR_AC:    autoscale AC current
 *   MEAS_HP34401A_AUTOSCALE_CURR_DC:    autoscale DC current
 *   MEAS_HP34401A_AUTOSCALE_FREQ:       autoscale frequency voltage
 *   MEAS_HP34401A_AUTOSCALE_PERIODIC:   autoscale periodic voltage
 * 
 * setting = 
 *   1:      autoscale on
 *   0:      autoscale off
 *
 */

EXPORT int meas_hp34401a_set_autoscale(int unit, int autos, int setting) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(hp34401a_fd[unit] == -1) 
    meas_err("meas_hp34401a_set_autoscale: Non-existent unit.");
  if(autos < MEAS_HP34401A_AUTOSCALE_VOLT_AC || autos > MEAS_HP34401A_AUTOSCALE_PERIODIC) 
    meas_err("meas_hp34401a_set_autoscale: Unknown scale.");
  if(setting < 0 || setting > 1)
    meas_err("meas_hp34401a_set_autoscale: Unknown autoscale setting.");
  sprintf(buf, "%s %s", autoscale[autos], setting?"ON":"OFF");
  meas_gpib_write(hp34401a_fd[unit], buf, MEAS_HP34401A_CRLF);
  return 0;
}

/*
 * Set input impedance mode.
 *
 * autos =
 *   MEAS_HP34401A_IMPEDANCE_AUTO_ON:  10MOhm impedance in all voltage ranges.
 *   MEAS_HP34401A_IMPEDANCE_AUTO_OFF: 10GOhm for 100 mV, 1 V, 10 V and
 *                       10MOhm for 100 V, 1000 V ranges.
 *   
 */

EXPORT int meas_hp34401a_set_impedance(int unit, int autos) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(hp34401a_fd[unit] == -1)
    meas_err("meas_hp34401a_set_impedance: Non-existent unit.");
  switch(autos) {
  case MEAS_HP34401A_IMPEDANCE_AUTO_ON:
    sprintf(buf, "INP:IMP:AUTO ON");
    break;
  case MEAS_HP34401A_IMPEDANCE_AUTO_OFF:
    sprintf(buf, "INP:IMP:AUTO OFF");
    break;
  default:
    meas_err("meas_hp34401a_set_impedance: Invalid impedance mode.");
  }
  meas_gpib_write(hp34401a_fd[unit], buf, MEAS_HP34401A_CRLF);
  return 0;
}

/*
 * Set readout resolution.
 *
 * what  =
 *   MEAS_HP34401A_MODE_VOLT_AC:    voltage AC
 *   MEAS_HP34401A_MODE_VOLT_DC:    voltage DC
 *   MEAS_HP34401A_MODE_RESISTANCE: resistance
 *   MEAS_HP34401A_MODE_CURRENT_AC: current AC
 *   MEAS_HP34401A_MODE_CURRENT_DC: current DC
 *   MEAS_HP34401A_MODE_FREQUENCY:  frequency
 *   MEAS_HP34401A_MODE_PERIODIC:   periodic
 *
 * scale =
 *  For MEAS_HP34401A_MODE_VOLT_AC, MEAS_HP34401A_MODE_VOLT_DC,
 *      MEAS_HP34401A_MODE_FREQUENCY, and MEAS_HP34401A_MODE_PERIODIC:
 *   MEAS_HP34401A_SCALE_VOLT_100MV: 100 mV scale
 *   MEAS_HP34401A_SCALE_VOLT_1V:    1 V scale
 *   MEAS_HP34401A_SCALE_VOLT_10V:   10 V scale
 *   MEAS_HP34401A_SCALE_VOLT_100V:  100 V scale
 *   MEAS_HP34401A_SCALE_VOLT_1000V: 1000 V scale
 * For MEAS_HP34401A_MODE_RESISTANCE:
 *   MEAS_HP34401A_SCALE_RESISTANCE_100OHM:  100 Ohm scale
 *   MEAS_HP34401A_SCALE_RESISTANCE_1KOHM:   1K Ohm scale
 *   MEAS_HP34401A_SCALE_RESISTANCE_10KHM:   10K Ohm scale
 *   MEAS_HP34401A_SCALE_RESISTANCE_100KOHM: 100K Ohm scale
 *   MEAS_HP34401A_SCALE_RESISTANCE_1MOHM:   1M Ohm scale
 *   MEAS_HP34401A_SCALE_RESISTANCE_10MOHM:  10M Ohm scale
 *   MEAS_HP34401A_SCALE_RESISTANCE_100MOHM: 100M Ohm scale
 * For MEAS_HP34401A_MODE_CURRENT_AC and MEAS_HP34401A_MODE_CURRENT_DC:
 *   MEAS_HP34401A_SCALE_CURRENT_10MA:    10 mA scale
 *   MEAS_HP34401A_SCALE_CURRENT_100MA:   100 mA scale
 *   MEAS_HP34401A_SCALE_CURRENT_1A:      1 A scale
 *   MEAS_HP34401A_SCALE_CURRENT_3A:      3 A scale
 *
 * For each scale, MEAS_HP34401A_SCALE_MIN, MEAS_HP34401A_SCALE_MAX, 
 * MEAS_HP34401A_SCALE_DEF, MEAS_HP34401A_SCALE_AUTO_ON, 
 * MEAS_HP34401A_SCALE_AUTO_OFF can also be defined.
 *
 * resol = resolution in the above units. If resol = {MEAS_HP34401A_RESOL_MIN,
 *         MEAS_HP34401A_RESOL_MAX, MEAS_HP34401A_RESOL_DEF} then
 * the minimum, maximum, and default resolutions are used.
 *
 */

EXPORT int meas_hp34401a_set_resolution(int unit, int what, int scale, double resol) {

  char buf[MEAS_GPIB_BUF_SIZE], res[MEAS_GPIB_BUF_SIZE];

  if(hp34401a_fd[unit] == -1)
    meas_err("meas_hp34401a_set_resolution: Non-existent unit.");
  if(what < MEAS_HP34401A_MODE_VOLT_AC || what > MEAS_HP34401A_MODE_FREQUENCY)
    meas_err("meas_hp34401a_set_resolution: Invalid mode.");
  if(scale < MEAS_HP34401A_SCALE_VOLT_100MV || scale > MEAS_HP34401A_SCALE_DEF)
    meas_err("meas_hp34401a_set_resolution: Invalid scale.");
  if(resol < MEAS_HP34401A_RESOL_DEF) 
    meas_err("meas_hp34401a_set_resolution: Invalid resolution.");

  if(resol == MEAS_HP34401A_RESOL_DEF) sprintf(res, "DEF");
  else if(resol == MEAS_HP34401A_RESOL_MAX) sprintf(res, "MAX");
  else if(resol == MEAS_HP34401A_RESOL_MIN) sprintf(res, "MIN");
  else sprintf(res, "%lf", resol);
  sprintf(buf, "%s %s,%s", modes[what], scales[scale], res);
  meas_gpib_write(hp34401a_fd[unit], buf, MEAS_HP34401A_CRLF);
  return 0;
}

/*
 * Set A/D converter integration time or data analysis time window.
 *
 * what =
 *   MEAS_HP34401A_MODE_VOLT_DC:    voltage DC
 *   MEAS_HP34401A_MODE_RESISTANCE: resistance
 *   MEAS_HP34401A_MODE_CURRENT_DC: current DC
 *   MEAS_HP34401A_MODE_FREQUENCY:  frequency (data analysis window)
 *   MEAS_HP34401A_MODE_PERIODIC:   periodic signal (data analysis window)
 * 
 * Integration time (in units of NLPC)       Resolution (x full scale)
 * 0.02 (MEAS_HP34401A_INTEGRATION_TIME_20MNLPC)           1E-4 
 * 0.2 (MEAS_HP34401A_INTEGRATION_TIME_200MNLPC)           1E-5
 * 1 (MEAS_HP34401A_INTEGRATION_TIME_1NLPC)                3E-6
 * 10 (MEAS_HP34401A_INTEGRATION_TIME_10NLPC)              1E-6
 * 100 (MEAS_HP34401A_INTEGRATION_TIME_100NLPC)            3E-7
 * 
 * NLPC is the number of power line cycles (50 or 60 Hz).
 *
 * Note that this will naturally affect the readout resolution.
 *
 * OR
 *
 * Time analysis window length               Window length
 * MEAS_HP34401A_TIME_WINDOW_10MS                          10 ms
 * MEAS_HP34401A_TIME_WINDOW_100MS                         100 ms
 * MEAS_HP34401A_TIME_WINDOW_1S                            1 s
 *
 */

EXPORT int meas_hp34401a_set_integration_time(int unit, int what, int it) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(hp34401a_fd[unit] == -1) 
    meas_err("meas_hp34401a_set_integration_time: Non-existent unit.");
  if(what < MEAS_HP34401A_MODE_VOLT_AC || what > MEAS_HP34401A_MODE_PERIODIC || itime[what] == NULL)
    meas_err("meas_hp34401a_set_integration_time: Error setting integration time.");
  if(it < MEAS_HP34401A_INTEGRATION_TIME_20MNLPC || it > MEAS_HP34401A_INTEGRATION_TIME_100NLPC) 
    meas_err("meas_hp34401a_set_integration_time: Unknown time integration mode.");
  sprintf(buf, "%s %s", itime[what], itime_opts[it]);
  meas_gpib_write(hp34401a_fd[unit], buf, MEAS_HP34401A_CRLF);
  return 0;
}

/*
 * Auto-zero input.
 *
 * setting = MEAS_HP34401A_AUTOZERO_OFF, MEAS_HP34401A_AUTOZERO_ONCE,
 *           MEAS_HP34401A_AUTOZERO_ON.
 *
 */

EXPORT int meas_hp34401a_autozero(int unit, int setting) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(hp34401a_fd[unit] == -1) 
    meas_err("meas_hp34401a_autozero: Non-existent unit.");
  switch(setting) {
  case MEAS_HP34401A_AUTOZERO_OFF:
    sprintf(buf, "SENS:ZERO:AUTO OFF");
    break;
  case MEAS_HP34401A_AUTOZERO_ONCE:
    sprintf(buf, "SENS:ZERO:AUTO ONCE");
    break;
  case MEAS_HP34401A_AUTOZERO_ON:
    sprintf(buf, "SENS:ZERO:AUTO ON");
    break;
  default:
    meas_err("meas_hp34401a_autozero: Unknown autozero setting.");
  }
  meas_gpib_write(hp34401a_fd[unit], buf, MEAS_HP34401A_CRLF);
  return 0;
}

/*
 * Set measurement mode.
 *
 * mode = 
 *   MEAS_HP34401A_MODE_VOLT_AC:    voltage AC
 *   MEAS_HP34401A_MODE_VOLT_DC:    voltage DC
 *   MEAS_HP34401A_MODE_RESISTANCE: resistance
 *   MEAS_HP34401A_MODE_CURRENT_AC: current AC
 *   MEAS_HP34401A_MODE_CURRENT_DC: current DC
 *   MEAS_HP34401A_MODE_FREQUENCY:  frequency
 *   MEAS_HP34401A_MODE_PERIODIC:   periodic
 *
 */

EXPORT int meas_hp34401a_set_mode(int unit, int what) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(hp34401a_fd[unit] == -1)
    meas_err("meas_hp34401a_set_mode: Non-existent unit.");
  if(what < MEAS_HP34401A_MODE_VOLT_AC || what > MEAS_HP34401A_MODE_FREQUENCY)
    meas_err("meas_hp34401a_set_mode: Invalid mode.");
  sprintf(buf, "%s", setmodes[what]);
  meas_gpib_write(hp34401a_fd[unit], buf, MEAS_HP34401A_CRLF);
  return 0;
}

/*
 * Read sample from the instrument. This uses the settings
 * that were sent to the instrument previously.
 *
 * The calling sequence: initiate_read(); sleep; complete_read();
 *
 */


EXPORT int meas_hp34401a_initiate_read(int unit) {
  
  if(hp34401a_fd[unit] == -1)
    meas_err("meas_hp34401a_initiate_read: Non-existent unit.");
  meas_gpib_write(hp34401a_fd[unit], "READ?", MEAS_HP34401A_CRLF);
  return 0;
}

EXPORT double meas_hp34401a_complete_read(int unit) {

  char buf[MEAS_GPIB_BUF_SIZE];

  if(hp34401a_fd[unit] == -1)
    meas_err("meas_hp34401a_complete_read: Non-existent unit.");
  meas_gpib_read(hp34401a_fd[unit], buf);
  return atof(buf);
}

/*
 * Read sample from the instrument. The instrument predetermines the best settings.
 * The previous settings may be changed.
 *
 */

EXPORT double meas_hp34401a_read_auto(int unit) {
  
  char buf[MEAS_GPIB_BUF_SIZE];

  if(hp34401a_fd[unit] == -1) 
    meas_err("meas_hp34401a_read_auto: Non-existent unit.");
  meas_gpib_write(hp34401a_fd[unit], "MEAS?", MEAS_HP34401A_CRLF);
  meas_gpib_read(hp34401a_fd[unit], buf);
  return atof(buf);
}

/*
 * Set trigger source.
 *
 * source =
 *   MEAS_HP34401A_TRIGGER_BUS:         software trigger
 *   MEAS_HP34401A_TRIGGER_IMMEDIATE:   internal trigger
 *   MEAS_HP34401A_TRIGGER_EXTERNAL:    external trigger
 *
 */

EXPORT int meas_hp34401a_set_trigger_source(int unit, int source){

  char buf[MEAS_GPIB_BUF_SIZE];

  if(hp34401a_fd[unit] == -1)
    meas_err("meas_hp34401a_set_trigger_source: Non-existent unit.");
  if(source < MEAS_HP34401A_TRIGGER_BUS || source > MEAS_HP34401A_TRIGGER_EXTERNAL)
    meas_err("meas_hp34401a_set_trigger_source: Invalid trigger source.");
  sprintf(buf, "TRIG:SOUR %s", trigger_sources[source]);
  meas_gpib_write(hp34401a_fd[unit], buf, MEAS_HP34401A_CRLF);  
  return 0;
}

/*
 * Set trigger delay.
 *
 * delay = trigger delay time in seconds. Set to MEAS_HP34401A_TRIGGER_AUTO to 
 * enable the autmatic mode.
 *
 */

EXPORT int meas_hp34401a_set_trigger_delay(int unit, double delay){

  char buf[MEAS_GPIB_BUF_SIZE];

  if(hp34401a_fd[unit] == -1) 
    meas_err("meas_hp34401a_set_trigger_delay: Non-existent unit.");
  if(delay == MEAS_HP34401A_TRIGGER_AUTO) {
    meas_gpib_write(hp34401a_fd[unit], "TRIG:DEL:AUTO ON", MEAS_HP34401A_CRLF);   
    return 0;
  }
  if(delay < 0.0 || delay > 3600.0)
    meas_err("meas_hp34401a_set_trigger_delay: Invalid trigger delay.");
  /* setting a delay manually disables automatic trigger delay */
  sprintf(buf, "TRIG:DEL:%le", delay);
  meas_gpib_write(hp34401a_fd[unit], buf, MEAS_HP34401A_CRLF);  
  return 0;
}

/*
 * Set trigger sample count (i.e. how many samples the instrument will collect / trigger).
 *
 * count = number of counts.
 *
 */

EXPORT int meas_hp34401a_set_trigger_sample_count(int unit, int count) {
  
  char buf[MEAS_GPIB_BUF_SIZE];

  if(hp34401a_fd[unit] == -1)
    meas_err("meas_hp34401a_set_trigger_sample_count: Non-existent unit.");
  if(count < 0 || count > 50000) 
    meas_err("meas_hp34401a_set_trigger_sample_count: Invalid count for triggering.");
  sprintf(buf, "SAMP:COUNT %d", count);
  meas_gpib_write(hp34401a_fd[unit], buf, MEAS_HP34401A_CRLF);  
  return 0;
}

/*
 * Set trigger count (i.e. how many triggers the instrument will collect before returning to idle state).
 *
 * count = number of counts.
 *
 */

EXPORT int meas_hp34401a_set_trigger_count(int unit, int count) {
  
  char buf[MEAS_GPIB_BUF_SIZE];

  if(hp34401a_fd[unit] == -1) 
    meas_err("meas_hp34401a_set_trigger_count: Non-existent unit.");
  if(count < 0 || count > 50000)
    meas_err("meas_hp34401a_set_trigger_count: Invalid count for triggering.");
  sprintf(buf, "TRIG:COUNT %d", count);
  meas_gpib_write(hp34401a_fd[unit], buf, MEAS_HP34401A_CRLF);  
  return 0;
}

#endif /* GPIB */
