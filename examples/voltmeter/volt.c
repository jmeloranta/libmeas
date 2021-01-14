/*
 * Read data from two voltmeters (HP 34401A) as a function of time.
 *
 */

#include <stdio.h>
#include <math.h>
#include <meas/meas.h>

#define HPA 22
//#define HPB 14

int main(int argc, char **argv) {

  double va, vb;

  meas_hp34401a_open(0, 0, HPA);
//  meas_hp34401a_open(1, 0, HPB);
  meas_misc_set_reftime();
  while(1) {
    va = meas_hp34401a_read_auto(0);
//    vb = meas_hp34401a_read_auto(1);
//    printf("%le %le %le\n", meas_misc_get_reftime(), va, vb);
    printf("%le %le\n", meas_misc_get_reftime(), va);
  }
}
