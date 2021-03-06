/*
 * filename: matrix.c
 *
 * Newport Oriel MMS Spectrometer Driver
 *
 * TODO: Only one instrument supported.
 *
 * NOTE: You may want to use matrixwrapper.c instead!
 *
 * Notes:
 *    File requires matrix.h
 *    Tested only on x86 machines, may not work on other machines. (Will not work on 64-bit machines as-is)
 *    Needs root privileges to access USB devices.
 *
 * Last updated June 25, 2008
 *
 * This driver was developed using the ClearShotII - USB port interface communications and
 * control information specification provided by Centice Corporation (http://www.centice.com)
 *
 * Copyright (c) 2008, Clinton McCrowey.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Clinton McCrowey ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Clinton McCrowey BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


/* This file provides a complete set of functions for the instrument */

#include "matrix.h"
#include "misc.h"

/*
 * unsigned char matrix_module_init()
 *
 * This function detects and initializes the Newport spectrometer and must be called before
 * it can be used. All other functions will fail if called upon before calling this function.
 *
 * Input: None.
 *
 * Return Value: pointer to usb_dev_handle (success) or NULL on failure.
 *
 */

EXPORT struct usb_dev_handle *meas_matrix_module_init(int sd) {

  struct usb_bus *busses, *bus;
  struct usb_device *dev;
  struct usb_dev_handle *udev;
  int cd;
  
  meas_misc_root_on();
  
  usb_init();
  usb_find_busses();
  usb_find_devices();
  
  busses = usb_get_busses();
  
  /* scan each device on each bus for the Newport MMS Spectrometer */
  for (bus = busses, cd = 0; bus; bus = bus->next) {
    for (dev = bus->devices; dev; dev=dev->next)
      if(dev->descriptor.idVendor == 0x184c && dev->descriptor.idProduct == 0x0000) {
	if(cd == sd) break; 
	cd++;
      }    
    if(dev && cd == sd) break;
  }
  
  if(!bus || !dev) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer Not Found\n");
    return NULL;
  }
  
  if(!(udev = usb_open(dev))) {
    fprintf(stderr, "Newport Oriel MMS Spectrometer: Can't open instrument.\n");
    return NULL;
  }
  
  fprintf(stderr, "Newport Oriel MMS Spectrometer: Instrument found.\n");
  
  if(usb_set_configuration(udev, 1) < 0) {
    usb_close(udev);
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer: Can't set active configuration.\n");
    return NULL;
  }
  
  if(usb_claim_interface(udev, 0) < 0) {
    usb_close(udev);
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer: Can't claim interface.\n");
    return NULL;
  }
  
  if(usb_set_altinterface(udev, 0) < 0) {
    usb_close(udev);
    fprintf(stderr, "Newport Oriel MMS Spectrometer: Can't set alternate interface.\n");
    return NULL;
  }
  
  meas_misc_root_off();
  return udev;
}

/*
 * void matrix_set_exposure_time(float time)
 *
 * This function assigns a new image exposure time to the spectrometer. If the exposure time is
 * below that of the spectrometer's minimum exposure time, the exposure time will be set to that
 * of the spectrometer's minimum exposure time. Likewise If the exposure time is above that of
 * the spectrometer;s maximum exposure time, theexposure time will be set to that of the
 * spectrometer's maximum exposure time.
 *
 * Input:
 *   udev strcture
 *   float time: the time that should be set (seconds)
 *
 * Return value: 0 for success, -1 for error.
 *
 */

EXPORT int meas_matrix_set_exposure_time(struct usb_dev_handle *udev, float time) {

  unsigned char writebuf[10], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x0A; /* command - Set exposure time */
  *len = 10; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  *((float *) &writebuf[6]) = time;
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error setting exposure time.\n");
    return -1;
  }
  return 0;
}

/*
 * float get_exposure_time()
 *
 * This function fetches the spectrometer's currently configured exposure time. The exposure time
 * returned by the command will be within or equal to the minimum and maximum exposure times that 
 * are allowed by the spectrometer.
 *
 * Input: None
 *
 * Return value: if successful returns a float of the exposure time (in seconds) else
 * returns a negative number
 *
 */

EXPORT float meas_matrix_get_exposure_time(struct usb_dev_handle *udev) {
  
  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x09; /* command - Get exposure time */
  *len = 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error reading exposure time.\n");
    return -1.0;
  }
  
  return *((float *) &readbuf[7]);
}

/*
 * void matrix_reset()
 *
 * This function will reset the spectrometer to it's default settings. This function may/will
 * affect the following: CCD temperature regulation and set point, shutter, exposure time,
 * reconstruction mode, clock rate, pixel mode, on/off status of laser, laser power setting(s),
 * and temperature regulation and set point of lasers. **** NOTE **** This function can sometimes
 * cause problems with the Newport Spectrometer and it may be better to use the module_close()
 * function then.
 *
 * Input: udev structure
 *
 * Return value: -1 for error, 0 for success.
 *
 */

