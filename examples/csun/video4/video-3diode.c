/* 
 * DG535 (Master clock):
 * A -> Minilite trigger (Flash lamp; TTL high Z)
 * B -> Minilite trigger (Q-switch; TTL high Z)
 * C -> CCD shutter (exposure; TTL high Z)
 * D -> BNC565 ext trigger in (diode trgger through BNC565)
 * 
 * BNC565 (slave):
 * A -> Diode(R) flash signal (tri-color LED; up to 12 V)
 * B -> Diode(G) flash signal (tri-color LED; up to 12 V)
 * C -> Diode(B) flash signal (tri-color LED) up to 12 V)
 * D -> Not in use
 * 
 * CCD = DFK 23U445 (ImagingSource; USB3)
 *
 * NOTE: THe old BNC one channel delay gnerator is no longer needed.
 *   
 *  ---------                               -----------                -------------                ------------
 *  !       !                               !         !                !           !                !          !
 *  !  Abl  !                               ! Red LED !                ! Green LED !                ! Blue LED !    ....
 *  ! laser !   <t0+tstep> + <diode_delay1> !         ! <diode_delay2> !           ! <diode_delay3> !          !
 *  !       !                               !         !                !           !                !          !
 * --------------------------------------------------------------------------------------------------------------------------
 *  Time ->
 *
 * The first delay can be scanned in increments of tstep. The CCD (global mechanical shutter) is open during the diode sequence.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <meas/meas.h>

#define QSWITCH 180.0E-6    /* Q-switch delay */
#define MINILITE_FIRE_DELAY 180E-6 /* how long it will take to get the pulse out (depends on Q-switch) */
#define CAMERA_DELAY 100E-6 /* Time to open the shutter */
#define DG535  16
#define BNC565 15

#define VEHO 1  /* Use Veho microscope */
/* #define DFK23U445 1 /* Use Imaging Source DFK 23U445 (color) camera */

#ifdef VEHO
#define FORMAT 0    /* Only YUV422 available */
#define RESOL 0
#else
#define FORMAT 0    /* BA81 for DFK 23U445 */
#define RESOL  0
#endif

unsigned char *rgb, *red, *green, *blue, *buffer;

static void exit_handler(void) {

  meas_bnc565_run(0, MEAS_BNC565_STOP);
  meas_dg535_run(0, MEAS_DG535_STOP);
}

