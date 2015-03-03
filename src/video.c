/*
 * Simple frame grabbing interface using video for linux 2.
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
#include <linux/videodev2.h>

struct device {
  int fd;                       /* Camera device file descriptor */

  struct v4l2_capability camera_info;   /* Camera capabilities */

  struct v4l2_format current_format;    /* Current format & frame size */

  int nframe_formats;
  struct v4l2_fmtdesc *frame_formats[MEAS_VIDEO_MAXFMT];    /* Available frame formats */

  int nframe_sizes[MEAS_VIDEO_MAXFMT];
  struct v4l2_frmsizeenum *frame_sizes[MEAS_VIDEO_MAXFMT][MEAS_VIDEO_MAXFRAME]; /* Available frame sizes for each frame format (last member has index = -1) */

  struct v4l2_cropcap crop_info;        /* Cropping capabilities */
  struct v4l2_crop current_crop;        /* Current cropping settings */

  struct v4l2_requestbuffers buffer_info; /* Buffer information */
  size_t *buffer_lengths;          /* buffer length array */
  void **buffers;                  /* buffer array */
  
  int nctrls;
  struct v4l2_query_ctrl *ctrls[MEAS_VIDEO_MAXCTRL];
  struct v4l2_querymenu *menus[MEAS_VIDEO_MAXCTRL][MEAS_VIDEO_MAXCTRL];
} devices[MEAS_VIDEO_MAXDEV];

static int been_here = 0;

/*
 * Open video device at specified resolution.
 *
 * device = Device name (e.g., /dev/video0, /dev/video1, ...).
 *
 * Returns descriptor to the video device.
 *
 */

