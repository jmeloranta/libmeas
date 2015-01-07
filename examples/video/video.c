#include <stdio.h>
#include <stdlib.h>
#include <meas/meas.h>

#define WIDTH 640
#define HEIGHT 480

#define SCALE 1

unsigned char ri[WIDTH * HEIGHT], gi[WIDTH * HEIGHT], bi[WIDTH * HEIGHT];
unsigned char ro[SCALE * SCALE * WIDTH * HEIGHT], go[SCALE * SCALE * WIDTH * HEIGHT], bo[SCALE * SCALE * WIDTH * HEIGHT];

main() {

  int i, fd, s;
  double mi, ma;

  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, SCALE*WIDTH, SCALE*HEIGHT, 0, "test");
  fd = meas_video_open("/dev/video0", WIDTH, HEIGHT);
  meas_video_start(fd);
  meas_video_set_frame_rate(fd, 2);
  while (1) {
    meas_video_read_rgb(fd, ri, gi, bi, 1);
    meas_graphics_scale_rgb(ri, gi, bi, WIDTH, HEIGHT, SCALE, ro, go, bo);
    meas_graphics_update_image(0, ro, go, bo);
    meas_graphics_update();
  }
  meas_video_stop(fd);
  meas_video_close(fd);
}
