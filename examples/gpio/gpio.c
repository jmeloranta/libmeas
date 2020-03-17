#include <stdio.h>
#include <meas/meas.h>

int main(int argc, char **argv) {

  int i;

  meas_gpio_open();
  meas_gpio_mode(27, 2); // GPIO pin 27 (BCM) to output
  meas_gpio_interrupt(0);  // Disable interrupts (computer will freeze while running but timings are more consistent)
  for (i = 0; i < 10000000; i++) {
    meas_gpio_write_fast_off(27); meas_gpio_timer(10); // set 27 to low and delay for 10 microsec.
    meas_gpio_write_fast_on(27); meas_gpio_timer(10); // set 27 to high and delay for 10 microsec.
  }
  meas_gpio_interrupt(1); // enable interrupts again
}