EXPORT int meas_video_open(char *device, unsigned int width, unsigned int height) {

  int fd, cd;
  unsigned int i, j, k, l;
  struct v4l2_query_ctrl tmp;

  if(!been_here) {
    for(i = 0; i < MEAS_VIDEO_MAXDEV; i++)
      devices[i].fd = -1;
    been_here = 1;
  }

  for(i = 0; i < MEAS_VIDEO_MAXDEV; i++)
    if(devices[i].fd == -1) break;
  if(i == MEAS_VIDEO_MAXDEV) meas_err("video: Maximum number of devices open (increase MAXDEV).");
  cd = i;

  meas_misc_root_on();
  if((fd = open(device, O_RDWR | O_NONBLOCK, 0)) < 0) meas_err("video: Can't open device.");
  devices[cd].fd = fd;

  if(ioctl(fd, VIDIOC_QUERYCAP, &devices[cd].cap) < 0) {
    fprintf(stderr, "libmeas: Error in VIDIOC_QUERYCAP.\n");
    close(fd);
    return -1;
  }
  if(!(devices[cd].cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    fprintf(stderr, "libmeas: Device not a capture device.\n");
    close(fd);
    return -1;
  }
  if(!(devices[cd].cap.capabilities & V4L2_CAP_STREAMING)) {
    printf(stdrer, "libmeas: Camera does not support streaming.\n");
    close(fd);
    return -1;
  }

  /* Cropping - default no cropping */
  bzero(&devices[cd].cropcap, sizeof(cropcap));
  devices[cd].cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(ioctl(fd, VIDIOC_CROPCAP, &devices[cd].cropcap) < 0) meas_err("video: ioctl(VIDIOC_CROPCAP).");
  devices[cd].crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  devices[cd].crop.c = cropcap.defrect; /* default */
  if(ioctl(fd, VIDIOC_S_CROP, &devices[cd].crop) < 0) fprintf(stderr, "video: cropping not supported (info).\n");

  /* Enumerate camera image formats */
  for (i = 0; i < MEAS_VIDEO_MAXFMT; i++) {
    struct v4l2_fmtdesc tmp;
    tmp.index = i;
    if(ioctl(fd, VIDIOC_ENUM_FMT, &tmp) < 0) break;
    if(!(devices[cd].frame_formats[i] = malloc(sizeof(struct v4l2_fmtdesc)))) {
      fprintf(stderr, "libmeas: Out of memory in allocating format descriptions.\n");
      exit(1);
    }
    bcopy(&tmp, devices[cd].frame_formats[i], sizeof(struct v4l2_fmtdesc));
  }
  devices[cd].nframe_formats = i;
  if(devices[cd].nframe_formats == MEAS_VIDEO_MAXFMT) {
    fprintf(stderr, "libmeas: Increase MEAS_VIDEO_MAXFMT.\n");
    exit(1);
  }

  /* Enumerate frame sizes */
  if(!(devices[cd].frame_sizes = malloc(sizeof(struct v4l2_frmsizeenum) * devices[cd].nframe_formats))) {
    fprintf(stderr, "libmeas: out of memory allocating frame formats.\n");
    exit(1);
  }
  for (i = 0; i < devices[cd].nframe_formats; i++) {
    for (j = 0; j < MEAS_VIDEO_MAXFRAME; j++) {
      struct v4l2_frmsizeenum tmp;
      tmp.index = j;
      tmp.pixel_format = devices[cd].frame_formats[i]->pixel_format;
      if(ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &tmp) < 0) break;
      if(!(devices[cd].frame_sizes[i][j] = malloc(sizeof(struct v4l2_frmsizeenum)))) {
	fprintf(stderr, "libmeas: Out of memory in allocating frame size descriptions.\n");
	exit(1);
      }
      bcopy(&tmp, devices[cd].frame_sizes[i][j], sizeof(struct v4l2_frmsizeenum));
    }
    if(j == MEAS_VIDEO_MAXFRAME) {
      fprintf(stderr, "libmeas: Incerase MEAS_VIDEO_MAXFRAME.\n");
      exit(1);
    }
    nframe_sizes[i] = j;
  }
  
  /* Setup device for mmap and allocate buffers */
  bzero(&devices[cd].buffer_info, sizeof(buffer_info));
  devices[cd].buffer_info.count = 4;
  devices[cd].buffer_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  devices[cd].buffer_info.memory = V4L2_MEMORY_MMAP;
  if(ioctl(fd, VIDIOC_REQBUFS, &(devices[cd].buffer_info)) < 0) {
    fprintf(stderr, "video: Error in VIDIOC_REQBUFS.\n");
    exit(1);
  }
  if(devices[cd].buffer_info.count < 2) {
    fprintf(stderr, "video: insufficient video memory.\n");
    exit(1);
  }

  if(!(devices[cd].buffer_lengths = calloc(devices[cd].buffer_info.count, sizeof(size_t)))) {
    fprintf(stderr, "video: out of memory in calloc().\n");
    exit(1);
  }
  if(!(devices[cd].buffers = calloc(devices[cd].buffer_info.count, sizeof(void *)))) {
    fprintf(stderr, "video: out of memory in calloc().\n");
    exit(1);
  }
  for (i = 0; i < devices[cd].buffer_info.count; i++) {
    struct v4l2_buffer tmp;
    bzero(tmp, sizeof(tmp));
    tmp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    tmp.memory = V4L2_MEMORY_MMAP;
    tmp.index = i;
    if(ioctl(fd, VIDIOC_QUERYBUF, &tmp) < 0) {
      fprintf(stderr, "video: ioctl(VIDIOC_QUERYBUF).\n");
      exit(1);
    }
    devices[cd].buffers[i] = mmap(NULL, tmp.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, tmp.m.offset);
    devices[cd].buffer_lengths[i] = tmp.length;
    if(devices[cd].buffers[i] == MAP_FAILED) {
      fprintf(stderr, "video: mmap() failed.\n");
      exit(1);
    }
  }

  devices[cd].current_format.type = 0;     /* Format not set */

  /* Enumerate controls */
  tmp.id = V4L2_CTRL_CLASS_USER | V4L2_CTRL_FLAG_NEXT_CTRL;
  for(i = 0; i < MEAS_VIDEO_MAXCTRL; i++) {
    if(ioctl(devices[cd].fd, VIDIOC_QUERYCTRL, &tmp) < 0) {
      if(errno == EINVAL) break;
      fprintf(stderr, "libmeas: error in VIDIOC_QUERYCTRL.\n");
      exit(1);
    }
    if(tmp.flags & V4L2_CTRL_FLAG_DISABLED) {
      tmp.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
      i--;
      continue;
    }
    if(!(devices[cd].ctrls[i] = malloc(sizeof(struct v4l2_queryctrl)))) {
      fprintf(stderr, "libmeas: Out of memory in allocating menu controls.\n");
      exit(1);
    }
    bcopy(&tmp, devices[cd].ctrls[i], sizeof(struct v4l2_queryctrl));    
    if(tmp.type == V4L2_CTRL_TYPE_MENU) {
      /* Enumerate menu items */
      for (j = 0, k = tmp.minimum; k < tmp.maximum && k < MEAS_VIDEO_MAXCTRL; j++, k++) {
	struct v4l2_querymenu tmp2;
	tmp2.id = devices[cd].menu_ctrls[i].id;
	tmp2.index = k;
	if(ioctl(devices[cd].fd, VIDIOC_QUERYMENU, &tmp) < 0) continue;
	if(!(devices[cd].menu_items[i][j] = malloc(sizeof(struct v4l2_querymenu)))) {
	  fprintf(stderr, "libmeas: Out of memory in allocating menu controls.\n");
	  exit(1);
	}
	bcopy(&tmp2, devices[cd].menu_items[i][j], sizeof(struct v4l2_querymenu));
      }
      devices[cd].nmenu_items = j;
      if(j == MEAS_VIDEO_MAXCTRL) {
	fprintf(stderr, "libmeas: Increase MEAS_VIDEO_MAXCTRL.\n");
	exit(1);
      }
    } else devices[cd].menus[i][0] = NULL;
    tmp.id |= V4L2_CTRL_FLAG_NEXT_CTRL;    
  }
  devices[cd].nctrls = i;
  if(i == MEAS_VIDEO_MAXCTRL) {
    fprintf(stderr, "libmeas: Increase MEAS_VIDEO_MAXCTRL.\n");
    exit(1);
  }

  meas_misc_root_off();

  return cd;
}

