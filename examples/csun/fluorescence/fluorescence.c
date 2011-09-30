/*
 * Command line driver for simple fluorescence experiment.
 *
 * Usage: exp file.exp
 *
 * Instruments: delay generator (GPIB), HP 34401A, monochromator (dk240).
 *
 * Instruments connected to the delay generator: Minilite-II, Boxcar SR250.
 *
 * NOTE: The delay generator is the master clock!
 * 
 * Connections on delay generator:
 * 
 * A -> Minilite flash lamp.
 * B -> Minilite Q-switch lamp.
 * C -> Boxcar.
 * D -> not used.
 * Ext trig. -> not used.
 * 
 * SR250 boxcar:
 *
 * Signal input <- PMT signal.
 * Trigger input <- Delay generator C.
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
