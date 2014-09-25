/*
 * Graphics functions implemented using the plplot library.
 * It is possible to do more advanced output by accessing the graphs with
 * the native plplot calls.
 *
 * TODO: add colormap functions.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <forms.h>
#include "graphics.h"
#include "misc.h"

#define MAX_LABEL 256

static struct window {
  int type;          /* not in use (0), 2D (1) or contour (3) */
  int nx, ny, ns, maxns;        /* for 2-D ns indicates the data length */
  unsigned char *img_data;   /* ordered: b, g, r, 0 */
  float *xvalues;
  char xtitle[MAX_LABEL];
  float *yvalues;
  char ytitle[MAX_LABEL];
  FL_FORM *form;
  FL_OBJECT *canvas;   /* XY plot area or image area */
  GC canvasGC;
  XVisualInfo visualinfo;
  XImage *img;
  Colormap map;
} wins[MEAS_GRAPHICS_MAX_WIN];  

/*
 * Initialize graphics display.
 * 
 * win   = window to init (int).
 * type  = 2D graph or image (MEAS_GRAPHICS_XY or MEAS_GRAPHICS_IMAGE).
 * nx    = window size along x direction.
 * ny    = window size along y direction.
 * maxns = maximum number of data points for 2D plots.
 * title = window title.
 *
 * Note: The window numbering starts from 0.
 *
 * Return 0 on success.
 *
 */

int meas_graphics_init(int win, int type, int nx, int ny, int maxns, char *title) {

  static int been_here = 0;
  char *av[1] = {""};
  int ac = 1;

  if(!been_here) {
    int i;
    for (i = 0; i < MEAS_GRAPHICS_MAX_WIN; i++) {
      wins[i].type = MEAS_GRAPHICS_EMPTY;
      wins[i].canvasGC = NULL;
      wins[i].img = NULL;
      wins[i].form = NULL;
      wins[i].canvas = NULL;
      wins[i].img_data = NULL;
      wins[i].nx = wins[i].ny = wins[i].ns = 0;
      wins[i].xvalues = wins[i].yvalues = NULL;
    }
    been_here = 1;
  }

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_init: Illegal window number specified.\n");
  
  fl_initialize(&ac, av, "meas", 0, 0);
    
  wins[win].type = type;
  wins[win].nx = nx;
  wins[win].ny = ny;
  wins[win].ns = 0;
  switch(type) {
  case MEAS_GRAPHICS_XY:
    wins[win].form = fl_bgn_form(FL_NO_BOX, nx, ny);
    wins[win].canvas = fl_add_xyplot(FL_NORMAL_XYPLOT, 0, 0, nx, ny, title);
    fl_end_form();
    if(!(wins[win].xvalues = (float *) malloc(sizeof(float) * maxns)))
      meas_err("meas_graphics_init: out of memory.");
    if(!(wins[win].yvalues = (float *) malloc(sizeof(float) * maxns)))
      meas_err("meas_graphics_init: out of memory.");
    wins[win].img_data =  NULL;
    wins[win].img = NULL;
    wins[win].xtitle[0] = wins[win].ytitle[0] = 0;
    fl_show_form(wins[win].form, FL_PLACE_FREE, FL_FULLBORDER, "");
    break;
  case MEAS_GRAPHICS_IMAGE:
    if(!(wins[win].img_data = (unsigned char *) malloc(4 * sizeof(unsigned char) * nx * ny)))
      meas_err("meas_graphics_init: out of memory.");
    wins[win].form = fl_bgn_form(FL_NO_BOX, nx, ny);
    wins[win].canvas = fl_add_canvas(FL_NORMAL_CANVAS, 0, 0, nx, ny, "");
    fl_end_form();
    wins[win].canvasGC = XCreateGC(fl_get_display(), fl_state[fl_vmode].trailblazer, 0, 0);
    fl_show_form(wins[win].form, FL_PLACE_FREE, FL_FULLBORDER, title);
    XMatchVisualInfo(fl_get_display(), 0, 32, 0, &(wins[win].visualinfo));
    wins[win].img = XCreateImage(fl_get_display(), wins[win].visualinfo.visual, fl_get_canvas_depth(wins[win].canvas), ZPixmap, 0, (char *) wins[win].img_data, nx, ny, 32, 0);
    wins[win].map = fl_create_colormap(fl_state[fl_vmode].xvinfo, 30);
    fl_set_canvas_colormap(wins[win].canvas, wins[win].map);
    wins[win].xvalues = wins[win].yvalues = NULL;
    break;
  default:
    meas_err("meas_graphics_init: Illegal window type.");
  }
  
  return 0;
}

