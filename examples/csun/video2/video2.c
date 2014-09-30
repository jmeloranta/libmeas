/*
 * Read frames from PI-MAX and display on screen.
 *
 */

/* Lasers on BNC565 */

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

unsigned char ri[NX * NY], gi[NX * NY], bi[NX * NY];
unsigned char ro[SCALE * SCALE * NX * NY], go[SCALE * SCALE * NX * NY], bo[SCALE * SCALE * NX * NY];
unsigned short img[NX * NY];

int main(int argc, char **argv) {

  double tstep, delay, gate, t0, qsw;
  char filebase[512], filename[512];
  int fd, aves;
  FILE *fp;
  unsigned char gain;

  printf("Enter output file name (0 = no save): ");
  scanf("%s", filebase);
  printf("Enter Q-switch (microsec; ex. 300): ");
  scanf(" %le", &qsw);
  printf("Enter number of averages (1 - ...): ");
  scanf("%d", &aves);
  printf("Enter T0 (ns): ");
  scanf("%le", &t0);
  printf("Enter time step (ns): ");
  scanf(" %le", &tstep);
  printf("Running... press ctrl-c to stop.\n");
  tstep *= 1E-9;
  t0 *= 1E-9;
  qsw *= 1E-6;

  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, SCALE * 640, SCALE * 480, 0, "video");

  /* Set up delay generators */
  meas_bnc565_init(0, 0, BNC565);   /* sure lite on A & B */
  meas_dg535_init(0, 0, DG535);     /* PI-MAX on A, B, AB */
  /* 1. Ablation laser (Surelite) */
  meas_bnc565_set(0, 1, 0, 0.0, 10.0E-6, 5.0, 1);        // Ch A
  meas_bnc565_set(0, 2, 1, qsw, 10.0E-6, 5.0, 1);        // Ch B
  meas_bnc565_enable(0, 1, 1);
  meas_bnc565_enable(0, 2, 1);
  meas_bnc565_mode(0, 1, 0, 0, 0); /* regular single shot */
  meas_bnc565_mode(0, 2, 0, 0, 0); /* regular single shot */  
  meas_bnc565_trigger(0, MEAS_BNC565_TRIG_INT, 10.0, 0); /* 10 Hz (master) */
  /* 2. CCD */
  meas_dg535_set(0, MEAS_DG535_CHA, MEAS_DG535_T0, 1E-6 + CCD_DELAY, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHB, MEAS_DG535_CHA, gate, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
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
  printf("Intensifier gain (0 - 255): ");
  scanf(" %le", &gain);
  meas_pi_max_gain(gain);   /* 0 - 255 */
  printf("Intensifier gate (s): ");
  scanf(" %le", &gate);
  meas_pi_max_init(TEMP);
  meas_pi_max_speed_index(1);
  meas_pi_max_roi(0, NX-1, 1, 0, NY-1, 1);

  for(delay = t0; ; delay += tstep) {
    printf("Delay = %le ns.\n", delay*1E9);
    meas_dg535_set(0, MEAS_DG535_CHA, MEAS_DG535_T0, delay + CCD_DELAY, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
    meas_pi_max_read(aves, img);
    meas_graphics_convert_img_to_rgb(img, NX, NY, ri, gi, bi, 0);
    meas_graphics_scale_rgb(ri, gi, bi, 512, 512, SCALE, ro, go, bo);
    meas_graphics_update_image(0, ro, go, bo);
    meas_graphics_update();
    sprintf(filename, "%s-%le.img", filebase, delay);
    if(filename[0] != '0') {
      if(!(fp = fopen(filename, "w"))) {
	fprintf(stderr, "Error writing file.\n");
	exit(1);
      }
      fwrite((void *) ri, sizeof(unsigned char) * 640 * 480, 1, fp);
      fwrite((void *) gi, sizeof(unsigned char) * 640 * 480, 1, fp);
      fwrite((void *) bi, sizeof(unsigned char) * 640 * 480, 1, fp);
      fclose(fp);
    }
  }
}