/*
 * Set image format and size.
 *
 * cd     = Camera descriptor.
 * f      = Format #.
 * width  = Image width.
 * height = Image height.
 *
 * Returns 0 on sucess, -1 on error.
 *
 */

EXPORT int meas_video_set_format(int cd, int f, int width, int height) {

  if(cd >= MEAS_VIDEO_MAXDEV || devices[cd].fd == -1 || f >= devices[cd].nframe_formats) return -1;

  bzero(&devices[cd].current_format, sizeof(struct v4l2_format));
  devices[cd].current_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  devices[cd].current_format.fmt.pix.width = width;
  devices[cd].current_format.fmt.pix.height = height;

  devices[cd].fmt.fmt.pix.pixelformat = devices[cd].frame_formats[f].pixelformat;
  devices[cd].fmt.fmt.pix.field = V4L2_FIELD_NONE;   /* was INTERLACED? */

  if (ioctl(fd, VIDIOC_S_FMT, &devices[cd].current_format)) {
    fprintf(stderr, "video: ioctl(VIDIOC_S_FMT).\n");
    return -1;
  }
  return 0;
}

/*
 * Output a list of supported video modes for a given camera.
 *
 * d = Camera descriptor.
 *
 * Return 0 on success, -1 on error.
 *
 */

EXPORT int meas_video_info_camera(int cd) {

  int i, j;

  if(cd >= MEAS_VIDEO_MAXDEV || devices[cd].fd == -1) return -1;
  
  printf("Camera %d\n", cd);
  for (i = 0; i < devices[cd].nframe_formats; i++) {
    printf("%s with resolutions: ", devices[cd].frame_formats[i]->description);
    for (j = 0; devices[cd].frame_sizes[i][j]->index != -1; j++)
      printf("%dX%d, ", devices[cd].frame_sizes[i][j]->width, devices[cd].frame_sizes[i][j]->height);
    printf("\n");
  }
  return 0;
}

/*
 * Print information about the cameras and the available formats.
 *
 * Returns 0 on sucess, -1 on error.
 *
 */

EXPORT int meas_video_info_all() {
  
  int m;
  
  if(!been_here) return -1;
  
  /* Loop through cameras */
  for (m = 0; m < MEAS_VIDEO_MAXDEV; m++)
    if(devices[m].fd != -1) meas_video_info_camera(m);
}

/*
 * Reutrn control setting value.
 *
 */