/*
 * Set up X axis scale and ticks (2D plot).
 *
 * win       = window number to apply the axis definition on.
 * begin     = X axis begin.
 * end       = X axis end.
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_xscale(int win, double begin, double end) {

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_xscale: Illegal window id.");

  if(wins[win].type != MEAS_GRAPHICS_XY) meas_err("meas_graphics_xscale: wrong window type.");

  fl_set_xyplot_xbounds(wins[win].canvas, begin, end);
  return 0;
}

/*
 * Set up Y axis scale and ticks.
 *
 * win       = window number to apply the axis definition on.
 * begin     = Y axis begin.
 * end       = Y axis end.
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_yscale(int win, double begin, double end) {

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_yscale: Illegal window id.");

  fl_set_xyplot_ybounds(wins[win].canvas, begin, end);
  return 0;
}

/*
 * Clear specified window.
 *
 * win = window number to clear.
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_clear(int win) {

  float x, y;

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_clear: Illegal window id.");  
  
  switch(wins[win].type) {
  case MEAS_GRAPHICS_EMPTY:
    break;
  case MEAS_GRAPHICS_XY:
    fl_set_xyplot_data(wins[win].canvas, &x, &y, 0, "", "", "");
    break;
  case MEAS_GRAPHICS_IMAGE:
    bzero(wins[win].img_data, sizeof(unsigned char) * 4);
    break;
  default:
    meas_err("meas_graphics_clear: Illegal window type.");
  }
  fl_check_forms();
  return 0;
}

/*
 * Set X axis title.
 *
 * win = window number.
 * str = window title string.
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_xtitle(int win, char *str) {

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_xtitle: Illegal window id.");  
  strncpy(wins[win].xtitle, str, MAX_LABEL);
  return 0;
}

/*
 * Set Y axis title.
 *
 * win = window number.
 * str = window title string.
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_ytitle(int win, char *str) {

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_ytitle: Illegal window id.");  
  strncpy(wins[win].ytitle, str, MAX_LABEL);
  return 0;
}

/*
 * Update graphics output.
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_update() {

  int i;

  for (i = 0; i < MEAS_GRAPHICS_MAX_WIN; i++)
    if(wins[i].type == MEAS_GRAPHICS_IMAGE)
      XPutImage(fl_get_display(), fl_get_canvas_id(wins[i].canvas), wins[i].canvasGC, wins[i].img, 0, 0, 0, 0, wins[i].nx, wins[i].ny);
  fl_check_forms(); /* 2D taken care by xforms */
  return 0;
}

/*
 * Update data points in given window & 2D data set.
 *
 * win    = window number (0, ...).
 * xdata  = an array of X points.
 * ydata  = an array of Y points.
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_update_xy(int win, double *xdata, double *ydata, int ns) {

  int i;

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_update2d: Illegal window id.");  
  if(wins[win].type == MEAS_GRAPHICS_IMAGE || wins[win].type == MEAS_GRAPHICS_EMPTY)
    meas_err("meas_graphics_update2d: Wrong data set type.");
  if(ns >= wins[win].maxns) {
    free(wins[win].xvalues);
    free(wins[win].yvalues);
    if(!(wins[win].xvalues = (float *) malloc(sizeof(float) * ns)))
      meas_err("meas_graphics_init: out of memory.");
    if(!(wins[win].yvalues = (float *) malloc(sizeof(float) * ns)))
      meas_err("meas_graphics_init: out of memory.");
    wins[win].maxns = ns;
  }
  wins[win].ns = ns;
  for (i = 0; i < wins[win].ns; i++) {
    wins[win].xvalues[i] = xdata[i];
    wins[win].yvalues[i] = ydata[i];
  }
  fl_set_xyplot_data(wins[win].canvas, wins[win].xvalues, wins[win].yvalues, wins[win].ns, "", wins[win].xtitle, wins[win].ytitle);
  return 0;
}

/*
 * Update data points in given window & 3D data set (contour plot).
 *
 * win     = window number (0, ...).
 * r       = red component (unsigned char; 0 - 255).
 * g       = green component (unsigned char; 0 - 255).
 * b       = blue component (unsigned char; 0 - 255).
 *
 * r, g, b arrays form a two dimensional array that defines the image.
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_update_image(int win, unsigned char *r, unsigned char *g, unsigned char *b) {

  int i, k;

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_update3d: Illegal window id.");  

  k = 0;
  for (i = 0; i < wins[win].nx * wins[win].ny; i++) {
    wins[win].img_data[k] = b[i]; 
    wins[win].img_data[k+1] = g[i];
    wins[win].img_data[k+2] = r[i];
    wins[win].img_data[k+3] = 0;
    k += 4;
  }

  return 0;
}

/*
 * Autoscale X axis.
 *
 * win    = window number to autoscale.
 *
 * Return 0 on success.
 *
 */

