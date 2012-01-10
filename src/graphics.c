/*
 * Graphics functions implemented using the plplot library.
 * It is possible to do more advanced output by accessing the graphs with
 * the native plplot calls.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <plplot/plplot.h>
#include "graphics.h"
#include "misc.h"

#define EPS 1.0E-1

/* TODO: too many static things... limits things and is a memory hog */

static int max_windows = 0;
static struct window {
  char comment[MEAS_GRAPHICS_MAX_STRING];
  double xmin, xmax;  /* min & max values for X axis */
  double ymin, ymax;  /* min & max values for Y axis */
  double zmin, zmax;  /* min & max values for Z axis */
  double xmaticks, xmiticks; /* ticks */
  double ymaticks, ymiticks; /* ticks */
  int zaxis_scale;   /* 0 = linear, 1 = log */
  char xlabel[MEAS_GRAPHICS_MAX_STRING];
  char ylabel[MEAS_GRAPHICS_MAX_STRING];
  char title[MEAS_GRAPHICS_MAX_STRING];
  int type;          /* not in use (0), 2D (1) or contour (3) */
  int colormap;      /* colormap to use: 0 = rgb, 1 = gray scale */
  int colormap_init; /* colormap initialized? */
  int color;
  void *data;        /* data for window */
} wins[MEAS_GRAPHICS_MAX_WIN];  

struct data2d {
  int color[MEAS_GRAPHICS_MAX_SETS];
  double xvalues[MEAS_GRAPHICS_MAX_SETS][MEAS_GRAPHICS_MAX_POINTS];
  double yvalues[MEAS_GRAPHICS_MAX_SETS][MEAS_GRAPHICS_MAX_POINTS];
  int lengths[MEAS_GRAPHICS_MAX_SETS];
};

struct data3d {
  int nx, ny;
  double ox, oy;
  double sx, sy;
  double *zdata[MEAS_GRAPHICS_MAX_2DPOINTS];
};

static int meas_graphics_pixel_style = 0; /* -1 = one pixel (looks ugly but faster) or 0 (looks good but slower) */
static int meas_graphics_axis_xdir = 0; /* X axis direction for 3D plots */
static int meas_graphics_axis_ydir = 0; /* Y axis direction for 3D plots */

static void coords(double x, double y, double *tx, double *ty, void *data) {

  struct data3d *tmp2;

  tmp2 = (struct data3d *) data;
  *tx = tmp2->ox + tmp2->sx * x;
  *ty = tmp2->oy + tmp2->sy * y;
}

/*
 * Initialize graphics display.
 * 
 * nwin   = number of graphics in the display.
 * driver = PLPLOT driver name (e.g., xwin, ps, psc, svg, etc.)
 *          NULL defaults to xwin.
 * file   = Output file (use NULL with xwin).
 *
 * Note: The window numbering starts from 0.
 *
 * Return 0 on success.
 *
 */