EXPORT int meas_video_get_property(int cd, unsigned int id, void *value) {

  struct v4l2_control tmp;
  
  tmp.id = id;
  if(ioctl(devices[cd].fd, VIDIOC_G_CTRL, &tmp) < 0) {
    fprintf(stderr, "libmeas: VIDIOC_G_CTRL failed.\n");
    return -1;
  }
  bcopy(&(tmp.value), value, 4);
  return 0;
}

/*
 * Set control value.
 *
 */

EXPORT int meas_video_set_property(int cd, unsigned int id, void *value) {

  struct v4l2_control tmp;
  
  tmp.id = id;
  bcopy(value, &(tmp.value), 4);
  if(ioctl(devices[cd].fd, VIDIOC_S_CTRL, &tmp) < 0) {
    fprintf(stderr, "libmeas: VIDIOC_G_CTRL failed.\n");
    return -1;
  }
  return 0;
}

/*
 * Print properties for a given camera.
 *
 * d = Camera descriptor.
 *
 * Return 0 on sucess, -1 on error.
 *
 */

EXPORT int meas_video_properties(int cd) {

  int i;
  int ival;
  unsigned int uval;

  if(cd >= MEAS_VIDEO_MAXDEV || devices[cd].fd == -1) return -1;

  printf("Control summary\n");
  printf("---------------\n\n");
  for(i = 0; i < devices[cd].nctrls; i++) {
    printf("%s: ", devices[cd].ctrls[i]->name);
    switch(devices[cd].ctrls[i]->type) {
    case V4L2_CTRL_TYPE_INTEGER:
      meas_video_get_property(cd, devices[cd].ctrls[i]->id, (void *) &ival);
      printf("Integer, Value = %d, Min = %d, Max = %d, Step = %u, Default = %d", ival, devices[cd].ctrls[i]->minimum, devices[cd].ctrls[i]->maximum, devices[cd].ctrls[i]->step, devices[cd].ctrls[i]->default_value); 
      break;
    case V4L2_CTRL_TYPE_BOOLEAN:
      meas_video_get_property(cd, devices[cd].ctrls[i]->id, (void *) &ival);
      printf("Boolean, Value = %d\n", ival?1:0);
      break;
    case V4L2_CTRL_TYPE_MENU:
      meas_video_get_property(cd, devices[cd].ctrls[i]->id, (void *) &uval);
      printf("Menu, Value = %s, Choices: ", uval - devices[cf].ctrls[i]->minimum);
      for (j = 0; j < devices[cd].ctrls[i]->maximum - devices[cf].ctrls[i]->minimum; j++)
	printf("%s(%u) ", devices[cd].menus[i][j].name, j);
      printf("\n");
      break;
    case V4L2_CTRL_TYPE_INTEGER_MENU:
      meas_video_get_property(cd, devices[cd].ctrls[i]->id, (void *) &uval);
      printf("Menu, Value = %d, Choices: ", uval - devices[cf].ctrls[i]->minimum);
      for (j = 0; j < devices[cd].ctrls[i]->maximum - devices[cf].ctrls[i]->minimum; j++)
	printf("%d(%u) ", devices[cd].menus[i][j].value, j);
      printf("\n");
      break;
    case V4L2_CTRL_TYPE_BITMASK:
      meas_video_get_property(cd, devices[cd].ctrls[i]->id, (void *) &uval);
      printf("Bitmask, Value = %x\n", uval);
      break;
    }
  }
 TODO: LEFT OFF HERE
}

/*
 * Start video capture.
 *
 * cd  = Video device descriptor as return by meas_video_open().
 *
 */

