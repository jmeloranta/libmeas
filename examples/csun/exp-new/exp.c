/*
 * Command line driver for experiments.
 *
 * Usage: exp file.exp
 *
 * Instruments: delay generator (GPIB), HP 34401A, Lambda Physik dye laser.
 *
 * Instruments connected to the delay generator: Surelite-II, Minilite-II, 
 * Boxcar SR250.
 *
 * NOTE: The delay generator is the master clock!
 * 
 * Connections on delay generator:
 * 
 * A -> Surelite-II flash lamp trigger (negative).
 * B -> Surelite-II Q-switch trigger (negative).
 * C -> Minilite-II combined flash lamp / Q-switch trigger (jitter).
 * D -> SR250 boxcar external trigger input.
 * Ext trig. -> N/C.
 * 
 * SR250 boxcar:
 *
 * Signal input <- PMT signal.
 * Trigger input <- Delay generator D.
 * Last sample output -> Input port of HP 34401A.
 * Busy output -> N/C.
 * 
 * NOTE: To use 50 Ohm input from PMT use terminators at signal out as well as
 * in the oscilloscope end. For high impedance mode, remove both terminators.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "csun.h"

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

  exp_init();
  
  exp_setup(p);

  exp_run(p);

  if(exp_save_data(p) == -1)
    fprintf(stderr, "Warning: unable to save file.\n");

  return 0;
}
