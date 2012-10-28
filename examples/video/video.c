#include <stdio.h>
#include <stdlib.h>
#include <meas/meas.h>

unsigned char r[640 * 480], g[640 * 480], b[640 * 480];

main() {

  int i, fd, s;
  double mi, ma;

  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, 640, 480, 0, "test");
  if((fd = meas_video_open("/dev/video0")) < 0) {
    fprintf(stderr, "Can't open /dev/video0\n");
    exit(1);
  }
  while (1) {
    meas_video_start(fd);
    meas_video_read_rgb(fd, r, g, b);
    meas_video_stop(fd);
    meas_graphics_update_image(0, r, g, b);
    meas_graphics_update();
  }
  meas_video_stop(fd);
  meas_video_close(fd);
}
