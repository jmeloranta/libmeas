/*
 * Simple frame grabbing interface using libunicap (for more serious work, use libunicap directly).
 *
 * Unfortunately, there are a number of bugs in libunicap:
 *
 * 1) USER_BUFFERS does not work - it hangs in unicap_wait_buffer() forever.
 * 2) Each time stop is issued to a device, it hangs for a moment and spits out V4L2 errors.
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
#include <string.h>

/* This does not seem to work */
/* #define USER_BUFFERS /**/

struct device {
  unicap_handle_t fd;
  int width, height;
  unicap_device_t device;
  int nformats; /* Number of formats */
  unicap_format_t formats[MEAS_VIDEO_MAXFMT]; /* supported video formats */
  int current_format;
  int nprop;   /* Number of properties */
  unicap_property_t properties[MEAS_VIDEO_MAXPROP]; /* supported camera properties (settigns) */
} cameras[MEAS_VIDEO_MAXDEV];
static int nvideo = -1; /* # of video devices */

static int been_here = 0;

/*
 * Output a list of supported video modes for a given camera.
 *
 * d = Camera number.
 *
 * Return 0 on success, -1 on error.
 *
 */

EXPORT int meas_video_info_camera(int d) {

  int m, n;

  if(d >= nvideo || cameras[d].fd == NULL || nvideo == -1) return -1;
  /* Loop through available video formats */
  for (m = 0; m < cameras[d].nformats; m++) {
    /* Available resolutions for the chosen format */
    for (n = 0; n < cameras[d].formats[m].size_count; n++)
      printf("Camera %d, Format %d (%s), %dX%d (%d), bits/pixel = %d.\n", d, m, cameras[d].formats[m].identifier, cameras[d].formats[m].sizes[n].width,
	     cameras[d].formats[m].sizes[n].height, n, cameras[d].formats[m].bpp);
  }
  if(m == 0) fprintf(stderr, "libmeas: Camera %d: No video formats available.", d);
  return 0;
}

/*
 * Print information about the cameras and the available formats.
 *
 */

EXPORT void meas_video_info_all() {

  int m;
  
  if(nvideo == -1) meas_video_open(0);
  /* Loop through cameras */
  for (m = 0; m < nvideo; m++)
    meas_video_info_camera(m);
}

/*
 * Print properties for a given camera.
 *
 * d = Camera descriptor.
 *
 */

EXPORT void meas_video_properties(int d) {

  int m, n;

  /* Range properties */
  for (m = 0; m < cameras[d].nprop; m++) {
    unicap_get_property(cameras[d].fd, &(cameras[d].properties[m]));
    switch(cameras[d].properties[m].type) {
    case UNICAP_PROPERTY_TYPE_RANGE:
      printf("Property(range): %s. Current value = %f, [%f,%f].\n", cameras[d].properties[m].identifier, cameras[d].properties[m].value,
	     cameras[d].properties[m].range.min, cameras[d].properties[m].range.max); 
      break;
    case UNICAP_PROPERTY_TYPE_MENU:
      printf("Property(menu): %s. Current value = %s.\n", cameras[d].properties[m].identifier, cameras[d].properties[m].menu_item);
      printf("              : Possible values: ");
      for (n = 0; n < cameras[d].properties[m].menu.menu_item_count; n++)
	printf("%s, ", cameras[d].properties[m].menu.menu_items[n]);
      printf("\n");
      break;
    case UNICAP_PROPERTY_TYPE_VALUE_LIST:
      printf("Property(Value list): %s. Current = %le. Available: ", cameras[d].properties[m].identifier, cameras[d].properties[m].value);
      for (n = 0; n < cameras[d].properties[m].value_list.value_count; n++)
	printf("%le ", cameras[d].properties[m].value_list.values[n]);
      printf("\n");
      break;
    case UNICAP_PROPERTY_TYPE_DATA:   /* TODO: what are these? */
      printf("Property(Data): %s.\n", cameras[d].properties[m].identifier);
      break;
    case UNICAP_PROPERTY_TYPE_FLAGS:
      printf("Property(Flags): %s. Manual = %d, Auto = %d, One Push = %d, Read Out = %d, On Off = %d, Read Only = %d, Format Change = %d, Write Only = %d, Check Stepping = %d.\n", cameras[d].properties[m].identifier, (cameras[d].properties[m].flags & UNICAP_FLAGS_MANUAL)?1:0, (cameras[d].properties[m].flags & UNICAP_FLAGS_AUTO)?1:0, (cameras[d].properties[m].flags & UNICAP_FLAGS_ONE_PUSH)?1:0, (cameras[d].properties[m].flags & UNICAP_FLAGS_READ_OUT)?1:0, (cameras[d].properties[m].flags & UNICAP_FLAGS_ON_OFF)?1:0, (cameras[d].properties[m].flags & UNICAP_FLAGS_READ_ONLY)?1:0, (cameras[d].properties[m].flags & UNICAP_FLAGS_FORMAT_CHANGE)?1:0, (cameras[d].properties[m].flags & UNICAP_FLAGS_WRITE_ONLY)?1:0);
      break;      
    default:
      fprintf(stderr, "libmeas: Unknown video property type %d.\n", cameras[d].properties[m].type);
      break;
    }
  }
}

