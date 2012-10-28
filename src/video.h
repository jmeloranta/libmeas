
/* Maximum number of devices that can be open simultaneously */
#define MEAS_VIDEO_MAXDEV 4

/* Width */
#define MEAS_VIDEO_WIDTH  640
#define MEAS_VIDEO_HEIGHT 480

/* Prototypes */
int meas_video_open(char *), meas_video_start(int), meas_video_stop(int), meas_video_close(int), meas_video_read_rgb(int, unsigned char *, unsigned char *, unsigned char *, int);