EXPORT int meas_matrix_reset(struct usb_dev_handle *udev) {
  
  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  unsigned int *len = (unsigned int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x1F; /* command - Get reset the spectrometer */
  *len = (unsigned int) 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error resetting device.\n");
    return -1;
  }
  sleep(2);
  return 0;
}

/*
 * void matrix_print_info()
 *
 * This function simply prints out various information about the spectrometer to the console
 *
 * Input: udev structure.
 *
 * Return value: 0 for success, -1 for error.
 *
 */

EXPORT int meas_matrix_print_info(struct usb_dev_handle *udev) {
  
  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  int major, minor, build; /* temp varables to hold version information */
  char tmp[256];
  int meas_matrix_get_model_number(struct usb_dev_handle *, char *);
  int meas_matrix_get_serial_number(struct usb_dev_handle *, char *);
  
  writebuf[0] = 0x16; /* command - Get spectrometer info */
  *len = 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(usb_bulk_write(udev, 4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, 8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error reading device info.\n");
    return -1;
  }
  
  printf("\n");
  /* subtract 20???? WHY ??? */
  printf("CCD width in pixels: %u\n", *((unsigned short *) &readbuf[7]) - 20);
  printf("CCD height in pixels: %u\n", *((unsigned short *) &readbuf[9]));
  printf("CCD bits per pixel: %u\n", readbuf[11]);
  printf("CCD Type: %u\n", readbuf[12]);
  printf("CCD Shutter support: %s\n", readbuf[13]?"true":"false");
  printf("Minimum exposure time (s): %f\n", ((float) *((unsigned int *) &readbuf[14])) / (float) 1000.0);
  printf("Maximum exposure time (s): %f\n", ((float) *((unsigned int *) &readbuf[18])) / (float) 1000.0);
  printf("Calibration support: %s\n", readbuf[22]?"true":"false");
  printf("Spectra reconstruction support: %s\n", readbuf[23]?"true":"false");
  printf("Spectra temperature regulation support: %s\n", readbuf[24]?"true":"false");
  
  if(readbuf[24]) {
    printf("   Minimum CCD setpoint (C): %i\n", *((short *) &readbuf[25]));
    printf("   Maximum CCD setpoint (C): %i\n", *((short *) &readbuf[27]));
  }

  printf("Number of supported lasers: %i\n", readbuf[29]);
  printf("Laser temperature regulation support: %s\n", readbuf[30]?"true":"false");
  
  if(readbuf[30]) {
    printf("   Minimum laser setpoint: %i\n", *((short *) &readbuf[31]));
    printf("   Maximum laser setpoint: %i\n", *((short *) &readbuf[33]));
  }
  
  printf("Laser Power Regulation: %s\n", readbuf[35]?"true":"false");
  
  if(readbuf[35]) {
    printf("   Minimum laser power setpoint: %i\n", *((short *) &readbuf[36]));
    printf("   Maximum laser power setpoint: %i\n", *((short *) &readbuf[38]));
  }
  
  /* DSP firmware info */
  major = (*((unsigned int *) &readbuf[40]) & 0xFF000000) >> 24;
  minor = (*((unsigned int *) &readbuf[40]) & 0x00FF0000) >> 16;
  build = (*((unsigned int *) &readbuf[40]) & 0x0000FFFF);
  printf("DSP firmware version: %u.%u.%u\n", major, minor, build);
  
  /* FPGA firmware info */
  major = (*((unsigned int *) &readbuf[44]) & 0xFF000000) >> 24;
  minor = (*((unsigned int *) &readbuf[44]) & 0x00FF0000) >> 16;
  build = (*((unsigned int *) &readbuf[44]) & 0x0000FFFF);
  printf("FPGA firmware version: %u.%u.%u\n", major, minor, build);
  
  /* USB firmware info */
  major = (*((unsigned int *) &readbuf[48]) & 0xFF000000) >> 24;
  minor = (*((unsigned int *) &readbuf[48]) & 0x00FF0000) >> 16;
  build = (*((unsigned int *) &readbuf[48]) & 0x0000FFFF);
  printf("USB firmware version: %u.%u.%u\n", major, minor, build);
  
  printf("Number of general I/O lines: %d\n", readbuf[52]);
  
  switch(readbuf[54]) {    
  case 0x00:
    printf("A/D clock frequency: N/A\n");
    break;    
  case 0x01:
    printf("A/D clock frequency: default\n");
    break;
  case 0x02:
    printf("A/D clock frequency: 150KHz\n");
    break;
  case 0x03:
    printf("A/D clock frequency: 300KHz\n");
    break;
  case 0x04:
    printf("A/D clock frequency: 500KHz\n");
    break;
  case 0x05:
    printf("A/D clock frequency: 1000KHz\n");
    break;
  default:
    printf("A/D clock frequency: Error\n");
    break;
  }

  switch(readbuf[56]) {    
  case 0x00:
    printf("Pixel binning mode: N/A\n");
    break;
  case 0x01:
    printf("Pixel binning mode: Full Well\n");
    break;
  case 0x02:
    printf("Pixel binning mode: Dark Current\n");
    break;
  case 0x03:
    printf("Pixel binning mode: Line Binning\n");
    break;
  default:
    printf("Pixel binning mode: Error\n");
    break;
  }
  
  meas_matrix_get_model_number(udev, tmp);
  printf("Model Number: %s\n", tmp);
  
  meas_matrix_get_serial_number(udev, tmp);
  printf("Serial Number: %s\n", tmp);
  return 0;
}

/*
 * void matrix_start_exposure(unsigned char shutterState, unsigned char exposureType)
 *
 * This command informs the spectrometer to start a camera exposure.
 * The exposure that is taken will have a time duration equal to that of the last
 * configured exposure time. The exposure that is taken can either use the existing
 * shutter setting or force the state of the shutter. Shutter support can be determined
 * by calling the print_info() function.
 *
 * Inputs:  udev structure
 *   unsigned char shutterState: an 8-bit unsigned integer which defines what
 *                               should occur with the spectrometer's camera shutter.
 *                               Acceptable values are:
 *                               0x00 - close shutter
 *                               0x01 - open shutter
 *                               0x02 - leave shutter in its current state
 *   unsigned char exposureType: an 8-bit unsigned integer which defines the type
 *                               of exposure that is to be taken. Acceptable values are:
 *                               0x00 - none
 *                               0x01 - light exposure
 *                               0x02 - dark exposure
 *                               0x03 - reference exposure
 * 
 * Return value: -1 for error, 0 for success.
 *
 */

EXPORT int meas_matrix_start_exposure(struct usb_dev_handle *udev, unsigned char shutterState, unsigned char exposureType) {

  unsigned char writebuf[8], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x0B; /* command - Start Exposure */
  *len = 8; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(shutterState != 0x00 && shutterState != 0x01 && shutterState != 0x02) { /* test if we have a valid shutterState */
    fprintf(stderr, "libmeas: Invalid matrix shutter state.\n");
    return -1;
  }
  if(exposureType != 0x00 && exposureType != 0x01 && exposureType != 0x02 && exposureType != 0x03) { /* test if we have a valid exposure type */
    fprintf(stderr, "libmeas: Invalid matrix exposure type.\n");
    return -1;
  }
  
  writebuf[6] = shutterState;  /* set the shutter state */
  writebuf[7] = exposureType;  /* set the exposure type */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error starting exposure.\n");
    return -1;
  }
  return 0;
}

