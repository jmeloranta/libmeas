#include <stdlib.h>
#include <string.h>
#include <usb.h>
#include <stdbool.h>
#include "matrix.h"
#include "matrixwrapper.h"
#include "misc.h"

struct usb_dev_handle *udevs[MEAS_MATRIX_MAXDEV];   /* USB device handle */

static int been_here = 0;

/*
 * Initialize spectrometer (must be called first).
 *
 * Sets the Newport spectrometer up for use. 
 *
 * sd = Spectrometer number (0, 1, 2, ...).
 *
 * Return 0 for success, -1 for error.
 *
 */

EXPORT int meas_matrix_init(int sd) {

  extern struct usb_dev_handle *meas_matrix_module_init(int);

  if(!been_here) {
    int i;
    for (i = 0; i < MEAS_MATRIX_MAXDEV; i++)
      udevs[i] = NULL;
    been_here = 1;
  }
  if(sd < 0 || sd >= MEAS_MATRIX_MAXDEV || udevs[sd] != NULL) return -1;

  if(!(udevs[sd] = meas_matrix_module_init(sd))) return -1;
  return 0;
}

/*
 * Set CCD element temperature.
 *
 * sd   = Spectrometer #.
 * temp = Temperature in oC.
 *
 * Return 0 for success, -1 for error.
 *
 */

EXPORT int meas_matrix_temperature(int sd, double temp) {

  if(sd < 0 || sd >= MEAS_MATRIX_MAXDEV || udevs[sd] == NULL) return -1;
  return meas_matrix_set_CCD_temp(udevs[sd], temp);
}

/*
 * Return the number of points in spectrum array.
 *
 * sd     = Spectrometer #.
 * width  = Width
 * height = Height
 * 
 */

EXPORT int meas_matrix_size(int sd, int *width, int *height) {
  
  unsigned short w, h;

  if(sd < 0 || sd >= MEAS_MATRIX_MAXDEV || udevs[sd] == NULL) return -1;
  meas_matrix_get_pixel_hw(udevs[sd], &w, &h);
  if(width) *width = w;
  if(height) *height = h;
  return 0;
}

/*
 *
 * This function is used for retrieving an averaged reconstructed image 
 * form the spectrometer.
 *
 * sd  = Spectrometer #.
 * exp = Exposure time in s.
 * ave = Number of averages to take.
 * dst = a pointer to a double array where the spectral (reconstructed)
 *        data will be writen.
 *
 * Return 0 for success, -1 for error.
 *
 */

EXPORT int meas_matrix_read(int sd, double exp, int ave, double *dst) {

  int i, j;
  unsigned int npts, data_size;
  unsigned char query = 0x00, bpp; 
  unsigned short width, height;
  static float *spc = NULL;

  if(sd < 0 || sd >= MEAS_MATRIX_MAXDEV || udevs[sd] == NULL || dst == NULL || ave < 0) return -1;

  /* Temporary space for CCD image */
  meas_matrix_set_exposure_time(udevs[sd], (float) exp);

  /* Temporary space */
  meas_matrix_get_pixel_hw(udevs[sd], &width, &height);
  bpp = meas_matrix_get_ccd_bpp(udevs[sd]);
  data_size = ((unsigned int) width) * ((unsigned int) height) * (unsigned int) bpp;
  if(!spc) {
    if(!(spc = (float *) malloc(sizeof(float) * width))) {
      fprintf(stderr, "libmeas: Memory allocation failure in matrix.");
      return -1;
    }
  }

  /* start a dark exposure */
  meas_matrix_start_exposure(udevs[sd], 0x00,0x02);   /* shutter closed, dark exposure */
  while(query != 0x01) {
    query = meas_matrix_query_exposure(udevs[sd]);
    if(query == 0x02) {
      fprintf(stderr, "libmeas: Failure in matrix query_exposure()\n");
      return -1;
    }
    /*    usleep(MEAS_MATRIX_SLEEP); */
  }
  query = 0x00;
  meas_matrix_get_exposure(udevs[sd]);
  meas_matrix_end_exposure(udevs[sd], 0x00);  /* end dark exposure - leave shutter closed */
  meas_matrix_set_reconstruction(udevs[sd]);

  for(i = 0; i < ave; i++) {
    meas_matrix_start_exposure(udevs[sd], 0x01, 0x01);  /* shutter open, light exposure */
    while(query != 0x01) {
      query = meas_matrix_query_exposure(udevs[sd]);
      if(query == 0x02) {
	fprintf(stderr, "libmeas: Failure in matrix query_exposure()\n");
	return -1;
      }
      /*      usleep(MEAS_MATRIX_SLEEP); */
    }
    meas_matrix_get_exposure(udevs[sd]);
    meas_matrix_end_exposure(udevs[sd], 0x00);         /* end light exposure, leave shutter closed */
    
    /* Get reconstructed spectrum */
    npts = meas_matrix_get_reconstruction(udevs[sd], 0x01, spc);
    
    for(j = 0; j < npts; j++)
      dst[j] = dst[j] + (double) spc[j];  /* take a sum of the intensities */
  }
  
  /* Divide by number of samples */
  for(j = 0; j < npts; j++)
    dst[j] = dst[j] / ((double) ave);

  return 0;
}

/*
 * Used to release the Newport spectrometer after use.
 * It is good practice to call this function when done.
 *
 * sd = Spectrometer #.
 *
 * Return 0 for success, -1 for error.
 *
 */

EXPORT int meas_matrix_close(int sd) {

  if(sd < 0 || sd >= MEAS_MATRIX_MAXDEV || udevs[sd] == NULL) return -1;
  meas_matrix_module_close(udevs[sd]);
  udevs[sd] = NULL;
  return 0;
}

/*
 * Print CCD status.
 *
 * sd = Specrometer #.
 *
 * Return 0 for success, -1 for error.
 *
 */

EXPORT int meas_matrix_status(int sd) {

  if(sd < 0 || sd >= MEAS_MATRIX_MAXDEV || udevs[sd] == NULL) return -1;
  meas_matrix_print_info(udevs[sd]);
  return 0;
}

/*
 * Wavelength calibration.
 * 
 * pixel = Pixel number on the CCD.
 *
 * Returns the wavelength corresponding to the given pixel.
 *
 * TODO: The chould should allow changing the calibration parameters.
 *
 */

EXPORT double meas_matrix_calib(int pixel) {

  return (MEAS_MATRIX_A + MEAS_MATRIX_B * ((double) pixel));
}