int meas_graphics_init(int nwin, char *driver, char *file) {

  int i, j;

  if(nwin <= 0 || nwin >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_init: Illegal number of windows specified.\n");
  max_windows = nwin;

  if(driver == NULL) driver = "xwin";
  if(!strcmp(driver, "xwin")) {
    plsetopt("drvopt","usepth=0");
    plsetopt("db","");
  } else {
    /* use family of output files (i.e., each frame into a separate file) */
    plsfam(1, 0, 1);
    plsfnam(file);
  }

  /* PS files have 60 dpi resolution (the files become very big with larger dpi ) */
  if(!strcmp(driver, "ps") || !strcmp(driver, "psc")) 
    plsetopt("dpi", "100");

  if(file != NULL) plsetopt("o", file);

  plsdev(driver);

  switch(nwin) {
  case 1:
    plssub(1, 1);
    break;
  case 2:
    plssub(1, 2);
    break;
  case 3:
    plssub(1, 3);
    break;
  case 4:
    plssub(2, 2);
    break;
  default:
    meas_err("meas_graphics_init: Unknown window layout scheme.");
  }
  plinit();
  plspause(0);
  for (i = 0; i < nwin; i++) {
    wins[i].type = MEAS_GRAPHICS_EMPTY;
    wins[i].xmin = wins[i].ymin = 0.0;
    wins[i].xmax = wins[i].ymax = 1.0;
    wins[i].zaxis_scale = 0;
    wins[i].xmaticks = wins[i].xmiticks = wins[i].ymaticks = wins[i].ymiticks = 0.0;
    wins[i].title[0] = wins[i].xlabel[0] = wins[i].ylabel[0] = 0;
    wins[i].data = (void *) NULL;
    wins[i].colormap = 0;       /* default colormap */
    wins[i].colormap_init = 0;
    wins[i].color = MEAS_GRAPHICS_WHITE;
  }
  return 0;
}

/*
 * Set up X axis scale and ticks.
 *
 * win       = window number to apply the axis definition on.
 * begin     = X axis begin.
 * end       = X axis end.
 * tickmajor = major tick spacing on X axis.
 * tickminor = minor tick spacing on X axis.
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_xscale(int win, double begin, double end, double tickmajor, double tickminor) {

  int tmp1, tmp2;

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_xscale: Illegal window id.");
  wins[win].xmin = begin;
  wins[win].xmax = end;
  wins[win].xmaticks = tickmajor;
  wins[win].xmiticks = tickminor;
#if 0 /* What's this? */
  if(wins[win].xmaticks > 0.0) tmp1 = (int) (wins[win].xmaticks / wins[win].xmiticks);
  else tmp1 = 0;
  if(wins[win].ymaticks > 0.0) tmp2 = (int) (wins[win].ymaticks / wins[win].ymiticks);
  else tmp2 = 0;
#endif
  return 0;
}

/*
 * Set up Y axis scale and ticks.
 *
 * win       = window number to apply the axis definition on.
 * begin     = Y axis begin.
 * end       = Y axis end.
 * tickmajor = major tick spacing on Y axis.
 * tickminor = minor tick spacing on Y axis.
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_yscale(int win, double begin, double end, double tickmajor, double tickminor) {

  int tmp1, tmp2;

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_yscale: Illegal window id.");
  wins[win].ymin = begin;
  wins[win].ymax = end;
  wins[win].ymaticks = tickmajor;
  wins[win].ymiticks = tickminor;
#if 0 /* what's this? */
  if(wins[win].xmaticks > 0.0) tmp1 = (int) (wins[win].xmaticks / wins[win].xmiticks);
  else tmp1 = 0;
  if(wins[win].ymaticks > 0.0) tmp2 = (int) (wins[win].ymaticks / wins[win].ymiticks);
  else tmp2 = 0;
#endif

  return 0;
}

/*
 * Set up Z axis scale (color).
 *
 * win       = window number to apply the axis definition on.
 * begin     = minimum Z value.
 * end       = maximum Z value.
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_zscale(int win, double begin, double end) {

  int tmp1, tmp2;

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_zscale: Illegal window id.");
  wins[win].zmin = begin;
  wins[win].zmax = end;
  return 0;
}

/*
 * Set z-axis scale as linear or logarithmic.
 *
 * win   = window number.
 * scale = MEAS_GRAPHICS_LINEAR3D (for linear) and
 *         MEAS_GRAPHICS_LOG3D (for logarithmic).
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_zlog(int win, int scale) {

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_zlog: Illegal window id.");
  if(scale != MEAS_GRAPHICS_LINEAR3D && scale != MEAS_GRAPHICS_LOG3D)
    meas_err("meas_graphics_zlog: Illegal z-axis scale.");
  wins[win].zaxis_scale = scale;
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

  int i;

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_clear: Illegal window id.");  
  plbop();
  pladv(win+1);
  plclear();
  plvsta();
  plwind(wins[win].xmin, wins[win].xmax, wins[win].ymin, wins[win].ymax);
  pleop();
  return 0;
}

/*
 * Set window title.
 *
 * win = window number.
 * str = window title string.
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_title(int win, char *str) {

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_title: Illegal window id.");  
  strncpy(wins[win].title, str, MEAS_GRAPHICS_MAX_STRING);
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
  strncpy(wins[win].xlabel, str, MEAS_GRAPHICS_MAX_STRING);
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
  strncpy(wins[win].ylabel, str, MEAS_GRAPHICS_MAX_STRING);
  return 0;
}

/*
 * Set 3D plot pixel style.
 *
 * value = Fast plotting (MEAS_GRAPHICS_FAST3D) or
 *         Smooth plotting (MEAS_GRAPHICS_SMOOTH3D).
 *
 */ 

