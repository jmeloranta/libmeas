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

#define MINILITE_FIRE_DELAY 189.9E-6   /* 189.9E-6 */
#define DG535  16
#define BNC565 15

#define CAMERA 0
#define FORMAT 1    /* Y16 from DMK 23U445 */
#define HEIGHT 1280
#define WIDTH 960
#define CAMERA_DELAY 4.0E-6    /* TODO: Check this */

unsigned char *rgb, *y16;

static void sig_handler(int x) {

  meas_bnc565_run(0, MEAS_BNC565_STOP);
  meas_dg535_run(0, MEAS_DG535_STOP);
  exit(0);
}

int main(int argc, char **argv) {

  double tstep, cur_time, t0, diode_drive, diode_length, diode_delay, gain;
  char filebase[512], filename[512];
  int fd, diode_npulses, nimg = 0;
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
  scanf(" %le", &gain);

  printf("Running... press ctrl-c to stop.\n");

  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, HEIGHT, WIDTH, 0, "video");

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

  /* Diode triggering */
  meas_bnc565_set(0, MEAS_BNC565_CHA, MEAS_BNC565_T0, CAMERA_DELAY, diode_length, diode_drive, MEAS_BNC565_POL_NORM);
  meas_bnc565_mode(0, MEAS_BNC565_CHA, MEAS_BNC565_MODE_BURST, diode_npulses, diode_npulses, diode_delay);

  /* Camera triggering */
  meas_bnc565_set(0, MEAS_BNC565_CHB, MEAS_BNC565_T0, 0.0, 10E-6, 5.0, MEAS_BNC565_POL_NORM);   /* This has to go first (slow) */
  meas_bnc565_mode(0, MEAS_BNC565_CHB, MEAS_BNC565_MODE_SINGLE_SHOT, 0, 0, 0);

  meas_bnc565_run(0, MEAS_BNC565_RUN); /* start unit */
  meas_dg535_run(0, MEAS_DG535_RUN); /* start unit */

  fd = meas_video_open(CAMERA);
  frame_size = meas_video_format(CAMERA, FORMAT, WIDTH, HEIGHT);
  if(!(rgb = (unsigned char *) malloc(WIDTH * HEIGHT * 3))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(y16 = (unsigned char *) malloc(frame_size))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  meas_video_set_range(CAMERA, "Trigger Mode", 1.0); /* external trigger */
  meas_video_set_range(CAMERA, "Trigger Delay", 0.0); /* Immediate triggering, no delay */
  meas_video_set_range(CAMERA, "Exposure Time (us)", 1000.0);  /* millisecond */
  meas_video_set_range(CAMERA, "Gain (dB/100)", gain);
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
    meas_video_read(CAMERA, 1, y16);
    meas_image_y16_to_rgb(y16, rgb, WIDTH, HEIGHT);
    meas_graphics_update_image(0, rgb);
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
      fwrite((void *) y16, frame_size, 1, fp);
      fclose(fp);
    }
  }
}
