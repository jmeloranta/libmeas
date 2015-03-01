/* Configuration */
#define MEAS_GRAPHICS_MAX_STRING 4096
#define MEAS_GRAPHICS_MAX_WIN 4
#define MEAS_GRAPHICS_MAX_SETS 6
#define MEAS_GRAPHICS_MAX_CONTOUR 65535
#define MEAS_GRAPHICS_MAX_POINTS 65535
#define MEAS_GRAPHICS_MAX_2DPOINTS 2048
/* X window driver. If undefined, use xcairo */
#define MEAS_GRAPHICS_XWIN

/* Graph types */
#define MEAS_GRAPHICS_EMPTY  0
#define MEAS_GRAPHICS_XY     1      /* regular X-Y plot */
#define MEAS_GRAPHICS_IMAGE  2      /* color image */
