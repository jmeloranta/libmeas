/* 
 * BNC565 (slave):
 * A -> Surelite flash lamp
 * B -> Surelite q-switch
 * C -> Camera shutter
 * D -> Backlight diode
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <meas/meas.h>

#define SURELITE_FIRE_DELAY 101.1E-6
#define SURELITE_QSWITCH  300E-6

#define DIODE_PULSE_LENGTH 90E-9
#define DIODE_PULSE_VOLTAGE 12.0

#define CAMERA_BEFORE 300E-6
#define CAMERA_AFTER  100E-3

#define BNC565 15

/* #define VEHO 1 /* For veho USB camera */

#ifdef VEHO
#define FORMAT 0
#define RESOL 0
#else
#define FORMAT 1    /* Y16 from DMK 23U445 */
#define RESOL  0
#endif

unsigned char *rgb, *buffer;

static void exit_handler(void) {

  meas_bnc565_run(0, MEAS_BNC565_STOP);
}

static void sig_handler(int x) {

  meas_bnc565_run(0, MEAS_BNC565_STOP);
  exit(0);
}

int main(int argc, char **argv) {

  double tstep, delay, t0, reprate, cur_time;
  char filebase[512], filename[512];
  int cd, nimg = 0, width, height, one = 1, zero = 0, gain, exposure, i, j;
  size_t frame_size;
  FILE *fp, *fp2;

  signal(SIGINT, &sig_handler);
  printf("Enter output file name (0 = no save): ");
  scanf("%s", filebase);
  printf("Enter T0 (microsec): ");
  scanf("%le", &t0);
  t0 *= 1E-6;
  printf("Enter time step (microsec): ");
  scanf(" %le", &tstep);
  tstep *= 1E-6;
#ifdef VEHO
  printf("Camera brightness (-16 to 16): ");
#else
  printf("Camera gain (0, 3039): ");
#endif
  scanf(" %d", &gain);
  printf("Repetition rate (max 10 Hz): ");
  scanf(" %le", &reprate);
  if(reprate > 10.0 || reprate < 0.0) {
    fprintf(stderr, "Incorrect repetition rate.\n");
    exit(1);
  }
  
  printf("Running... press ctrl-c to stop.\n");

  meas_bnc565_open(0, 0, BNC565);

  meas_bnc565_run(0, MEAS_BNC565_STOP);

  /* BNC565 is the master */
  meas_bnc565_trigger(0, MEAS_BNC565_TRIG_INT, reprate, 0);

  /* Surelite triggering (channels A & B) */
  meas_bnc565_set(0, MEAS_BNC565_CHA, MEAS_BNC565_T0, 0.0, 10E-6, 5.0, MEAS_BNC565_POL_INV);
  meas_bnc565_mode(0, MEAS_BNC565_CHA, MEAS_BNC565_MODE_CONTINUOUS, 0, 0, 0);
  meas_bnc565_set(0, MEAS_BNC565_CHB, MEAS_BNC565_CHA, SURELITE_QSWITCH, 10E-6, 5.0, MEAS_BNC565_POL_INV);
  meas_bnc565_mode(0, MEAS_BNC565_CHB, MEAS_BNC565_MODE_CONTINUOUS, 0, 0, 0);

  /* Camera shutter triggering (C) */
  meas_bnc565_set(0, MEAS_BNC565_CHC, MEAS_BNC565_T0, 0.0, CAMERA_AFTER, 4.0, MEAS_BNC565_POL_NORM);
  meas_bnc565_mode(0, MEAS_BNC565_CHC, MEAS_BNC565_MODE_CONTINUOUS, 0, 0, 0);

  /* Backlight diode triggering (D) */
  delay = SURELITE_QSWITCH - SURELITE_FIRE_DELAY + t0;
  if(delay < 0.0) {
    fprintf(stderr, "Warning: delay < 0.0 !\n");
    delay = 0.0;
  }
  meas_bnc565_set(0, MEAS_BNC565_CHD, MEAS_BNC565_T0, delay, DIODE_PULSE_LENGTH, DIODE_PULSE_VOLTAGE, MEAS_BNC565_POL_NORM);
  meas_bnc565_mode(0, MEAS_BNC565_CHD, MEAS_BNC565_MODE_CONTINUOUS, 0, 0, 0);
  
  meas_bnc565_run(0, MEAS_BNC565_RUN); /* start unit */

  sleep(4);

  cd = meas_video_open("/dev/video0", 2);
  frame_size = meas_video_set_format(cd, FORMAT, RESOL);
  width = meas_video_get_width(cd);
  height = meas_video_get_height(cd);
  meas_graphics_open(0, MEAS_GRAPHICS_IMAGE, width, height, 0, "video");
  
  if(!(rgb = (unsigned char *) malloc(width * height * 3))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(buffer = (unsigned char *) malloc(frame_size))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
#ifdef VEHO
  meas_video_set_control(cd, meas_video_get_control_id(cd, "exposure_auto"), &zero); /* manual exposure */
  meas_video_exposure_time(cd, 1250);  /* exposure time (removes the scanline artefact; 1250 seems good) */
  meas_video_set_brightness(cd, gain);
#else
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Trigger Mode"), &zero); /* External trigger */
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Trigger Delay"), &zero); /* Immediate triggering, no delay */
  exposure = 100;   /* 1 msec in units of 10 microsec */
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Exposure (Absolute)"), &exposure);
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Gain (dB/100)"), &gain);
#endif
  if(filebase[0] != '0') {
    sprintf(filename, "%s.info", filebase);
    if(!(fp = fopen(filename, "w"))) {
      fprintf(stderr, "Can't open file for writing.\n");
      exit(1);
    }
    fprintf(fp, "%.15le %.15le %.15le %d\n", t0, tstep, reprate, gain);
    fclose(fp);
  }

  printf("Hit ctrl-C to exit...\n");

  atexit(&exit_handler);

  meas_video_start(cd);
  meas_video_flush(cd);
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Trigger Mode"), &one); /* External trigger */
  meas_video_info_controls(cd);
  for(cur_time = t0; ; cur_time += tstep) {
    /* Timings */
    delay = SURELITE_QSWITCH - SURELITE_FIRE_DELAY + cur_time;
    if(delay < 0.0) {
      fprintf(stderr, "Warning: delay < 0.0 !\n");
      delay = 0.0;
    }
    
    /* Surelite */
    meas_bnc565_set(0, MEAS_BNC565_CHA, MEAS_BNC565_T0, 0.0, 10.0E-6, 5.0, MEAS_BNC565_POL_INV); /* negative logic / TTL */

    /* camera shutter */
    meas_bnc565_set(0, MEAS_BNC565_CHC, MEAS_BNC565_T0, 0.0, CAMERA_AFTER, 5.0, MEAS_BNC565_POL_NORM);

    /* backlight diode */
    meas_bnc565_set(0, MEAS_BNC565_CHD, MEAS_BNC565_T0, delay, DIODE_PULSE_LENGTH, DIODE_PULSE_VOLTAGE, MEAS_BNC565_POL_NORM);

    meas_video_flush(cd);

    meas_video_read(cd, buffer, 1);
#ifdef VEHO
    meas_image_yuv422_to_rgb3(buffer, rgb, width, height);
#else
    meas_image_y16_to_rgb3(buffer, rgb, width, height);
#endif
    meas_graphics_update_image(0, rgb);
    meas_graphics_update();

    printf("Time = %le\n", cur_time); fflush(stdout);
    if(filebase[0] != '0') {
      if(tstep != 0.0)
	sprintf(filename, "%s-%le.pgm", filebase, cur_time);
      else
	sprintf(filename, "%s-%le-%d.pgm", filebase, cur_time, nimg++);
      if(!(fp = fopen(filename, "w"))) {
	fprintf(stderr, "Cannot write data to file.\n");
	exit(1);
      }
#ifdef VEHO
      meas_image_rgb3_to_ppm(fp, rgb, width, height);
#else
      meas_image_y16_to_pgm(fp, buffer, width, height);
#endif
      fclose(fp);
    }
  }
  meas_video_stop(cd);
}