/*
 * Open video device at specified resolution.
 *
 * d = Device number (0, 1, 2...).
 *
 * Note: Use meas_video_info_all() to display cameras and available video formats.
 *
 * Returns -1 for error, otherwise the total number of devices found is returned.
 *
 */

EXPORT int meas_video_open(int d) {

  int m, n;

  meas_misc_root_on();
  if(nvideo == -1) {
    for (m = 0; m < MEAS_VIDEO_MAXDEV; m++) {
      if(unicap_enumerate_devices(NULL, &(cameras[m].device), m) != STATUS_SUCCESS) {
	cameras[m].fd = NULL;
	if(m == 0) fprintf(stderr, "libmeas: No video devices found.\n");
	continue;
      }
      unicap_open(&(cameras[m].fd), &(cameras[m].device));
      /* Formats */
      for (n = 0; n < MEAS_VIDEO_MAXFMT; n++) {
	if(unicap_enumerate_formats(cameras[m].fd, NULL, &(cameras[m].formats[n]), n) != STATUS_SUCCESS) break;
	if(cameras[d].formats[n].bpp == 0) {
	  fprintf(stderr, "libmeas: Warning BPP = 0 - setting to 16.\n");
	  cameras[d].formats[n].bpp = 16;
	}
      }
      cameras[m].nformats = n;
      /* Properties */
      for (n = 0; n < MEAS_VIDEO_MAXPROP; n++) {
	if(unicap_enumerate_properties(cameras[m].fd, NULL, &(cameras[m].properties[n]), n) != STATUS_SUCCESS) break;
	unicap_get_property(cameras[m].fd, &(cameras[m].properties[n]));
      }
      cameras[m].nprop = n;
    } 
    nvideo = m;
  }
  meas_misc_root_off();
  
  if(d < 0 || d >= nvideo) meas_err("video: Illegal video device number.");
  
  return nvideo;  
}
 
/*
 * Return image format (YU12, Y800, etc.) for the given device and format # & resolution.
 *
 * d      = Camera number.
 * f      = Format number.
 * width  = Image width.
 * height = Image height.
 * fcc    = Char array of at least 5 bytes to hold the four byte FOURCC and the null termination.
 *          For a list of allocated FOURCC's, see http://www.fourcc.org
 *
 * Return value is 0 on success or -1 on error.
 *
 */

EXPORT int meas_video_image_format(int d, int f, int width, int height, char *fcc) {

  if(d >= nvideo || cameras[d].fd == NULL || nvideo == -1) return -1;
  strncpy(fcc, (char *) &(cameras[d].formats[f].fourcc), 4);
  fcc[4] = 0;
  return 0;
}

/*
 * Set video format.
 *
 * d      = Camera.
 * f      = Format (a number; use meas_video_info_all() to see the available modes).
 * width  = Image width.
 * height = Image height.
 *
 * Returns -1 for non-existing video mode or number of bytes required per frame.
 *
 */

