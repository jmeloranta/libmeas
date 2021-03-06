/* 
 * DG535 (Master clock):
 * A -> Minilite trigger (Flash lamp)
 * B -> Minilite trigger (Q-switch)
 * C -> CCD shutter (exposure)
 * D -> BNC565 ext trigger in (diode trgger)
 * 
 * BNC565 (slave):
 * A -> Diode flash signal
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
  meas_dg535_run(0, MEAS_DG535_STOP);
}

int main(int argc, char **argv) {

  double tstep, cur_time, t0, diode_drive, diode_length, diode_delay, reprate;
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
  printf("Enter diode drive (V): ");
  scanf(" %le", &diode_drive);
  printf("Diode pulse length (microsec.): ");
  scanf(" %le", &diode_length);
  diode_length *= 1E-6;
  printf("Number of pulses: ");
  scanf(" %d", &diode_npulses);
  printf("Delay between diode pulses (microsec.): ");
  scanf(" %le", &diode_delay);
  diode_delay *= 1E-6;
#ifdef VEHO
  printf("Camera brightness (-16 to 16): ");
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
  meas_dg535_set(0, MEAS_DG535_CHA, MEAS_DG535_T0, 0.0, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);

  /* Surelite Q-switch */
  meas_dg535_set(0, MEAS_DG535_CHB, MEAS_DG535_T0, QSWITCH, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  
  /* Camera shutter trigger */
  meas_dg535_set(0, MEAS_DG535_CHC, MEAS_DG535_T0, MINILITE_FIRE_DELAY - CAMERA_DELAY, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  
  /* Diode triggering */
  meas_dg535_set(0, MEAS_DG535_CHD, MEAS_DG535_T0, MINILITE_FIRE_DELAY, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  
  /* Diode triggering (from channel D of DG535) */
  meas_bnc565_set(0, MEAS_BNC565_CHA, MEAS_BNC565_T0, 0.0, diode_length, diode_drive, MEAS_BNC565_POL_NORM);
  meas_bnc565_mode(0, MEAS_BNC565_CHA, MEAS_BNC565_MODE_BURST, diode_npulses, diode_npulses, diode_delay);

  /* Clear other BNC565 channels just to be safe */
  meas_bnc565_set(0, MEAS_BNC565_CHB, MEAS_BNC565_T0, 0.0, diode_length, diode_drive, MEAS_BNC565_POL_NORM);
  meas_bnc565_mode(0, MEAS_BNC565_CHB, MEAS_BNC565_MODE_BURST, diode_npulses, diode_npulses, diode_delay);
  meas_bnc565_set(0, MEAS_BNC565_CHC, MEAS_BNC565_T0, 0.0, diode_length, diode_drive, MEAS_BNC565_POL_NORM);
  meas_bnc565_mode(0, MEAS_BNC565_CHC, MEAS_BNC565_MODE_BURST, diode_npulses, diode_npulses, diode_delay);
  meas_bnc565_set(0, MEAS_BNC565_CHD, MEAS_BNC565_T0, 0.0, diode_length, diode_drive, MEAS_BNC565_POL_NORM);
  meas_bnc565_mode(0, MEAS_BNC565_CHD, MEAS_BNC565_MODE_BURST, diode_npulses, diode_npulses, diode_delay);

  meas_bnc565_run(0, MEAS_BNC565_RUN); /* start unit */
  meas_dg535_run(0, MEAS_DG535_RUN); /* start unit */

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
    fprintf(fp, "%.15le %.15le %lf %.15le %d %.15le %d\n", t0, tstep, diode_drive, diode_length, diode_npulses, diode_delay, gain);
    fclose(fp);
  }
  printf("Hit ctrl-C to exit...\n");

  atexit(&exit_handler);

  meas_video_start(cd);
  meas_video_read(cd, buffer, 1);
  meas_video_read(cd, buffer, 1);  /* TODO: why do we need to fill buffers before switching to ext trigger? */
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Trigger Mode"), &one); /* External trigger */
  for(cur_time = t0; ; cur_time += tstep) {
    meas_video_flush(cd);   // make sure that we get the frame with current delay settings
    printf("Diode delay = %le s.\n", cur_time);fflush(stdout);
    meas_dg535_set(0, MEAS_DG535_CHD, MEAS_DG535_T0, MINILITE_FIRE_DELAY + cur_time, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
    meas_video_read(cd, buffer, 1);
#ifdef VEHO
    meas_image_yuv422_to_rgb3(buffer, rgb, width, height);
#else
    meas_image_y16_to_rgb3(buffer, rgb, width, height);
#endif
    meas_graphics_update_image(0, rgb);
    meas_graphics_update();
    if(filebase[0] != '0') {
      if(tstep != 0.0) 
	sprintf(filename, "%s-%le.pgm", filebase, cur_time);
      else
	sprintf(filename, "%s-%le-%d.pgm", filebase, cur_time, nimg++);
      if(!(fp = fopen(filename, "w"))) {
	fprintf(stderr, "Error writing file.\n");
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
