#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <forms.h>
#include "meas.h"

/*
 * Image handling routines.
 *
 */

/*
 * Convert Y800 (= Y8) to RGB.
 *
 * yuv    = Y800 image (source).
 * rgb    = RGB image (destination).
 * width  = Image width.
 * height = Image height.
 *
 * Note that yuv and rgb arrays can be the same.
 *
 */

EXPORT void meas_image_y800_to_rgb(unsigned char *y800, unsigned char *rgb, unsigned int width, unsigned int height) {

  int i;
  
  for (i = 0; i < width * height; i++)
    rgb[3*i] = rgb[3*i+1] = rgb[3*i+2] = y800[i];
}

/*
 * Convert Y16 to RGB.
 *
 * yuv    = Y16 image (source).
 * rgb    = RGB image (destination).
 * width  = Image width.
 * height = Image height.
 *
 * Note that Y16 is a 16bit/pixel format and RGB is 3x8bits.
 * So, bits are lost. This just for displaying images.
 *
 */

EXPORT void meas_image_y16_to_rgb(unsigned char *y16, unsigned char *rgb, unsigned int width, unsigned int height) {

  int i, j;
  
  for (i = j = 0; i < 3 * width * height; i += 3, j += 2)
    rgb[i] = rgb[i+1] = rgb[i+2] = 255 * (y16[j+1]*256 + y16[j]) / 65535;
}

/*
 * Convert YUV to RGB.
 *
 * yuv    = YUV image (source).
 * rgb    = RGB image (destination).
 * width  = Image width.
 * height = Image height.
 *
 * Note that yuv and rgb arrays can be the same.
 *
 */

EXPORT void meas_image_yuv_to_rgb(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned int height) {

  int i, y, u, v;
  
  for (i = 0; i < width * height; i += 3) {
    y = yuv[i]; u = yuv[i+1]; v = yuv[i+2];
    rgb[i] = 1.164 * (y - 16) + 1.596 * (v - 128);
    rgb[i+1] = 1.164 * (y - 16) - 0.813 * (v - 128) - 0.391 * (u - 128);
    rgb[i+2] = 1.164 * (y - 16) + 2.018 * (u - 128);
  }
}

/*
 * Conver RGB to YUV.
 *
 * yuv    = YUV image (source).
 * rgb    = RGB image (destination).
 * width  = Image width.
 * height = Image height.
 *
 * Note that yuv and rgb arrays can be the same.
 *
 */

EXPORT void meas_image_rgb_to_yuv(unsigned char *rgb, unsigned char *yuv, unsigned int width, unsigned int height) {

  int i, r, g, b;

  for (i = 0; i < width * height; i++) {
    r = rgb[i]; g = rgb[i+1]; b = rgb[i+2];
    yuv[i] = (0.257 * r) + (0.504 * g) + (0.098 * b) + 16;
    yuv[i+1] = -(0.148 * r) - (0.291 * g) + (0.439 * b) + 128;
    yuv[i+2] = (0.439 * r) - (0.368 * g) - (0.071 * b) + 128;
  }
}

/*
 * Convert YUV422 to RGB.
 *
 * yuv    = YUV image (source).
 * rgb    = RGB image (destination).
 * width  = Image width.
 * height = Image height.
 *
 */

