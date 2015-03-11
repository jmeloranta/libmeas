/* 
 * DG535 (Master clock):
 * A/B -> Minilite trigger (Flash lamp)
 * C/D -> BNC565 ext trigger in
 * 
 * BNC565 (slave):
 * A -> Diode flash signal
 * B -> TTL trigger for DMK 23U445
 *
 * Note: Minilite q-switch is set by an external delay generator
 *       Not controlled by computer. The internal q-swtich timer sucks!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <meas/meas.h>

#define MINILITE_FIRE_DELAY 179.8E-6   /* 189.9E-6 */
#define DG535  16
#define BNC565 15

#define CAMERA 0
#define FORMAT 1    /* Y16 from DMK 23U445 */
#define RESOL  0
#define CAMERA_DELAY 10.0E-6    /* TODO: Check this (was 4E-6) */

unsigned char *rgb, *y16;

static void sig_handler(int x) {

  meas_bnc565_run(0, MEAS_BNC565_STOP);
  meas_dg535_run(0, MEAS_DG535_STOP);
  exit(0);
}

int main(int argc, char **argv) {

  double tstep, cur_time, t0, diode_drive, diode_length, diode_delay;
  char filebase[512], filename[512];
  int cd, diode_npulses, nimg = 0, width, height, one = 1, zero = 0, gain, exposure;
  size_t frame_size;
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
  printf("Camera gain (0, 3039): ");
  scanf(" %d", &gain);

  printf("Running... press ctrl-c to stop.\n");

  meas_bnc565_init(0, 0, BNC565);
  meas_dg535_init(0, 0, DG535);

  meas_bnc565_run(0, MEAS_BNC565_STOP);
  meas_dg535_run(0, MEAS_DG535_STOP);

  /* DG535 is the master clock at 10 Hz */
  meas_dg535_trigger(0, MEAS_DG535_TRIG_INT, 10.0, 0, MEAS_DG535_IMP_50);

  /* Minilite triggering */
  meas_dg535_set(0, MEAS_DG535_CHA, MEAS_DG535_T0, 0.0, 4.0, MEAS_DG535_POL_INV, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHB, MEAS_DG535_CHA, 10E-6, 4.0, MEAS_DG535_POL_INV, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHAB, 0, 0.0, 4.0, MEAS_DG535_POL_INV, MEAS_DG535_IMP_50);

  /* BNC565 triggering signal from DG353 */
  meas_dg535_set(0, MEAS_DG535_CHC, MEAS_DG535_T0, MINILITE_FIRE_DELAY + t0 - CAMERA_DELAY, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHD, MEAS_DG535_CHC, 10E-6, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHCD, 0, 0.0, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);

  /* BNC565 to external trigger */
  meas_bnc565_trigger(0, MEAS_BNC565_TRIG_EXT, 2.0, MEAS_BNC565_TRIG_RISE);

  /* Camera triggering */
  meas_bnc565_set(0, MEAS_BNC565_CHB, MEAS_BNC565_T0, 0.0, 10E-6, 5.0, MEAS_BNC565_POL_NORM);   /* This has to go first (slow) */
  meas_bnc565_mode(0, MEAS_BNC565_CHB, MEAS_BNC565_MODE_BURST, 1, diode_npulses, diode_delay);

  /* Diode triggering */
  meas_bnc565_set(0, MEAS_BNC565_CHA, MEAS_BNC565_T0, CAMERA_DELAY, diode_length, diode_drive, MEAS_BNC565_POL_NORM);
  meas_bnc565_mode(0, MEAS_BNC565_CHA, MEAS_BNC565_MODE_BURST, diode_npulses, diode_npulses, diode_delay);

  meas_bnc565_run(0, MEAS_BNC565_RUN); /* start unit */
  meas_dg535_run(0, MEAS_DG535_RUN); /* start unit */

  cd = meas_video_open("/dev/video0", 2);
  frame_size = meas_video_set_format(cd, FORMAT, RESOL);
  width = meas_video_get_width(cd);
  height = meas_video_get_height(cd);
  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, width, height, 0, "video");
  
  if(!(rgb = (unsigned char *) malloc(width * height * 3))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(y16 = (unsigned char *) malloc(frame_size))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Trigger Mode"), &one); /* External trigger */
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Trigger Delay"), &zero); /* Immediate triggering, no delay */
  exposure = 100;   /* 1 msec in units of 10 microsec */
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Exposure (Absolute)"), &exposure);
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Gain (dB/100)"), &gain);
  meas_video_info_controls(cd);
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

  meas_video_start(cd);
  for(cur_time = t0; ; cur_time += tstep) {
    printf("Diode delay = %le s.\n", cur_time);
    meas_dg535_set(0, MEAS_DG535_CHC, MEAS_DG535_T0, MINILITE_FIRE_DELAY + cur_time - CAMERA_DELAY, 4.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
    meas_video_read(cd, y16, 1);
    meas_image_y16_to_rgb3(y16, rgb, width, height);
    meas_graphics_update_image(0, rgb);
    meas_graphics_update();
    if(filebase[0] != '0') {
      if(tstep != 0.0) 
	sprintf(filename, "%s-%le.pgm", filebase, diode_delay);
      else
	sprintf(filename, "%s-%le-%d.pgm", filebase, diode_delay, nimg);
      if(!(fp = fopen(filename, "w"))) {
	fprintf(stderr, "Error writing file.\n");
	exit(1);
      }
      meas_image_y16_to_pgm(fp, y16, width, height);
      fclose(fp);
    }
  }
  meas_video_stop(cd);
}
