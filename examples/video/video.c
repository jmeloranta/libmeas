#include <stdio.h>
#include <stdlib.h>
#include <meas/meas.h>

#define SCALE 1

main() {

  int i, s, d, f, width, height;
  double mi, ma;
  size_t frame_size;
  unsigned char *buffer, *rgb, fmt[5];

  meas_video_info_all();
  printf("Enter device #, format #, width, height: ");
  scanf("%d %d %d %d", &d, &f, &width, &height);
  meas_video_open(d);
  frame_size = meas_video_format(d, f, width, height);
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
  meas_video_properties(0);
  meas_video_image_format(d, f, width, height, fmt);
  printf("Image format = %s\n", fmt);
  if(!strcmp(fmt, "RGB3")) {
    meas_video_start(d);
    while(1) {
      meas_video_read(d, 1, buffer);
      meas_graphics_update_image(0, buffer);
      meas_graphics_update();
    }
    /* not reached */
  }
  if(!strcmp(fmt, "BGR3")) {
    meas_video_start(d);
    while(1) {
      meas_video_read(d, 1, buffer);
      meas_image_bgr3_to_rgb3(buffer, buffer, width, height);
      meas_graphics_update_image(0, buffer);
      meas_graphics_update();
    }
    /* not reached */
  }
  if(!strcmp(fmt, "UYVY")) {
    meas_video_start(d);
    while(1) {
      meas_video_read(d, 1, buffer);
      meas_image_yuv422_to_rgb3(buffer, rgb, width, height);
      meas_graphics_update_image(0, rgb);
      meas_graphics_update();
    }
    /* not reached */
  }
  if(!strcmp(fmt, "Y800") || !strcmp(fmt, "Y8")) {
    meas_video_start(d);
    while(1) {
      meas_video_read(d, 1, buffer);
      meas_image_y800_to_rgb3(buffer, rgb, width, height);
      meas_graphics_update_image(0, rgb);
      meas_graphics_update();
    }
    /* not reached */
  }
  if(!strcmp(fmt, "Y16")) {
    meas_video_start(d);
    while(1) {
      meas_video_read(d, 1, buffer);
      meas_image_y16_to_rgb3(buffer, rgb, width, height);
      meas_graphics_update_image(0, rgb);
      meas_graphics_update();
    }
    /* not reached */
  }
  printf("Image format not supported.\n");
  meas_video_start(d);
  while(1) {
    meas_video_read(d, 1, buffer);
    printf("Frame read.\n");
  }
  free(buffer);
  meas_video_close(d);
}
