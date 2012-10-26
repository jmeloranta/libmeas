/*
 * Simple frame grabbing interface using video for linux 2.
 *
 * TODO: This assumes that the MMAP interface is available for the camera.
 *
 */

#include "video.h"
#include "misc.h"

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
} devices[MEAS_VIDEO_MAXDEV];

static int been_here = 0;

/*
 * Open video device. 640 x 480 resolution (hard coded at present).
 *
 * device = Device name (e.g., /dev/video0, /dev/video1, ...).
 *
 * Returns descriptor to the video device.
 *
 */

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
      devices[i].length = 0;
      devices[i].memory = NULL;
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
  meas_misc_root_off();

  return cd;
}

/*
 * Start video capture.
 *
 * cd  = Video device descriptor as return by meas_video_open().
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
 * cd  = Video device descriptor as return by meas_video_open().
 *
 */

int meas_video_stop(int cd) {

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

int meas_video_close(int cd) {

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
  devices[cd].fd = -1;
  devices[cd].nbufs = 0;
  meas_misc_root_off();
}

/* r[i * WIDTH + j] where i runs along Y and j along X */
static void convert_yuv_to_rgb(unsigned char *buf, unsigned int len, double *r, double *g, double *b) {

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
      printf("YUV: %u %u %u %u\n", y1, y2, u, v);
      /* yuv -> rgb */
      ind = h * MEAS_VIDEO_WIDTH + w/2;
      r[ind] = ((double) y1) + 1.13983 * (double) v;
      g[ind] = ((double) y1) - 0.39465 * ((double) u) - 0.58060 * (double) v;
      b[ind] = ((double) y1) + 2.03211 * (double) u;
      r[ind+1] = ((double) y2) + 1.13983 * (double) v;
      g[ind+1] = ((double) y2) - 0.39465 * ((double) u) - 0.58060 * (double) v;
      b[ind+1] = ((double) y2) + 2.03211 * (double) u;
      printf("RGB1: %lf %lf %lf\n", r[ind], g[ind], b[ind]);
      printf("RGB2: %lf %lf %lf\n", r[ind+1], g[ind+1], b[ind+1]);
    }
}

/*
 * Read frame from the camera.
 *
 * cd  = Video device descriptor as return by meas_video_open().
 * r   = Array for storing red component of RGB (double *).
 * g   = Array for storing green component of RGB (double *).
 * b   = Array for storing blue component of RGB (double *).
 *
 * The r, g, b arrays are two dimensional and are stored
 * in the same order as they come from the camera, i.e.
 * i * WIDTH + j  where i = 0, ..., HEIGHT and j = 0, ..., WIDTH.
 *
 */

int meas_video_read_rgb(int cd, double *r, double *g, double *b) {

  struct v4l2_buffer buf;
  struct timeval tv;
  fd_set fds;

  if(devices[cd].fd == -1) meas_err("video: Attempt to read without opening the device.");

  meas_misc_root_on();

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

#if 0
  {
    int i;
    unsigned long chk;
    printf("index = %u, length = %u\n", buf.index, buf.length);
    chk = 0;
    for(i = 0; i < buf.bytesused; i++) 
      chk += ((unsigned char *) devices[cd].memory[buf.index])[i];
    printf("check sum = %u\n", chk);
  }
#endif

  // process image
  convert_yuv_to_rgb((unsigned char *) devices[cd].memory[buf.index], buf.bytesused, r, g, b);

  /* put the empty buffer back into the queue */
  if(ioctl(devices[cd].fd, VIDIOC_QBUF, &buf) < 0) meas_err("video: ioctl(VIDIOC_QBUF).");
  meas_misc_root_off();
  return 1;
}