/*
 * unsigned char query_exposure()
 *
 * Returns the current state of the spectrometer's started exposure.
 * Repeated calls to this command will allow an outside process to determine
 * if a requested exposure has been taken by the spectrometer's camera.
 * This command will only successfully complete if it is called between both
 * start_exposure() and en end_exposure().
 *
 * Input: udev structure
 * 
 * Return value: if successful an unsigned char which indicates if the last
 *               requested exposure is available or not.
 *                an 8-bit unsigned integer which defines the type
 *                Acceptable values are:
 *                0x00 - not available
 *                0x01 - available
 *                0x02 - failure
 *
 */

EXPORT unsigned char meas_matrix_query_exposure(struct usb_dev_handle *udev) {

  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x0C; /* command - Query Exposure */
  *len = 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error querying exposure.\n");
    return 0x2;
  }
  return readbuf[7];
}

/*
 * void end_exposure()
 *
 * This function ends the use of the spectrometer's camera in regards to acquiring an exposure.
 * This function should be called only after calls to start_exposure() and when all
 * spectrometer exposure related commands have completed.Execution of this function will cause
 * all future query_exposure() function commands to fail until the next start_exposure()
 * command successfully completes.
 *
 * Input: udev structure
 *   unsigned char shutterState: an 8-bit unsigned integer which defines what
 *                               should occur with the spectrometer's camera shutter.
 *                               Possible values are:
 *                               0x00 - close shutter
 *                               0x01 - open shutter
 *                               0x02 - leave shutter in its current state
 * 
 * Return value: -1 error, 0 success
 *     
 */

EXPORT int meas_matrix_end_exposure(struct usb_dev_handle *udev, unsigned char shutterState) {

  unsigned char writebuf[7], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x0D; /* command - End Exposure */
  *len = 7; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  /* test if we have a valid shutterState */
  if(shutterState != 0x00 && shutterState != 0x01 && shutterState != 0x02) {
    fprintf(stderr, "libmeas: Invalid matrix shutter state.\n");
    return -1;
  }
  
  writebuf[6] = shutterState;  /* set the shutter state */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error ending exposure.\n");
    return -1;
  }
  return 0;
}

