#include <stdio.h>
#include <stdlib.h>
#include <meas/meas.h>

#define SCALE 2

unsigned char ri[640 * 480], gi[640 * 480], bi[640 * 480];
unsigned char ro[SCALE * SCALE * 640 * 480], go[SCALE * SCALE * 640 * 480], bo[SCALE * SCALE * 640 * 480];

main() {

  int i, fd, s;
  double mi, ma;

  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, SCALE*640, SCALE*480, 0, "test");
  fd = meas_video_open("/dev/video0");
  meas_video_start(fd);
  while (1) {
    meas_video_read_rgb(fd, ri, gi, bi, 1);
    meas_graphics_scale_rgb(ri, gi, bi, 640, 480, SCALE, ro, go, bo);
    meas_graphics_update_image(0, ro, go, bo);
    meas_graphics_update();
  }
  meas_video_stop(fd);
  meas_video_close(fd);
}