EXPORT int meas_video_start(int cd) {

  int i;
  struct v4l2_buffer buf;
  enum v4l2_buf_type type;

  if(devices[cd].fd == -1) meas_err("video: Attempt to start without opening the device.");

  meas_misc_root_on();
  // start_capturing
  for(i = 0; i < devices[cd].nbufs; i++) {
    bzero(&buf, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    if(ioctl(devices[cd].fd, VIDIOC_QBUF, &buf) < 0) meas_err("video: ioctl(VIDIOC_QBUF).");
  }
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(ioctl(devices[cd].fd, VIDIOC_STREAMON, &type) < 0) meas_err("video: ioctl(VIDIOC_STREAMON).");
  meas_misc_root_off();
}

/*
 * Stop video capture.
 *
 * cd  = Video device descriptor as return by meas_video_open().
 *
 */

EXPORT int meas_video_stop(int cd) {

  enum v4l2_buf_type type;

  if(devices[cd].fd == -1) meas_err("video: Attempt to stop without opening the device.");

  meas_misc_root_on();
  // stop capturing
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  ioctl(devices[cd].fd, VIDIOC_STREAMOFF, &type);
  meas_misc_root_off();
}

/*
 * Close video device.
 *
 * fd = Video device descriptor to close.
 *
 */

EXPORT int meas_video_close(int cd) {

  int i;

  if(devices[cd].fd == -1) return 1; /* already closed */

  meas_misc_root_on();
  meas_video_stop(cd);

  // uninit_device
  for (i = 0; i < devices[cd].nbufs; i++)
    munmap(devices[cd].memory[i], devices[cd].length[i]);
  
  close(devices[cd].fd);

  free(devices[cd].length);
  free(devices[cd].memory);
  free(devices[cd].ave_r);
  free(devices[cd].ave_g);
  free(devices[cd].ave_b);
  devices[cd].fd = -1;
  devices[cd].nbufs = 0;
  devices[cd].ave_r = devices[i].ave_g = devices[i].ave_b = NULL;
  meas_misc_root_off();
}

/*
 * Read frame from the camera.
 *
 * cd  = Video device descriptor as return by meas_video_open().
 * r   = Array for storing red component of RGB (unsigned char *).
 * g   = Array for storing green component of RGB (unsigned char *).
 * b   = Array for storing blue component of RGB (unsigned char *).
 * aves= Number of averages (int).
 *
 * The r, g, b arrays are two dimensional and are stored
 * in the same order as they come from the camera, i.e.
 * i * WIDTH + j  where i = 0, ..., HEIGHT and j = 0, ..., WIDTH.
 *
 */

EXPORT int meas_video_read_rgb(int cd, unsigned char *r, unsigned char *g, unsigned char *b, int aves) {

  struct v4l2_buffer buf;
  struct timeval tv;
  fd_set fds;
  int i, ave;

  if(devices[cd].fd == -1) meas_err("video: Attempt to read without opening the device.");

  meas_misc_root_on();

  bzero(devices[cd].ave_r, sizeof(unsigned int) * devices[cd].height * devices[cd].width);
  bzero(devices[cd].ave_g, sizeof(unsigned int) * devices[cd].height * devices[cd].width);
  bzero(devices[cd].ave_b, sizeof(unsigned int) * devices[cd].height * devices[cd].width);

  for(ave = 0; ave < aves; ave++) {

    FD_ZERO(&fds);
    FD_SET(devices[cd].fd, &fds);
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    
    if(select(devices[cd].fd + 1, &fds, NULL, NULL, &tv) <= 0) meas_err("video: time out in select.");
    
    // read_frame()
    bzero(&buf, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    /* dequeue */
    while(ioctl(devices[cd].fd, VIDIOC_DQBUF, &buf) < 0) {
      if(errno != EAGAIN && errno != EIO) meas_err("video: ioctl(VIDIOC_DQBUF).");
    }
    if(buf.index >= devices[cd].nbufs) meas_err("video: not enough buffers.");
    
    // process image
    convert_yuv_to_rgb((unsigned char *) devices[cd].memory[buf.index], buf.bytesused, r, g, b, devices[cd].width, devices[cd].height);
    for (i = 0; i < devices[cd].width * devices[cd].height; i++) {
      devices[cd].ave_r[i] += r[i];
      devices[cd].ave_g[i] += g[i];
      devices[cd].ave_b[i] += b[i];
    }
    /* put the empty buffer back into the queue */
    if(ioctl(devices[cd].fd, VIDIOC_QBUF, &buf) < 0) meas_err("video: ioctl(VIDIOC_QBUF).");
  }
  meas_misc_root_off();
  for (i = 0; i < devices[cd].width * devices[cd].height; i++) {
    r[i] = (unsigned char) (devices[cd].ave_r[i] / (unsigned int) aves);
    g[i] = (unsigned char) (devices[cd].ave_g[i] / (unsigned int) aves);
    b[i] = (unsigned char) (devices[cd].ave_b[i] / (unsigned int) aves);
  }
  return 1;
}

/*
 * Set automatic white balance.
 *
 * cd = video device descriptor from meas_video_open().
 * value = 1 (on), 0 (off).
 *
 */

EXPORT int meas_video_auto_white_balance(int cd, int value) {

  struct v4l2_control ctrl;

  bzero(&ctrl, sizeof(ctrl));
  ctrl.id = V4L2_CID_AUTO_WHITE_BALANCE;
  ctrl.value = value;
  if (ioctl(devices[cd].fd, VIDIOC_S_CTRL, &ctrl) < 0) meas_err("video: could not set auto white balance.");
}

/*
 * Set exposure mode.
 *
 * cd = video device descriptor from meas_video_open().
 * value = 0 - 3 (see your camera docs).
 *
 */

EXPORT int meas_video_set_exposure_mode(int cd, int value) {

  struct v4l2_control ctrl;

  bzero(&ctrl, sizeof(ctrl));
  ctrl.id = V4L2_CID_EXPOSURE_AUTO;
  ctrl.value = value;
  if (ioctl(devices[cd].fd, VIDIOC_S_CTRL, &ctrl) < 0) meas_err("video: could not set exposure mode.");
}

/*
 * Set exposure time.
 *
 * cd = video device descriptor from meas_video_open().
 * value = exposure time.
 *
 */

EXPORT int meas_video_exposure_time(int cd, int value) {

  struct v4l2_control ctrl;

  bzero(&ctrl, sizeof(ctrl));
  ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;   /* What is the difference between V4L2_CID_EXPOSURE and V4L2_CID_EXPOSURE_ABSOLUTE ? */
  ctrl.value = value;
  if (ioctl(devices[cd].fd, VIDIOC_S_CTRL, &ctrl) < 0) meas_err("video: could not set exposure time.");
}

/*
 * Set camera gain.
 *
 * cd = video device descriptor from meas_video_open().
 * value = -1 (auto on), -2 (auto off), other values correspond to the gain control value.
 *
 */

EXPORT int meas_video_gain(int cd, int value) {

  struct v4l2_control ctrl;

  bzero(&ctrl, sizeof(ctrl));
  switch(value) {
  case -1:
    ctrl.id = V4L2_CID_AUTOGAIN;
    ctrl.value = 1;
    break;
  case -2:
    ctrl.id = V4L2_CID_AUTOGAIN;
    ctrl.value = 0;
    break;
  default:
    ctrl.id = V4L2_CID_GAIN;
    ctrl.value = value;
  }
  if (ioctl(devices[cd].fd, VIDIOC_S_CTRL, &ctrl) < 0) meas_err("video: could not set gain.");
}


/*
 * Set horizontal flip.
 *
 * cd = video device descriptor from meas_video_open().
 * value = 1 (on), 0 (off).
 *
 */

EXPORT int meas_video_horizontal_flip(int cd, int value) {

  struct v4l2_control ctrl;

  bzero(&ctrl, sizeof(ctrl));
  ctrl.id = V4L2_CID_HFLIP;
  ctrl.value = value;
  if (ioctl(devices[cd].fd, VIDIOC_S_CTRL, &ctrl) < 0) meas_err("video: could not set horizontal flip.");
}

/*
 * Set vertical flip.
 *
 * cd = video device descriptor from meas_video_open().
 * value = 1 (on), 0 (off).
 *
 */

EXPORT int meas_video_vertical_flip(int cd, int value) {

  struct v4l2_control ctrl;

  bzero(&ctrl, sizeof(ctrl));
  ctrl.id = V4L2_CID_VFLIP;
  ctrl.value = value;
  if (ioctl(devices[cd].fd, VIDIOC_S_CTRL, &ctrl) < 0) meas_err("video: could not set vertical flip.");
}

/*
 * Set frame rate.
 *
 * cd = video device descriptor from meas_video_open().
 * fps = frame rate.
 *
 */

EXPORT int meas_video_set_frame_rate(int cd, int fps) {
  
  struct v4l2_streamparm sparm;

  bzero(&sparm, sizeof(sparm));
  sparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (ioctl(devices[cd].fd, VIDIOC_G_PARM, &sparm) < 0) meas_err("video: read frame rate failed.");
  sparm.parm.capture.timeperframe.numerator = 1;
  sparm.parm.capture.timeperframe.denominator = fps;
  if (ioctl(devices[cd].fd, VIDIOC_S_PARM, &sparm) < 0) meas_err("video: set frame rate failed.");
}

/*
 * Set brightness.
 *
 * cd    = video device.
 * value = brightness value (see v4l2-ctl --all -d /dev/video0 for range).
 *
 * TODO: perhaps scale between 0 and 100 so that this would camera independent?
 *
 */

EXPORT int meas_video_set_brightness(int cd, int value) {

  struct v4l2_control ctrl;

  bzero(&ctrl, sizeof(ctrl));
  ctrl.id = V4L2_CID_BRIGHTNESS;
  ctrl.value = value;
  if (ioctl(devices[cd].fd, VIDIOC_S_CTRL, &ctrl) < 0) meas_err("video: could not set brightness.");
}

/*
 * Set contrast.
 *
 * cd    = video device.
 * value = contrast value (see v4l2-ctl --all -d /dev/video0 for range).
 *
 */

EXPORT int meas_video_set_contrast(int cd, int value) {

  struct v4l2_control ctrl;

  bzero(&ctrl, sizeof(ctrl));
  ctrl.id = V4L2_CID_CONTRAST;
  ctrl.value = value;
  if (ioctl(devices[cd].fd, VIDIOC_S_CTRL, &ctrl) < 0) meas_err("video: could not set contrast.");
}

/*
 * Set saturation.
 *
 * cd    = video device.
 * value = saturation value (see v4l2-ctl --all -d /dev/video0 for range).
 *
 */

EXPORT int meas_video_set_saturation(int cd, int value) {

  struct v4l2_control ctrl;

  bzero(&ctrl, sizeof(ctrl));
  ctrl.id = V4L2_CID_SATURATION;
  ctrl.value = value;
  if (ioctl(devices[cd].fd, VIDIOC_S_CTRL, &ctrl) < 0) meas_err("video: could not set saturation.");
}

/*
 * Set hue.
 *
 * cd    = video device.
 * value = hue value (see v4l2-ctl --all -d /dev/video0 for range).
 *
 */

EXPORT int meas_video_set_hue(int cd, int value) {

  struct v4l2_control ctrl;

  bzero(&ctrl, sizeof(ctrl));
  ctrl.id = V4L2_CID_HUE;
  ctrl.value = value;
  if (ioctl(devices[cd].fd, VIDIOC_S_CTRL, &ctrl) < 0) meas_err("video: could not set hue.");
}

/*
 * Set gamma.
 *
 * cd    = video device.
 * value = gamma value (see v4l2-ctl --all -d /dev/video0 for range).
 *
 */

EXPORT int meas_video_set_gamma(int cd, int value) {

  struct v4l2_control ctrl;

  bzero(&ctrl, sizeof(ctrl));
  ctrl.id = V4L2_CID_GAMMA;
  ctrl.value = value;
  if (ioctl(devices[cd].fd, VIDIOC_S_CTRL, &ctrl) < 0) meas_err("video: could not set gamma.");
}

/*
 * Set power line frequency (flicker).
 *
 * cd    = video device.
 * value = frequency value (see v4l2-ctl --all -d /dev/video0 for range).
 *
 */

EXPORT int meas_video_set_power_line_frequency(int cd, int value) {

  struct v4l2_control ctrl;

  bzero(&ctrl, sizeof(ctrl));
  ctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
  ctrl.value = value;
  if (ioctl(devices[cd].fd, VIDIOC_S_CTRL, &ctrl) < 0) meas_err("video: could not set power line frequency.");
}

/*
 * Set sharpness.
 *
 * cd    = video device.
 * value = sharpness value (see v4l2-ctl --all -d /dev/video0 for range).
 *
 */

EXPORT int meas_video_set_sharpness(int cd, int value) {

  struct v4l2_control ctrl;

  bzero(&ctrl, sizeof(ctrl));
  ctrl.id = V4L2_CID_SHARPNESS;
  ctrl.value = value;
  if (ioctl(devices[cd].fd, VIDIOC_S_CTRL, &ctrl) < 0) meas_err("video: could not set sharpness.");
}