/*
 * void get_last_error()
 *
 * This function prints out to the console and returns the last known error that the spectrometer generated.
 *
 * Inputs: udev structure
 *         Array for error status (length 2)
 *               index = 0: Error Code
 *                          Possible values are:
 *                          0x0000 - No Errors
 *                          0x0001 - On-Chip Memory
 *                          0x0002 - On-Chip Digital I/O
 *                          0x0003 - On-Chip Interrupt Module
 *                          0x0004 - On-Chip Timer Module
 *                          0x0005 - On Chip I2C Module
 *                          0x0006 - On-Board ADC
 *                          0x0007 - On Board AFE
 *                          0x0008 - On Board EEPROM (DSP)
 *                          0x0009 - On-Board EEPROM (Cypress USB)
 *                          0x000A - On-Board Supply/Reference Voltage
 *                          0x000B - On-Board USB Controller (Cypress)
 *                          0x000C - USB Command Handler
 *                          0x000D - Non-Specific Source
 *                          0x000E - Calibration Module
 *                          0x000F - Timing FPGA
 *               index = 1: Error Type
 *                          0x0000 - No Errors
 *                          0x0001 - Read/Write Failure
 *                          0x0002 - Invalid Address
 *                          0x0004 - Invalid Parameter
 *                          0x0008 - Invalid State
 *                          0x0010 - Allocation Failure
 *                          0x0020 - Limit Exceeded
 *                          0x0040 - Test/Diagnostic Failure
 *                          0x0080 - Non-Specific Failure
 *
 * Reuturn value: -1 for error, 0 for success.
 *
 */

EXPORT int meas_matrix_get_last_error(struct usb_dev_handle *udev, unsigned int *error) {

  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  char errorCode[40]; /* char array that describes the error code. */
  char errorType[40]; /* char array that describes the error type. */
  
  writebuf[0] = 0x11; /* command - Get last error */
  *len = 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error retrieving error code.\n");
    return -1;
  }
  
  switch(readbuf[7]) {
  case 0x0000:
    strcpy(errorCode, "No Errors");
    break;    
  case 0x001:
    strcpy(errorCode, "On-Chip Memory");
    break;
  case 0x0002:
    strcpy(errorCode, "On-Chip Digital I/O");
    break;
  case 0x0003:
    strcpy(errorCode, "On-Chip Interrupt Module");
    break;
  case 0x0004:
    strcpy(errorCode, "On-Board Timer Module");
    break;
  case 0x0005:
    strcpy(errorCode, "On-Chip I2C Module");
    break;
  case 0x0006:
    strcpy(errorCode, "On-Board ADC");
    break;
  case 0x0007:
    strcpy(errorCode, "On-Board AFE");
    break;
  case 0x0008:
    strcpy(errorCode, "On-Board EEPROM (DSP)");
    break;
  case 0x0009:
    strcpy(errorCode, "On-Board EEPROM (Cypress USB)");
    break;
  case 0x000A:
    strcpy(errorCode, "On-Board USB Controller (Cypress)");
    break;
  case 0x000B:
    strcpy(errorCode, "On-Board Supply/References Voltage");
    break;
  case 0x000C:
    strcpy(errorCode, "USB Command Handler");
    break;
  case 0x000D:
    strcpy(errorCode, "Non-Specific Source");
    break;
  case 0x000E:
    strcpy(errorCode, "Calibration Module");
    break;
  case 0x000F:
    strcpy(errorCode, "Timing FPGA");
    break;
  default:
    strcpy(errorCode, "Invalid Error Code");
    break;
  }
  
  switch(readbuf[9]) {
  case 0x0000:
    strcpy(errorType, "No Errors");
    break;
  case 0x0001:
    strcpy(errorType, "Read/Write Failure");
    break;
  case 0x0002:
    strcpy(errorType, "Invalid Address");
    break;
  case 0x0004:
    strcpy(errorType, "Invalid Parameter");
    break;
  case 0x0008:
    strcpy(errorType, "Invalid State");
    break;
  case 0x0010:
    strcpy(errorType, "Allocation Failure");
    break;
  case 0x0020:
    strcpy(errorType, "Limit Exceeded");
    break;
  case 0x0040:
    strcpy(errorType, "Test/Diagnostic Failure");
    break;
  case 0x0080:
    strcpy(errorType, "Non-Specific Failure");
    break;
  default:
    strcpy(errorType, "Invalid Error Type");
    break;
  }
  
  fprintf(stderr, "Error Code: %s\nError Type: %s\n", errorCode, errorType);
  
  error[0] = *((unsigned int *) &readbuf[7]);
  error[1] = *((unsigned int *) &readbuf[9]);
}

/*
 * void open_CCD_shutter()
 *
 * This function will open the spectrometer's CCD shutter. This function will do
 * nothing if the spectrometer does not support a shutter. Shutter support can be
 * determined by the use of print_info() function.
 *
 * Inputs: udev structure
 * 
 * Return value: 0 for success, -1 for error.
 *     
 */

EXPORT int meas_matrix_open_CCD_shutter(struct usb_dev_handle *udev) {

  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x07; /* command - Open CCD Shutter */
  *len = 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libusb: Newport Oriel MMS Spectrometer - Error Opening CCD Shutter.\n");
    return -1;
  }
  return 0;
}

/*
 * void close_CCD_shutter()
 *
 * This function will close the spectrometer's CCD shutter. This function will do
 * nothing if the spectrometer does not support a shutter. Shutter support can be
 * determined by the use of print_info() function.
 *
 * Inputs: udev structure
 * 
 * Return value: -1 for error, 0 for success.
 *     
 */

