#include <stdio.h>
#include <math.h>
#include <meas/meas.h>

main() {

  int fd, value, it;
  char buf[512];

  fd = meas_rs232_open("/dev/ttyS0", MEAS_B38400 | MEAS_NOHANDSHAKE);
  it = 0;
  while(1) {
    meas_rs232_write(fd, "smh;", 4);
    meas_rs232_readeot(fd, buf, ";");
    sscanf(buf, "SMH%d;", &value);
    printf("%d %d\n", it, value);
    fflush(stdout);
    //    sleep(2);
    it++;
  }
  meas_rs232_close(fd);
}
