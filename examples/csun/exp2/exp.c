/*
 * Command line driver for experiments.
 *
 * Usage: exp file.exp
 *
 * Graphics using xmgrace.
 *
 * Instruments: pressure (RS232), temperature (RS232), delay generator (GPIB),
 * monochromator (RS232), diode array detector/spectrograph (USB),
 * SR245 (GPIB), dye laser (RS232).
 *
 * Instruments on A/D, D/A and delay generator: Surelite-II, Minilite-II,
 * PMT, SR250.
 *
 * NOTE: Boxcar (SR250) is the master clock! This way we avoid the requirement
 * for multithreaded programming and additional need for digital output ports.
 * The delay scale setting on SR250 is assumed to be 100 microsec!
 * 
 * Connections on delay generator:
 * 
 * A -> Surelite-II flash lamp trigger (negative).
 * B -> Surelite-II Q-switch trigger (negative).
 * C -> Minilite-II flash lamp trigger.
 * D -> Minilite-II Q-switch trigger.
 * Ext trigger <- Busy signal from boxcar (SR250)
 * 
 * Diode array (Newport IS):
 *
 * Ext sync. in <- Boxcar SR250 busy output.
 *
 * CCD (Newport Matrix):
 *
 * No external triggering (free run - requires long exposure times)
 *
 * SR245 DAQ Board (synchronous mode):
 *
 * Port 1 <- SR250 last sample output.
 * Port 7 -> SR250 (back panel) external delay control.
 * Port 8 -> Newport IS Sync in (does NOT work well)
 * Digital port 1 <- SR250 busy output (trigger for DAQ).
 *
 * SR250 boxcar:
 *
 * Signal input <- PMT signal.
 * Trigger input <- Empty (Boxcar is the master clock; must be set to 10 Hz!)
 * Last sample output -> A/D Port #1 in SR245.
 * Busy output -> Digital port 1 in SR245 & SYNC.IN in diode array.
 * External delay control (back panel) -> Port 7 in SR245.
 * 
 * Newport IS: Sync in -> SR 245 Port 8. This does not work well. However,
 * the reprate of the experiment (10 Hz) is the same as the shortes exposure
 * time -> we are bound to see the emissions induced by the laser. So, for
 * now, keep synchronous = 0 for Newport IS! 
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