EXPORT int meas_matrix_close_CCD_shutter(struct usb_dev_handle *udev) {

  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x08; /* command - Close CCD Shutter */
  *len = 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 && usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error Closing CCD Shutter.\n");
    return -1;
  }
  return 0;
}

/*
 * void module_close()
 *
 * This function will release the spectrometer's udev handle and make
 *
 * Inputs: udev struct
 * 
 * Return value: None
 *     
 */

EXPORT void meas_matrix_module_close(struct usb_dev_handle *udev) {

  usb_reset(udev); /* Most systems developed for windows don't know what close means... */
  usb_close(udev);
}

/*
 * void get_pixel_hw()
 *
 * Returns the pixel hieght and width of the spectrometer in use.
 *
 * Input: struct udev. unsigned short *width, unsigned short *height. On exit these are
 * CCD width in pixels and CCD height in pixels
 *
 * Return: -1 for error, 0 for success.
 *
 */

EXPORT int meas_matrix_get_pixel_hw(struct usb_dev_handle *udev, unsigned short *width, unsigned short *height) {

  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x16; /* command - Get spectrometer info */
  *len = 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(usb_bulk_write(udev, 4, (char *) writebuf, sizeof(writebuf), 0) <= 0
     || usb_bulk_read(udev, 8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error reading device info.\n");
    return -1;
  }
  /* NOTE: For some reason the camera returns 532 as the CCD width even though
   * it is 512. Hard coded for now.  FIXME!!!
   */
#if 0
  *width = *((unsigned short *) &readbuf[7]);
  *height = *((unsigned short *) &readbuf[9]);
#else
  *width = MEAS_MATRIX_WIDTH;
  *height = MEAS_MATRIX_HEIGHT;   
#endif
  return 0;
}

/*
 * unsigned char get_ccd_bpp()
 *
 * This function returns an 8-bit unsigned int which indicates the number of CCD byte per pixel
 *
 * Input: udev struct
 *
 * return value: 8-bit unsigned integer which indicates the number of CCD bytes per pixel
 * 0 on error.
 *
 */

EXPORT unsigned char meas_matrix_get_ccd_bpp(struct usb_dev_handle *udev) {

  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x16; /* command - Get spectrometer info */
  *len = 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(usb_bulk_write(udev, 4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, 8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error reading device info.\n");
    return 0;
  }
  return (unsigned char) ((int) readbuf[11]/((int) 8));
}

/*
 * void matrix_get_exposure()
 *
 * This just have to be called before matrix_get_recostruction() to get the spectrum.
 * No reason to return the image since it cannot be processed.
 *
 */

EXPORT int meas_matrix_get_exposure(struct usb_dev_handle *udev) {
  
  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  int i, j;
  unsigned int data_size;
  
  writebuf[0] = 0x0E; /* command - Get Exposure */
  *len = 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP6, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "Newport Oriel MMS Spectrometer - Error Getting Exposure.\n");
    return -1;
  }

  data_size = *((unsigned int *) &readbuf[8]);
  
  for(i = 0; i < data_size; i += 64)
    if(usb_bulk_read(udev, MEAS_MATRIX_EP6, readbuf, 64, 0) <= 0) {
      fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error Getting Exposure.\n");
      return -1;
    }
  return 0;
}

/*
 * void set_reconstruction()
 *
 * This function sets the reconstruction algorithm that will be used to
 * geternate spectra data on an camera exposure. This command will only
 * successfully complete if it is called outside a start_exposure() and
 * end_exposure() function calls
 * 
 * Inputs: udev structure
 *
 * Return value: -1 for error, 0 for success.
 *
 * ****NOTE**** Host Command Format (Schema 0x02) not implemented.
 *
 */

EXPORT int meas_matrix_set_reconstruction(struct usb_dev_handle *udev) {

  unsigned char writebuf[7], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x10; /* command - Set Reconstruction */
  *len = 7; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  writebuf[6] = 0x00; /* Algorithm */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error Setting Reconstruction.\n");
    return -1;
  }
  return 0;
}

/*
 * unsigned int matrix_get_reconstruction()
 *
 * This command fetches the digital spectra reconstruction of a captured exposure. This
 * command should only be used outside the use of the start_exposure() and end_exposure()
 * functions.
 *
 * Input: udev structure
 *   unsigned char type: Reconstruction Type - Denotes the type of reconstruction
 *                            data to retrieve. Acceptable values are:
 *                            0x01 - Reconstruction from Light Exposure - Requires that both light
 *                                   and dark exposure have been previously taken by the spectrometer
 *                                   prior to this command being used.
 *                            0x02 - Reconstruction from Dark Exposure - Requires that a dark
 *                                   exposure has been taken previously by the spectrometer prior to
 *                                   this being issued.
 *                            0x03 - Reconstruction from Reference Exposure - Requires that both a
 *                                   reference and dark exposure have been previously taken b the
 *                                   spectrometer priot to this command being issued.
 *
 * 
 *     float *data = Data array. This must be allocated by the caller.
 *
 * Return value: number of data points transferred. 0 on error.
 *
 */