EXPORT size_t meas_video_format(int d, int f, int width, int height) {
  
  int n;
  
  if(d < 0 || d >= nvideo) meas_err("libmeas: Invalid video device number.");
  if(f < 0 || f >= cameras[d].nformats) meas_err("libmeas: Invalid video format number.");
  
  /* Available resolutions for the chosen format */
  for (n = 0; n < cameras[d].formats[f].size_count; n++)
    if(cameras[d].formats[f].sizes[n].width == width && cameras[d].formats[f].sizes[n].height == height) break;
  if(n == cameras[d].formats[f].size_count) meas_err("video: Requested resolution not available.");
  
  cameras[d].formats[f].size.width = cameras[d].formats[f].sizes[n].width;
  cameras[d].formats[f].size.height = cameras[d].formats[f].sizes[n].height;
#ifdef USER_BUFFERS
  cameras[d].formats[f].buffer_type = UNICAP_BUFFER_TYPE_USER;
#else
  cameras[d].formats[f].buffer_type = UNICAP_BUFFER_TYPE_SYSTEM;
#endif
  if(unicap_set_format(cameras[d].fd, &(cameras[d].formats[f])) != STATUS_SUCCESS) meas_err("video: Can't set video format.");
  cameras[d].current_format = f;
  
  return cameras[d].formats[f].buffer_size;
  //  return (cameras[d].formats[f].bpp * width * height) / 8;
}

/*
 * Read a specified number of frames from the camera.
 *
 * d       = Video device number.
 * nframes = Number of frames to read.
 *
 * Note that meas_video_start() must be called before this and meas_video_stop() to stop the device.
 *
 */

#ifndef USER_BUFFERS
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
#endif

EXPORT void meas_video_read(int d, int nframes, unsigned char *buffer) {

  struct tmp tmp;
  int cf;
#ifdef USER_BUFFERS
  unicap_data_buffer_t unibuf;
#endif
  
  if(cameras[d].fd == NULL) {
    meas_err2("video: Read from non-existing camera.");
    return;
  }
  meas_misc_root_on();
#ifdef USER_BUFFERS
  unibuf.buffer_size = cameras[d].formats[cameras[d].current_format].buffer_size;
  unicap_start_capture(cameras[d].fd);
  for (cf = 0; cf < nframes; cf++) {
    unicap_data_buffer_t *rb;
    unibuf.data = buffer + cf * unibuf.buffer_size;
    unicap_queue_buffer(cameras[d].fd, &unibuf);
    unicap_wait_buffer(cameras[d].fd, &rb);
  }
  unicap_stop_capture(cameras[d].fd);
#else
  tmp.frame_count = 0;
  tmp.frames = buffer;
  tmp.max_frames = nframes;
  unicap_register_callback(cameras[d].fd, UNICAP_EVENT_NEW_FRAME, (unicap_callback_t) new_frame_sub, (void *) &tmp);
  while(tmp.frame_count < nframes) usleep(100);
#endif
  meas_misc_root_off();
}

/*
 * Start capture.
 *
 * d = Camera number.
 *
 */

EXPORT void meas_video_start(int d) {

  unicap_start_capture(cameras[d].fd);
}

/*
 * Stop capture.
 *
 * d = Camera number.
 *
 */

EXPORT void meas_video_stop(int d) {

  unicap_stop_capture(cameras[d].fd);
}

/*
 * Close video device.
 *
 * d = Video camera number to close. TODO: Can't be accessed after this anyore...
 *
 */

EXPORT void meas_video_close(int d) {

  if(cameras[d].fd == NULL) return;
  unicap_close (cameras[d].fd);
}

/*
 * Set range property.
 *
 * d     = Camera number.
 * name  = Property name.
 * value = Property value.
 *
 * -1 = for non-existing range property or value outside the limits. 0 for success.
 *
 */

EXPORT int meas_video_set_range(int d, char *name, double value) {
  
  int m;
  
  for (m = 0; m < cameras[d].nprop; m++) {
    unicap_get_property(cameras[d].fd, &(cameras[d].properties[m]));
    if(!strcmp(name, cameras[d].properties[m].identifier)) {
      if(cameras[d].properties[m].type != UNICAP_PROPERTY_TYPE_RANGE) meas_err("libmeas: Wrong property type (range).");
      if(value < cameras[d].properties[m].range.min || value > cameras[d].properties[m].range.max) meas_err("libmeas: Property value outside the range.");
      cameras[d].properties[m].value = value;
      unicap_set_property(cameras[d].fd, &(cameras[d].properties[m]));
      return 0;
    }
  }
  meas_err("libmeas: Range property not found.");
}

/*
 * Set menu property.
 *
 * d     = Camera number.
 * name  = Property name.
 * value = Menu item name.
 *
 * -1 = for non-existing range property or value outside the limits. 0 for success.
 *
 */

