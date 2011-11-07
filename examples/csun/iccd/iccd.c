/*
 * Command line driver for experiments.
 *
 * Usage: iccd file.exp
 *
 * Instruments: delay generator #1 (lasers, master clock) BNC565,
 * delay generator #2 (ICCD, inv. trig. by fixed sync out from surelite) DG535,
 * dye laser (RS232).
 *
 * Delay generator #1: A(surelite-II flash, neg), B(surelite-II Q-switch, neg),
 *                     C(minilite-II flash, pos), D(minilite-II Q-switch, pos).
 * Delay generator #2: A & B (special cable to ICCD), AB to ICCD controller
 *                     (ext sync), Ext trig to flash lamp sync out in surelite.
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
  
  exp_setup(p, 1);

  exp_run(p);

  if(exp_save_data(p) == -1)
    fprintf(stderr, "Warning: unable to save file.\n");

  return 0;
}
