#include <stdio.h>
#include <stdlib.h>
#include <meas/meas.h>

#define SCALE 1

main() {

  int i, s, d, f, width, height;
  double mi, ma;
  size_t frame_size;
  unsigned char *buffer, *rgb;

  meas_video_info();
  printf("Enter device #, format #, width, height: ");
  scanf("%d %d %d %d", &d, &f, &width, &height);
  frame_size = meas_video_open(d, f, width, height);
  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, SCALE*width, SCALE*height, 0, "test");
  if(!(buffer = (unsigned char *) malloc(frame_size))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(rgb = (unsigned char *) malloc(width*height*3))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  printf("Frame size = %u bytes.\n", frame_size);
  while (1) {
    meas_video_read(d, 1, buffer);
    printf("One frame done.\n");
    //meas_image_yuv422_to_rgb(buffer, rgb, width, height);
    //meas_image_y800_to_rgb(buffer, rgb, width, height);
    meas_image_y16_to_rgb(buffer, rgb, width, height);
    //    meas_image_scale_rgb(rgbi, width, height, SCALE, rgbo);
    meas_graphics_update_image(0, rgb);
    meas_graphics_update();
  }
  free(buffer);
  meas_video_close(d);
}
