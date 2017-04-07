
/*
 * Image processing and conversion routines.
 *
 * Note that Y16 is little endian format. On big endian machines,
 * the application program is responsible for the conversion if
 * the data is accessed as 16 bit integers.
 *
 */

#include <stdio.h>
#include "misc.h"

/*
 * Convert BGR3 to RGB3.
 *
 * bgr3   = BGR3 image (source).
 * rgb3   = RGB3 image (destination).
 * width  = Image width.
 * height = Image height.
 *
 * Note that bgr3 and rgb3 arrays can be the same.
 *
 */

EXPORT void meas_image_bgr3_to_rgb3(unsigned char *bgr3, unsigned char *rgb3, unsigned int width, unsigned int height) {

  int i;
  unsigned char tmp;
  
  for (i = 0; i < 3 * width * height; i += 3) {
    tmp = bgr3[i];
    rgb3[i] = bgr3[i + 2];
    rgb3[i + 1] = bgr3[i + 1];
    rgb3[i + 2] = tmp;
  }
}

/*
 * Convert Y800 (= Y8) to RGB3.
 *
 * yuv    = Y800 image (source).
 * rgb    = RGB3 image (destination).
 * width  = Image width.
 * height = Image height.
 *
 * Note that yuv and rgb arrays cannot be the same.
 *
 */

EXPORT void meas_image_y800_to_rgb3(unsigned char *y800, unsigned char *rgb3, unsigned int width, unsigned int height) {

  int i;
  
  for (i = 0; i < width * height; i++)
    rgb3[3*i] = rgb3[3*i+1] = rgb3[3*i+2] = y800[i];
}

/*
 * Convert Y16 to RGB3.
 *
 * yuv    = Y16 image (source).
 * rgb    = RGB3 image (destination).
 * width  = Image width.
 * height = Image height.
 *
 * Note that Y16 is a 16bit/pixel format and RGB3 is 3x8bits.
 * So, bits are lost. This just for displaying images.
 *
 * Y12 is just a special 12 bit version of Y16 with the two top most bits zero.
 *
 */

EXPORT void meas_image_y16_to_rgb3(unsigned char *y16, unsigned char *rgb3, unsigned int width, unsigned int height) {

  int i, j;
  
  /* Y16 is little endian format */
  for (i = j = 0; i < 3 * width * height; i += 3, j += 2)
    rgb3[i] = rgb3[i+1] = rgb3[i+2] = 255 * (y16[j+1] * 256 + y16[j]) / 65535;
}

/*
 * Convert YUV to RGB3.
 *
 * yuv    = 3 byte YUV image (source).
 * rgb    = RGB3 image (destination).
 * width  = Image width.
 * height = Image height.
 *
 * Note that yuv and rgb arrays can be the same.
 *
 */

EXPORT void meas_image_yuv_to_rgb3(unsigned char *yuv, unsigned char *rgb3, unsigned int width, unsigned int height) {

  int i, y, u, v;
  
  for (i = 0; i < 3 * width * height; i += 3) {
    y = yuv[i]; u = yuv[i+1]; v = yuv[i+2];
    rgb3[i] = 1.164 * (y - 16) + 1.596 * (v - 128);
    rgb3[i+1] = 1.164 * (y - 16) - 0.813 * (v - 128) - 0.391 * (u - 128);
    rgb3[i+2] = 1.164 * (y - 16) + 2.018 * (u - 128);
  }
}

/*
 * Convert RGB3 to YUV.
 *
 * yuv    = YUV image (source).
 * rgb    = RGB3 image (destination).
 * width  = Image width.
 * height = Image height.
 *
 * Note that yuv and rgb arrays can be the same.
 *
 */

EXPORT void meas_image_rgb3_to_yuv(unsigned char *rgb3, unsigned char *yuv, unsigned int width, unsigned int height) {

  int i, r, g, b;

  for (i = 0; i < 3 * width * height; i += 3) {
    r = rgb3[i]; g = rgb3[i+1]; b = rgb3[i+2];
    yuv[i] = (0.257 * r) + (0.504 * g) + (0.098 * b) + 16;
    yuv[i+1] = -(0.148 * r) - (0.291 * g) + (0.439 * b) + 128;
    yuv[i+2] = (0.439 * r) - (0.368 * g) - (0.071 * b) + 128;
  }
}

/*
 * Convert YUV422 to RGB3.
 *
 * yuv    = YUV image (source).
 * rgb    = RGB3 image (destination).
 * width  = Image width.
 * height = Image height.
 *
 */