int meas_graphics_pixel3d(int value) {

  if(value != MEAS_GRAPHICS_FAST3D && value != MEAS_GRAPHICS_SMOOTH3D)
    meas_err("meas_graphics_pixel3d: Illegal value.\n");
  meas_graphics_pixel_style = value;
  return 0;
}

/*
 * Normal/Reverted X axis for 3D plots.
 *
 * dir = Normal (MEAS_GRAPHICS_NORMAL3D) or
 *       Reverted (MEAS_GRAPHICS_REV3D).
 */

int meas_graphics_xaxis3d(int dir) {

  if(dir != MEAS_GRAPHICS_NORMAL3D && dir != MEAS_GRAPHICS_REV3D)
    meas_err("meas_graphics_xaxis3d: Illegal axis direction.");
  meas_graphics_axis_xdir = dir;
  return 0;
}

/*
 * Normal/Reverted Y axis for 3D plots.
 *
 * dir = Normal (MEAS_GRAPHICS_NORMAL3D) or
 *       Reverted (MEAS_GRAPHICS_REV3D).
 */

int meas_graphics_yaxis3d(int dir) {

  if(dir != MEAS_GRAPHICS_NORMAL3D && dir != MEAS_GRAPHICS_REV3D)
    meas_err("meas_graphics_yaxis3d: Illegal axis direction.");
  meas_graphics_axis_ydir = dir;
  return 0;
}

