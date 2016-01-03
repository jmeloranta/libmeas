#include <stdio.h>
#include <math.h>
#include <meas/meas.h>

main() {

  int fd, value, it, ave, cur;
  char buf[512];
  double freq, start, step, stop;

  fprintf(stderr,"Freq. start, stop, step, averages: ");
  scanf(" %le %le %le %d", &start, &stop, &step, &ave);
  fd = meas_rs232_open("/dev/ttyS0", MEAS_B38400 | MEAS_NOHANDSHAKE);
  for (freq = start; freq <= stop; freq += step) {
    if(freq < 10.0) 
      sprintf(buf, "FA0000%7.0lf;", freq * 1E6);
    else
      sprintf(buf, "FA000%8.0lf;", freq * 1E6);      
    meas_rs232_write(fd, buf, 14);
    cur = 0;
    for (it = 0; it < ave; it++) {
      meas_rs232_write(fd, "smh;", 4);
      meas_rs232_readeot(fd, buf, ";");
      sscanf(buf, "SMH%d;", &value);
      cur += value;
    }
    printf("%le %le\n", freq, ((double) cur) / (double) ave);
    fflush(stdout);
    it++;
  }
  meas_rs232_close(fd);
}
