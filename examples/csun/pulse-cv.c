/*
 * Pulsed CV experiment (in msec range).
 * 
 * Connections:
 * wavetek out -> {scope, boxcar trigger in, potentiostat sweep in}
 * boxcar gate out -> scope
 * boxcar average out -> multimeter input
 *
 */

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <meas/meas.h>

#define GPIB_ID_WAVETEK80 11
#define GPIB_ID_HP34401   22

double x[65536], y[65535];

int main(int argc, char **argv) {

  double pot_red, pot_neutral, pot_ox, val, bkg;
  unsigned long i, j, time_red, time_neutral, time_ox, ncyc, averages;

  /* Use the function generator to sweep the voltage for the potentiostat */
  meas_wavetek80_init(0, 0, GPIB_ID_WAVETEK80);
  /* Use the multimeter to read the voltage (current) from the potentiostat */
  meas_hp34401a_init(0, 0, GPIB_ID_HP34401);
 
  meas_wavetek80_output_mode(0, MEAS_WAVETEK80_OUTPUT_DISABLED);

  fprintf(stderr, "Enter reduction potential (V): ");
  scanf(" %le", &pot_red);
  if(fabs(pot_red) > 2.0) {
    fprintf(stderr, "Potential too large.\n");
    exit(1);
  }
  fprintf(stderr, "Enter reduction time (ms): ");
  scanf(" %u", &time_red);

  fprintf(stderr, "Enter neutral potential (V): ");
  scanf(" %le", &pot_neutral);
  if(fabs(pot_neutral) > 2.0) {
    fprintf(stderr, "Potential too large.\n");
    exit(1);
  }
  fprintf(stderr, "Enter neutral time (ms): ");
  scanf(" %u", &time_neutral);

  fprintf(stderr, "Enter oxidation potential (V): ");
  scanf(" %le", &pot_ox);
  if(fabs(pot_ox) > 2.0) {
    fprintf(stderr, "Potential too large.\n");
    exit(1);
  }
  fprintf(stderr, "Enter oxidation time (ms): ");
  scanf(" %u", &time_ox);
  fprintf(stderr, "Enter the number of cycles to run: ");
  scanf(" %u", &ncyc);
  fprintf(stderr, "Number of averages per point: ");
  scanf(" %u", &averages);

  /* Initialize graphics */
  meas_graphics_init(1, NULL, NULL);
  meas_graphics_xtitle(0, "Time (s)");
  meas_graphics_ytitle(0, "Current (arb)");

  /* setup hp-34401 */
  meas_hp34401a_set_mode(0, MEAS_HP34401A_MODE_VOLT_DC);
  meas_hp34401a_set_resolution(0, MEAS_HP34401A_MODE_VOLT_DC, MEAS_HP34401A_SCALE_VOLT_10V, MEAS_HP34401A_RESOL_MAX);
  meas_hp34401a_set_integration_time(0, MEAS_HP34401A_MODE_VOLT_DC, MEAS_HP34401A_INTEGRATION_TIME_20MNLPC);
  meas_hp34401a_set_impedance(0, MEAS_HP34401A_IMPEDANCE_AUTO_ON);    /* 10 GOhm */
  meas_hp34401a_autozero(0, MEAS_HP34401A_AUTOZERO_ON);           /* auto 0 V level */

  /* setup wavetek */
  meas_wavetek80_operating_mode(0, MEAS_WAVETEK80_MODE_NORMAL);
  meas_wavetek80_trigger_mode(0, MEAS_WAVETEK80_TRIGGER_CONTINUOUS);
  meas_wavetek80_control_mode(0, MEAS_WAVETEK80_CONTROL_OFF);
  meas_wavetek80_output_waveform(0, MEAS_WAVETEK80_WAVEFORM_DC);
  meas_wavetek80_set_output_level(0, 0.0);
  meas_wavetek80_output_mode(0, MEAS_WAVETEK80_OUTPUT_NORMAL);

  meas_misc_set_reftime();
  for (i = 0; i < ncyc; i++) {
    val = 0.0;
    for (j = 0; j < averages; j++) {
      meas_wavetek80_set_output_level(0, pot_ox);
      usleep((useconds_t) (1000 * time_ox));
      meas_wavetek80_set_output_level(0, pot_red);
      usleep((useconds_t) (1000 * time_red));
      meas_wavetek80_set_output_level(0, pot_neutral);
      usleep((useconds_t) (1000 * time_neutral));
      /* the boxcar is holding the data to be read */
      meas_hp34401a_initiate_read(0);
      sleep(1);
      val -= meas_hp34401a_complete_read(0); /* invert because of boxcar */
    }
    val /= (double) averages;
    if(!i) {
      bkg = val;
      val = 0.0;
    } else val -= bkg;
    x[i] = meas_misc_get_reftime();
    y[i] = val;
    meas_graphics_update2d(0, 0, 5, x, y, i);
    meas_graphics_update();
    meas_graphics_autoscale(0);
    printf("%le %le\n", x[i], val);
    fflush(stdout);
  }
  meas_wavetek80_output_mode(0, MEAS_WAVETEK80_OUTPUT_DISABLED);
}