int main(int argc, char **argv) {

  double tstep, cur_time, t0, diode_drive1, diode_drive2, diode_drive3, diode_length, diode_delay1, diode_delay2, diode_delay3, reprate;
  char filebase[512], filename[512];
  int cd, nimg = 0, width, height, one = 1, zero = 0, gain, exposure, dev;
  size_t frame_size;
  FILE *fp;

  printf("Enter output file name (0 = no save): ");
  scanf("%s", filebase);
  printf("Enter T0 (microsec): ");    /* adjustable delay between ablation laser and the diode sequence */
  scanf("%le", &t0);
  t0 *= 1E-6;
  printf("Enter time step (microsec): ");
  scanf(" %le", &tstep);
  tstep *= 1E-6;
  printf("Enter diodeR drive (V): ");
  scanf(" %le", &diode_drive1);
  printf("Enter diodeG drive (V): ");
  scanf(" %le", &diode_drive2);
  printf("Enter diodeB drive (V): ");
  scanf(" %le", &diode_drive3);
  printf("Diode pulse length for all diodes (microsec.): ");
  scanf(" %le", &diode_length);
  diode_length *= 1E-6;
  printf("Execution sequence: <laser> <t0+delayR> <red flash> <delayG> <green flash> <delayB> <blue flash>.\n");
  printf("DelayR between laser and diode(R) (microsec.): ");
  scanf(" %le", &diode_delay1);
  diode_delay1 *= 1E-6;
  printf("DelayG between diode(R) and diode(G) (microsec.): ");
  scanf(" %le", &diode_delay2);
  diode_delay2 *= 1E-6;
  printf("DelayB between diode(G) and diode(B) (microsec.): ");
  scanf(" %le", &diode_delay3);
  diode_delay3 *= 1E-6;
#ifdef VEHO
  printf("Camera gain (-16 to 16): ");
#else
  printf("Camera gain (0, 3039): ");
#endif
  scanf(" %d", &gain);
  printf("Enter repetition rate (10 Hz): ");
  scanf(" %le", &reprate);
  if(reprate > 10.0) {
    fprintf(stderr, "Repetition rate too high.\n");
    exit(1);
  }
  printf("Video device (0, 1, 2, ...): ");
  scanf(" %d", &dev);
  
  printf("Running... press ctrl-c to stop.\n");

  meas_bnc565_open(0, 0, BNC565);
  meas_dg535_open(0, 0, DG535);

  meas_bnc565_run(0, MEAS_BNC565_STOP);
  meas_dg535_run(0, MEAS_DG535_STOP);

  /* DG535 is the master clock at 10 Hz */
  meas_dg535_trigger(0, MEAS_DG535_TRIG_INT, reprate, 0, MEAS_DG535_IMP_50);

  /* BNC565 to external trigger (slave) */
  meas_bnc565_trigger(0, MEAS_BNC565_TRIG_EXT, 2.0, MEAS_BNC565_TRIG_RISE);

  /* Surelite flash lamp */
  meas_dg535_set(0, MEAS_DG535_CHA, MEAS_DG535_T0, 0.0, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_HIGH);

  /* Surelite Q-switch */
  meas_dg535_set(0, MEAS_DG535_CHB, MEAS_DG535_T0, QSWITCH, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_HIGH);
  
  /* Camera shutter trigger */
  meas_dg535_set(0, MEAS_DG535_CHC, MEAS_DG535_T0, MINILITE_FIRE_DELAY - CAMERA_DELAY, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_HIGH);
  
  /* Diode triggering through BNC565 */
  meas_dg535_set(0, MEAS_DG535_CHD, MEAS_DG535_T0, MINILITE_FIRE_DELAY, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_HIGH);
  
  /* Diode triggering (from channel D of DG535) */
  meas_bnc565_set(0, MEAS_BNC565_CHA, MEAS_BNC565_T0, diode_delay1, diode_length, diode_drive1, MEAS_BNC565_POL_NORM);
  meas_bnc565_mode(0, MEAS_BNC565_CHA, MEAS_BNC565_MODE_CONTINUOUS, 0, 0, 0);
  meas_bnc565_set(0, MEAS_BNC565_CHB, MEAS_BNC565_CHA, diode_delay2, diode_length, diode_drive2, MEAS_BNC565_POL_NORM);
  meas_bnc565_mode(0, MEAS_BNC565_CHB, MEAS_BNC565_MODE_CONTINUOUS, 0, 0, 0);
  meas_bnc565_set(0, MEAS_BNC565_CHC, MEAS_BNC565_CHB, diode_delay3, diode_length, diode_drive3, MEAS_BNC565_POL_NORM);
  meas_bnc565_mode(0, MEAS_BNC565_CHC, MEAS_BNC565_MODE_CONTINUOUS, 0, 0, 0);

  /* Clear other BNC565 channels just to be safe -- CHD not in use */
  meas_bnc565_set(0, MEAS_BNC565_CHD, MEAS_BNC565_T0, 0.0, diode_length, diode_drive3, MEAS_BNC565_POL_NORM);
  meas_bnc565_mode(0, MEAS_BNC565_CHD, MEAS_BNC565_MODE_CONTINUOUS, 0, 0, 0.0);

  meas_bnc565_run(0, MEAS_BNC565_RUN); /* start unit */
  meas_dg535_run(0, MEAS_DG535_RUN); /* start unit */

  sprintf(filename, "/dev/video%d", dev);  
  cd = meas_video_open(filename, 2);
  meas_video_info_camera(cd);
  frame_size = meas_video_set_format(cd, FORMAT, RESOL);
  if(!frame_size) {
    fprintf(stderr, "Illegal video format.\n");
    exit(1);
  }
#ifdef VEHO
  width = meas_video_get_width(cd);
  height = meas_video_get_height(cd);
#else
  width = meas_video_get_width(cd)/2;
  height = meas_video_get_height(cd)/2;
#endif
  meas_graphics_open(0, MEAS_GRAPHICS_IMAGE, width, height, 0, "red");
  meas_graphics_open(1, MEAS_GRAPHICS_IMAGE, width, height, 0, "green");
  meas_graphics_open(2, MEAS_GRAPHICS_IMAGE, width, height, 0, "blue");
  
  if(!(rgb = (unsigned char *) malloc(width * height * 3))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(red = (unsigned char *) malloc(width * height))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(green = (unsigned char *) malloc(width * height))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(blue = (unsigned char *) malloc(width * height))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(buffer = (unsigned char *) malloc(frame_size))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
#ifdef VEHO
  meas_video_set_control(cd, meas_video_get_control_id(cd, "exposure_auto"), &zero); /* manual exposure */
  meas_video_exposure_time(cd, 1250);  /* exposure time: removes the scanline artefact (rolling shutter); 1250 seems good */
  meas_video_set_brightness(cd, gain);
#else
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Trigger Mode"), &zero); /* External trigger */
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Trigger Delay"), &zero); /* Immediate triggering, no delay */
  exposure = 100;   /* 1 msec in units of 10 microsec */
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Exposure (Absolute)"), &exposure);
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Gain (dB/100)"), &gain);
#endif
  meas_video_info_controls(cd);
  if(filebase[0] != '0') {
    sprintf(filename, "%s.info", filebase);
    if(!(fp = fopen(filename, "w"))) {
      fprintf(stderr, "Can't open file for writing.\n");
      exit(1);
    }
    fprintf(fp, "%.15le %.15le %lf %lf %lf %.15le %.15le %.15le %.15le %d\n", t0, tstep, diode_drive1, diode_drive2, diode_drive3, diode_length, diode_delay1, diode_delay2, diode_delay3, gain);
    fclose(fp);
  }
  printf("Hit ctrl-C to exit...\n");

  atexit(&exit_handler);

  meas_video_start(cd);
  meas_video_flush(cd);
  //meas_video_read(cd, buffer, 1);
  //meas_video_read(cd, buffer, 1);  /* TODO: why do we need to fill buffers before switching to ext trigger? */
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Trigger Mode"), &one); /* External trigger */
#if 0
  // TODO: Why is this hack needed? Otherwise external triggering gets stuck...
  sleep(1);
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Trigger Mode"), &zero); /* External trigger */
  sleep(1);
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Trigger Mode"), &one); /* External trigger */
  // End hack
#endif
  for(cur_time = t0; ; cur_time += tstep) {
    meas_video_flush(cd);   // make sure that we get the frame with current delay settings
    printf("Diode delay = %le s.\n", cur_time);fflush(stdout);
    meas_dg535_set(0, MEAS_DG535_CHD, MEAS_DG535_T0, MINILITE_FIRE_DELAY + cur_time, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
    meas_video_read(cd, buffer, 1);
#ifdef VEHO
    meas_image_yuv422_to_rgb(buffer, red, green, blue, meas_video_get_width(cd), meas_video_get_height(cd));
#else
    meas_image_ba81_to_rgb(buffer, red, green, blue, meas_video_get_width(cd), meas_video_get_height(cd));
#endif
    meas_image_y800_to_rgb3(red, rgb, width, height);
    meas_graphics_update_image(0, rgb);
    meas_image_y800_to_rgb3(green, rgb, width, height);
    meas_graphics_update_image(1, rgb);
    meas_image_y800_to_rgb3(blue, rgb, width, height);
    meas_graphics_update_image(2, rgb);
    meas_graphics_update();
    if(filebase[0] != '0') {
      /* red */
      if(tstep != 0.0) 
	sprintf(filename, "%s-%le-red.pgm", filebase, cur_time);
      else
	sprintf(filename, "%s-%le-%d-red.pgm", filebase, cur_time, nimg);
      if(!(fp = fopen(filename, "w"))) {
	fprintf(stderr, "Error writing file.\n");
	exit(1);
      }
      /* was y16 by mistake */
      meas_image_y800_to_pgm(fp, red, width, height);
      fclose(fp);
      /* green */
      if(tstep != 0.0) 
	sprintf(filename, "%s-%le-green.pgm", filebase, cur_time);
      else
	sprintf(filename, "%s-%le-%d-green.pgm", filebase, cur_time, nimg);
      if(!(fp = fopen(filename, "w"))) {
	fprintf(stderr, "Error writing file.\n");
	exit(1);
      }
      /* was y16 by mistake */
      meas_image_y800_to_pgm(fp, green, width, height);
      fclose(fp);
      /* blue */
      if(tstep != 0.0) 
	sprintf(filename, "%s-%le-blue.pgm", filebase, cur_time);
      else
	sprintf(filename, "%s-%le-%d-blue.pgm", filebase, cur_time, nimg++);
      if(!(fp = fopen(filename, "w"))) {
	fprintf(stderr, "Error writing file.\n");
	exit(1);
      }
      /* was y16 by mistake */
      meas_image_y800_to_pgm(fp, blue, width, height);
      fclose(fp);
    }
  }
  meas_video_stop(cd);
}
