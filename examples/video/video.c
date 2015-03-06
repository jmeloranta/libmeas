#include <stdio.h>
#include <stdlib.h>
#include <meas/meas.h>

#define SCALE 1

#define MAXDEVS 2
char *devs[MAXDEVS];  /* actually file names can be longer but these video devices won't be */
int ndevs = MAXDEVS;

main() {

  int i, d, f, r, width, height;
  double mi, ma;
  unsigned int frame_size;
  unsigned char *buffer, *rgb3, *rgb3s;
  union {
    char str[4];
    unsigned int val;
  } fmt;

  if(meas_video_devices(devs, &ndevs) < 0) {
    fprintf(stderr, "Can't find device names.\n");
    exit(1);
  }
  for (i = 0; i < ndevs; i++)
    printf("Device: %d %s\n", i, devs[i]);
  printf("Enter device number to use: ");
  scanf("%d", &i);
  if((d = meas_video_open(devs[i], 4)) < 0) {
    fprintf(stderr, "Can't open %s - is the camera connected?\n", devs[i]);
    exit(1);
  }
  meas_video_info_camera(d);
  printf("Enter format #, Frame size #: ");
  scanf("%d %d", &f, &r);
  
  if((frame_size = meas_video_set_format(d, f, r)) == 0) {
    fprintf(stderr, "Error in setting video format.\n");
    exit(1);
  }
  width = meas_video_get_width(d);
  height = meas_video_get_height(d);
  printf("Frame size %dX%d.\n", width, height);
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
  meas_video_info_controls(d);
  meas_video_start(d);
  while(1) {
    meas_video_read(d, buffer, 1);
    if(!strncmp(fmt.str, "RGB3", 4)) bcopy(buffer, rgb3, 3 * width * height);
    else if(!strncmp(fmt.str, "BGR3", 4)) meas_image_bgr3_to_rgb3(buffer, rgb3, width, height);
    else if(!strncmp(fmt.str, "YUYV", 4)) meas_image_yuv422_to_rgb3(buffer, rgb3, width, height);
    else if(!strncmp(fmt.str, "Y800", 4) || !strncmp(fmt.str, "Y8", 2)) meas_image_y800_to_rgb3(buffer, rgb3, width, height);
    else if(!strncmp(fmt.str, "Y12", 3) || !strncmp(fmt.str, "Y16", 3)) meas_image_y16_to_rgb3(buffer, rgb3, width, height);
    else { printf("Unknown video format.\n"); exit(1);}
    meas_image_scale_rgb3(rgb3, width, height, SCALE, rgb3s);
    meas_graphics_update_image(0, rgb3s);
    meas_graphics_update();
  }
  free(buffer);
  meas_video_close(d);
}
