#include <stdio.h>
#include <stdlib.h>
#include <meas/meas.h>

#define SCALE 2

main() {

  int i, s, d, f, width, height;
  double mi, ma;
  size_t frame_size;
  unsigned char *buffer, *rgb3, *rgb3s, fmt[5];

  meas_video_info_all();
  printf("Enter device #, format #, width, height: ");
  scanf("%d %d %d %d", &d, &f, &width, &height);
  meas_video_open(d);
  frame_size = meas_video_set_image_format(d, f, width, height);
  meas_graphics_init(0, MEAS_GRAPHICS_IMAGE, SCALE*width, SCALE*height, 0, "test");
  if(!(buffer = (unsigned char *) malloc(frame_size))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(rgb3 = (unsigned char *) malloc(width * height * 3))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  if(!(rgb3s = (unsigned char *) malloc(SCALE * SCALE * width * height * 3))) {
    fprintf(stderr, "Out of memory.\n");
    exit(1);
  }
  printf("Frame size = %u bytes.\n", frame_size);
  meas_video_properties(0);
  meas_video_get_image_format(d, f, fmt);
  printf("Image format = %s\n", fmt);
  meas_video_start(d);
  while(1) {
    meas_video_read(d, 1, buffer);
    if(!strcmp(fmt, "RGB3")) bcopy(buffer, rgb3, 3 * width * height);
    else if(!strcmp(fmt, "BGR3")) meas_image_bgr3_to_rgb3(buffer, rgb3, width, height);
    else if(!strcmp(fmt, "UYVY")) meas_image_yuv422_to_rgb3(buffer, rgb3, width, height);
    else if(!strcmp(fmt, "Y800") || !strcmp(fmt, "Y8")) meas_image_y800_to_rgb3(buffer, rgb3, width, height);
    else if(!strcmp(fmt, "Y12") || !strcmp(fmt, "Y16")) meas_image_y16_to_rgb3(buffer, rgb3, width, height);
    else { printf("Unknown video format.\n"); exit(1);}
    meas_image_scale_rgb3(rgb3, width, height, SCALE, rgb3s);
    meas_graphics_update_image(0, rgb3s);
    meas_graphics_update();
  }
  free(buffer);
  meas_video_close(d);
}
