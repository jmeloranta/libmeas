#include <stdio.h>
#include <meas/meas.h>

int main(int argc, char **argv) {

  int i;

  meas_gpio_open();
  meas_gpio_mode(27, 2); // output
  meas_gpio_interrupt(0);
  for (i = 0; i < 1000000; i++) {
    meas_gpio_write(27, 0); meas_gpio_timer(10);
    meas_gpio_write(27, 1); meas_gpio_timer(10);
  }
  meas_gpio_interrupt(1);
}
