#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <meas/meas.h>

#define GPIB_ID_WAVETEK80 11
#define GPIB_ID_HP34401   22

/* wavetek calibration to 16.8 Kohm load (instead of 50 ohm) */

double voltage_calibration(double x) {

  return (x - 0.017666) / 2.0469;
}

double x[65536], y[65535];
int samples = 0;

int main(int argc, char **argv) {

  double scan_begin, scan_end, scan_step, cur, val, width;
  unsigned long scan_delay;

  /* Use the function generator to sweep the voltage for the potentiostat */
  meas_wavetek80_init(0, 0, GPIB_ID_WAVETEK80);
  /* Use the multimeter to read the voltage (current) from the potentiostat */
  meas_hp34401a_init(0, 0, GPIB_ID_HP34401);
  
  meas_wavetek80_output_mode(0, MEAS_WAVETEK80_OUTPUT_DISABLED);

  fprintf(stderr, "Enter starting voltage (V): ");
  scanf(" %le", &scan_begin);
  fprintf(stderr, "Enter end voltage (V): ");
  scanf(" %le", &scan_end);
  width = scan_end - scan_begin;
  if(fabs(scan_begin) > 2.0 || fabs(scan_end) > 2.0) {
    fprintf(stderr, "Scan exceeds +- 2.0 V!\n");
    exit(1);
  }
  fprintf(stderr, "Enter scan step (V): ");
  scanf(" %le", &scan_step);
  fprintf(stderr, "Enter scan delay (ms): ");
  scanf(" %u", &scan_delay);

  /* Initialize graphics */
  meas_graphics_init(1, NULL, NULL);
  meas_graphics_xscale(0, scan_begin, scan_end, width / 10.0, width / 50.0);
  meas_graphics_yscale(0, -1.0, 1.0, 0.25, 0.1);
  meas_graphics_xtitle(0, "Potential (V)");
  meas_graphics_ytitle(0, "Current (arb)");

  /* set up wavetek */
  meas_wavetek80_operating_mode(0, MEAS_WAVETEK80_MODE_NORMAL);
  meas_wavetek80_trigger_mode(0, MEAS_WAVETEK80_TRIGGER_CONTINUOUS);
  meas_wavetek80_control_mode(0, MEAS_WAVETEK80_CONTROL_OFF);
  meas_wavetek80_output_waveform(0, MEAS_WAVETEK80_WAVEFORM_DC);
  meas_wavetek80_set_output_level(0, 0.0);
  /* setup hp-34401 */
  meas_hp34401a_set_mode(0, MEAS_HP34401A_MODE_VOLT_DC);
  meas_hp34401a_set_resolution(0, MEAS_HP34401A_MODE_VOLT_DC, MEAS_HP34401A_SCALE_VOLT_10V, MEAS_HP34401A_RESOL_MAX);
  meas_hp34401a_set_integration_time(0, MEAS_HP34401A_MODE_VOLT_DC, MEAS_HP34401A_INTEGRATION_TIME_20MNLPC);
  meas_hp34401a_set_impedance(0, MEAS_HP34401A_IMPEDANCE_AUTO_ON);    /* 10 GOhm */
  meas_hp34401a_autozero(0, MEAS_HP34401A_AUTOZERO_ON);           /* auto 0 V level */

  /* Enable wavetek */
  meas_wavetek80_output_mode(0, MEAS_WAVETEK80_OUTPUT_NORMAL);

  /* sweep forward */
  for(cur = scan_begin; cur <= scan_end; cur += scan_step, samples++) {
    /* set voltage */
    meas_wavetek80_set_output_level(0, voltage_calibration(cur));
    usleep((useconds_t) (1000 * scan_delay/2));
    meas_hp34401a_initiate_read(0);
    usleep((useconds_t) (1000 * scan_delay/2));
    val = meas_hp34401a_complete_read(0);
    x[samples] = cur;
    y[samples] = val;
    meas_graphics_update2d(0, 0, 5, x, y, samples);
    meas_graphics_update();
    meas_graphics_autoscale(0);
    printf("%le %le\n", cur, val);
  }

  /* sweep back */
  for(cur = scan_end; cur >= scan_begin; cur -= scan_step, samples++) {
    /* set voltage */
    meas_wavetek80_set_output_level(0, voltage_calibration(cur));
    usleep((useconds_t) (1000 * scan_delay/2));
    meas_hp34401a_initiate_read(0);
    usleep((useconds_t) (1000 * scan_delay/2));
    val = meas_hp34401a_complete_read(0);
    x[samples] = cur;
    y[samples] = val;
    meas_graphics_update2d(0, 0, 5, x, y, samples);
    meas_graphics_update();
    meas_graphics_autoscale(0);
    printf("%le %le\n", cur, val);
  }
  meas_wavetek80_output_mode(0, MEAS_WAVETEK80_OUTPUT_DISABLED);
}