EXPORT unsigned int meas_matrix_get_reconstruction(struct usb_dev_handle *udev, unsigned char type, float *data) {

  unsigned char writebuf[7], readbuf[64]; /* read & write buffers */
  unsigned char saturation;
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  int i, j; 
  unsigned int data_size, pts = 0;
  
  writebuf[0] = 0x0F; /* command - Get Reconstruction */
  *len = 7; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(type != 0x01 && type != 0x02 && type != 0x03) { /* test if we have a valid reconType */
    fprintf(stderr, "libmeas: Invalid matrix reconstruction type.\n");
    return 0;
  }
  writebuf[6] = type; /* Set the reconstruction type */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error getting Reconstruction.\n");
    return 0;
  }
  
  saturation = *((unsigned char *) &readbuf[8]);
  if(saturation) fprintf(stderr, "libmeas: Saturated pixels in matrix (warning).\n");
  /* skip data count - seems to be always zero */
  data_size = *((unsigned int *) &readbuf[1]);
  
  for(i = 0; i < data_size - 64; i += 64) {
    if(usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0) {
      fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error Getting Exposure.\n");
      return -1;
    }
    
    for(j = 0; j < 64; j += 4) {
      data[i/4+j/4] = *((float *) &readbuf[j]);    
      pts++;
      if(pts >= MEAS_MATRIX_WIDTH) break; /* Ignore zero padding in the last packet */
    }
  }
  
  return pts;  /* # of spectral points transferred */
}

/*
 * void get_model_number()
 *
 * This function returns the spectrometer's assigned model number;
 *
 * Input Value: udev structure. Storage space for model number (char *).
 *
 * Return Value: -1 for error, 0 for success.
 *
 */

EXPORT int meas_matrix_get_model_number(struct usb_dev_handle *udev, char *model) {

  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  int i;
  
  writebuf[0] = 0x12; /* command - Get Model Number */
  *len = 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error getting model number.\n");
    return -1;
  }
  
  for(i = 0; i < *((unsigned short *) &readbuf[7]); i++)
    model[i] = (char) readbuf[i + 9]; 
  return 0;
}

/*
 * void get_serial_number()
 *
 * This function returns the spectrometer's assigned serial number;
 *
 * Input Value: udev structure. Storage space for model number (char *)
 *
 * Return Value: -1 for error, 0 for success.
 *
 */

EXPORT int meas_matrix_get_serial_number(struct usb_dev_handle *udev, char *serial) {

  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  int i;
  
  writebuf[0] = 0x14; /* command - Get Serial Number */
  *len = 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error getting serial number.\n");
    return -1;
  }
  
  for(i = 0; i < *((unsigned short *) &readbuf[7]); i++)
    serial[i] = (char) readbuf[i + 9];  
  return 0;
}

/*
 * unsigned short get_clock_rate()
 *
 * This function returns the clock frequency that the spectrometer's camera uses to
 * convert analog data to digital data.
 *
 * Input Value: udev structure
 *
 * Return Value: an unsigned short which specifies the clock frequncy of the A/D converter.
 *               Possible return values:
 *               0x00 = N/A (error)
 *               0x01 = Default
 *               0x02 = 150 KHz
 *               0x03 = 300 KHz
 *               0x04 = 500 KHz
 *               0x05 = 1000 KHz
 *               0x06 = Error getting clock rate
 */

EXPORT unsigned short meas_matrix_get_clock_rate(struct usb_dev_handle *udev) {

  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x1B; /* command - Get Clock Rate */
  *len = 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error getting clock rate.\n");
    return 0x00;
  }
  
  return *((unsigned int *) &readbuf[7]);
}

/*
 * void set_clock_rate(unsigned short freq)
 *
 * This function is used to set the clock frequency that the spectrometer's camera uses to convert
 * analog data to digital data. Faster clock rates allow for image data to be processed faster but
 * with increased electronic noise within the captured CCD image. Slower clock rates lengthen the
 * image processing time but results in a lowered electronic noise withing the captured CCD image.
 *
 * Input: udev structure
 *   unsigned short freq: Specifies the clock frequency of the A/D converter.
 *                        Acceptable values are:
 *                        0x00 = N/A
 *                        0x01 = Default
 *                        0x02 = 150 KHz
 *                        0x03 = 300 KHz
 *                        0x04 = 500 KHz
 *                        0x05 = 1000 KHz
 * 
 * Return value: -1 for error, 0 for success.
 *
 */