/*
 * Update graphics output.
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_update() {

  int i, j, k;
  struct data2d *tmp1;
  struct data3d *tmp2;
  double zmin, zmax;
  double ii[3], r[3], g[3], b[3];

  plbop();
  for (j = 0; j < max_windows; j++) {
    pladv(j+1);
    plclear();
    plvsta();
    plwind(wins[j].xmin, wins[j].xmax, wins[j].ymin, wins[j].ymax);
    plcol0(wins[j].color);
    plbox("bcnst", 0.0, 0, "bcnstv", 0.0, 0);
    if(wins[j].xmiticks == 0) wins[j].xmiticks = wins[j].xmaticks;
    if(wins[j].ymiticks == 0) wins[j].ymiticks = wins[j].ymaticks;
    plbox("afn", wins[j].xmaticks, (int) (wins[j].xmaticks / wins[j].xmiticks) - 1,
    	  "afn", wins[j].ymaticks, (int) (wins[j].ymaticks / wins[j].ymiticks) - 1);
    pllab(wins[j].xlabel, wins[j].ylabel, wins[j].title);
    switch(wins[j].type) {
    case MEAS_GRAPHICS_EMPTY:
      break;
    case MEAS_GRAPHICS_2D:
      tmp1 = (struct data2d *) wins[j].data;
      for (i = 0; i < MEAS_GRAPHICS_MAX_SETS; i++) {
	plcol0(tmp1->color[i]);
	plline(tmp1->lengths[i], tmp1->xvalues[i], tmp1->yvalues[i]);
      }
      break;
    case MEAS_GRAPHICS_3D:
      tmp2 = (struct data3d *) wins[j].data;
      if(wins[j].zmin == wins[j].zmax) {
	zmin = 1E99; zmax = -zmin;
	for (i = 0; i < tmp2->nx; i++)
	  for (k = 0; k < tmp2->ny; k++) {
	    if(tmp2->zdata[i][k] < zmin) zmin = tmp2->zdata[i][k];
	    if(tmp2->zdata[i][k] > zmax) zmax = tmp2->zdata[i][k];
	  }
      } else {
	zmax = wins[j].zmax;
	zmin = wins[j].zmin;
      }
      if(!wins[j].colormap_init) {
	switch(wins[j].colormap) {
	case MEAS_GRAPHICS_RGB:
	  ii[0] = 0.0; r[0] = 0.0; g[0] = 0.0; b[0] = 1.0;
	  ii[1] = 0.5; r[1] = 1.0; g[1] = 1.0; b[1] = 0.0;
	  ii[2] = 1.0; r[2] = 1.0; g[2] = 0.0; b[2] = 0.0;
	  break;
	case MEAS_GRAPHICS_GRAY:
	  ii[0] = 0.0; r[0] = 0.0; g[0] = 0.0; b[0] = 0.0;
	  ii[1] = 0.5; r[1] = 0.5; g[1] = 0.5; b[1] = 0.5;
	  ii[2] = 1.0; r[2] = 1.0; g[2] = 1.0; b[2] = 1.0;
	  break;
	}
	plscmap1l(1, 3, ii, r, g, b, NULL);      
	wins[j].colormap_init = 1;
      }
      for (i = 0; i < tmp2->nx; i++)
	for (k = 0; k < tmp2->ny; k++) {
	  double xx, yy, t;
	  t = (tmp2->zdata[k][i] - zmin) / fabs(zmax - zmin);
	  if(t < 0.0) t = 0.0;
	  if(t > 1.0) t = 1.0;
	  if(wins[j].zaxis_scale == MEAS_GRAPHICS_LOG3D) {
	    t += EPS;
	    t = (log(t) - log(EPS)) / (M_E - log(EPS));
	  }
	  plcol1(t);
	  if(meas_graphics_axis_xdir == MEAS_GRAPHICS_NORMAL3D)
	    xx = i;
	  else
	    xx = tmp2->nx - i - 1;
	  if(meas_graphics_axis_ydir == MEAS_GRAPHICS_NORMAL3D)
	    yy = k;
	  else
	    yy = tmp2->ny - k - 1;
	  plpoin(1, &xx, &yy, meas_graphics_pixel_style);
	}
      break;
    }
  }
  pleop();
  return 0;
}

/*
 * Update data points in given window & 2D data set.
 *
 * win    = window number (0, ...).
 * set    = data set number (0, ...).
 * color  = color number (see graphics.h for color definitions).
 * xdata  = an array of X points.
 * ydata  = an array of Y points.
 * length = length of the xdata and ydata arrays.
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_update2d(int win, int set, int color, double *xdata, double *ydata, int length) {

  int i, j;
  struct data2d *tmp;
  struct data3d *tmp2;

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_update2d: Illegal window id.");  
  if(length >= MEAS_GRAPHICS_MAX_POINTS)
    meas_err("meas_graphics_update2d: Too many data points. Increase MAX_POINTS in graphics.h");
  if(set < 0 || set >= MEAS_GRAPHICS_MAX_SETS)
    meas_err("meas_graphics_update2d: Incorrect set number.");
  if(wins[win].type != MEAS_GRAPHICS_2D && wins[win].type != MEAS_GRAPHICS_EMPTY) {
    tmp2 = (struct data3d *) wins[win].data;
    for(i = 0; i < MEAS_GRAPHICS_MAX_2DPOINTS; i++)
      free(tmp2->zdata[i]);
    free(wins[win].data);
  }
  wins[win].type = MEAS_GRAPHICS_2D;
  if(!(tmp = wins[win].data)) {
    if(!(tmp = wins[win].data = (void *) malloc(sizeof(struct data2d))))
      meas_err("meas_graphics_update2d: Out of memory.");
    for(i = 0; i < MEAS_GRAPHICS_MAX_SETS; i++) tmp->lengths[i] = 0;
  }
  tmp->lengths[set] = length;
  tmp->color[set] = color;
  bcopy(xdata, tmp->xvalues[set], sizeof(double) * length);
  bcopy(ydata, tmp->yvalues[set], sizeof(double) * length);
  return 0;
}

/*
 * Update data points in given window & 3D data set (contour plot).
 *
 * win     = window number (0, ...).
 * zdata   = two dimensional array of Z points.
 * nx      = number of points along the X axis.
 * ny      = number of points along the Y axis.
 * ox      = origin (X)
 * oy      = origin (Y)
 * sx      = step (X)
 * sy      = step (Y)
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_update3d(int win, double *zdata, int nx, int ny, double ox, double oy, double sx, double sy) {

  int i, j, len;
  struct data3d *tmp;

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_update3d: Illegal window id.");  
  len = nx * ny;
  if(nx > MEAS_GRAPHICS_MAX_2DPOINTS || ny > MEAS_GRAPHICS_MAX_2DPOINTS)
    meas_err("meas_graphics_update3d: Too many data points. Increase MAX_POINTS in graphics.h");
  if(wins[win].type != MEAS_GRAPHICS_3D && wins[win].type != MEAS_GRAPHICS_EMPTY)
    free(wins[win].data);
  wins[win].type = MEAS_GRAPHICS_3D;
  if(!(tmp = wins[win].data)) {
    if(!(tmp = wins[win].data = (void *) malloc(sizeof(struct data3d))))
      meas_err("meas_graphics_update3d: Out of memory.");
    for(i = 0; i < MEAS_GRAPHICS_MAX_2DPOINTS; i++)
      if(!(tmp->zdata[i] = (double *) malloc(sizeof(double) * MEAS_GRAPHICS_MAX_2DPOINTS)))
	meas_err("meas_graphics_update3d: Out of memory.");
  }
  tmp->nx = nx;
  tmp->ny = ny;
  tmp->ox = ox;
  tmp->oy = oy;
  tmp->sx = sx;
  tmp->sy = sy;
  wins[win].xmin = ox;
  wins[win].xmax = ox + sx * (double) nx;
  wins[win].ymin = oy;
  wins[win].ymax = oy + sy * (double) ny;
  // wins[win].zmin = wins[win].zmax = 0.0; /* default to autoscale */
  for (i = 0; i < ny; i++)
    bcopy(&zdata[i*nx], tmp->zdata[i], sizeof(double) * nx);
  return 0;
}