int meas_graphics_xautoscale(int win) {

  int i;
  double xmin = 1E99, xmax = -xmin;
  float *tmp;

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_xautoscale: Illegal window id.");  
  if(wins[win].type != MEAS_GRAPHICS_XY)
    meas_err("meas_graphics_xautoscale: Illegal window type for autoscale.\n");

  tmp = wins[win].xvalues;
  for(i = 0; i < wins[win].nx; i++) {
    if(tmp[i] < xmin) xmin = tmp[i];
    if(tmp[i] > xmax) xmax = tmp[i];
  }
  if(xmin == xmax) {
    xmin = 0.0;
    xmax = 1.0;
  }
  meas_graphics_xscale(win, xmin, xmax);
  return 0;
}

/*
 * Autoscale Y axis.
 *
 * win    = window number to autoscale.
 *
 * Return 0 on success.
 *
 */

int meas_graphics_yautoscale(int win) {

  int i;
  double ymin = 1E99, ymax = -ymin;
  float *tmp;

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_yautoscale: Illegal window id.");  
  if(wins[win].type != MEAS_GRAPHICS_XY)
    meas_err("meas_graphics_yautoscale: Illegal window type for autoscale.\n");

  tmp = wins[win].yvalues;
  for(i = 0; i < wins[win].nx; i++) {
    if(tmp[i] < ymin) ymin = tmp[i];
    if(tmp[i] > ymax) ymax = tmp[i];
  }
  if(ymin == ymax) {
    ymin = 0.0;
    ymax = 1.0;
  }
  meas_graphics_yscale(win, ymin, ymax);
  return 0;
}


/*
 * Autoscale X and Y axes.
 *
 * win    = window number to autoscale.
 *
 * Return 0 on success.
 *
 */

int meas_graphics_autoscale(int win) {

  meas_graphics_xautoscale(win);
  meas_graphics_yautoscale(win);  
  return 0;
}

/*
 * Close graphics output.
 *
 */

int meas_graphics_close() {

  int i;

  fl_finish();
  for(i = 0; i < MEAS_GRAPHICS_MAX_WIN; i++) {
    if(wins[i].form) fl_free(wins[i].form);
    if(wins[i].xvalues) free(wins[i].xvalues);
    if(wins[i].yvalues) free(wins[i].yvalues);
  }
  return 0;
}

/*
 * Map a floating point value to RGB.
 *
 * value = value to be mapped (between 0.0 and 1.0; double).
 * r     = r(ed) value (unsigned char *).
 * g     = g(reen) value (unsigned char *).
 * b     = b(lue) value (unsigned char *).
 *
 */

void meas_graphics_rgb(double value, unsigned char *r, unsigned char *g, unsigned char *b) {

  if(value < 0.0) value = 0.0;
  if(value > 1.0) value = 1.0;

  if(value > 0.5) *r = 0;
  else *r = (int) (255.0 * 2.0 * (0.5 - value));

  if(value < 0.5) *g = (int) (255.0 * (2.0 * value));
  else *g = (int) (255.0 * (2.0 * (1.0 - value)));

  if(value < 0.5) *b = 0;
  else *b = (int) (255.0 * (2.0 * (value - 0.5)));

}

