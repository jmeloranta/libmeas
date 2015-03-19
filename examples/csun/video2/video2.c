/*
 * Read frames from PI-MAX and display on screen.
 *
 */

/* Lasers on BNC565 A/B = surelite, C/D = minilite */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <meas/meas.h>

#define TEMP (-20.0)
#define SURELITE_DELAY 0.380E-6
#define CCD_DELAY 30E-9    /* approx. this varies with the qswitch slightly */
#define DG535  16
#define BNC565 15

#define SCALE 2
#define NX 512
#define NY 512

unsigned char rgbi[3 * NX * NY];
unsigned char rgbo[3 * SCALE * SCALE * NX * NY];
unsigned char y16[2 * NX * NY];

int main(int argc, char **argv) {

  double tstep, delay, gate, t0, qsw;
  char filebase[512], filename[512];
  int fd, aves;
  FILE *fp;
  int gain;

  printf("Enter output file name (0 = no save): ");
  scanf("%s", filebase);
  printf("Enter Q-switch (microsec; ex. 300): ");
  scanf(" %le", &qsw);
  printf("Enter number of averages (1 - ...): ");
  scanf(" %d", &aves);
  printf("Enter T0 (ns): ");
  scanf(" %le", &t0);
  printf("Enter time step (ns): ");
  scanf(" %le", &tstep);
  tstep *= 1E-9;
  t0 *= 1E-9;
  qsw *= 1E-6;

  /* Set up delay generators */
  meas_bnc565_open(0, 0, BNC565);   /* sure lite on A & B */
  meas_dg535_open(0, 0, DG535);     /* PI-MAX on A, B, AB */
  /* 1. Ablation laser (Surelite) */
  meas_bnc565_set(0, MEAS_BNC565_CHA, 0, 0.0, 10.0E-6, 5.0, MEAS_BNC565_POL_INV);        // Ch A
  meas_bnc565_set(0, MEAS_BNC565_CHB, 0, qsw, 10.0E-6, 5.0, MEAS_BNC565_POL_INV);        // Ch B
  meas_bnc565_enable(0, MEAS_BNC565_CHA, MEAS_BNC565_CHANNEL_ENABLE);
  meas_bnc565_enable(0, MEAS_BNC565_CHB, MEAS_BNC565_CHANNEL_ENABLE);
  meas_bnc565_mode(0, MEAS_BNC565_CHA, MEAS_BNC565_MODE_SINGLE_SHOT, 0, 0, 0); /* regular single shot */
  meas_bnc565_mode(0, MEAS_BNC565_CHB, MEAS_BNC565_MODE_SINGLE_SHOT, 0, 0, 0); /* regular single shot */  
  meas_bnc565_trigger(0, MEAS_BNC565_TRIG_INT, 10.0, 0); /* 10 Hz (master) */
  /* 2. CCD */
  meas_dg535_set(0, MEAS_DG535_CHA, MEAS_DG535_T0, 1E-6 + CCD_DELAY, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHAB, 0, 0.0, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_trigger(0, MEAS_DG535_TRIG_EXT, 1.0, MEAS_DG535_TRIG_FALL, MEAS_DG535_IMP_50); /* triggered by BNC565 */

  if(filename[0] != '0') {
    sprintf(filename, "%s.info", filebase);
    if(!(fp = fopen(filename, "w"))) {
      fprintf(stderr, "Can't open file for writing.\n");
      exit(1);
    }
    fprintf(fp, "%d %.15le %.15le\n", aves, t0, tstep);
    fclose(fp);
  }
  /* Init PI-MAX */
  meas_pi_max_open(TEMP);
  printf("Intensifier gain (0 - 255): ");
  scanf(" %d", &gain);
  meas_pi_max_gain(gain);   /* 0 - 255 */
  printf("Intensifier gate (s): ");
  scanf(" %le", &gate);
  meas_dg535_set(0, MEAS_DG535_CHB, MEAS_DG535_CHA, gate, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);

  meas_pi_max_speed_index(1);
  meas_pi_max_roi(0, NX-1, 1, 0, NY-1, 1);

  meas_graphics_open(0, MEAS_GRAPHICS_IMAGE, SCALE * NX, SCALE * NY, 0, "video");
  printf("Running... press ctrl-c to stop.\n");
  meas_bnc565_run(0, MEAS_BNC565_RUN);
  meas_dg535_run(0, MEAS_DG535_RUN);
  for(delay = t0; ; delay += tstep) {
    printf("Delay = %le ns.\n", delay*1E9);
    meas_dg535_set(0, MEAS_DG535_CHA, MEAS_DG535_T0, delay + CCD_DELAY, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
    meas_pi_max_read(aves, y16);
    meas_graphics_y16_to_rgb3(y16, rgbi, NX, NY);
    meas_graphics_scale_rgb3(rgbi, NX, NY, SCALE, rgbo);
    meas_graphics_update_image(0, rgbo);
    meas_graphics_update();
    sprintf(filename, "%s-%le.img", filebase, delay);
    if(filename[0] != '0') {
      if(!(fp = fopen(filename, "w"))) {
	fprintf(stderr, "Error writing file.\n");
	exit(1);
      }
      meas_image_y16_to_pgm(fp, y16, NX, NY);
      fclose(fp);
    }
  }
  meas_bnc565_run(0, MEAS_BNC565_STOP);
  meas_dg535_run(0, MEAS_DG535_STOP);
}