/*
 * Insert comment string for a data set in a given window.
 *
 * win     = window number.
 * set     = dataset number.
 * comment = comment string.
 *
 * The comment string will be saved by meas_graphics_save().
 *
 * Returns 0 on success.
 *
 */

 int meas_graphics_comment(int win, char *comment) {

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_comment: Illegal window id.");  
  strncpy(wins[win].comment, comment, MEAS_GRAPHICS_MAX_STRING);
  return 0;
}

/*
 * Save datasets in all windows to a file.
 *
 * file = output file name.
 *
 * Returns 0 on success.
 *
 */

int meas_graphics_save(char *file) {

  FILE *fp;
  int i, j, k;
  struct data2d *tmp1;
  struct data3d *tmp2;
  
  if(!(fp = fopen(file, "w")))
    meas_err("meas_graphics_save: Can't open output file.");
  for(k = 0; k < MEAS_GRAPHICS_MAX_WIN; k++)
    switch(wins[k].type) {
    case MEAS_GRAPHICS_EMPTY:
      break;
    case MEAS_GRAPHICS_2D:
      tmp1 = (struct data2d *) wins[k].data;
      for(i = 0; i < MEAS_GRAPHICS_MAX_SETS; i++)
	if(tmp1->lengths[i] != 0) {
	  fprintf(fp, "# Graph %d\n", k);
	  fprintf(fp, "# Set %d\n", i);
	  fprintf(fp, "# %s\n", wins[k].comment);
	  for (j = 0; j < tmp1->lengths[i]; j++) 
	    fprintf(fp, "%le %le\n", tmp1->xvalues[i][j], tmp1->yvalues[i][j]);
	  fprintf(fp, "\n");
	}
      break;
    case MEAS_GRAPHICS_3D:
      tmp2 = (struct data3d *) wins[k].data;
      fprintf(fp, "# Graph %d\n", k);
      fprintf(fp, "# nx = %d, ny = %d\n", tmp2->nx, tmp2->ny);
      for (i = 0; i < tmp2->nx; i++)
	for (j = 0; j < tmp2->ny; j++)
	  fprintf(fp, "%d\n", tmp2->zdata[tmp2->nx * i + j]);
      fprintf(fp, "\n");
      break;
    }
  fclose(fp);
  return 0;
}

/*
 * Autoscale X axis.
 *
 * win    = window number to autoscale.
 * minadd = add this much to the obtained optimal X axis minimum scale.
 * maxadd = add this much to the obtained optimal X axis maximum scale.
 *
 * Return 0 on success.
 *
 */