/*
 * Contour plot based on a array (double).
 *
 * win = window number (int).
 * data= data, which is indexed according to nx, ny (double *).
 *
 */

void meas_graphics_update_image_contour(int win, double *data) {

  int i, j;
  double mi = 1E99, ma = -1E99;

  for (i = 0; i < wins[win].nx * wins[win].ny; i++) {
    if(data[i] > ma) ma = data[i];
    if(data[i] < mi) mi = data[i];
  }
  fprintf(stderr, "libmeas: Minimum contour = %le, Maximum contour = %le.\n", mi, ma);
  j = 0;
  for (i = 0; i < wins[win].nx * wins[win].ny; i++) {
    meas_graphics_rgb((data[i] - mi) / (ma - mi), &wins[win].img_data[j+2], &wins[win].img_data[j+1], &wins[win].img_data[j]);
    wins[win].img_data[j+3] = 0;
    j += 4;
  }
}

/*
 * Convert pi-max (16 bit unsigned) gray scale to RGB.
 *
 * img  = Gray scale image array (16 bit; unsigned short *; length nx * ny; input).
 * nx   = number of pixels along x (unsigned int; input).
 * ny   = number of pixels along y (unsigned int; input).
 * r    = red component (8 bit; unsigned char *; length nx * ny; output).
 * g    = green component (8 bit; unsigned char *; length nx * ny; output).
 * b    = blue component (8 bit; unsigned char *; length nx * ny; output).
 * as   = 0: auto scale, > 0: set image maximum pixel value to as (use as = 65535 for no scaling) (unsigned short).
 * 
 * Returns -1 on failure.
 *
 */

int meas_graphics_convert_img_to_rgb(unsigned short *img, unsigned int nx, unsigned int ny, unsigned char *r, unsigned char *g, unsigned char *b, unsigned short as) {

  unsigned int i;

  if(!as) {
    as = 0;
    for (i = 0; i < nx * ny; i++)
      if(img[i] > as) as = img[i];
  }

  for(i = 0; i < nx * ny; i++) {
    r[i] = (unsigned char) ((255.0 / (double) as) * (double) img[i]);
    g[i] = (unsigned char) ((255.0 / (double) as) * (double) img[i]);
    b[i] = (unsigned char) ((255.0 / (double) as) * (double) img[i]);
  }
  return 0;
}

/*
 * Scale up RGB image.
 *
 * ri = Input red component (unsigned char *; input).
 * gi = Input green component (unsigned char *; input).
 * bi = Input blue component (unsigned char *; input).
 * nx = Image size along x (unsigned int; input).
 * ny = Image size along y (unsigned int; input).
 * sc = Scale: 1, 2, 3 ... (unsigned int; input).
 * ro = Output red component (unsigned char *; output). Note size nx * ny * sc.
 * go = Output green component (unsigned char *; output). Note size nx * ny * sc.
 * bo = Output blue component (unsigned char *; output). Note size nx * ny * sc.
 * 
 * Returns -1 on failure.
 *
 * TODO: check array indexing!!!! 
 *
 */

int meas_graphics_scale_rgb(unsigned char *ri, unsigned char *gi, unsigned char *bi, unsigned int nx, unsigned int ny, unsigned int sc, unsigned char *ro, unsigned char *go, unsigned char *bo) {

  unsigned int i, j, si, sj;

  if(sc < 1 || sc > 10) meas_err("meas_graphics_scale_rgb: Illegal scale value.\n");
  if(sc == 1) {
    for (i = 0; i < nx*ny; i++) {
      ro[i] = ri[i];
      go[i] = gi[i];
      bo[i] = bi[i];
    }
    return 0;
  }
  for (i = 0; i < nx; i++)
    for (j = 0; j < ny; j++)
      for (si = 0; si < sc; si++)
	for (sj = 0; sj < sc; sj++) {
	  ro[(j * sc + sj) * nx * sc + (i * sc + si)] = ri[j * nx + i];
	  go[(j * sc + sj) * nx * sc + (i * sc + si)] = gi[j * nx + i];
	  bo[(j * sc + sj) * nx * sc + (i * sc + si)] = bi[j * nx + i];
	}
  return 0;
}

