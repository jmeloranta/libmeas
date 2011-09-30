/*
 * Command line driver for experiments (UCI).
 *
 * Usage: exp file.exp
 *
 * Graphics using xmgrace.
 *
 * Instruments: pressure (Baratron from SRS242 ch#1/GPIB),
 *              temperature (itc503/RS232), 2 X delay generator (dg535/GPIB),
 *              SR245 (GPIB), dye laser (FL3001/GPIB), PI-MAX2 gated
 *              CCD camera (via PVCAM library).
 *
 * Delay generator 1 (master clock):
 * 
 * T0      = Delay generator 2 trigger in
 * NEG(AB) = Flash lamp of Surelite-II
 * NEQ(CD) = Q-switch of Surelite-II
 *
 * Delay generator 2:
 *
 * Trigger in = T0 from delay generator 1.
 * A, B, AB, etc = For triggering the excimer laser. A = trigger signal
 * from the back panel (10x voltage).
 * C, D = Hook up to PI-MAX2 CCD gating unit (by pass the onboard 
 * delay generator; special cable).
 * CD   = Hook up to Ext. Sync. in ST-133 controller
 *
 * SR245:
 * 
 * CH1 = Possibly optogalvanic (see example.exp)
 * CH2 = Possibly reference diode for the dye laser (see example.exp)
 * CH8 = Pressure gauge.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include "uci.h"

int main(int argc, char **argv) {

  struct experiment *p;

  if(argc != 2) {
    fprintf(stderr, "Usage: exp file.exp\n");
    exit(1);
  }

  if(!(p = exp_read(argv[1]))) {
    fprintf(stderr, "Error in defining experiment.\n");
    exit(1);
  }

  exp_init(p);
  
  exp_setup(p);

  exp_run(p);

  if(exp_save_data(p) == -1)
    fprintf(stderr, "Warning: unable to save file.\n");

  return 0;
}