int meas_graphics_xautoscale(int win, double minadd, double maxadd) {

  int i, j;
  struct data2d *tmp1;

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_xautoscale: Illegal window id.");  
  if(wins[win].type != MEAS_GRAPHICS_2D)
    meas_err("meas_graphics_xautoscale: Illegal window type for autoscale.\n");
  wins[win].xmin = 1E99;
  wins[win].xmax = -1E99;
  tmp1 = (struct data2d *) wins[win].data;
  for (i = 0; i < MEAS_GRAPHICS_MAX_SETS; i++) {
    for (j = 0; j < tmp1->lengths[i]; j++) {
      if(tmp1->xvalues[i][j] < wins[win].xmin) wins[win].xmin = tmp1->xvalues[i][j];
      if(tmp1->xvalues[i][j] > wins[win].xmax) wins[win].xmax = tmp1->xvalues[i][j];
    }
  }
  if(wins[win].xmin == wins[win].xmax) {
    wins[win].xmin = 0.0;
    wins[win].xmax = 1.0;
  }
  wins[win].xmin += minadd;
  wins[win].xmax += maxadd;
  return 0;
}

/*
 * Autoscale Y axis.
 *
 * win    = window number to autoscale.
 * minadd = add this much to the obtained optimal Y axis minimum scale.
 * maxadd = add this much to the obtained optimal Y axis maximum scale.
 *
 * Return 0 on success.
 *
 */

int meas_graphics_yautoscale(int win, double minadd, double maxadd) {

  int i, j;
  struct data2d *tmp1;

  if(win < 0 || win >= MEAS_GRAPHICS_MAX_WIN)
    meas_err("meas_graphics_yautoscale: Illegal window id.");  
  if(wins[win].type != MEAS_GRAPHICS_2D)
    meas_err("meas_graphics_yautoscale: Illegal window type for autoscale.\n");
  wins[win].ymin = 1E99;
  wins[win].ymax = -1E99;
  tmp1 = (struct data2d *) wins[win].data;
  for (i = 0; i < MEAS_GRAPHICS_MAX_SETS; i++) {
    for (j = 0; j < tmp1->lengths[i]; j++) {
      if(tmp1->yvalues[i][j] < wins[win].ymin) wins[win].ymin = tmp1->yvalues[i][j];
      if(tmp1->yvalues[i][j] > wins[win].ymax) wins[win].ymax = tmp1->yvalues[i][j];
    }
  }
  if(wins[win].ymin == wins[win].ymax) {
    wins[win].ymin -= 10.0;
    wins[win].ymax += 10.0;
  }
  wins[win].ymin += minadd;
  wins[win].ymax += maxadd;
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

  meas_graphics_xautoscale(win, 0.0, 0.0);
  meas_graphics_yautoscale(win, 0.0, 0.0);  
  return 0;
}

/*
 * Close graphics output.
 *
 */

int meas_graphics_close() {

  int i, j;
  struct data3d *tmp2;

  for (i = 0; i < max_windows; i++)
    if(wins[i].type != MEAS_GRAPHICS_EMPTY) {
      if(wins[i].type == MEAS_GRAPHICS_3D) {
	tmp2 = (struct data3d *) wins[i].data;
	for (j = 0; j < MEAS_GRAPHICS_MAX_2DPOINTS; j++)
	  free(tmp2->zdata[j]);
      }
      free(wins[i].data);
    }
  plend();
  return 0;
}

/*
 * Set line color.
 *
 */

int meas_graphics_line_color(int win, int color) {

  if(color < MEAS_GRAPHICS_BLACK || color > MEAS_GRAPHICS_WHITE) 
    meas_err("meas_graphics_line_color: Illegal line color.");
  wins[win].color = color;
  return 0;
}

/*
 * Set colormap for contour plots.
 *
 */

int meas_graphics_colormap(int win, int colormap) {

  if(colormap < MEAS_GRAPHICS_RGB || colormap > MEAS_GRAPHICS_GRAY) 
    meas_err("meas_graphics_colormap: Illegal colormap.");
  wins[win].colormap = colormap;
  return 0;
}
