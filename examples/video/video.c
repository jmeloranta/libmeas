#include <stdio.h>
#include <stdlib.h>
#include <meas/meas.h>

#define SCALE 2

main() {

  int i, d, f, r, width, height;
  double mi, ma;
  unsigned int frame_size;
  unsigned char *buffer, *rgb3, *rgb3s;
  union {
    char str[4];
    unsigned int val;
  } fmt;
    
  if((d = meas_video_open("/dev/video0", 4)) < 0) {
    fprintf(stderr, "Can't open /dev/video0 - is the camera connected?\n");
    exit(1);
  }
  meas_video_info_all();
  printf("Enter format #, Frame size #: ");
  scanf("%d %d %d %d", &f, &r);
  
  frame_size = meas_video_set_image_format(d, f, r);
  width = meas_video_get_width(d);
  height = meas_video_get_height(d);
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
  fmt.val = meas_video_get_pxielformat(d);
  printf("Image format = %c%c%c%c\n", fmt.str[0], fmt.str[1], fmt.str[2], fmt.str[3]);
  meas_video_start(d);
  while(1) {
    meas_video_read(d, buffer, 1);
    if(!strncmp(fmt, "RGB3"), 4) bcopy(buffer, rgb3, 3 * width * height);
    else if(!strncmp(fmt, "BGR3", 4)) meas_image_bgr3_to_rgb3(buffer, rgb3, width, height);
    else if(!strncmp(fmt, "UYVY", 4)) meas_image_yuv422_to_rgb3(buffer, rgb3, width, height);
    else if(!strncmp(fmt, "Y800", 4) || !strncmp(fmt, "Y8", 2)) meas_image_y800_to_rgb3(buffer, rgb3, width, height);
    else if(!strncmp(fmt, "Y12", 3) || !strncmp(fmt, "Y16", 3)) meas_image_y16_to_rgb3(buffer, rgb3, width, height);
    else { printf("Unknown video format.\n"); exit(1);}
    meas_image_scale_rgb3(rgb3, width, height, SCALE, rgb3s);
    meas_graphics_update_image(0, rgb3s);
    meas_graphics_update();
  }
  free(buffer);
  meas_video_close(d);
}
