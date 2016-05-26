/* 
 * BNC565 (slave):
 * A -> Surelite flash lamp (visualization pulse)
 * B -> Surelite q-switch
 * C -> Minilite flash lamp (heating pulse)
 * D -> Minilite Q-switch
 *
 * DG535 (master clock):
 * A\B -> BNC 565 ext trigger
 * C\D -> CCD shutter
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <meas/meas.h>

#define MINILITE_FIRE_DELAY 0.160E-6
#define MINILITE_QSWITCH  182.52E-6
#define SURELITE_FIRE_DELAY 0.314E-6
#define SURELITE_QSWITCH  180E-6
#define CAMERA_DELAY 20.0E-6    /* TODO: Check this (was 4E-6) */

#define AVE 1

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

  double tstep, cur_time, t0, reprate, tot_minilite, tot_surelite, diff, intens, cur_press;
  char filebase[512], filename[512];
  int cd, nimg = 0, width, height, one = 1, zero = 0, gain, exposure, i, j;
  size_t frame_size;
  FILE *fp, *fp2;

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

  /*open pdr2000 port => pressure sensor*/
  if(meas_pdr2000_open(0, "/dev/ttyUSB0") == -1) {
    fprintf(stderr, "Cannot open port.\n");
    exit(1);
  }

  meas_bnc565_open(0, 0, BNC565);
  meas_dg535_open(0, 0, DG535);

  meas_bnc565_run(0, MEAS_BNC565_STOP);
  meas_dg535_run(0, MEAS_DG535_STOP);

  /* DG535 is the master clock at 10 Hz */
  meas_dg535_trigger(0, MEAS_DG535_TRIG_INT, reprate, 0, MEAS_DG535_IMP_50);

  /* BNC565 is the slave */
  meas_bnc565_trigger(0, MEAS_BNC565_TRIG_EXT, 2.0, MEAS_BNC565_TRIG_RISE);

  /* Surelite triggering (channels A & B) -- visualization (532 nm) */
  meas_bnc565_set(0, MEAS_BNC565_CHA, MEAS_BNC565_T0, 0.0, 10E-6, 5.0, MEAS_BNC565_POL_INV);
  meas_bnc565_mode(0, MEAS_BNC565_CHA, MEAS_BNC565_MODE_CONTINUOUS, 0, 0, 0);
  meas_bnc565_set(0, MEAS_BNC565_CHB, MEAS_BNC565_CHA, SURELITE_QSWITCH, 10E-6, 5.0, MEAS_BNC565_POL_INV);
  meas_bnc565_mode(0, MEAS_BNC565_CHB, MEAS_BNC565_MODE_CONTINUOUS, 0, 0, 0);

  /* Minilite triggering (channels C & D) -- heating (355 nm) */
  meas_bnc565_set(0, MEAS_BNC565_CHC, MEAS_BNC565_T0, 0.0, 10E-6, 5.0, MEAS_BNC565_POL_NORM);
  meas_bnc565_mode(0, MEAS_BNC565_CHC, MEAS_BNC565_MODE_CONTINUOUS, 0, 0, 0);
  meas_bnc565_set(0, MEAS_BNC565_CHD, MEAS_BNC565_CHC, MINILITE_QSWITCH, 10E-6, 5.0, MEAS_BNC565_POL_NORM);
  meas_bnc565_mode(0, MEAS_BNC565_CHD, MEAS_BNC565_MODE_CONTINUOUS, 0, 0, 0);
  
  /* BNC565 external triggering from DG535 (from A\B) */
  meas_dg535_set(0, MEAS_DG535_CHA, MEAS_DG535_T0, 0.0, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHB, MEAS_DG535_CHA, 10E-6, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHAB, 0, 0.0, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);  

  /* CCD camera shutter triggering from DG535 (from C\D) */
  meas_dg535_set(0, MEAS_DG535_CHC, MEAS_DG535_T0, 0.0, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHD, MEAS_DG535_CHC, 1500E-6, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
  meas_dg535_set(0, MEAS_DG535_CHCD, 0, 0.0, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);  

  meas_bnc565_run(0, MEAS_BNC565_RUN); /* start unit */
  meas_dg535_run(0, MEAS_DG535_RUN);

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
  meas_video_info_controls(cd);
  if(filebase[0] != '0') {
    sprintf(filename, "%s.info", filebase);
    if(!(fp = fopen(filename, "w"))) {
      fprintf(stderr, "Can't open file for writing.\n");
      exit(1);
    }
    fprintf(fp, "%.15le %.15le %.15le %d\n", t0, tstep, reprate, gain);
    fclose(fp);
  }
  if(!(fp2 = fopen("intens.dat", "w"))) {
    fprintf(stderr, "Can't open intens.dat\n");
    exit(1);
  }
  printf("Hit ctrl-C to exit...\n");

  atexit(&exit_handler);

  meas_video_start(cd);
  meas_video_read(cd, buffer, 1);   /* we seem to be getting couple of blank frames in the very beginning ??? (TODO) */
  meas_video_read(cd, buffer, 1);
  meas_video_set_control(cd, meas_video_get_control_id(cd, "Trigger Mode"), &one); /* External trigger */
  for(cur_time = t0; ; cur_time += tstep) {
    /* read pressure */
    cur_press = meas_pdr2000_read(0, 1);
    if(cur_press < 0.0) {
      fprintf(stderr, "Cannot read data.\n");
      meas_pdr2000_close(0); /*close port*/
      exit(1);
    }
    meas_video_flush(cd);
    printf("Flash laser delay = %le s.\n", cur_time);
    /* Timings */
    tot_minilite = MINILITE_FIRE_DELAY + MINILITE_QSWITCH;
    tot_surelite = SURELITE_FIRE_DELAY + SURELITE_QSWITCH;    
    diff = tot_minilite - tot_surelite + cur_time;
    
    if(diff > 0.0) { /* minilite goes first */
       /* Surelite */
      meas_bnc565_set(0, MEAS_BNC565_CHA, MEAS_BNC565_T0, diff, 10.0E-6, 5.0, MEAS_BNC565_POL_INV); /* negative logic / TTL */
      /* Minilite */
      meas_bnc565_set(0, MEAS_BNC565_CHC, MEAS_BNC565_T0, 0.0, 10.0E-6, 5.0, MEAS_BNC565_POL_NORM); /* positive logic / TTL */
    } else { /* Surelite goes first */
      /* Surelite */
      meas_bnc565_set(0, MEAS_BNC565_CHA, MEAS_BNC565_T0, 0.0, 10.0E-6, 5.0, MEAS_BNC565_POL_INV); /* negative logic / TTL */
      /* Minilite */
      meas_bnc565_set(0, MEAS_BNC565_CHC, MEAS_BNC565_T0, -diff, 10.0E-6, 5.0, MEAS_BNC565_POL_NORM); /* positive logic / TTL */
    }
    /* camera shutter */
    diff = fabs(diff) - 10.0E-6; /* 10 us to open the shutter */
    if(diff < 0.0) diff = 0.0;
    meas_dg535_set(0, MEAS_DG535_CHC, MEAS_DG535_T0, diff, 4.0, 0.0, MEAS_DG535_POL_NORM, MEAS_DG535_IMP_50);
    /* end timings */

    intens = 0.0;
    for (j = 0; j < AVE; j++) {
      meas_video_read(cd, buffer, 1);
      for (i = 0; i < 2 * width * height; i++)
	intens += (buffer[i+1] * 256 + buffer[i]);
    }
    fprintf(fp2, "%le %le\n", cur_time, intens);
    fflush(fp2);
#ifdef VEHO
    meas_image_yuv422_to_rgb3(buffer, rgb, width, height);
#else
    meas_image_y16_to_rgb3(buffer, rgb, width, height);
#endif
    meas_graphics_update_image(0, rgb);
    meas_graphics_update();

    if(filebase[0] != '0') {
      if(tstep != 0.0)
	sprintf(filename, "%s-%he-%4.2d.pgm", filebase, cur_time, cur_press);
      else
	sprintf(filename, "%s-%he-%d-%4.2d.pgm", filebase, cur_time, nimg++, cur_press);
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
  fclose(fp2);
  meas_video_stop(cd);
}
