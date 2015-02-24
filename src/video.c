/*
 * Simple frame grabbing interface using libunicap (for more serious work, use libunicap directly).
 *
 * TODO: This assumes that the MMAP interface is available for the camera.
 *
 * Not all cameras support all settings provided here. See v4l2-ctl --all -d /dev/video0
 * for supported options for your camera.
 *
 */

#include "video.h"
#include "misc.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unicap/unicap.h>

struct device {
  unicap_handle_t fd;
  int width, height;
} cameras[MEAS_VIDEO_MAXDEV];

static int been_here = 0;

/*
 * Print information about the cameras and the available formats.
 *
 */

EXPORT void meas_video_info() {

  int n, m, o;
  unicap_device_t device;
  unicap_handle_t fd;
  unicap_format_t format;
  static int been_here = 0;

  meas_misc_root_on();
  /* Loop through cameras */
  for (n = 0; n < MEAS_VIDEO_MAXDEV; n++) {
    if(unicap_enumerate_devices(NULL, &device, n) != STATUS_SUCCESS) break;
    unicap_open(&fd, &device);
    /* Loop through available video formats */
    for (m = 0; m < MEAS_VIDEO_MAXFMT; m++) {
      if(unicap_enumerate_formats(fd, NULL, &format, m) != STATUS_SUCCESS) break;
      /* Available resolutions for the chosen format */
      for (o = 0; o < format.size_count; o++)
	printf("Camera %d, Format %d (%s), %dX%d (%d), bits/pixel = %d.\n", n, m, format.identifier, format.sizes[o].width, format.sizes[o].height, o, format.bpp);
    }
    if(m == 0) fprintf(stderr, "libmeas: Camera %d: No video formats available.", n);
    unicap_close(fd);
  }
  meas_misc_root_off();

  if(n == 0) fprintf(stderr, "libmeas: No video devices available.");
  return;
}

/*
 * Open video device at specified resolution.
 *
 * d = Device number (0, 1, 2...).
 * f = Video format (0, 1, 2, ...).
 * width  = Image width.
 * height = Image height.
 *
 * Note: Use meas_video_info() to display cameras and available video formats.
 *
 * Returns the number of bytes required by one frame.
 *
 */

EXPORT size_t meas_video_open(int d, int f, int width, int height) {

  int m, n;
  unicap_device_t devices[MEAS_VIDEO_MAXDEV];
  unicap_format_t formats[MEAS_VIDEO_MAXFMT];
  static int been_here = 0;

  if(!been_here) {
    for (n = 0; n < MEAS_VIDEO_MAXDEV; n++)
      cameras[n].fd = NULL;
    been_here = 1;
  }

  meas_misc_root_on();
  /* Get camera handle */
  for (n = 0; n < MEAS_VIDEO_MAXDEV; n++)
    if(unicap_enumerate_devices(NULL, &devices[n], n) != STATUS_SUCCESS) break;
  if(n == 0) {
    meas_err2("video: No video devices available.");
    meas_misc_root_off();
    return -1;
  }
  if(d >= n) {
    meas_err2("video: Illegal video device number.");
    meas_misc_root_off();
    return -1;
  }
  unicap_open(&(cameras[d].fd), &devices[d]);

  /* Set video format */
  for (n = 0; n < MEAS_VIDEO_MAXFMT; n++)
    if(unicap_enumerate_formats(cameras[d].fd, NULL, &formats[n], n) != STATUS_SUCCESS) break;
  if(n == 0) {
    meas_err2("video: No video formats available.");
    meas_misc_root_off();
    return -1;
  }
  if(f >= n) {
    meas_err2("video: Illegal video format number.");
    meas_misc_root_off();
    return -1;
  }
  
  /* Available resolutions for the chosen format */
  for (n = 0; n < formats[f].size_count; n++)
    if(formats[f].sizes[n].width == width && formats[f].sizes[n].height == height) break;
  if(n == formats[f].size_count) {
    meas_err2("video: Requested resolution not available.");
    meas_misc_root_off();
    return -1;
  }
  formats[f].size.width = formats[f].sizes[n].width;
  formats[f].size.height = formats[f].sizes[n].height;
  if(unicap_set_format(cameras[d].fd, &formats[f]) != STATUS_SUCCESS) {
    meas_err2("video: Can't set video format.");
    meas_misc_root_off();
    return -1;
  }

  meas_misc_root_off();
  return (formats[f].bpp * width * height) / 8;
}

/*
 * Read a specified number of frames from the camera.
 *
 * d       = Video device number.
 * nframes = Number of frames to read.
 *
 */

struct tmp {
  volatile int frame_count; /* Note: volatile because the frame_sub routine is changing this inside a while loop in read */
  int max_frames;
  unsigned char *frames;
};

static void new_frame_sub(unicap_event_t event, unicap_handle_t handle, unicap_data_buffer_t *buffer, void *user_data) {

  struct tmp *tmp = (struct tmp *) user_data;

  // Async -> could go one over and overwrite beyond the buffer
  if(tmp->frame_count < tmp->max_frames)
    bcopy(buffer->data, &(tmp->frames[tmp->frame_count * buffer->buffer_size]), buffer->buffer_size);
  (tmp->frame_count)++;
}

EXPORT void meas_video_read(int d, int nframes, unsigned char *buffer) {

  struct tmp tmp;

  if(cameras[d].fd == NULL) {
    meas_err2("video: Read from non-existing camera.");
    return;
  }
  meas_misc_root_on();
  tmp.frame_count = 0;
  tmp.frames = buffer;
  tmp.max_frames = nframes;
  unicap_register_callback(cameras[d].fd, UNICAP_EVENT_NEW_FRAME, (unicap_callback_t) new_frame_sub, (void *) &tmp);
  unicap_start_capture(cameras[d].fd);
  while(tmp.frame_count < nframes) usleep(100000);
  unicap_stop_capture(cameras[d].fd);
  meas_misc_root_off();
}

/*
 * Close video device.
 *
 * fd = Video device descriptor to close.
 *
 */

EXPORT void meas_video_close(int d) {

  if(cameras[d].fd == NULL) return;
  meas_misc_root_on();
  unicap_close (cameras[d].fd);
  meas_misc_root_off();
}
