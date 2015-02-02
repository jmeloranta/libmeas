/*
 * Read frames from PI-MAX and display on screen.
 *
 * Use minilite-II for ablation and diode for backlight.
 *
 * DG535 triggers BNC565 (CD -> Ext trig in).
 * Diode on BNC565 channel A (burst mode).
 * Minilite-II triggered from DG565 A & B.
 *
 * The camera opens 10 ns before the first diode flash
 * and closes 10 ns after the last flash.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <meas/meas.h>
#include <signal.h>

#define TEMP (-20.0)
#define MINILITE_FIRE_DELAY 189.9E-6
#define CCD_DELAY 30E-9    /* CCD trigger delay */

#define SCALE 1
#define NX 512
#define NY 512

#define DG535  16
#define BNC565 15

unsigned char ri[NX * NY], gi[NX * NY], bi[NX * NY];
unsigned char ro[SCALE * SCALE * NX * NY], go[SCALE * SCALE * NX * NY], bo[SCALE * SCALE * NX * NY];
unsigned short img[NX * NY];

static void sig_handler(int x) {
  
  meas_bnc565_run(0, MEAS_BNC565_STOP);
  meas_dg535_run(0, MEAS_DG535_STOP);
  exit(0);
}

int main(int argc, char **argv) {

  double tstep, cur_time, t0, diode_drive, diode_length, diode_delay, ccd_time;
  char filebase[512], filename[512];
  int fd, diode_npulses, nimg = 0, gain;
  FILE *fp;
  
  printf("Enter output file name (0 = no save): ");
  scanf("%s", filebase);
  printf("Enter T0 (microsec): ");
  scanf("%le", &t0);
  t0 *= 1E-6;
  printf("Enter time step (microsec): ");
  scanf(" %le", &tstep);
  tstep *= 1E-6;
  printf("Diode drive voltage (V): ");
  scanf(" %le", &diode_drive);
  printf("Diode pulse length (microsec.): ");
  scanf(" %le", &diode_length);
  diode_length *= 1E-6;
  printf("Number of pulses: ");
  scanf(" %d", &diode_npulses);
  printf("Delay between diode pulses (microsec.): ");
  scanf(" %le", &diode_delay);
  diode_delay *= 1E-6;
  printf("Camera gain (0 to 255): ");
  scanf(" %d", &gain);
  ccd_time = diode_npulses * diode_length + (diode_npulses - 1) * diode_delay + 20E-9;   /* + 10 ns at start and end */

  printf("Running... press ctrl-c to stop.\n");
  
  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, NY*SCALE, NX*SCALE, 0, "video");
  
  meas_bnc565_init(0, 0, BNC565);
  meas_dg535_init(0, 0, DG535);
  
  /* DG535 is the master clock at 10 Hz */
  meas_dg535_trigger(0, MEAS_DG535_TRIG_INT, 10.0, 0, MEAS_DG535_IMP_50);
  
  /* Minilite triggering */
  meas_dg535_set(0, MEAS_DG535_CHA, MEAS_DG535_T0, 0.0, 4.0, MEAS_DG535_POL_INV, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHB, MEAS_DG535_CHA, 10E-6, 4.0, MEAS_DG535_POL_INV, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHAB, 0, 0.0, 4.0, MEAS_DG535_POL_INV, MEAS_DG535_IMP_50);
  
  /* BNC565 triggering */
  meas_dg535_set(0, MEAS_DG535_CHC, MEAS_DG535_T0, MINILITE_FIRE_DELAY + t0, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHD, MEAS_DG535_CHC, 10E-6, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHCD, 0, 0.0, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  
  /* BNC565 to external trigger */
  meas_bnc565_trigger(0, MEAS_BNC565_TRIG_EXT, 2.0, MEAS_BNC565_TRIG_RISE);
  
  /* Diode flash */
  meas_bnc565_set(0, MEAS_BNC565_CHA, MEAS_BNC565_T0, CCD_DELAY - 10E-9, diode_length, diode_drive, MEAS_BNC565_POL_NORM); /* let CCD open 10 ns before */
  meas_bnc565_mode(0, MEAS_BNC565_CHA, MEAS_BNC565_MODE_BURST, diode_npulses, diode_npulses, diode_delay);

  /* PI-MAX on BNC-565 B/C/D */
  meas_bnc565_set(0, MEAS_BNC565_CHB, MEAS_BNC565_T0, 0.0, ccd_time, 4.0, MEAS_BNC565_POL_NORM);   /* only rising edge used */
  meas_bnc565_mode(0, MEAS_BNC565_CHB, MEAS_BNC565_MODE_SINGLE_SHOT, 0, 0, 0);
  meas_bnc565_set(0, MEAS_BNC565_CHC, MEAS_BNC565_CHB, ccd_time, 100E-6, 4.0, MEAS_BNC565_POL_NORM);   /* only rising edge used */
  meas_bnc565_mode(0, MEAS_BNC565_CHA, MEAS_BNC565_MODE_SINGLE_SHOT, 0, 0, 0);
  meas_bnc565_set(0, MEAS_BNC565_CHD, MEAS_BNC565_T0, 0.0, ccd_time, 4.0, MEAS_BNC565_POL_NORM);   /* on for the sync time */
  meas_bnc565_mode(0, MEAS_BNC565_CHA, MEAS_BNC565_MODE_SINGLE_SHOT, 0, 0, 0);
  
  meas_bnc565_run(0, MEAS_BNC565_RUN); /* start unit */
  meas_dg535_run(0, MEAS_DG535_RUN); /* start unit */
  
  /* Init PI-MAX */
  meas_pi_max_init(TEMP);
  meas_pi_max_speed_index(1);
  meas_pi_max_roi(0, NX-1, 1, 0, NY-1, 1);
  meas_pi_max_gain(gain);

  if(filebase[0] != '0') {
    sprintf(filename, "%s.info", filebase);
    if(!(fp = fopen(filename, "w"))) {
      fprintf(stderr, "Can't open file for writing.\n");
      exit(1);
    }
    fprintf(fp, "%.15le %.15le %lf %.15le %d %.15le\n", t0, tstep, diode_drive, diode_length, diode_npulses, diode_delay);
    fclose(fp);
  }
  printf("Hit ctrl-C to exit...\n");

  signal(SIGINT, &sig_handler);

  for(cur_time = t0; ; cur_time += tstep) {
    printf("Diode delay = %le s.\n", cur_time);
    meas_dg535_set(0, MEAS_DG535_CHC, MEAS_DG535_T0, MINILITE_FIRE_DELAY + cur_time, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
    meas_pi_max_read(1, img);
    meas_graphics_convert_img_to_rgb(img, NX, NY, ri, gi, bi, 0);
    meas_graphics_scale_rgb(ri, gi, bi, NX, NY, SCALE, ro, go, bo);
    meas_graphics_update_image(0, ro, go, bo);
    meas_graphics_update();
    if(filebase[0] != '0') {
      if(tstep != 0.0) 
	sprintf(filename, "%s-%le.img", filebase, diode_delay);
      else
	sprintf(filename, "%s-%le-%d.img", filebase, diode_delay, nimg);
      if(!(fp = fopen(filename, "w"))) {
	fprintf(stderr, "Error writing file.\n");
	exit(1);
      }
      fwrite((void *) ri, sizeof(unsigned char) * NX * NY, 1, fp);
      fwrite((void *) gi, sizeof(unsigned char) * NX * NY, 1, fp);
      fwrite((void *) bi, sizeof(unsigned char) * NX * NY, 1, fp);
      fclose(fp);
      if(tstep != 0.0) 
	sprintf(filename, "%s-%le.ppm", filebase, diode_delay);
      else
	sprintf(filename, "%s-%le-%d.ppm", filebase, diode_delay, nimg++);
      if(!(fp = fopen(filename, "w"))) {
	fprintf(stderr, "Error writing file.\n");
	exit(1);
      }
      meas_video_rgb_to_ppm(fp, ri, gi, bi, NX, NY);
      fclose(fp);
    }
  }
}
