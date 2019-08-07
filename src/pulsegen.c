/*
 * Simple pulse generator using Raspberry Pi (using gpio.c).
 *
 * To make it more complete, allow overlap between pulses on different channels. At the moment only one channel can fire at given
 * time. 
 *
 * Channel to BCM GPIO pin map for Raspberry Pi 3 (use gpio readall to see):
 *
 * Channel		BCM pin #
 * MEAS_PULSE_CH1	GPIO4 (7)
 * MEAS_PULSE_CH2	GPIO5 (29)
 * MEAS_PULSE_CH3	GPIO6 (31)
 * MEAS_PULSE_CH4	GPIO12 (32)
 * MEAS_PULSE_CH5	GPIO13 (33)
 * MEAS_PULSE_CH6	GPIO16 (36)
 * MEAS_PULSE_CH7	GPIO17 (11)
 * MEAS_PULSE_CH8	GPIO18 (12)
 * MEAS_PULSE_CH9	GPIO19 (35)
 * MEAS_PULSE_CH10	GPIO20 (38)
 * MEAS_PULSE_CH11	GPIO21 (40)
 * MEAS_PULSE_CH12	GPIO22 (41)
 * MEAS_PULSE_CH13	GPIO23 (16)
 * MEAS_PULSE_CH14	GPIO24 (18)
 * MEAS_PULSE_CH15	GPIO25 (22)
 * MEAS_PULSE_CH16	GPIO26 (37)
 * MEAS_PULSE_CH17	GPIO27 (13)
 *
 * Note: these are NOT wiringPi numberings.
 * 
 * It is useful to construct a box that has opto-isolators and proper BNC connectors for connecting cables.
 *
 * The time resolution is 1 microsecond +- 0.1 microsec.
 *
 */

#ifdef RPI

#include <stdio.h>
#include <stdlib.h>
#include "pulsegen.h"
#include "misc.h"

/*
 * Initialize the pulse generator.
 *
 */

EXPORT int meas_pulse_open() {

  return meas_gpio_open();  
}

/*
 * Execute pulse sequence.
 *
 * pulses  = Array of pulse information (struct meas_pulse).
 * npulses = Number of pulses in array (int).
 *
 * No return value.
 *
 */

EXPORT void meas_pulse_exec(struct meas_pulse *pulses, int npulses) {

  int i;

  /* Make sure that all accessed channels are in output mode */
  for(i = 0; i < npulses; i++) {
    meas_gpio_mode(pulses[i].channel, 2);
    meas_gpio_write(pulses[i].channel, 0);
  }

  meas_gpio_interrupt(0); /* Disable interrupts */

  for(i = 0; i < npulses; i++) {
    meas_gpio_write(pulses[i].channel, 1);
    meas_gpio_timer(pulses[i].length);
    meas_gpio_write(pulses[i].channel, 0);
    meas_gpio_timer(pulses[i].delay);
  }

  meas_gpio_interrupt(1); /* Enable interrupts */
}

#endif /* RPI */
