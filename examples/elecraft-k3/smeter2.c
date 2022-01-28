#include <stdio.h>
#include <time.h>
#include <math.h>
#include <meas/meas.h>

#define DEVICE "/dev/ttyUSB0"
#define DEVICE_SPEED MEAS_B38400

#define AVE 5
#define WAIT (1*60)   // every 1 min.

int main(int argc, char **argv) {

  int fd, value, j;
  char buf[512];
  double val;
  time_t stim;
  long cur_time;
  
  fd = meas_rs232_open(DEVICE, DEVICE_SPEED | MEAS_NOHANDSHAKE);

  stim = time(0);
  while(1) {
    val = 0.0;
    for (j = 0; j < AVE; j++) {
      meas_rs232_write(fd, "smh;", 4);
      meas_rs232_readeot(fd, buf, ";");
      sscanf(buf, "SMH%d;", &value);
      val += value;
    }
    val /= (double) AVE;
    cur_time = time(0) - stim;
    printf("%ld %le\n", cur_time, val);
    fflush(stdout);
    sleep(WAIT);    
  }

  meas_rs232_close(fd);
}
