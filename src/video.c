/*
 * Simple frame grabbing interface using video for linux 2.
 *
 * TODO: This assumes that the MMAP interface is available for the camera.
 *
 * one can use v4l2-ctl --all -d /dev/video0 for supported options for your camera.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include "video.h"

struct device {
  int fd;                       /* Camera device file descriptor */

  struct v4l2_capability camera_info;   /* Camera capabilities */

  struct v4l2_format current_format;    /* Current format & frame size */

  int nframe_formats;
  struct v4l2_fmtdesc *frame_formats[MEAS_VIDEO_MAXFMT];    /* Available frame formats */

  int nframe_sizes[MEAS_VIDEO_MAXFMT];
  struct v4l2_frmsizeenum *frame_sizes[MEAS_VIDEO_MAXFMT][MEAS_VIDEO_MAXFRAME]; /* Available frame sizes for each frame format */
  
  struct v4l2_cropcap crop_info;        /* Cropping capabilities */
  struct v4l2_crop current_crop;        /* Current cropping settings */

  struct v4l2_requestbuffers buffer_info; /* Buffer information (includes count) */
  size_t *buffer_lengths;          /* buffer length array */
  void **buffers;                  /* buffer array */
  
  int nctrls;
  struct v4l2_queryctrl *ctrls[MEAS_VIDEO_MAXCTRL];
  struct v4l2_querymenu *menu_items[MEAS_VIDEO_MAXCTRL][MEAS_VIDEO_MAXCTRL];

  char camera_state;               /* 0 = stopped, 1 = started */
} devices[MEAS_VIDEO_MAXDEV];

static int been_here = 0;

static void setup_cropping(int cd) {

  /* Cropping - default no cropping */
  bzero(&devices[cd].crop_info, sizeof(struct v4l2_cropcap));
  devices[cd].crop_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(ioctl(devices[cd].fd, VIDIOC_CROPCAP, &devices[cd].crop_info) < 0) {
    fprintf(stderr, "libmeas: Cropping not supported -- skipping.\n");
    return;
  }
  devices[cd].current_crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  devices[cd].current_crop.c = devices[cd].crop_info.defrect; /* default */
  if(ioctl(devices[cd].fd, VIDIOC_S_CROP, &devices[cd].current_crop) < 0)
    fprintf(stderr, "libmeas: Video cropping not supported.\n");
}  

static void enumerate_formats(int cd) {

  struct v4l2_fmtdesc tmp;
  int i;
  
  /* Enumerate camera image formats */
  for (i = 0; i < MEAS_VIDEO_MAXFMT; i++) {
    tmp.index = i;
    tmp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(devices[cd].fd, VIDIOC_ENUM_FMT, &tmp) < 0) break;
    if(!(devices[cd].frame_formats[i] = malloc(sizeof(struct v4l2_fmtdesc)))) {
      fprintf(stderr, "libmeas: Out of memory in allocating format descriptions.\n");
      exit(1);
    }
    bcopy(&tmp, devices[cd].frame_formats[i], sizeof(struct v4l2_fmtdesc));
  }
  if(devices[cd].nframe_formats == MEAS_VIDEO_MAXFMT) {
    fprintf(stderr, "libmeas: Increase MEAS_VIDEO_MAXFMT.\n");
    exit(1);
  }
  devices[cd].nframe_formats = i;
}

