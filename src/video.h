
/* Maximum number of devices that can be open simultaneously */
#define MEAS_VIDEO_MAXDEV  16
#define MEAS_VIDEO_MAXFMT  16
#define MEAS_VIDEO_MAXPROP 64

/* Special exposure times */
#define MEAS_VIDEO_AUTO_EXPOSURE_ON  (-1)
#define MEAS_VIDEO_AUTO_EXPOSURE_OFF (-2)

/* Flags settings from UNICAP */
#define MEAS_VIDEO_FLAGS_MANUAL         (1ULL)
#define MEAS_VIDEO_FLAGS_AUTO           (1ULL<<1ULL)
#define MEAS_VIDEO_FLAGS_ONE_PUSH       (1ULL<<2ULL)
#define MEAS_VIDEO_FLAGS_READ_OUT       (1ULL<<3ULL)
#define MEAS_VIDEO_FLAGS_ON_OFF         (1ULL<<4ULL)
#define MEAS_VIDEO_FLAGS_READ_ONLY      (1ULL<<5ULL)
#define MEAS_VIDEO_FLAGS_FORMAT_CHANGE  (1ULL<<6ULL)
#define MEAS_VIDEO_FLAGS_WRITE_ONLY     (1ULL<<7ULL)
#define MEAS_VIDEO_FLAGS_CHECK_STEPPING (1ULL<<32ULL)
