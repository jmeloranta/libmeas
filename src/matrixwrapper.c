#include <stdlib.h>
#include <string.h>
#include <usb.h>
#include <stdbool.h>
#include "matrix.h"
#include "matrixwrapper.h"
#include "misc.h"

/*
 * Initialize spectrometer (must be called first).
 *
 * Sets the Newport spectrometer up for use. 
 *
 * TODO: Support multiple spectrometers.
 *
 */

EXPORT int meas_matrix_init() {

  matrix_module_init();
  return 0;
}

/*
 * Set CCD element temperature.
 *
 * temp = Temperature in oC.
 *
 */

EXPORT int meas_matrix_temperature(double temp) {

  matrix_set_CCD_temp(temp);
  return 0;
}

/*
 * Return the number of points in spectrum array.
 *
 */

EXPORT int meas_matrix_size() {
  
  unsigned short width, height;

  matrix_get_pixel_hw(&width, &height);
  return (int) width;
}

/*
 *
 * This function is used for retrieving an averaged reconstructed image 
 * form the spectrometer.
 *
 * exp = Exposure time in s.
 * ave = Number of averages to take.
 * dst = a pointer to a double array where the spectral (reconstructed)
 *        data will be writen.
 *
 */

EXPORT int meas_matrix_read(double exp, int ave, double *dst) {

  int i, j;
  unsigned int npts, data_size;
  unsigned char query = 0x00, bpp; 
  unsigned short width, height;
  static float *spc = NULL;

  /* Temporary space for CCD image */
  matrix_set_exposure_time((float) exp);

  /* Temporary space */
  matrix_get_pixel_hw(&width, &height);
  bpp = matrix_get_ccd_bpp();
  data_size = ((unsigned int) width) * ((unsigned int) height) * (unsigned int) bpp;
  if(!spc) {
    if(!(spc = (float *) malloc(sizeof(float) * width)))
      meas_meas_err2("matrix: Memory allocation failure.");
  }

  /* start a dark exposure */
  matrix_start_exposure(0x00,0x02);   /* shutter closed, dark exposure */
  while(query != 0x01) {
    query = matrix_query_exposure();
    if(query == 0x02)
      meas_meas_err2("matrix: failure in  query_exposure()\n");
    /*    usleep(MATRIX_SLEEP); */
  }
  query = 0x00;
  matrix_get_exposure();
  matrix_end_exposure(0x00);  /* end dark exposure - leave shutter closed */
  matrix_set_reconstruction();

  for(i = 0; i < ave; i++) {
    matrix_start_exposure(0x01,0x01);  /* shutter open, light exposure */
    while(query != 0x01) {
      query = matrix_query_exposure();
      if(query == 0x02)
	meas_meas_err2("matrix: failure in  query_exposure()\n");
      /*      usleep(MATRIX_SLEEP); */
    }
    matrix_get_exposure();
    matrix_end_exposure(0x00);         /* end light exposure, leave shutter closed */
    
    /* Get reconstructed spectrum */
    npts = matrix_get_reconstruction(0x01, spc);
    
    for(j = 0; j < npts; j++)
      dst[j] = dst[j] + (double) spc[j];  /* take a sum of the intensities */
  }
  
  /* Divide by number of samples */
  for(j = 0; j < npts; j++)
    dst[j] = dst[j]/( (double) ave);
  return 0;
}

/*
 * Used to release the Newport spectrometer after use.
 * It is good practice to call this function when done.
 *
 */

EXPORT int meas_matrix_close() {

  matrix_module_close();
  return 0;
}

/*
 * Print CCD status.
 *
 */

EXPORT int meas_matrix_status() {

  matrix_print_info();
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