EXPORT void meas_image_yuv422_to_rgb(unsigned char *yuv, unsigned char *rgb, unsigned int width, unsigned int height) {

  int i, j;
  double tmp;
  unsigned char y1, y2, u, v;

  for(i = j = 0; i < 3 * height * width; i += 6, j += 4) {
    /* two pixels interleaved */
    y1 = yuv[j];
    y2 = yuv[j+2];
    u = yuv[j+1];
    v = yuv[j+3];
    /* yuv -> rgb */
    tmp = (1.164 * (((double) y1) - 16.0) + 1.596 * (((double) v) - 128.0));
    if(tmp < 0.) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    rgb[i] = (unsigned char) tmp;
    
    tmp = (1.164 * (((double) y1) - 16.0) - 0.813 * (((double) v) - 128.0) - 0.391 * (((double) u) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    rgb[i+1] = (unsigned char) tmp;
    
    tmp = (1.164 * (((double) y1) - 16.0) + 2.018 * (((double) u) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    rgb[i+2] = (unsigned char) tmp;

    tmp = (1.164 * (((double) y2) - 16.0) + 1.596 * (((double) v) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    rgb[i+3] = (unsigned char) tmp;
    
    tmp = (1.164 * (((double) y2) - 16.0) - 0.813 * (((double) v) - 128.0) - 0.391 * (((double) u) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    rgb[i+4] = (unsigned char) tmp;
    
    tmp = (1.164 * (((double) y2) - 16.0) + 2.018 * (((double) u) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    rgb[i+5] = (unsigned char) tmp;
  }
}

/*
 * Write RGB array (8 bit) into a PPM file.
 *
 * fp    = File pointer for writing the file.
 * Image = RGB array (an array of (R,G,B) triples). 
 *
*/

EXPORT void meas_image_rgb_to_ppm(FILE *fp, unsigned int *rgb, unsigned int width, unsigned int height) {
  
  fprintf(fp, "P6 ");
  fprintf(fp, "%u ", height);
  fprintf(fp, "%u ", width);
  fprintf(fp, "255 ");
  fwrite((void *) rgb, width * height, 1, fp);
}

/*
 * Write YUV800 (8 bit) into a PGM file.
 *
 * fp    = File pointer for writing the file.
 * Image = Y400 array (an array of 8 bit grayscale values). 
 *
*/

EXPORT void meas_image_y800_to_pgm(FILE *fp, unsigned int *yuv800, unsigned int width, unsigned int height) {

  fprintf(fp, "P5 ");
  fprintf(fp, "%u ", height);
  fprintf(fp, "%u ", width);
  fprintf(fp, "255 ");
  fwrite((void *) yuv800, width * height, 1, fp);
}

/*
 * Write YUV16 (16 bit) into a PGM file.
 *
 * fp    = File pointer for writing the file.
 * Image = Y16 array (an array of 16 bit gray scale values). 
 *
*/

EXPORT void meas_image_y16_to_pgm(FILE *fp, unsigned int *yuv16, unsigned int width, unsigned int height) {

  int i;

  fprintf(fp, "P5 ");
  fprintf(fp, "%u ", height);
  fprintf(fp, "%u ", width);
  fprintf(fp, "65535 ");

  for (i = 0; i < 2 * width * height; i += 2) {
    fwrite((void *) (yuv16+1), 1, 1, fp);  /* Swap byte order */
    fwrite((void *) yuv16, 1, 1, fp);
  }
}

/*
 * Convert (16 bit unsigned) gray scale to RGB.
 *
 * img  = Gray scale image array (16 bit; unsigned short *; length nx * ny; input).
 * nx   = number of pixels along x (unsigned int; input).
 * ny   = number of pixels along y (unsigned int; input).
 * rgb  = RGB image (output).
 * as   = 0: auto scale, > 0: set image maximum pixel value to as (use as = 65535 for no scaling) (unsigned short).
 * 
 * Returns -1 on failure.
 *
 */

EXPORT void meas_image_img_to_rgb(unsigned short *img, unsigned int nx, unsigned int ny, unsigned char *rgb, unsigned short as) {

  unsigned int i, j;
  unsigned short lv = 65535;

  if(!as) {
    as = 0;
    for (i = 0; i < nx * ny; i++) {
      if(img[i] < lv) lv = img[i];
      if(img[i] > as) as = img[i];
    }
    as -= lv;
    printf("libmeas: autoscale by %u\n", as);
  } else lv = 0;

  for(i = j = 0; j < nx * ny; i += 3, j += 1) {
    rgb[i]   = (unsigned char) ((255.0 / (double) as) * (double) (img[j] - lv));
    rgb[i+1] = (unsigned char) ((255.0 / (double) as) * (double) (img[j] - lv));
    rgb[i+2] = (unsigned char) ((255.0 / (double) as) * (double) (img[j] - lv));
  }
}

/*
 * Scale up RGB image.
 *
 * rgbi = Input RGB (unsigned char *; input).
 * nx = Image size along x (unsigned int; input).
 * ny = Image size along y (unsigned int; input).
 * sc = Scale: 1, 2, 3 ... (unsigned int; input).
 * rgbo = Output RGB (unsigned char *; output). Note size nx * ny * sc.
 * 
 * Returns -1 on failure.
 *
 * TODO: check array indexing!!!! 
 *
 */

EXPORT void meas_image_scale_rgb(unsigned char *rgbi, unsigned int nx, unsigned int ny, unsigned int sc, unsigned char *rgbo) {

  unsigned int i, j, k, si, sj;

  if(sc < 1 || sc > 10) meas_err("meas_image_scale_rgb: Illegal scale value.\n");
  if(sc == 1) {
    bcopy(rgbi, rgbo, nx * ny);
    return;
  }
  for (i = 0; i < nx; i++)
    for (j = 0; j < ny; j++)
      for (si = 0; si < sc; si++)
	for (sj = 0; sj < sc; sj++)
	  for (k = 0; k < 3; k++)
	    rgbo[(j * sc + sj) * nx * sc * 3 + (i * sc + si) * 3 + k] = rgbi[j * nx * 3 + i * 3 + k];
  return;
}
