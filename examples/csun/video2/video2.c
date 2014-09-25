/*
 * Read frames from PI-MAX and display on screen.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <meas/meas.h>
#include <math.h>

#define TEMP (-20.0)
#define QSWITCH 300E-6

#define SURELITE_DELAY 0.380E-6
#define CCD_DELAY 120E-9

#define DG535  16
#define BNC565 15

#define SCALE 2

#define NX 512
#define NY 512

int main(int argc, char **argv) {

  unsigned short *img;
  unsigned char *ri, *gi, *bi, *ro, *go, *bo, gain;
  int i;
  double delay, gate;

  meas_pi_max_init(TEMP);
  printf("Intensifier gain (0 - 255): ");
  scanf(" %le", &gain);
  meas_pi_max_gain(gain);   /* 0 - 255 */
  printf("Intensifier gate (s): ");
  scanf(" %le", &gate);
  printf("Delay between ablation and CCD (s): ");
  scanf(" %le", &delay);
  meas_pi_max_speed_index(1);
  meas_pi_max_roi(0, NX-1, 1, 0, NY-1, 1);
  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, NX*SCALE, NY*SCALE, 0, "CCD image");
  if(!(img = (unsigned short *) malloc(sizeof(unsigned short) * NX * NY))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(ri = (unsigned char *) malloc(sizeof(unsigned char) * NX * NY))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(gi = (unsigned char *) malloc(sizeof(unsigned char) * NX * NY))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(bi = (unsigned char *) malloc(sizeof(unsigned char) * NX * NY))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(ro = (unsigned char *) malloc(SCALE * SCALE * sizeof(unsigned char) * NX * NY))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(go = (unsigned char *) malloc(SCALE * SCALE * sizeof(unsigned char) * NX * NY))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(bo = (unsigned char *) malloc(SCALE * SCALE * sizeof(unsigned char) * NX * NY))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }

  /* Set up delay generators */
  meas_bnc565_init(0, 0, BNC565);
  meas_dg535_init(0, 0, DG535);
  /* 1. Ablation laser (Surelite) */
  meas_bnc565_set(0, 1, 0, 0.0, 10.0E-6, 5.0, 1);        // Ch A
  meas_bnc565_set(0, 2, 1, QSWITCH, 10.0E-6, 5.0, 1);    // Ch B
  meas_bnc565_enable(0, 1, 1);
  meas_bnc565_enable(0, 2, 1);
  meas_bnc565_mode(0, 1, 0, 0, 0); /* regular single shot */
  meas_bnc565_mode(0, 2, 0, 0, 0); /* regular single shot */  
  meas_bnc565_trigger(0, MEAS_BNC565_TRIG_INT, 10.0, 0); /* 10 Hz */
  /* 2. CCD */
  delay += CCD_DELAY;    // approx. 100 ns delay between Fixed Sync Out and actual laser output + intl. trig. delay of CCD
  meas_dg535_set(0, MEAS_DG535_CHA, MEAS_DG535_T0, delay, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHB, MEAS_DG535_CHA, gate, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHAB, 0, 0.0, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  /* Run without the laser */
  //  meas_dg535_trigger(0, MEAS_DG535_TRIG_EXT, 1.0, MEAS_DG535_TRIG_FALL, MEAS_DG535_IMP_50);
  meas_dg535_trigger(0, MEAS_DG535_TRIG_INT, 10.0, MEAS_DG535_TRIG_FALL, MEAS_DG535_IMP_50);

  /* Run delay generators */
  meas_dg535_run(0, 1);
  meas_bnc565_run(0, 1);
  /* End delay generators */

  while(1) {
    meas_pi_max_read(1, img);
    meas_graphics_convert_img_to_rgb(img, 512, 512, ri, gi, bi, 0);
    meas_graphics_scale_rgb(ri, gi, bi, 512, 512, SCALE, ro, go, bo);
    meas_graphics_update_image(0, ro, go, bo);
    meas_graphics_update();
    sleep(1);
  }

  //  meas_graphics_close();
  meas_pi_max_close();
  free(img); free(ri); free(gi); free(bi); free(ro); free(go); free(bo);
  return 0;
}