EXPORT int meas_matrix_set_clock_rate(struct usb_dev_handle *udev, unsigned short freq) {

  unsigned char writebuf[8], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x1C; /* command - Set Clock Rate */
  *len = 8; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  /* test if we have a valid freq */
  if(freq != 0x00 && freq != 0x01 && freq != 0x02 && freq != 0x03 && freq != 0x04 && freq != 0x05) {
    fprintf(stderr, "libmeas: Invalid matrix frequency.\n");
    return -1;
  }
  
  writebuf[6] = ((char *) &freq)[0];  /* set the zeroth byte of the A/D Clock Frequency */
  writebuf[7] = ((char *) &freq)[1];  /* set the first byte ot the A/D Clock Frequency */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "matrix: Newport Oriel MMS Spectrometer - Error setting clock rate.\n");
    return -1;
  }
}

/*
 * void set_pixel_mode(unsigned short mode)
 *
 * This function is used to set the pixel binning mode that the spectrometer's camera will
 * use with its processing. Pixel binning is where the physical light wells (pixels) on the
 * spectrometer's CCD are grouped together for processing as a single pixel.
 *
 * Input: udev structure
 *   unsigned short mode: Specifies the pixel binning mode to be used.
 *                        Acceptable values are:
 *                        0x00 = N/A
 *                        0x01 = Full Well
 *                        0x02 = Dark Current
 *                        0x03 = Line Binning
 * 
 * Return value: -1 for error, 0 for success.
 *
 */

EXPORT int meas_matrix_set_pixel_mode(struct usb_dev_handle *udev, unsigned short mode) {

  unsigned char writebuf[8], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x1E; /* command - Set Pixel Mode */
  *len = 8; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  /* test if we have a valid mode */
  if(mode != 0x00 && mode != 0x01 && mode != 0x02 && mode != 0x03) {
    fprintf(stderr, "libmeas: Invalid matrix pixel mode.\n");
    return -1;
  }
  
  writebuf[6] = ((char *) &mode)[0];  /* set the first byte of the Pixel Mode */
  writebuf[7] = ((char *) &mode)[1];  /* set the second byte ot the Pixel Mode */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error setting pixel mode.\n");
    return -1;
  }
  return 0;
}

/*
 * unsigned short get_pixel_mode()
 *
 * This function returns the pixel binning mode that the spectrometer's camera uses.
 * Pixel binning is where physical light wells (pixels) on the spectrometer's CCD
 * are grouped together for processing as a single pixel.
 *
 * Input Value: udev structure
 *
 * Return Value: an unsigned short which specifies the pixel binning mode of the CCD.
 *               Possible return values:
 *               0x00 = N/A (error)
 *               0x01 = Full Well
 *               0x02 = Dark Current
 *               0x03 = Line Binning
 *               0x04 = Error getting pixel binning mode
 *
 */

EXPORT unsigned short meas_matrix_get_pixel_mode(struct usb_dev_handle *udev) {

  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */

  writebuf[0] = 0x1D; /* command - Get Pixel Mode */
  *len = 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
    || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error getting pixel mode.\n");
    return 0x00;
  }
  
  return *((unsigned int *) &readbuf[7]);
}

/*
 * float get_CCD_temp()
 *
 * This function retrieves the temperature information from the spectrometer and returns
 * this information via a temperature structure.
 *
 * Input: udev structure.
 *
 * Return value: Temperature in oC. 999.0 = error.
 *
 */

