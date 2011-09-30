/* Configuration */
#define MEAS_GRAPHICS_MAX_STRING 4096
#define MEAS_GRAPHICS_MAX_WIN 4
#define MEAS_GRAPHICS_MAX_SETS 6
#define MEAS_GRAPHICS_MAX_CONTOUR 65535
#define MEAS_GRAPHICS_MAX_POINTS 65535
#define MEAS_GRAPHICS_MAX_2DPOINTS 2048
/* X window driver. If undefined, use xcairo */
#define MEAS_GRAPHICS_XWIN

/* Colors */
#define MEAS_GRAPHICS_BLACK      0
#define MEAS_GRAPHICS_RED        1
#define MEAS_GRAPHICS_YELLOW     2
#define MEAS_GRAPHICS_GREEN      3
#define MEAS_GRAPHICS_AQUAMARINE 4
#define MEAS_GRAPHICS_PINK       5
#define MEAS_GRAPHICS_WHEAT      6
#define MEAS_GRAPHICS_GREY       7
#define MEAS_GRAPHICS_BROWN      8
#define MEAS_GRAPHICS_BLUE       9
#define MEAS_GRAPHICS_BLUEVIOLET 10
#define MEAS_GRAPHICS_CYAN       11
#define MEAS_GRAPHICS_TURQUOISE  12
#define MEAS_GRAPHICS_MAGENTA    13
#define MEAS_GRAPHICS_SALMON     14
#define MEAS_GRAPHICS_WHITE      15

/* Graph types */
#define MEAS_GRAPHICS_EMPTY  0      
#define MEAS_GRAPHICS_2D     1      /* regular X-Y plot */
#define MEAS_GRAPHICS_3D     2      /* contour plot */

/* Color maps */
#define MEAS_GRAPHICS_RGB  0       /* RGB colormap */
#define MEAS_GRAPHICS_GRAY 1       /* gray scale colormap */

/* 3D pixel styles */
#define MEAS_GRAPHICS_FAST3D  -1
#define MEAS_GRAPHICS_SMOOTH3D 0

/* 3D axis directions */
#define MEAS_GRAPHICS_NORMAL3D 0
#define MEAS_GRAPHICS_REV3D    1

/* 3D axis scales */
#define MEAS_GRAPHICS_LINEAR3D 0
#define MEAS_GRAPHICS_LOG3D    1

/* Prototypes */
int meas_graphics_init(int, char *, char *), meas_graphics_xscale(int, double, double, double, double);
int meas_graphics_yscale(int, double, double, double, double), meas_graphics_zscale(int, double, double), meas_graphics_clear(int);
int meas_graphics_title(int, char *), meas_graphics_update();
int meas_graphics_update2d(int, int, int, double *, double *, int);
int meas_graphics_update3d(int, double *, int, int, double, double, double, double);
int meas_graphics_xtitle(int, char *), meas_graphics_ytitle(int, char *);
int meas_graphics_comment(int, char *), meas_graphics_save(char *), meas_graphics_autoscale(int);
int meas_graphics_xautoscale(int, double, double), meas_graphics_yautoscale(int, double, double), meas_graphics_close();
int meas_graphics_type(int, char);
int meas_graphics_pixel3d(int);
int meas_graphics_line_color(int, int);
int meas_graphics_colormap(int, int);
int meas_graphics_xaxis3d(int);
int meas_graphics_yaxis3d(int);
int meas_graphics_zlog(int, int);
