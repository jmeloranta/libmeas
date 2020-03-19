#include <stdio.h>
#include <meas/meas.h>

/* BCMXX pin number for GPIO output */
#define PIN 27 

/* Hardware (defined) of software (not defined) timer? Hardware timer has resolution of 1 microsec. and software timer some tens of nanosec. */
#define HWTIMER

/* Delay time: in microseconds for HWTIMER and nanoseconds otherwise */
#define DELAY 1000

int main(int argc, char **argv) {

  int i;

  meas_gpio_open();
#ifndef HWTIMER
  meas_gpio_cpu_scaling(1); // Turn CPU clock frequency scaling as this will affect the delay loop
#endif
  meas_gpio_mode(PIN, 2); // set the GPIO pin (BCM) for output
  meas_gpio_interrupt(0);  // Disable interrupts (computer will freeze while running but timings are more consistent)
  for (i = 0; i < 10000000; i++) {
    meas_gpio_write_fast_off(PIN);
#ifdef HWTIMER
    meas_gpio_timer(DELAY);
#else
    meas_gpio_timer2(DELAY);
#endif
    meas_gpio_write_fast_on(PIN); 
#ifdef HWTIMER
    meas_gpio_timer(DELAY);
#else
    meas_gpio_timer2(DELAY);
#endif
  }
  meas_gpio_interrupt(1); // enable interrupts again
}