EXPORT int meas_video_set_menu(int d, char *name, char *value) {
  
  int m, n;
  
  for (m = 0; m < cameras[d].nprop; m++) {
    unicap_get_property(cameras[d].fd, &(cameras[d].properties[m]));
    if(!strcmp(name, cameras[d].properties[m].identifier)) {
      if(cameras[d].properties[m].type != UNICAP_PROPERTY_TYPE_MENU) meas_err("libmeas: Wrong property type (menu).");
      for (n = 0; n < cameras[d].properties[m].menu.menu_item_count; n++)
	if(!strcmp(cameras[d].properties[m].menu.menu_items[n], value)) break;
      if(n == cameras[d].properties[m].menu.menu_item_count) meas_err("libmeas: Wrong property menu item.");
      strcpy(cameras[d].properties[m].menu_item, value);
      unicap_set_property(cameras[d].fd, &(cameras[d].properties[m]));
      return 0;
    }
  }
  meas_err("libmeas: Menu property not found.");  
}

/*
 * Clear flag property.
 *
 * d     = Camera number.
 * name  = Property name.
 * value = Property flag value (preprocessor defines, MEAS_VIDEO_FLAGS_XXX).
 *
 * -1 = for non-existing range property or value outside the limits. 0 for success.
 *
 */

EXPORT int meas_video_Clear_flag(int d, char *name, unsigned int value) {
  
  int m;
  
  for (m = 0; m < cameras[d].nprop; m++) {
    unicap_get_property(cameras[d].fd, &(cameras[d].properties[m]));
    if(!strcmp(name, cameras[d].properties[m].identifier)) {
      // All properties can have flags
      //      if(cameras[d].properties[m].type != UNICAP_PROPERTY_TYPE_FLAGS) meas_err("libmeas: Wrong property type (flags).");
      cameras[d].properties[m].flags &= ~value;
      unicap_set_property(cameras[d].fd, &(cameras[d].properties[m]));
      return 0;
    }
  }
  meas_err("libmeas: Flag property not found.");
}

/*
 * Set flag property.
 *
 * d     = Camera number.
 * name  = Property name.
 * value = Property flag value (preprocessor defines, MEAS_VIDEO_FLAGS_XXX).
 *
 * -1 = for non-existing range property or value outside the limits. 0 for success.
 *
 */

EXPORT int meas_video_set_flag(int d, char *name, unsigned int value) {
  
  int m;
  
  for (m = 0; m < cameras[d].nprop; m++) {
    unicap_get_property(cameras[d].fd, &(cameras[d].properties[m]));
    if(!strcmp(name, cameras[d].properties[m].identifier)) {
      // All properties can have flags
      //      if(cameras[d].properties[m].type != UNICAP_PROPERTY_TYPE_FLAGS) meas_err("libmeas: Wrong property type (flags).");
      cameras[d].properties[m].flags |= value;
      unicap_set_property(cameras[d].fd, &(cameras[d].properties[m]));
      return 0;
    }
  }
  meas_err("libmeas: Flag property not found.");
}

/*
 * Set value list property.
 *
 * d     = Camera number.
 * name  = Property name.
 * value = Value (must be on the list).
 *
 * -1 = for non-existing range property or value outside the limits. 0 for success.
 *
 */

EXPORT int meas_video_set_value_list(int d, char *name, double value) {
  
  int m, n;
  
  for (m = 0; m < cameras[d].nprop; m++) {
    unicap_get_property(cameras[d].fd, &(cameras[d].properties[m]));
    if(!strcmp(name, cameras[d].properties[m].identifier)) {
      if(cameras[d].properties[m].type != UNICAP_PROPERTY_TYPE_VALUE_LIST) meas_err("libmeas: Wrong property type (value list).");
      for (n = 0; n < cameras[d].properties[m].value_list.value_count; n++)
	if(value == cameras[d].properties[m].value_list.values[n]) break;
      if(n == cameras[d].properties[m].value_list.value_count) meas_err("libmeas: Wrong property value list item.");
      cameras[d].properties[m].value = value;
      unicap_set_property(cameras[d].fd, &(cameras[d].properties[m]));
      return 0;
    }
  }
  meas_err("libmeas: Value list property not found.");  
}
