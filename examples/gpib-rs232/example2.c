#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <meas/meas.h>

#define MAXP 65535

#define BOARD  0
#define GPIBID 22

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
  fprintf(stderr,"Graphical output? (y/n) ");
  scanf("%s", &graphix);
  if(graphix != 'y' && graphix != 'n')
    {
      printf("Invalid input, exiting\n");
      exit(1);
    }

  if(graphix == 'y')
    meas_graphics_init(0, MEAS_GRAPHICS_XY, 512, 512, 65535, "eap2");
  i = 0;
  meas_pdr2000_init(0, "/dev/ttyS0"); /* com1 */
  meas_misc_set_reftime();
  while(i < loop_amt) {
    meas_misc_set_time();
    xval[i] = meas_misc_get_reftime();
    yval[i] = meas_pdr2000_read(0, 2);
    if(graphix == 'y')
      meas_graphics_update_xy(0, xval, yval, i+1);
    if(i && graphix == 'y')
      meas_graphics_autoscale(0);
    if(graphix == 'y') meas_graphics_update();
    printf("%le %le\n", xval[i], yval[i]);
    fflush(stdout);
    fprintf(fp, "%le %le\n", xval[i], yval[i]);
    fflush(fp);
    meas_misc_sleep_rest(sleepsec, sleepnsec);
    i++;
  }
  fclose(fp);
  printf("Press any enter to stop:");
  gets(buf);  
  meas_graphics_close();
  meas_pdr2000_close(0);
}

