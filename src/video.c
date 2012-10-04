/*
 * Simple frame grabbing interface using video for linux 2.
 *
 */

#include "video.h"

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

/*
 * Open video device. 640 x 480 resolution (hard coded at present).
 *
 * device = Device name (e.g., /dev/video0).
 *
 * Returns descriptor to the video device.
 *
 */

struct buffer {
  void *start;
  size_t length;
};

struct device {
  int fd;
  int nbufs;
  struct buffer *bufs;
} devices[MEAS_VIDEO_MAXDEV];

static int been_here = 0;

int meas_video_open(char *device) {

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
      devices[i].bufs = NULL;
    }
    been_here = 1;
  }

  for(i = 0; i < MEAS_VIDEO_MAXDEV; i++)
    if(devices[i].fd == -1) break;
  if(i == MEAS_VIDEO_MAXDEV) meas_err("video: Maximum number of devices open (increase MAXDEV).");
  cd = i;

  meas_misc_root_on();
  if((fd = open(device, O_RDWR | O_NONBLOCK, 0))) meas_err("video: Can't open device.");
  devices[cd].fd = fd;
  
  if(ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) meas_err("video: ioctl(VIDIOC_QUERYCAP).");
  if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) meas_err("video: Not a video capture device.");
  if(!(cap.capabilities & V4L2_CAP_STREAMING)) meas_err("video: Camera does not support streaming.");
  
  bzero(&cropcap, sizeof(cropcap));
  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(ioctl(fd, VIDIOC_CROPCAP, &cropcap) < 0) meas_err("video: ioctl(VIDIOC_CROPCAP).");
  crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  crop.c = cropcap.defrect; /* default */
  if(ioctl(fd, VIDIOC_S_CROP, &crop) < 0) meas_err("video: ioctl(VIDIOC_S_CROP).");
  
  bzero(&fmt, sizeof(fmt));
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = MEAS_VIDEO_WIDTH;
  fmt.fmt.pix.height = MEAS_VIDEO_HEIGHT;
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
  if(!(devices[cd].bufs = calloc(req.count, sizeof(struct buffer)))) meas_err("video: out of memory in calloc().");
  for (i = 0; i < req.count; i++) {
    bzero(&buf, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    if(ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) meas_err("video: ioctl(VIDIOC_QUERYBUF).");
    devices[cd].bufs[i].length = buf.length;
    devices[cd].bufs[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if(devices[cd].bufs[i].start == MAP_FAILED) meas_err("video: mmap() failed.");    
  }
  meas_misc_root_off();

  return cd;
}

/*
 * Start video capture.
 *
 */

int meas_video_start(int cd) {

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
 */

int meas_video_stop(int cd) {

  enum v4l2_buf_type type;

  if(devices[cd].fd == -1) meas_err("video: Attempt to stop without opening the device.");

  meas_misc_root_on();
  // stop capturing
  ioctl(devices[cd].fd, VIDIOC_STREAMOFF, &type);
  meas_misc_root_off();
}

/*
 * Close video device.
 *
 * fd = Video device descriptor to close.
 *
 */

int meas_video_close(int cd) {

  int i;

  if(devices[cd].fd == -1) return 1; /* already closed */

  meas_misc_root_on();
  meas_video_stop(cd);

  // uninit_device
  for (i = 0; i < devices[cd].nbufs; i++)
    munmap(devices[cd].bufs[i].start, devices[cd].bufs[i].length);
  
  close(devices[cd].fd);

  free(devices[cd].bufs);
  free(devices[cd].bufs);
  devices[cd].fd = -1;
  devices[cd].nbufs = 0;
  meas_misc_root_off();
}

/* r[i * WIDTH + j] where i runs along Y and j along X */
static void convert_yuv_ro_rgb(unsigned char *buf, unsigned int len, double *r, double *g, double *b) {

  int w, h;
  unsigned char y1, y2, u, v, ind;

  for(h = 0; h < MEAS_VIDEO_HEIGHT; h++)
    for(w = 0; w < MEAS_VIDEO_WIDTH; w += 4) {
      ind = h * MEAS_VIDEO_WIDTH + w;
      /* two pixels interleaved */
      y1 = buf[ind];
      y2 = buf[ind+2];
      u = buf[ind+1];
      v = buf[ind+3];
      /* yuv -> rgb */
      ind = h * MEAS_VIDEO_WIDTH + w/2;
      r[ind] = y1 + 1.13983 * v;
      g[ind] = y1 - 0.39465 * u - 0.58060 * v;
      b[ind] = y1 + 2.03211 * u;
      r[ind+1] = y2 + 1.13983 * v;
      g[ind+1] = y2 - 0.39465 * u - 0.58060 * v;
      b[ind+1] = y2 + 2.03211 * u;
    }
}

int meas_video_read_rgb(int cd, double *r, double *g, double *b) {

  //mainloop()
  struct v4l2_buffer buf;
  int i;

  if(devices[cd].fd == -1) meas_err("video: Attempt to read without opening the device.");

  meas_misc_root_on();
  // read_frame()
  bzero(&buf, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  if(ioctl(devices[cd].fd, VIDIOC_DQBUF, &buf) < 0) {
    switch(errno) {
    case EAGAIN: return 0; /* no frames available */
    case EIO: break;
    default: meas_err("video: ioctl(VIDIOC_DQBUF).");
    }
  }
  if(buf.index < devices[cd].nbufs) meas_err("video: not enough buffers.");

  // process image
  convert_yuv_to_rgb(devices[cd].bufs[buf.index].start, buf.bytesused);

  if(ioctl(devices[cd].fd, VIDIOC_QBUF, &buf) < 0) meas_err("video: ioctl(VIDIOC_QBUF).");
  meas_misc_root_off();
}