EXPORT float meas_matrix_get_CCD_temp(struct usb_dev_handle *udev) {

  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  float temp;
  unsigned char meas_matrix_is_temp_supported();
  
  writebuf[0] = 0x03; /* command - Get CCD Temperature Info */
  *len = 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  /* Check if temperature regulation is supported */
  /* before sending a get CCD temperature info command. */
  /* Spectrometer will stall if the command is sent to a */
  /* spectrometer that doesn't support temperature regulation */
  if(!meas_matrix_is_temp_supported()) {
    fprintf(stderr, "libmeas: Matrix temperature control not supported.\n");
    return 999.0;
  }
  
  if(usb_bulk_write(udev, MEAS_MATRIX_EP4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, MEAS_MATRIX_EP8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error getting CCD temperature information.\n");
    return 999.0;
  }
  
  temp = *((float *) &readbuf[12]);
  if(readbuf[16]) fprintf(stderr, "libmeas: Matrix CCD thermal fault (warning).\n");
  if(readbuf[17]) fprintf(stderr, "libmeas: Matrix CCD temperature not locked (warning).\n");
  
  return temp;
}

/*
 * unsigned char is_temp_supported()
 *
 * Checks if temperature regulation is supported.
 *
 * Input: udev structure.
 *
 * Return value:  Returns false if there was a problem getting the temperature info else
 *                a boolean value which indicates if temperature regulation is supported.
 *
 */

EXPORT unsigned char meas_matrix_is_temp_supported(struct usb_dev_handle *udev) {

  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x16; /* command - Get spectrometer info */
  *len = 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(usb_bulk_write(udev, 4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, 8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error reading device info.\n");
    return 0;
  }
  
  return (unsigned char) readbuf[24];
}

/*
 * float set_CCD_temp(float temp);
 *
 * This function sets the CCD temperature setpoint if temperature regulation is supported.
 *
 * Input: udev structure.
 *   float temp: The desired CCD temperature setpoint in degrees C.
 *
 * Return value: 0 for success, -1 for error.
 *
 */

EXPORT int meas_matrix_set_CCD_temp(struct usb_dev_handle *udev, float temperature) {
  
  unsigned char writebuf[11], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x04; /* command - Set CCD Temperature Info */
  *len = 11; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  writebuf[6] = 0x01;  /* enable CCD temperature regulation */
  writebuf[7] = ((char *) &temperature)[0];  /* set the zeroth byte of the temperature */
  writebuf[8] = ((char *) &temperature)[1];  /* set the first byte of the temperature */
  writebuf[9] = ((char *) &temperature)[2];  /* set the second byte of the temperature */
  writebuf[10] = ((char *) &temperature)[3];  /* set the third byte of the temperature */
  
  if(usb_bulk_write(udev, 4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, 8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error reading device info.\n");
    return -1;
  }
  return 0;
}

/*
 * void disable_CCD_temp_regulation()
 *
 * Disabled CCD temperature regulation of the spectrometer
 *
 * Input: udev structure
 *
 * Return value: 0 for success, -1 for error.
 *
 */

EXPORT int meas_matrix_disable_CCD_temp_regulation(struct usb_dev_handle *udev) {
  
  unsigned char writebuf[11], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  writebuf[0] = 0x04; /* command - Set CCD Temperature Info */
  *len = 11; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  writebuf[6] = 0x00;  /* disable CCD temperature regulation */
  
  if(usb_bulk_write(udev, 4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, 8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error reading device info.\n");
    return -1;
  }
  return 0;
}

/*
 * void set_AFE_parameters(unsigned short offset, unsigned short gain)
 *
 * This command set the current AFE offset and gain settings of the spectrometer's camera.
 *
 * Inputs: udev structure
 *   short offset: Specifies the current dark current compensation
 *                 Acceptable values: 0 - 1024.
 *   short gain: Specifies the current input sensitivity to the full scale output of the CCD
 *               Acceptable values: 0 - 63.
 *
 * Return value: return 0 for success, -1 for error.
 *
 * ****NOTE**** I have yet to get this function to work properly with my spectrometer my guess
 *              is that not all spectrometers support this function. Unfortunatley there is no
 *              way to test if your spectrometer supports this function other than trying the funnction
 *              yourself and seeing if the function works without throwing an error.
 * 
 */

EXPORT int meas_matrix_set_AFE_parameters(struct usb_dev_handle *udev, short offset, short gain) {

  unsigned char writebuf[13], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */
  
  if(offset < 0 || offset > 1024) { /* test if we have a valid offset */
    fprintf(stderr, "libmeas: Matrix offset out of bounds.\n");
    return -1;
  }
  if(gain < 0 || gain > 63) { /* test if we have a valid gain */
    fprintf(stderr, "libmeas: Matrix gain out of bounds.\n");
    return -1;
  }
  writebuf[0] = 0x2B; /* command - Set CCD Temperature Info */
  *len = 13; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  writebuf[9] = ((char *) &offset)[0];  /* set the zeroth byte of the offset */
  writebuf[10] = ((char *) &offset)[1];  /* set the first byte of the offset */
  writebuf[11] = ((char *) &gain)[0];  /* set the zeroth byte of the gain */
  writebuf[12] = ((char *) &gain)[1];  /* set the first byte of the gain */
  
  if(usb_bulk_write(udev, 4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, 8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error setting AFE parameters.\n");
    return -1;
  }
  return 0;
}

/* 
 * short *get_AFE_parameters()
 *
 * This function returns the current AFE offset and gain settings of the spectrometer's camera.
 *
 * Input: udev structure
 *
 * Return value: an short array of size 2 which contains the following inormation:
 *               index = 0: offset - Specifies the current dark current
 *                          Possible values: 0 - 1024.
 *               index = 1; gain - specifies the current input sensitivity to the full scale output of the CCD.
 *                          Possible values: 0 - 63.
 *               error is NULL is returned
 *
 * Returns 0 for success, -1 for error.
 *
 */

EXPORT int meas_matrix_get_AFE_parameters(struct usb_dev_handle *udev, short *params) {

  unsigned char writebuf[6], readbuf[64]; /* read & write buffers */
  int *len = (int *) &writebuf[1]; /* pointer to the command size of the writebuf array */

  writebuf[0] = 0x2A; /* command - Set CCD Temperature Info */
  *len = 6; /* set the length of the command */
  writebuf[5] = 0x01; /* Schema number */
  
  if(usb_bulk_write(udev, 4, (char *) writebuf, sizeof(writebuf), 0) <= 0 || usb_bulk_read(udev, 8, (char *) readbuf, sizeof(readbuf), 0) <= 0
     || readbuf[6] == 0x00) {
    fprintf(stderr, "libmeas: Newport Oriel MMS Spectrometer - Error getting AFE parameters.\n");
    return -1;
  }
  
  params[0] = *((short *) &readbuf[9]);
  params[1] = *((short *) &readbuf[11]);
  return 0;
}
