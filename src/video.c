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
  int fd;
  int nbufs;
  size_t *length;   /* buffer length array */
  void **memory;    /* buffer array */
  unsigned int *ave_r, *ave_g, *ave_b;
  unsigned int width, height;
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

  int i, fd, cd;
  struct v4l2_capability cap;
  struct v4l2_cropcap cropcap;
  struct v4l2_crop crop;
  struct v4l2_format fmt;
  struct v4l2_requestbuffers req;
  struct v4l2_buffer buf;

  if(!been_here) {
    for(i = 0; i < MEAS_VIDEO_MAXDEV; i++) {
      devices[i].fd = -1;
      devices[i].nbufs = 0;
      devices[i].length = 0;
      devices[i].memory = NULL;
      devices[i].ave_r = devices[i].ave_g = devices[i].ave_b = NULL;
    }
    been_here = 1;
  }

  for(i = 0; i < MEAS_VIDEO_MAXDEV; i++)
    if(devices[i].fd == -1) break;
  if(i == MEAS_VIDEO_MAXDEV) meas_err("video: Maximum number of devices open (increase MAXDEV).");
  cd = i;

  meas_misc_root_on();
  if((fd = open(device, O_RDWR | O_NONBLOCK, 0)) < 0) meas_err("video: Can't open device.");
  devices[cd].fd = fd;
  
  if(ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) meas_err("video: ioctl(VIDIOC_QUERYCAP).");
  if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) meas_err("video: Not a video capture device.");
  if(!(cap.capabilities & V4L2_CAP_STREAMING)) meas_err("video: Camera does not support streaming.");
  
  bzero(&cropcap, sizeof(cropcap));
  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(ioctl(fd, VIDIOC_CROPCAP, &cropcap) < 0) meas_err("video: ioctl(VIDIOC_CROPCAP).");
  crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  crop.c = cropcap.defrect; /* default */
  if(ioctl(fd, VIDIOC_S_CROP, &crop) < 0) fprintf(stderr, "video: cropping not supported (info).\n");
  
  bzero(&fmt, sizeof(fmt));
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = width;
  fmt.fmt.pix.height = height;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
  fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;  
  if (ioctl(fd, VIDIOC_S_FMT, &fmt)) meas_err("video: ioctl(VIDIOC_S_FMT).");

  // mmap_init
  bzero(&req, sizeof(req));
  req.count = 4;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;  
  if(ioctl(fd, VIDIOC_REQBUFS, &req) < 0) meas_err("video: ioctl(VIDIOC_REQBUFS).");
  if(req.count < 2) meas_err("video: insufficient video memory.");
  devices[cd].nbufs = req.count;
  if(!(devices[cd].length = calloc(req.count, sizeof(size_t)))) meas_err("video: out of memory in calloc().");
  if(!(devices[cd].memory = calloc(req.count, sizeof(void *)))) meas_err("video: out of memory in calloc().");

  for (i = 0; i < req.count; i++) {
    bzero(&buf, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    if(ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) meas_err("video: ioctl(VIDIOC_QUERYBUF).");
    devices[cd].memory[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    devices[cd].length[i] = buf.length;
    if(devices[cd].memory[i] == MAP_FAILED) meas_err("video: mmap() failed.");    
  }
  if(!(devices[cd].ave_r = (unsigned int *) malloc(sizeof(unsigned int) * height * width))) meas_err("video: out of memory in malloc().");
  if(!(devices[cd].ave_g = (unsigned int *) malloc(sizeof(unsigned int) * height * width))) meas_err("video: out of memory in malloc().");
  if(!(devices[cd].ave_b = (unsigned int *) malloc(sizeof(unsigned int) * height * width))) meas_err("video: out of memory in malloc().");
  devices[cd].width = width;
  devices[cd].height = height;
  meas_misc_root_off();

  return cd;
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

/* r[i * width + j] where i runs along Y and j along X */
static void convert_yuv_to_rgb(unsigned char *buf, unsigned int len, unsigned char *r, unsigned char *g, unsigned char *b, unsigned int width, unsigned int height) {

  int i, j;
  double tmp;
  unsigned char y1, y2, u, v;

  for(i = j = 0; i < height * width; i += 2, j += 4) {
    /* two pixels interleaved */
    y1 = buf[j];
    y2 = buf[j+2];
    u = buf[j+1];
    v = buf[j+3];
    /* yuv -> rgb */
    tmp = (1.164 * (((double) y1) - 16.0) + 1.596 * (((double) v) - 128.0));
    if(tmp < 0.) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    r[i] = (unsigned char) tmp;
    
    tmp = (1.164 * (((double) y1) - 16.0) - 0.813 * (((double) v) - 128.0) - 0.391 * (((double) u) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    g[i] = (unsigned char) tmp;
    
    tmp = (1.164 * (((double) y1) - 16.0) + 2.018 * (((double) u) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    b[i] = (unsigned char) tmp;

    tmp = (1.164 * (((double) y2) - 16.0) + 1.596 * (((double) v) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    r[i+1] = (unsigned char) tmp;
    
    tmp = (1.164 * (((double) y2) - 16.0) - 0.813 * (((double) v) - 128.0) - 0.391 * (((double) u) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    g[i+1] = (unsigned char) tmp;
    
    tmp = (1.164 * (((double) y2) - 16.0) + 2.018 * (((double) u) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    b[i+1] = (unsigned char) tmp;
  }
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
 * Write R, G, & B array into a PPM file.
 *
 */

EXPORT void meas_video_rgb_to_ppm(FILE *fp, unsigned char *r, unsigned char *g, unsigned char *b, unsigned int width, unsigned int height) {
  
  int i;

  fprintf(fp, "P6 ");
  fprintf(fp, "%u ", height);
  fprintf(fp, "%u ", width);
  fprintf(fp, "255 ");
  for(i = 0; i < width * height; i++) {
    fwrite((void *) &r[i], sizeof(unsigned char), 1, fp);
    fwrite((void *) &g[i], sizeof(unsigned char), 1, fp);
    fwrite((void *) &b[i], sizeof(unsigned char), 1, fp);
  }
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
