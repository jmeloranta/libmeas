#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <meas/meas.h>

#define GPIB_INTERFACE /* (problems with GPIB slowing down with long measurements) */

#define MAXP 65535

#define BOARD  0
#define GPIBID 22
#define DEVICE "/dev/ttyS0"

int main(int argc, char **argv) {

  int fd, i, loop_amt, sleepsec, sleepnsec;
  char buf[2048];
  double xval[MAXP], yval[MAXP];
  char graphix;
  float sleept;
  FILE *fp;

  if(argc != 2) {
    fprintf(stderr, "Usage: eap outputfile.dat\n");
    exit(1);
  }
  fprintf(stderr, "Output file = %s.\n", argv[1]);
  if(!(fp = fopen(argv[1], "w"))) {
    fprintf(stderr, "Can't open file %s for writing.\n", argv[1]);
    exit(1);
  }
  fprintf(stderr,"Delay (s) (float)? ");
  scanf("%f", &sleept);
  sleepsec = (int) sleept;
  sleepnsec = (int) ((sleept - (float) sleepsec) * 1e9);
  fprintf(stderr,"Number of scans? ");
  scanf("%d", &loop_amt);
  fprintf(stderr,"Graphical output (y/n) ");
  scanf("%s", &graphix);
  if(graphix != 'y' && graphix != 'n')
    {
      printf("Invalid input, exiting\n");
      exit(1);
    }

  if(graphix == 'y')
    meas_graphics_open(0, MEAS_GRAPHICS_XY, 512, 512, 65535, "eap");
  i = 0;
  meas_misc_set_reftime();
#ifdef GPIB_INTERFACE
  fd = meas_gpib_open(BOARD, GPIBID);
#else
  fd = meas_rs232_open(DEVICE, 1);
#endif
  while(i < loop_amt) {
    meas_misc_set_time();
#ifdef GPIB
    meas_gpib_write(fd, "MEAS?", 1);
    meas_gpib_read(fd, buf);
#else
    meas_rs232_write(fd, "MEAS?\r", 6);
    meas_rs232_readnl(fd, buf);
#endif
    xval[i] = meas_misc_get_reftime();
    yval[i] = atof(buf);
    if(graphix == 'y')
      meas_graphics_update_xy(0, xval, yval, i+1);
    if(i && graphix == 'y')
      meas_graphics_autoscale(0);
    if(graphix == 'y') meas_graphics_update();
    printf("%le %le\n", xval[i], yval[i]);
    fprintf(fp, "%le %le\n", xval[i], yval[i]);
    fflush(stdout);
    fflush(fp);
    meas_misc_sleep_rest(sleepsec, sleepnsec);
    i++;
  }
#ifdef GPIB
  meas_gpib_close(BOARD, fd);
#else
  meas_rs232_close(fd);
#endif
  fclose(fp);
  printf("Press any enter to stop:");
  fgets(buf, sizeof(buf), stdin);  
  meas_graphics_close(-1);
}