EXPORT void meas_image_yuv422_to_rgb3(unsigned char *yuv, unsigned char *rgb3, unsigned int width, unsigned int height) {

  int i, j;
  double tmp;
  unsigned char y1, y2, u, v;

  /* for two pixels: 4 bytes of YUV (butes / pixel) -> 6 bytes of RGB (3 bytes/pixel) */
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
    rgb3[i] = (unsigned char) tmp;
    
    tmp = (1.164 * (((double) y1) - 16.0) - 0.813 * (((double) v) - 128.0) - 0.391 * (((double) u) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    rgb3[i+1] = (unsigned char) tmp;
    
    tmp = (1.164 * (((double) y1) - 16.0) + 2.018 * (((double) u) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    rgb3[i+2] = (unsigned char) tmp;

    tmp = (1.164 * (((double) y2) - 16.0) + 1.596 * (((double) v) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    rgb3[i+3] = (unsigned char) tmp;
    
    tmp = (1.164 * (((double) y2) - 16.0) - 0.813 * (((double) v) - 128.0) - 0.391 * (((double) u) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    rgb3[i+4] = (unsigned char) tmp;
    
    tmp = (1.164 * (((double) y2) - 16.0) + 2.018 * (((double) u) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    rgb3[i+5] = (unsigned char) tmp;
  }
}

/*
 * Convert YUV422 to RGB.
 *
 * yuv    = YUV image (source).
 * red    = red image (destination).
 * green  = red image (destination).
 * blue   = red image (destination).
 * width  = Image width.
 * height = Image height.
 *
 */

EXPORT void meas_image_yuv422_to_rgb(unsigned char *yuv, unsigned char *red, unsigned char *green, unsigned char *blue, unsigned int width, unsigned int height) {

  int i, j;
  double tmp;
  unsigned char y1, y2, u, v;

  /* for two pixels: 4 bytes of YUV (butes / pixel) -> 6 bytes of RGB (3 bytes/pixel) */
  for(i = j = 0; i < height * width; i += 2, j += 4) {
    /* two pixels interleaved */
    y1 = yuv[j];
    y2 = yuv[j+2];
    u = yuv[j+1];
    v = yuv[j+3];
    /* yuv -> rgb */
    tmp = (1.164 * (((double) y1) - 16.0) + 1.596 * (((double) v) - 128.0));
    if(tmp < 0.) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    red[i] = (unsigned char) tmp;
    
    tmp = (1.164 * (((double) y1) - 16.0) - 0.813 * (((double) v) - 128.0) - 0.391 * (((double) u) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    green[i] = (unsigned char) tmp;
    
    tmp = (1.164 * (((double) y1) - 16.0) + 2.018 * (((double) u) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    blue[i] = (unsigned char) tmp;

    tmp = (1.164 * (((double) y2) - 16.0) + 1.596 * (((double) v) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    red[i+1] = (unsigned char) tmp;
    
    tmp = (1.164 * (((double) y2) - 16.0) - 0.813 * (((double) v) - 128.0) - 0.391 * (((double) u) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    green[i+1] = (unsigned char) tmp;
    
    tmp = (1.164 * (((double) y2) - 16.0) + 2.018 * (((double) u) - 128.0));
    if(tmp < 0.0) tmp = 0.0;
    if(tmp > 255.0) tmp = 255.0;
    blue[i+1] = (unsigned char) tmp;
  }
}

/*
 * Read PPM file into (8 bit) RGB3 array.
 *
 * fp     = File pointer for writing the file.
 * image  = RGB3 array (an array of (R,G,B) triples; pointer). 
 * width  = Image width (pointer).
 * height = Image height (pointer).
 * 
 * Returns 0 on success, -1 on error.
 *
 */

EXPORT int meas_image_ppm_to_rgb3(FILE *fp, unsigned char *rgb3, unsigned int *width, unsigned int *height) {
  
  if(fscanf(fp, "P6%*[ \r\n\t]%u%*[ \r\n\t]%u%*[ \r\n\t]255%*[ \n\r\t]", width, height) != 2) return -1;
  fread((void *) rgb3, 3 * *width * *height, 1, fp);
  return 0;
}

/*
 * Read PGM file into YUV800 (8 bit) array.
 *
 * fp     = File pointer for writing the file.
 * image  = Y800 array (an array of 8 bit grayscale values). 
 * width  = Image width (pointer).
 * height = Image height (pointer).
 *
 * Returns 0 on success, -1 on error.
 *
 */

EXPORT int meas_image_pgm_to_y800(FILE *fp, unsigned char *y800, unsigned int *width, unsigned int *height) {

  if(fscanf(fp, "P5%*[ \r\n\t]%u%*[ \r\n\t]%u%*[ \r\n\t]255%*[ \r\n\t]", width, height) != 2) return -1;
  fread((void *) y800, *width * *height, 1, fp);
  return 0;
}

/*
 * Read PGM file into YUV16 (16 bit) array.
 *
 * fp     = File pointer for writing the file.
 * image  = Y16 array (an array of 16 bit gray scale values). 
 * width  = Image width (pointer).
 * height = Image height (pointer).
 *
 * Return 0 on success, -1 on error.
 *
 */

EXPORT int meas_image_pgm_to_y16(FILE *fp, unsigned char *y16, unsigned int *width, unsigned int *height) {

  int i;
  
  if(fscanf(fp, "P5%*[ \r\n\t]%u%*[ \r\n\t]%u%*[ \r\n\t]65535", width, height) != 2) return -1;
  (void) fgetc(fp); /* read the space - note: fscanf can match actual data for white spaces! */
  /* PGM is big endian format */
  for (i = 0; i < 2 * *width * *height; i += 2) {
    if(fread((void *) (y16+i+1), 1, 1, fp) != 1) return -1;
    if(fread((void *) (y16+i), 1, 1, fp) != 1) return -1;
  }
  return 0;
}

/*
 * Write RGB3 array to PPM file (8 bit).
 *
 * fp     = File pointer for reading the file.
 * image  = RGB3 array (an array of (R,G,B) triples). 
 * width  = Image width.
 * height = Image height.
 *
 * Return 0 on success, -1 on error.
 *
 */

EXPORT int meas_image_rgb3_to_ppm(FILE *fp, unsigned char *rgb3, unsigned int width, unsigned int height) {
  
  if(fprintf(fp, "P6\n") < 0) return -1;
  if(fprintf(fp, "%u %u\n", width, height) < 0) return -1;
  if(fprintf(fp, "255\n") < 0) return -1;
  if(fwrite((void *) rgb3, 3 * width * height, 1, fp) != width * height) return -1;
  return 0;
}

/*
 * Write YUV800 (8 bit) into a PGM file.
 *
 * fp     = File pointer for writing the file.
 * image  = Y400 array (an array of 8 bit grayscale values). 
 * width  = Image width.
 * height = Image height.
 *
 * Return 0 on success, -1 on error.
 *
 */

EXPORT int meas_image_y800_to_pgm(FILE *fp, unsigned char *yuv800, unsigned int width, unsigned int height) {

  if(fprintf(fp, "P5\n") < 0) return -1;
  if(fprintf(fp, "%u %u\n", width, height) < 0) return -1;
  if(fprintf(fp, "255\n") < 0) return -1;
  if(fwrite((void *) yuv800, width * height, 1, fp) != width * height) return -1;
  return 0;
}

/*
 * Write YUV16 (16 bit) into a PGM file.
 *
 * fp     = File pointer for writing the file.
 * image  = Y16 array (an array of 16 bit gray scale values). 
 * width  = Image width.
 * height = Image height.
 *
 * Return 0 on success, -1 on error.
 *
 */

EXPORT int meas_image_y16_to_pgm(FILE *fp, unsigned char *yuv16, unsigned int width, unsigned int height) {

  int i;

  if(fprintf(fp, "P5\n") < 0) return -1;
  if(fprintf(fp, "%u %u\n", width, height) < 0) return -1;
  if(fprintf(fp, "65535\n") < 0) return -1;

  for (i = 0; i < 2 * width * height; i += 2) {
    if(fwrite((void *) (yuv16+i+1), 1, 1, fp) != 1) return -1;  /* Swap byte order */
    if(fwrite((void *) (yuv16+i), 1, 1, fp) != 1) return -1;
  }
  return 0;
}

/*
 * Scale up RGB3 image.
 *
 * rgbi = Input RGB3 (unsigned char *; input).
 * nx = Image size along x (unsigned int; input).
 * ny = Image size along y (unsigned int; input).
 * sc = Scale: 1, 2, 3 ... (unsigned int; input).
 * rgbo = Output RGB3 (unsigned char *; output). Note size nx * ny * sc.
 * 
 * Returns -1 on failure, 0 on success.
 *
 * TODO: check array indexing!!!! 
 *
 */

EXPORT int meas_image_scale_rgb3(unsigned char *rgb3i, unsigned int nx, unsigned int ny, unsigned int sc, unsigned char *rgb3o) {

  unsigned int i, j, k, si, sj;

  if(sc < 1 || sc > 10) meas_err("meas_image_scale_rgb3: Illegal scale value.\n");
  if(sc == 1) {
    for (i = 0; i < 3 * nx * ny; i++) rgb3o[i] = rgb3i[i];
    return 0;
  }
  for (i = 0; i < nx; i++)
    for (j = 0; j < ny; j++)
      for (si = 0; si < sc; si++)
	for (sj = 0; sj < sc; sj++)
	  for (k = 0; k < 3; k++)
	    rgb3o[(j * sc + sj) * nx * sc * 3 + (i * sc + si) * 3 + k] = rgb3i[j * nx * 3 + i * 3 + k];
  return 0;
}

/*
 * Flip RGB3 in vertical direction.
 *
 * rgb3 = RGB3 image.
 *
 */

EXPORT void meas_image_rgb3_vertical_flip(unsigned char *rgb3, int width, int height) {

  int i, j, k, w3 = width * 3;
  unsigned char tmp;

  for (i = 0; i < height; i++)
    for (j = 0; j < width; j++)
      for (k = 0; k < 3; k++) {      
	tmp = rgb3[i * w3 + 3 * j + k];
	rgb3[i * w3 + 3 * j + k] = rgb3[(height - i - 1) * w3 + 3 * j + k];
	rgb3[(height - i - 1) * w3 + 3 * j + k] = tmp;
      }
}

/*
 * Flip RGB3 in horizontal direction.
 *
 * rgb3 = RGB3 image.
 *
 */

EXPORT void meas_image_rgb3_horizontal_flip(unsigned char *rgb3, int width, int height) {

  int i, j, k, w3 = width * 3;
  unsigned char tmp;
  
  for (i = 0; i < height; i++)
    for (j = 0; j < width; j++)
      for (k = 0; k < 3; k++) {
	tmp = rgb3[i * w3 + 3 * j + k];
	rgb3[i * w3 + 3 * j + k] = rgb3[i * w3 + 3 * (width - j - 1) + k];
	rgb3[i * w3 + 3 * (width - j - 1) + k] = tmp;
      }
}

/*
 * Split BA81 format color image into R, G, B components. Raw conversion - no interpolation.
 *
 * ba81   = array of pixel values (unsigned char *).
 * img_r  = array of red pixels (unsighed char *). Dimension is width / 2 x height / 2
 * img_g  = array of green pixels (unsighed char *). Dimension is width / 2 x height / 2
 * img_b  = array of blue pixels (unsighed char *). Dimension is width / 2 x height / 2
 * width  = image width in pixels of BA81 image (int).
 * height = image height in pixels of BA81 image (int).
 *
 * TODO: clean up indexing
 *
 */

EXPORT void meas_image_ba81_to_rgb(unsigned char *ba81, unsigned char *img_r, unsigned char *img_g, unsigned char *img_b, int width, int height) {

  int i, j, w2 = width / 2;
  unsigned int tmp;

  for (i = 0; i < height/2; i++) /* vertical */
    for (j = 0; j < w2; j++) {   /* horzontal */
      if(img_r) img_r[i * w2 + j] = ba81[(2 * i + 1) * width + (2 * j + 1)]; /* red */
      tmp = (((unsigned int) ba81[(2 * i + 1) * width + 2 * j]) + ((unsigned int) ba81[2 * i * width + (2 * j + 1)])) / 2; /* green */      
      if(img_g) img_g[i * w2 + j] = tmp;
      if(img_b) img_b[i * w2 + j] = ba81[2 * i * width + 2 * j]; /* blue */
    }
}

/*
 * Convert BA81 format color image into RGB3. Warning: Very rough code - no interpolation etc.
 *
 * ba81   = array of pixel values (unsigned char *).
 * rgb3  = array of red pixels (unsighed char *). Dimension is 3 * width / 2 x height / 2.
 * width  = image width in pixels (int) of BA81 image.
 * height = image height in pixels (int) of BA81 image.
 *
 * TODO: clean up indexing
 *
 */

EXPORT void meas_image_ba81_to_rgb3(unsigned char *ba81, unsigned char *rgb3, int width, int height) {

  int i, j, w2 = width / 2;
  unsigned int tmp;
  
  for (i = 0; i < height/2; i++) /* vertical */
    for (j = 0; j < w2; j++) {   /* horzontal */
      rgb3[i * 3 * w2 + 3 * j + 0] = ba81[(2 * i + 1) * width + (2 * j + 1)]; /* red */
      /* make sure not to overflow */
      tmp = (((unsigned int) ba81[(2 * i + 1) * width + 2 * j]) + ((unsigned int) ba81[2 * i * width + (2 * j + 1)])) / 2; /* green */
      rgb3[i * 3 * w2 + 3 * j + 1] = tmp;
      rgb3[i * 3 * w2 + 3 * j + 2] = ba81[2 * i * width + 2 * j]; /* blue */
    }
}