static void enumerate_frame_sizes(int cd) {

  int i, j;
  struct v4l2_frmsizeenum tmp;
  
  /* Enumerate frame sizes */
  for (i = 0; i < devices[cd].nframe_formats; i++) {
    for (j = 0; j < MEAS_VIDEO_MAXFRAME; j++) {
      tmp.index = j;
      tmp.pixel_format = devices[cd].frame_formats[i]->pixelformat;
      if(ioctl(devices[cd].fd, VIDIOC_ENUM_FRAMESIZES, &tmp) < 0) break;
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
    devices[cd].nframe_sizes[i] = j;
  }
}

static void destroy_buffers(int cd) {

  struct v4l2_buffer buf;
  int i;
  
  meas_misc_root_on();
  bzero(&buf, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  /* dequeue */
  while(ioctl(devices[cd].fd, VIDIOC_DQBUF, &buf) == 0);

  for (i = 0; i < devices[cd].buffer_info.count; i++)
    munmap(devices[cd].buffers[i], devices[cd].buffer_lengths[i]);

  bzero(&devices[cd].buffer_info, sizeof(struct v4l2_requestbuffers));
  devices[cd].buffer_info.count = 0;
  devices[cd].buffer_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  devices[cd].buffer_info.memory = V4L2_MEMORY_MMAP;
  if(ioctl(devices[cd].fd, VIDIOC_REQBUFS, &(devices[cd].buffer_info)) < 0) {
    fprintf(stderr, "libmeas: Error in releasing video buffers.\n");
    exit(1);
  }
  
  free(devices[cd].buffer_lengths);
  free(devices[cd].buffers);
  meas_misc_root_off();
}

static void setup_buffers(int cd, int nbuf) {

  struct v4l2_buffer tmp;
  int i;
  
  if(nbuf < 1) {
    fprintf(stderr, "libmeas: Not enough video buffers - at least two needed.\n");
    exit(1);
  }
        
  /* Setup device for mmap and allocate buffers */
  bzero(&devices[cd].buffer_info, sizeof(struct v4l2_requestbuffers));
  devices[cd].buffer_info.count = nbuf;
  devices[cd].buffer_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  devices[cd].buffer_info.memory = V4L2_MEMORY_MMAP;
  if(ioctl(devices[cd].fd, VIDIOC_REQBUFS, &(devices[cd].buffer_info)) < 0) {
    fprintf(stderr, "libmeas: Error in requesting video buffers.\n");
    exit(1);
  }

  if(devices[cd].buffer_info.count < nbuf)
    fprintf(stderr, "libmeas: Less video buffers available than requested.\n");
  
  if(!(devices[cd].buffer_lengths = calloc(devices[cd].buffer_info.count, sizeof(size_t)))) {
    fprintf(stderr, "libmeas: Out of memory in allocating video buffers.\n");
    exit(1);
  }
  if(!(devices[cd].buffers = calloc(devices[cd].buffer_info.count, sizeof(void *)))) {
    fprintf(stderr, "libmeas: Out of memory in allocating video buffers.\n");
    exit(1);
  }

  for (i = 0; i < devices[cd].buffer_info.count; i++) {
    bzero(&tmp, sizeof(tmp));
    tmp.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    tmp.memory = V4L2_MEMORY_MMAP;
    tmp.index = i;
    if(ioctl(devices[cd].fd, VIDIOC_QUERYBUF, &tmp) < 0) {
      fprintf(stderr, "video: ioctl(VIDIOC_QUERYBUF).\n");
      exit(1);
    }
    if((devices[cd].buffers[i] = mmap(NULL, tmp.length, PROT_READ | PROT_WRITE, MAP_SHARED, devices[cd].fd, tmp.m.offset)) == MAP_FAILED) {
      fprintf(stderr, "video: mmap() failed.\n");
      exit(1);
    }
    devices[cd].buffer_lengths[i] = tmp.length;
  }
}

static void enumerate_controls(int cd) {

  struct v4l2_queryctrl tmp;
  struct v4l2_querymenu tmp2;
  int i, j, k;
  
  /* Enumerate controls */
  tmp.id = V4L2_CTRL_CLASS_USER | V4L2_CTRL_FLAG_NEXT_CTRL;
  for(i = 0; i < MEAS_VIDEO_MAXCTRL; i++) {
    if(ioctl(devices[cd].fd, VIDIOC_QUERYCTRL, &tmp) < 0) {
      if(errno == EINVAL) break; /* No more controls */
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
    if(devices[cd].ctrls[i]->step == 0) {
      fprintf(stderr, "libmeas: Warning control with zero step - resetting to 1.\n");
      devices[cd].ctrls[i]->step = 1;
    }
    if(tmp.type == V4L2_CTRL_TYPE_MENU) {
      /* Enumerate menu items */
      for (j = 0, k = tmp.minimum; k <= tmp.maximum && j < MEAS_VIDEO_MAXCTRL; j++, k++) {
	bzero(&tmp2, sizeof(struct v4l2_querymenu));
	tmp2.id = devices[cd].ctrls[i]->id;
	tmp2.index = k;
	if(ioctl(devices[cd].fd, VIDIOC_QUERYMENU, &tmp2) < 0)
	  strcpy(tmp2.name, "N/A");
	if(!(devices[cd].menu_items[i][j] = malloc(sizeof(struct v4l2_querymenu)))) {
	  fprintf(stderr, "libmeas: Out of memory in allocating menu controls.\n");
	  exit(1);
	}
	bcopy(&tmp2, devices[cd].menu_items[i][j], sizeof(struct v4l2_querymenu));
      }
      if(j == MEAS_VIDEO_MAXCTRL) {
	fprintf(stderr, "libmeas: Increase MEAS_VIDEO_MAXCTRL.\n");
	exit(1);
      }
    }
    tmp.id |= V4L2_CTRL_FLAG_NEXT_CTRL;    
  }
  devices[cd].nctrls = i;
  if(i == MEAS_VIDEO_MAXCTRL) {
    fprintf(stderr, "libmeas: Increase MEAS_VIDEO_MAXCTRL.\n");
    exit(1);
  }
}

/*
 * Print out available video devices.
 *
 * Returns the number of devices available or zero for no devices.
 *
 * The return value is 0 for success, -1 for error.
 *
 */

EXPORT int meas_video_devices(char **names, int *ndev) {

  DIR *dp;
  struct dirent *info;
  int maxdev = *ndev, devnum;
  
  if(maxdev < 1) return -1;
  if(!(dp = opendir("/dev"))) return -1;
  *ndev = 0;
  while((info = readdir(dp)) != NULL && *ndev < maxdev)
    if(sscanf(info->d_name, "video%d", &devnum) == 1) {
      if(!(names[*ndev] = (char *) malloc(strlen(info->d_name) + 5 + 1))) {
	fprintf(stderr, "libmeas: Out of memory in meas_video_devices().\n");
	exit(1);
      }
      sprintf(names[*ndev], "/dev/%s", info->d_name);
      (*ndev)++;
    }
  closedir(dp);
  return 0;
}

/*
 * Set image format and size.
 *
 * cd     = Camera descriptor.
 * f      = Format # (image format).
 * r      = Frame size # (resolution).
 *
 * Returns current frame size or 0 on error.
 *
 */

EXPORT unsigned int meas_video_set_format(int cd, int f, int r) {

  unsigned int nbuf;
  
  if(!been_here || cd >= MEAS_VIDEO_MAXDEV || cd < 0 || devices[cd].fd == -1 || f >= devices[cd].nframe_formats || f < 0 || r < 0 || r >= devices[cd].nframe_sizes[f]) return 0;

  devices[cd].current_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (ioctl(devices[cd].fd, VIDIOC_G_FMT, &devices[cd].current_format)) {
    fprintf(stderr, "libmeas: Failed to get video format.\n");
    return 0;
  }

  nbuf = devices[cd].buffer_info.count;
  destroy_buffers(cd);

  devices[cd].current_format.fmt.pix.pixelformat = devices[cd].frame_formats[f]->pixelformat;
  devices[cd].current_format.fmt.pix.width = devices[cd].frame_sizes[f][r]->discrete.width;
  devices[cd].current_format.fmt.pix.height = devices[cd].frame_sizes[f][r]->discrete.height;
   
  if (ioctl(devices[cd].fd, VIDIOC_S_FMT, &devices[cd].current_format)) {
    perror("VIDIOC_S_FMT");
    fprintf(stderr, "libmeas: Failed to set video format - using current.\n");
    devices[cd].current_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(devices[cd].fd, VIDIOC_G_FMT, &devices[cd].current_format);
  }

  setup_buffers(cd, nbuf);

  return devices[cd].current_format.fmt.pix.sizeimage;
}

/*
 * Open video device at specified resolution.
 *
 * device   = Device name (e.g., /dev/video0, /dev/video1, ...).
 * nbuffers = Number of video buffers to be allocated.
 *
 * Returns descriptor to the video device or -1 on error.
 *
 */

EXPORT int meas_video_open(char *device, int nbuffers) {

  int fd, cd;
  unsigned int i;
  struct v4l2_capability tmp;
  
  if(!been_here) {
    for(i = 0; i < MEAS_VIDEO_MAXDEV; i++)
      devices[i].fd = -1;
    been_here = 1;
  }

  for(i = 0; i < MEAS_VIDEO_MAXDEV; i++)
    if(devices[i].fd == -1) break;
  if(i == MEAS_VIDEO_MAXDEV) {
    fprintf(stderr, "libmeas: Maximum number of video devices open (increase MEAS_VIDEO_MAXDEV).\n");
    exit(1);
  }
  cd = i;

  meas_misc_root_on();
  if((fd = open(device, O_RDWR | O_NONBLOCK, 0)) < 0) {
    fprintf(stderr, "libmeas: Can't open video device.\n");
    return -1;
  }
  devices[cd].fd = fd;
  
  if(ioctl(fd, VIDIOC_QUERYCAP, &tmp) < 0) {
    fprintf(stderr, "libmeas: Error in VIDIOC_QUERYCAP.\n");
    close(fd);
    return -1;
  }

  if(!(tmp.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    fprintf(stderr, "libmeas: Device not a video capture device.\n");
    close(fd);
    return -1;
  }

  if(!(tmp.capabilities & V4L2_CAP_STREAMING)) {
    fprintf(stderr, "libmeas: Camera does not support streaming.\n");
    close(fd);
    return -1;
  }

  setup_cropping(cd);

  enumerate_formats(cd);

  enumerate_frame_sizes(cd);

  enumerate_controls(cd);

  setup_buffers(cd, nbuffers);

  meas_video_set_format(cd, 0, 0);

  devices[cd].camera_state = 1; /* Make sure we stop streaming */
  meas_video_stop(cd);

  meas_misc_root_off();
  
  return cd;
}
/*
 * Return current image width.
 *
 * d = Camera #.
 *
 * Returns image width or -1 for error.
 *
 */

EXPORT unsigned int meas_video_get_width(int cd) {

  if(cd < 0 || cd >= MEAS_VIDEO_MAXDEV || devices[cd].fd == -1) return -1;
  return devices[cd].current_format.fmt.pix.width;
}

/*
 * Return current image height.
 *
 * d = Camera #.
 *
 * Returns image height or -1 for error.
 *
 */

EXPORT unsigned int meas_video_get_height(int cd) {

  if(cd < 0 || cd >= MEAS_VIDEO_MAXDEV || devices[cd].fd == -1) return -1;
  return devices[cd].current_format.fmt.pix.height;
}

/*
 * Return current image pixel format.
 *
 * d = Camera #.
 *
 * Returns pxiel format or -1 for error.
 *
 */

EXPORT unsigned int meas_video_get_pxielformat(int cd) {

  if(cd < 0 || cd >= MEAS_VIDEO_MAXDEV || devices[cd].fd == -1) return -1;
  return devices[cd].current_format.fmt.pix.pixelformat;
}

/*
 * Output a list of supported video modes and resolutions for a given camera.
 *
 * d = Camera descriptor.
 *
 * Return 0 on success, -1 on error.
 *
 */

EXPORT int meas_video_info_camera(int cd) {

  int i, j;

  if(!been_here || cd >= MEAS_VIDEO_MAXDEV || cd < 0 || devices[cd].fd == -1) return -1;
  
  printf("Camera %d\n", cd);
  for (i = 0; i < devices[cd].nframe_formats; i++) {
    printf("Format %d/%s with resolutions:\n", i, devices[cd].frame_formats[i]->description);
    for (j = 0; j < devices[cd].nframe_sizes[i]; j++)
      printf("%dX%d(%d), ", devices[cd].frame_sizes[i][j]->discrete.width, devices[cd].frame_sizes[i][j]->discrete.height, j);
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

EXPORT int meas_video_get_control(int cd, unsigned int id, void *value) {

  struct v4l2_control tmp;
  
  if(!been_here || cd >= MEAS_VIDEO_MAXDEV || cd < 0 || devices[cd].fd == -1) return -1;
  tmp.id = id;
  if(ioctl(devices[cd].fd, VIDIOC_G_CTRL, &tmp) < 0) {
    fprintf(stderr, "libmeas: VIDIOC_G_CTRL failed.\n");
    return -1;
  }
  bcopy(&(tmp.value), value, 4);   /* 4 bytes long */
  return 0;
}

/*
 * Set control value.
 *
 * Return 0 for success, -1 for error.
 *
 */

EXPORT int meas_video_set_control(int cd, unsigned int id, void *value) {

  struct v4l2_control tmp;
  
  if(!been_here || cd >= MEAS_VIDEO_MAXDEV || cd < 0 || devices[cd].fd == -1) return -1;
  tmp.id = id;
  bcopy(value, &(tmp.value), 4);
  if(ioctl(devices[cd].fd, VIDIOC_S_CTRL, &tmp) < 0) {
    fprintf(stderr, "libmeas: VIDIOC_G_CTRL failed.\n");
    return -1;
  }
  return 0;
}

/*
 * Get control type. 
 * 
 * cd = Camera number.
 * id = Control id.
 * 
 * Return control type or -1 on error.
 *
 */

EXPORT int meas_video_get_control_type(int cd, int id) {

  int i;

  if(!been_here || cd < 0 || cd >= MEAS_VIDEO_MAXDEV || devices[cd].fd == -1) return -1;
  for (i = 0; i < devices[cd].nctrls; i++)
    if(devices[cd].ctrls[i]->id == id) return devices[cd].ctrls[i]->type;
  return -1;
}

/*
 * Get control maximum. 
 * 
 * cd = Camera number.
 * id = Control id.
 * 
 * Return control type or -1 on error.
 *
 */

EXPORT int meas_video_get_control_max(int cd, int id) {

  int i;
  
  if(!been_here || cd < 0 || cd >= MEAS_VIDEO_MAXDEV || devices[cd].fd == -1) return -1;
  for (i = 0; i < devices[cd].nctrls; i++)
    if(devices[cd].ctrls[i]->id == id) return devices[cd].ctrls[i]->maximum;
}

/*
 * Get control minimum.
 * 
 * cd = Camera number.
 * id = Control id.
 * 
 * Return control type or -1 on error.
 *
 */

EXPORT int meas_video_get_control_min(int cd, int id) {

  int i;
  
  if(!been_here || cd >= MEAS_VIDEO_MAXDEV || cd > 0 || devices[cd].fd == -1) return -1;
  for (i = 0; i < devices[cd].nctrls; i++)
    if(devices[cd].ctrls[i]->id == id) return devices[cd].ctrls[i]->minimum;
}

/*
 * Map control id to name.
 *
 * cd = Camera number.
 * id = Control id.
 *
 * Returns pointer to name or NULL on error.
 *
 */

EXPORT char *meas_video_get_control_name(int cd, int id) {

  int i;
  
  if(!been_here || cd < 0 || cd >= MEAS_VIDEO_MAXDEV || devices[cd].fd == -1) return NULL;
  for (i = 0; i < devices[cd].nctrls; i++)
    if(devices[cd].ctrls[i]->id == id) return devices[cd].ctrls[i]->name;
}

/*
 * Map control name to id.
 *
 * cd   = Camera number.
 * name = Control name.
 *
 * Returns id or 0 on error.
 *
 */

EXPORT unsigned int meas_video_get_control_id(int cd, char *name) {

  int i;

  if(!been_here || cd < 0 || name == NULL || cd >= MEAS_VIDEO_MAXDEV || devices[cd].fd == -1) return 0;
  for (i = 0; i < devices[cd].nctrls; i++)
    if(!strcmp(devices[cd].ctrls[i]->name, name)) break;
  if(i == devices[cd].nctrls) return 0;
  return devices[cd].ctrls[i]->id;
}

/*
 * Get control default value.
 * 
 * cd = Camera number.
 * id = Control id.
 * 
 * Return control type or -1 on error.
 *
 */

EXPORT int meas_video_get_control_default(int cd, int id) {

  int i;

  if(!been_here || cd < 0 || cd >= MEAS_VIDEO_MAXDEV || devices[cd].fd == -1) return -1;
  for (i = 0; i < devices[cd].nctrls; i++)
    if(devices[cd].ctrls[i]->id == id) return devices[cd].ctrls[i]->default_value;
}

/*
 * Get control step size.
 * 
 * cd = Camera number.
 * id = Control id.
 * 
 * Return control type or -1 on error.
 *
 */

EXPORT int meas_video_get_control_step(int cd, int id) {

  int i;

  if(!been_here || cd < 0 || cd >= MEAS_VIDEO_MAXDEV || devices[cd].fd == -1) return -1;
  for (i = 0; i < devices[cd].nctrls; i++)
    if(devices[cd].ctrls[i]->id == id) return devices[cd].ctrls[i]->step;
}

/*
 * Get menu items.
 * 
 * cd    = Camera number.
 * items = char * array of menu item names.
 * len   = int pointer to store the number of menu items.
 * 
 * Returns 0 on success, -1 on error.
 *
 */

EXPORT int meas_video_control_menu(int cd, int id, char **items, int *len) {

  int i, j;
  
  if(!been_here || cd < 0 || cd >= MEAS_VIDEO_MAXDEV || devices[cd].fd == -1) return -1;
  for(i = 0; i < devices[cd].nctrls; i++)
    if(devices[cd].ctrls[i]->id == id) break;
  if(i == devices[cd].nctrls) return -1;
  if(meas_video_get_control_type(cd, id) != MEAS_VIDEO_TYPE_MENU) return -1;
  *len = devices[cd].ctrls[i]->maximum - devices[cd].ctrls[i]->maximum;
  for(j = devices[cd].ctrls[i]->minimum; j < devices[cd].ctrls[i]->maximum; j++)
    strcpy(items[j - devices[cd].ctrls[i]->minimum], devices[cd].ctrls[j - devices[cd].ctrls[i]->minimum]->name);
  return 0;
}

/*
 * Print controls for a given camera.
 *
 * d = Camera descriptor.
 *
 * Return 0 on sucess, -1 on error.
 *
 */

EXPORT int meas_video_info_controls(int cd) {

  int i, j;
  int ival;
  unsigned int uval;

  if(!been_here || cd < 0 || cd >= MEAS_VIDEO_MAXDEV || devices[cd].fd == -1) return -1;

  printf("Control summary for camera %d\n", cd);
  for(i = 0; i < devices[cd].nctrls; i++) {
    printf("%s: ", devices[cd].ctrls[i]->name);
    switch(devices[cd].ctrls[i]->type) {
    case V4L2_CTRL_TYPE_INTEGER: /* TODO: signed or unsigned ? */
      meas_video_get_control(cd, devices[cd].ctrls[i]->id, (void *) &ival);
      printf("Integer, Value = %d, Min = %d, Max = %d, Step = %u, Default = %d\n", ival, devices[cd].ctrls[i]->minimum*devices[cd].ctrls[i]->step, devices[cd].ctrls[i]->maximum*devices[cd].ctrls[i]->step, devices[cd].ctrls[i]->step, devices[cd].ctrls[i]->default_value); 
      break;
    case V4L2_CTRL_TYPE_BOOLEAN:
      meas_video_get_control(cd, devices[cd].ctrls[i]->id, (void *) &ival);
      printf("Boolean, Value = %d\n", ival?1:0);
      break;
    case V4L2_CTRL_TYPE_MENU:
      meas_video_get_control(cd, devices[cd].ctrls[i]->id, (void *) &uval);
      printf("Menu, Value = %u, Choices: ", uval);
      for (j = devices[cd].ctrls[i]->minimum; j <= devices[cd].ctrls[i]->maximum; j++)
	printf("%s(%u) ", devices[cd].menu_items[i][j-devices[cd].ctrls[i]->minimum]->name, j);
      printf("\n");
      break;
    case V4L2_CTRL_TYPE_BUTTON:
      printf("Button.\n");
      break;
    case V4L2_CTRL_TYPE_INTEGER64:
      printf("Integer64: Extended controls not implemented yet.\n");
      break;
    case V4L2_CTRL_TYPE_CTRL_CLASS:
      printf("Ctrl-class - No value.\n");
      break;
    case V4L2_CTRL_TYPE_STRING:
      printf("String - Extended controls not implemented yet.\n");
      //      printf("String:  Value = %s, Minimum length = %d, Maximum length = %d, Length must be multiple of = %d.\n",
      //     xxx, devices[cd].ctrls[i]->minimum, devices[cd].ctrls[i]->maximum, devices[cd].ctrls[i]->step);
      break;
    case V4L2_CTRL_TYPE_BITMASK:
      meas_video_get_control(cd, devices[cd].ctrls[i]->id, (void *) &uval);
      printf("Bitmask, Value = %x\n", uval);
      break;
    case V4L2_CTRL_TYPE_INTEGER_MENU:
      meas_video_get_control(cd, devices[cd].ctrls[i]->id, (void *) &uval);
      printf("Menu, Value = %d, Choices: ", uval);
      for (j = devices[cd].ctrls[i]->minimum; j < devices[cd].ctrls[i]->maximum; j++)
	printf("%d ", devices[cd].menu_items[i][j-devices[cd].ctrls[i]->minimum]->value);
      printf("\n");
      break;
    case V4L2_CTRL_TYPE_U8:
    case V4L2_CTRL_TYPE_U16:
    case V4L2_CTRL_TYPE_U32:
      printf("Unsigned types not implemented yet.\n");
      break;
    default:
      fprintf(stderr, "libmeas: Warning - unknown control type.\n");
      printf("\n");
    }
  }
  return 0;
}

/*
 * Start video capture.
 *
 * cd  = Video device descriptor as return by meas_video_open().
 *
 * Return 0 on success, -1 on error.
 *
 */

EXPORT int meas_video_start(int cd) {

  int i;
  struct v4l2_buffer buf;
  enum v4l2_buf_type type;

  if(!been_here || cd >= MEAS_VIDEO_MAXDEV || cd < 0 || devices[cd].fd == -1) {
    fprintf(stderr, "libmeas: Attempt to start video without opening the device.\n");
    return -1;
  }
  if(devices[cd].camera_state) return -1; /* camera already streaming */
  
  meas_misc_root_on();
  /* Insert buffers into queue */
  for(i = 0; i < devices[cd].buffer_info.count; i++) {
    bzero(&buf, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    if(ioctl(devices[cd].fd, VIDIOC_QBUF, &buf) < 0) {
      fprintf(stderr, "libmeas: error in ioctl(VIDIOC_QBUF).\n");
      return -1;
    }
  }
  /* start_capturing */
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(ioctl(devices[cd].fd, VIDIOC_STREAMON, &type) < 0) {
    fprintf(stderr, "libmeas: error in ioctl(VIDIOC_STREAMON).\n");
    return -1;
  }
  meas_misc_root_off();
  devices[cd].camera_state = 1;
}

/*
 * Stop video capture.
 *
 * cd  = Video device descriptor as return by meas_video_open().
 *
 * Return 0 on success, -1 on error.
 *
 */

EXPORT int meas_video_stop(int cd) {

  enum v4l2_buf_type type;

  if(!been_here || cd >= MEAS_VIDEO_MAXDEV || cd < 0 || devices[cd].fd == -1) {
    fprintf(stderr, "libmeas: Attempt to stop video without opening the device.\n");
    return -1;
  }
  if(!devices[cd].camera_state) return -1; /* camera already stopped */

  meas_misc_root_on();
  /* stop capturing */
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  ioctl(devices[cd].fd, VIDIOC_STREAMOFF, &type);
  meas_misc_root_off();
  devices[cd].camera_state = 0;
}

/*
 * Close video device.
 *
 * fd = Video device descriptor to close.
 *
 * Return 0 on success, -1 on error.
 *
 */

EXPORT int meas_video_close(int cd) {

  int i, j, k;

  if(!been_here || cd >= MEAS_VIDEO_MAXDEV || cd < 0 || devices[cd].fd == -1) {
    fprintf(stderr, "libmeas: Attempting to close non-existent video device,\n");
    return -1;
  }

  meas_misc_root_on();
  meas_video_stop(cd);
  meas_misc_root_off();

  for (i = 0; i < devices[cd].nframe_formats; i++)
    free(devices[cd].frame_formats[i]);

  for (i = 0; i < devices[cd].nframe_formats; i++)
    for (j = 0; j < devices[cd].nframe_sizes[i]; j++)
      free(devices[cd].frame_sizes[i][j]);
  
  // unmap buffers & free
  meas_misc_root_on();
  for (i = 0; i < devices[cd].buffer_info.count; i++)
    munmap(devices[cd].buffers[i], devices[cd].buffer_lengths[i]);
  meas_misc_root_off();
  free(devices[cd].buffer_lengths);
  free(devices[cd].buffers);
    
  for (i = 0; i < devices[cd].nctrls; i++) {
    if(devices[cd].ctrls[i]->type == V4L2_CTRL_TYPE_MENU)
      for (k = 0, j = devices[cd].ctrls[i]->minimum; j <= devices[cd].ctrls[i]->maximum && k < MEAS_VIDEO_MAXCTRL; j++, k++)
	free(devices[cd].menu_items[i][k]);
    free(devices[cd].ctrls[i]);
  }
  
  close(devices[cd].fd);

  devices[cd].fd = -1;
}

/*
 * Read frame from a camera.
 *
 * cd      = Video device descriptor as return by meas_video_open().
 * buffer  = Buffer for storing the frame(s).
 * nframes = Number of frames to read (note buffer must be able to hold them all).
 *
 * Return value 0 on success, -1 on error.
 *
 */

EXPORT int meas_video_read(int cd, unsigned char *buffer, int nframes) {

  struct v4l2_buffer buf;
  struct timeval tv;
  fd_set fds;
  int i;

  if(!been_here || buffer == NULL || nframes < 0 || cd >= MEAS_VIDEO_MAXDEV || cd < 0 || devices[cd].fd == -1) {
    fprintf(stderr, "libmeas: Attempt to read video device without opening.");
    return -1;
  }
  meas_misc_root_on();

  bzero(buffer, devices[cd].current_format.fmt.pix.sizeimage * nframes);
  
  for(i = 0; i < nframes; i++) {
    FD_ZERO(&fds);
    FD_SET(devices[cd].fd, &fds);
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    
    if(select(devices[cd].fd + 1, &fds, NULL, NULL, &tv) <= 0) {
      fprintf(stderr, "libmeas: Time out waiting for video frame.\n");
      return -1;
    }
    
    /* Read the frames */
    bzero(&buf, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    /* dequeue */
    while(ioctl(devices[cd].fd, VIDIOC_DQBUF, &buf) < 0)
      if(errno != EAGAIN && errno != EIO) {
	fprintf(stderr, "libmeas: error in ioctl(VIDIOC_DQBUF).\n");
	return -1;
      }
    if(buf.index >= devices[cd].buffer_info.count) {
      fprintf(stderr, "libmeas: not enough video buffers available.\n");
      return -1;
    }

    /* Copy over to user buffer */
    /* TODO: Strange, buf.bytesused is different from frame size??! */
    /* Eventhough bytesused was set to zero when allocating buffers */
    //*   bcopy((unsigned char *) devices[cd].buffers[buf.index], buffer + i * buf.bytesused, buf.bytesused); */
    bcopy((unsigned char *) devices[cd].buffers[buf.index], buffer + i * devices[cd].current_format.fmt.pix.sizeimage, devices[cd].current_format.fmt.pix.sizeimage);

    if(ioctl(devices[cd].fd, VIDIOC_QBUF, &buf) < 0) {
      fprintf(stderr, "libmeas: Error in ioctl(VIDIOC_QBUF).\n");
      return -1;
    }
  }
  meas_misc_root_off();
  return 0;
}

/*
 * Flush video buffers.
 *
 * cd      = Video device descriptor as return by meas_video_open().
 *
 * Return value 0 on success, -1 on error.
 *
 * TODO: Not sure if this is needed at all.
 *
 */

EXPORT int meas_video_flush(int cd) {

  struct v4l2_buffer buf;
  struct timeval tv;
  fd_set fds;
  int i;

  if(!been_here || cd >= MEAS_VIDEO_MAXDEV || cd < 0 || devices[cd].fd == -1) {
    fprintf(stderr, "libmeas: Attempt to read video device without opening.");
    return -1;
  }
  meas_misc_root_on();

  while(1) {
    FD_ZERO(&fds);
    FD_SET(devices[cd].fd, &fds);
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    
    if(select(devices[cd].fd + 1, &fds, NULL, NULL, &tv) <= 0) {
      fprintf(stderr, "libmeas: Time out waiting for video frame.\n");
      return -1;
    }
    
    // read_frame()
    bzero(&buf, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    /* dequeue */
    while(ioctl(devices[cd].fd, VIDIOC_DQBUF, &buf) < 0)
      if(errno != EAGAIN && errno != EIO) {
	fprintf(stderr, "libmeas: error in ioctl(VIDIOC_DQBUF).\n");
	return -1;
      }
    if(buf.index >= devices[cd].buffer_info.count) return 0; /* TODO: how could this happen? */
    /* put the empty buffer back into the queue */
    if(ioctl(devices[cd].fd, VIDIOC_QBUF, &buf) < 0) {
      fprintf(stderr, "libmeas: Error in ioctl(VIDIOC_QBUF).\n");
      return -1;
    }
  }
  meas_misc_root_off();
  return 0;
}

/*
 * Set automatic white balance.
 *
 * cd = video device descriptor from meas_video_open().
 * value = 1 (on), 0 (off).
 *
 */

EXPORT int meas_video_auto_white_balance(int cd, int value) {

  if (value) value = 1;
  return meas_video_set_control(cd, V4L2_CID_AUTO_WHITE_BALANCE, (void *) &value);
}

/*
 * Set exposure time.
 *
 * cd    = video device descriptor from meas_video_open().
 * value = exposure time (in units of 100 microsec).
 *         if value == -1, turn on auto exposure,
 *         if value == -2, turn off auto exposure.
 *
 */

EXPORT int meas_video_exposure_time(int cd, int value) {

  switch (value) {
  case -1:
    value = 1;
    return meas_video_set_control(cd, V4L2_CID_EXPOSURE_AUTO, (void *) &value);
  case -2:
    value = 0;
    return meas_video_set_control(cd, V4L2_CID_EXPOSURE_AUTO, (void *) &value);
  default:
    if(value < meas_video_get_control_min(cd, V4L2_CID_EXPOSURE_ABSOLUTE) || value > meas_video_get_control_max(cd, V4L2_CID_EXPOSURE_ABSOLUTE)) return -1;
    return meas_video_set_control(cd, V4L2_CID_EXPOSURE_ABSOLUTE, (void *) &value);
  }
}

/*
 * Set camera gain.
 *
 * cd = video device descriptor from meas_video_open().
 * value = -1 (auto on), -2 (auto off), other values correspond to the gain control value.
 *
 */

EXPORT int meas_video_gain(int cd, int value) {

  switch (value) {
  case -1:
    value = 1;
    return meas_video_set_control(cd, V4L2_CID_AUTOGAIN, (void *) &value);
  case -2:
    value = 0;
    return meas_video_set_control(cd, V4L2_CID_AUTOGAIN, (void *) &value);
  default:
    if(value < meas_video_get_control_min(cd, V4L2_CID_GAIN) || value > meas_video_get_control_max(cd, V4L2_CID_GAIN)) return -1;
    return meas_video_set_control(cd, V4L2_CID_GAIN, (void *) &value);
  }
}

/*
 * Set horizontal flip.
 *
 * cd = video device descriptor from meas_video_open().
 * value = 1 (on), 0 (off).
 *
 */

EXPORT int meas_video_horizontal_flip(int cd, int value) {

  if(value) value = 1;
  return meas_video_set_control(cd, V4L2_CID_HFLIP, (void *) &value);
}

/*
 * Set vertical flip.
 *
 * cd = video device descriptor from meas_video_open().
 * value = 1 (on), 0 (off).
 *
 */

EXPORT int meas_video_vertical_flip(int cd, int value) {

  if(value) value = 1;
  return meas_video_set_control(cd, V4L2_CID_VFLIP, (void *) &value);
}

/*
 * Set frame rate.
 *
 * cd = video device descriptor from meas_video_open().
 * fps = frame rate.
 *
 * TODO: How to obtain supported fps?
 */

EXPORT int meas_video_set_frame_rate(int cd, double fps) {
  
  struct v4l2_streamparm sparm;

  if(!been_here || cd < 0 || cd >= MEAS_VIDEO_MAXDEV || devices[cd].fd == -1) return -1;
  bzero(&sparm, sizeof(sparm));
  sparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (ioctl(devices[cd].fd, VIDIOC_G_PARM, &sparm) < 0) {
    fprintf(stderr, "libmeas: Read video frame rate failed.\n");
    return -1;
  }
  sparm.parm.capture.timeperframe.numerator = 100;
  sparm.parm.capture.timeperframe.denominator = (int) (100.0*fps);
  if (ioctl(devices[cd].fd, VIDIOC_S_PARM, &sparm) < 0) {
    fprintf(stderr, "libmeas: Set video frame rate failed.\n");
    return -1;
  }
  return 0;
}

/*
 * Set brightness.
 *
 * cd    = video device.
 * value = brightness value
 *
 */

EXPORT int meas_video_set_brightness(int cd, int value) {

  if(value < meas_video_get_control_min(cd, V4L2_CID_BRIGHTNESS) || value > meas_video_get_control_max(cd, V4L2_CID_BRIGHTNESS)) return -1;
  return meas_video_set_control(cd, V4L2_CID_BRIGHTNESS, (void *) &value);
}

/*
 * Set contrast.
 *
 * cd    = video device.
 * value = contrast value.
 *
 */

EXPORT int meas_video_set_contrast(int cd, int value) {

  if(value < meas_video_get_control_min(cd, V4L2_CID_CONTRAST) || value > meas_video_get_control_max(cd, V4L2_CID_CONTRAST)) return -1;
  return meas_video_set_control(cd, V4L2_CID_CONTRAST, (void *) &value);
}

/*
 * Set saturation.
 *
 * cd    = video device.
 * value = saturation value.
 *
 */

EXPORT int meas_video_set_saturation(int cd, int value) {

  if(value < meas_video_get_control_min(cd, V4L2_CID_SATURATION) || value > meas_video_get_control_max(cd, V4L2_CID_SATURATION)) return -1;
  return meas_video_set_control(cd, V4L2_CID_SATURATION, (void *) &value);
}

/*
 * Set hue.
 *
 * cd    = video device.
 * value = hue value.
 *
 */

EXPORT int meas_video_set_hue(int cd, int value) {

  if(value < meas_video_get_control_min(cd, V4L2_CID_HUE) || value > meas_video_get_control_max(cd, V4L2_CID_HUE)) return -1;
  return meas_video_set_control(cd, V4L2_CID_HUE, (void *) &value);
}

/*
 * Set gamma.
 *
 * cd    = video device.
 * value = gamma value.
 *
 */

EXPORT int meas_video_set_gamma(int cd, int value) {

  if(value < meas_video_get_control_min(cd, V4L2_CID_GAMMA) || value > meas_video_get_control_max(cd, V4L2_CID_GAMMA)) return -1;
  return meas_video_set_control(cd, V4L2_CID_GAMMA, (void *) &value);
}

/*
 * Set power line frequency (flicker).
 *
 * cd    = video device.
 * value = frequency value.
 *
 */

EXPORT int meas_video_set_power_line_frequency(int cd, int value) {

  if(value < meas_video_get_control_min(cd, V4L2_CID_POWER_LINE_FREQUENCY) || value > meas_video_get_control_max(cd, V4L2_CID_POWER_LINE_FREQUENCY)) return -1;
  return meas_video_set_control(cd, V4L2_CID_POWER_LINE_FREQUENCY, (void *) &value);
}

/*
 * Set sharpness.
 *
 * cd    = video device.
 * value = sharpness value.
 *
 */

EXPORT int meas_video_set_sharpness(int cd, int value) {

  if(value < meas_video_get_control_min(cd, V4L2_CID_SHARPNESS) || value > meas_video_get_control_max(cd, V4L2_CID_SHARPNESS)) return -1;
  return meas_video_set_control(cd, V4L2_CID_SHARPNESS, (void *) &value);
}

/*
 * Press (by software) a button.
 *
 * Return 0 for success, -1 for error.
 *
 */

EXPORT int meas_video_press_button(int cd, unsigned int id) {
  
  unsigned int value = 0;
  int i;
  
  if(!been_here || cd >= MEAS_VIDEO_MAXDEV || cd < 0 || devices[cd].fd == -1) return -1;
  for (i = 0; i < devices[cd].nctrls; i++)
    if(devices[cd].ctrls[i]->id == id) break;
  if(i == devices[cd].nctrls) return -1;
  if(devices[cd].ctrls[i]->type != V4L2_CTRL_TYPE_BUTTON) return -1;
  return meas_video_set_control(cd, id, (void *) &value);
}
