/* filename: matrix.h
*
* Newport Oriel MMS Spectrometer Driver
*
* Notes:
*    File requires misc.c
*    Requires libusb 0.1 or greater
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


#ifndef _MATRIX_H_
#define _MATRIX_H_

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>

#define EP4 4  /* Endpoint 4 (instrument input) */
#define EP6 6  /* Endpoint 6 (CCD image data stream) */
#define EP8 8  /* Endpoint 8 (instrument output) */

/*
 * CCD element dimensions. Hard coded for now - see comments in
 * matrix_get_pixel_hw().
 *
 */
#define MATRIX_WIDTH 512
#define MATRIX_HEIGHT 128

/* void matrix_print_info()
 *
 * This function simply prints out various information about the spectrometer to the console
 *
 * Input: None
 *
 * Return value: none
 *
 */
extern void matrix_print_info();


/* unsigned char matrix_module_init()
 *
 * This function detects and initializes the Newport spectrometer and must be called before
 * it can be used. All other functions will fail if called upon before calling this function.
 *
 * Input: None.
 *
 * Return Value: true is successful otherwise returns false.
 *
 */

extern void matrix_module_init();


/* void matrix_set_exposure_time(float time)
 *
 * This function assigns a new image exposure time to the spectrometer. If the exposure time is
 * below that of the spectrometer's minimum exposure time, the exposure time will be set to that
 * of the spectrometer's minimum exposure time. Likewise If the exposure time is above that of
 * the spectrometer;s maximum exposure time, theexposure time will be set to that of the
 * spectrometer's maximum exposure time.
 *
 * Input:
 *   float time: the time that should be set (seconds)
 *
 * Return value: none
 */
extern void matrix_set_exposure_time(float time);


/* float get_exposure_time()
 *
 * This function fetches the spectrometer's currently configured exposure time. The exposure time
 * returned by the command will be within or equal to the minimum and maximum exposure times that 
 * are allowed by the spectrometer.
 *
 * Input: None
 *
 * Return value: if successful returns a float of the exposure time (in seconds) else
 * returns a negative number
 */
extern float matrix_get_exposure_time();


/* void matrix_reset()
 *
 * This function will reset the spectrometer to it's default settings. This function may/will
 * affect the following: CCD temperature regulation and set point, shutter, exposure time,
 * reconstruction mode, clock rate, pixel mode, on/off status of laser, laser power setting(s),
 * and temperature regulation and set point of lasers. **** NOTE **** This function can sometimes
 * cause problems with the Newport Spectrometer and it may be better to use the module_close()
 * function then.
 *
 * Input: none
 *
 * Return value: none
 */
extern void matrix_reset();


/* void matrix_start_exposure(unsigned char shutterState, unsigned char exposureType)
 *
 * This command informs the spectrometer to start a camera exposure.
 * The exposure that is taken will have a time duration equal to that of the last
 * configured exposure time. The exposure that is taken can either use the existing
 * shutter setting or force the state of the shutter. Shutter support can be determined
 * by calling the print_info() function.
 *
 * Inputs:
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
 * Return value: none
 */
extern void matrix_start_exposure(unsigned char shutterState, unsigned char exposureType);


/* unsigned char query_exposure()
 *
 * Returns the current state of the spectrometer's started exposure.
 * Repeated calls to this command will allow an outside process to determine
 * if a requested exposure has been taken by the spectrometer's camera.
 * This command will only successfully complete if it is called between both
 * start_exposure() and en end_exposure().
 *
 * Input: None
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
extern unsigned char matrix_query_exposure();


/* void end_exposure()
 *
 * This function ends the use of the spectrometer's camera in regards to acquiring an exposure.
 * This function should be called only after calls to start_exposure() and when all
 * spectrometer exposure related commands have completed.Execution of this function will cause
 * all future query_exposure() function commands to fail until the next start_exposure()
 * command successfully completes.
 *
 * Input:
 *   unsigned char shutterState: an 8-bit unsigned integer which defines what
 *                               should occur with the spectrometer's camera shutter.
 *                               Possible values are:
 *                               0x00 - close shutter
 *                               0x01 - open shutter
 *                               0x02 - leave shutter in its current state
 * 
 * Return value: none
 *     
 */
extern void matrix_end_exposure(unsigned char shutterState);


/* void get_last_error()
 *
 * This function prints out to the console and returns the last known error that the spectrometer generated.
 *
 * Inputs: Array for error status (length 2)
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
 * Reuturn value: None
 */
extern void matrix_get_last_error(unsigned int *error);


/* void open_CCD_shutter()
 *
 * This function will open the spectrometer's CCD shutter. This function will do
 * nothing if the spectrometer does not support a shutter. Shutter support can be
 * determined by the use of print_info() function.
 *
 * Inputs: None
 * 
 * Return value: None
 *     
 */
extern void matrix_open_CCD_shutter();


/* void close_CCD_shutter()
 *
 * This function will close the spectrometer's CCD shutter. This function will do
 * nothing if the spectrometer does not support a shutter. Shutter support can be
 * determined by the use of print_info() function.
 *
 * Inputs: None
 * 
 * Return value: none
 *     
 */
extern void matrix_close_CCD_shutter();


/* void module_close()
 *
 * This function will release the spectrometer's udev handle and make
 *
 * Inputs: None
 * 
 * Return value: None
 *     
 */
extern void matrix_module_close();


/* void get_pixel_hw()
 *
 * Returns the pixel hieght and width of the spectrometer in use.
 *
 * Input: unsigned short *width, unsigned short *height. On exit these are
 * CCD width in pixels and CCD height in pixels
 *
 * Return: None
 */
extern void matrix_get_pixel_hw(unsigned short *width, unsigned short *height);


/* void matrix_get_exposure()
 *
 * This just have to be called before matrix_get_recostruction() to get the spectrum.
 * No reason to return the image since it cannot be processed.
 *
 */

extern void matrix_get_exposure();

/* unsigned char get_ccd_bpp()
 *
 * This function returns an 8-bit unsigned int which indicates the number of CCD byte per pixel
 *
 * Input: None
 *
 * return value: 8-bit unsigned integer which indicates the number of CCD bytes per pixel
 */
extern unsigned char matrix_get_ccd_bpp();


/* unsigned int matrix_get_reconstruction()
 *
 * This command fetches the digital spectra reconstruction of a captured exposure. This
 * command should only be used outside the use of the start_exposure() and end_exposure()
 * functions.
 *
 * Input:
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
 * Return value: number of data points transferred.
 */
extern unsigned int matrix_get_reconstruction(unsigned char type, float *data);


/* void set_reconstruction()
 *
 * This function sets the reconstruction algorithm that will be used to
 * geternate spectra data on an camera exposure. This command will only
 * successfully complete if it is called outside a start_exposure() and
 * end_exposure() function calls
 * 
 * Inputs: None
 *
 * Return value: none
 *
 * ****NOTE**** Host Command Format (Schema 0x02) not implemented.
 *
 */
extern void matrix_set_reconstruction();


/* void get_model_number()
 *
 * This function returns the spectrometer's assigned model number;
 *
 * Input Value: Storage space for model number (char *).
 *
 * Return Value: None.
 *
 */
extern void matrix_get_model_number(char *);


/* void get_serial_number()
 *
 * This function returns the spectrometer's assigned serial number;
 *
 * Input Value: Storage space for model number (char *)
 *
 * Return Value: None.
 *
 */
extern void matrix_get_serial_number(char *);


/* unsigned short get_clock_rate()
 *
 * This function returns the clock frequency that the spectrometer's camera uses to
 * convert analog data to digital data.
 *
 * Input Value: None
 *
 * Return Value: an unsigned short which specifies the clock frequncy of the A/D converter.
 *               Possible return values:
 *               0x00 = N/A
 *               0x01 = Default
 *               0x02 = 150 KHz
 *               0x03 = 300 KHz
 *               0x04 = 500 KHz
 *               0x05 = 1000 KHz
 *               0x06 = Error getting clock rate
 */
extern unsigned short matrix_get_clock_rate();


/* void set_clock_rate(unsigned short freq)
 *
 * This function is used to set the clock frequency that the spectrometer's camera uses to convert
 * analog data to digital data. Faster clock rates allow for image data to be processed faster but
 * with increased electronic noise within the captured CCD image. Slower clock rates lengthen the
 * image processing time but results in a lowered electronic noise withing the captured CCD image.
 *
 * Input:
 *   unsigned short freq: Specifies the clock frequency of the A/D converter.
 *                        Acceptable values are:
 *                        0x00 = N/A
 *                        0x01 = Default
 *                        0x02 = 150 KHz
 *                        0x03 = 300 KHz
 *                        0x04 = 500 KHz
 *                        0x05 = 1000 KHz
 * 
 * Return value: none.
 *
 */
extern void matrix_set_clock_rate(unsigned short freq);


/* unsigned short get_pixel_mode()
 *
 * This function returns the pixel binning mode that the spectrometer's camera uses.
 * Pixel binning is where physical light wells (pixels) on the spectrometer's CCD
 * are grouped together for processing as a single pixel.
 *
 * Input Value: None
 *
 * Return Value: an unsigned short which specifies the pixel binning mode of the CCD.
 *               Possible return values:
 *               0x00 = N/A
 *               0x01 = Full Well
 *               0x02 = Dark Current
 *               0x03 = Line Binning
 *               0x04 = Error getting pixel binning mode
 *
 */
extern unsigned short matrix_get_pixel_mode();


/* void set_pixel_mode(unsigned short mode)
 *
 * This function is used to set the pixel binning mode that the spectrometer's camera will
 * use with its processing. Pixel binning is where the physical light wells (pixels) on the
 * spectrometer's CCD are grouped together for processing as a single pixel.
 *
 * Input:
 *   unsigned short mode: Specifies the pixel binning mode to be used.
 *                        Acceptable values are:
 *                        0x00 = N/A
 *                        0x01 = Full Well
 *                        0x02 = Dark Current
 *                        0x03 = Line Binning
 * 
 * Return value: None.
 *
 */
extern void matrix_set_pixel_mode(unsigned short mode);


/* float get_CCD_temp()
 *
 * This function retrieves the temperature information from the spectrometer and returns
 * this information via a temperature structure.
 *
 * Input: None.
 *
 * Return value: Temperature in oC.
 */
extern float matrix_get_CCD_temp();


/* unsigned char is_temp_supported()
 *
 * Checks if temperature regulation is supported.
 *
 * Input: None.
 *
 * Return value:  Returns false if there was a problem getting the temperature info else
 *                a boolean value which indicates if temperature regulation is supported.
 *
 */
extern unsigned char matrix_is_temp_supported();


/* float set_CCD_temp(float temp);
 *
 * This function sets the CCD temperature setpoint if temperature regulation is supported.
 *
 * Input:
 *   float temp: The desired CCD temperature setpoint in degrees C.
 *
 * Return value: Returns -274.0 (below absolute zero) if there is an error else
 *               returns a CCD setpoint as a confirmation.
 *
 */
extern void matrix_set_CCD_temp(float temperature);


/* void disable_CCD_temp_regulation()
 *
 * Disabled CCD temperature regulation of the spectrometer
 *
 * Input: None
 *
 * Return value: None
 *
 */
extern void matrix_disable_CCD_temp_regulation();


/* void set_AFE_parameters(unsigned short offset, unsigned short gain)
 *
 * This command set the current AFE offset and gain settings of the spectrometer's camera.
 *
 * Inputs:
 *   short offset: Specifies the current dark current compensation
 *                 Acceptable values: 0 - 1024.
 *   short gain: Specifies the current input sensitivity to the full scale output of the CCD
 *               Acceptable values: 0 - 63.
 *
 * Return value: true if the function completed successfully else false.
 *
 * ****NOTE**** I have yet to get this function to work properly with my spectrometer my guess
 *              is that not all spectrometers support this function. Unfortunatley there is no
 *              way to test if your spectrometer supports this function other than trying the funnction
 *              yourself and seeing if the function works without throwing an error.
 * 
 */
extern void matrix_set_AFE_parameters(short offset, short gain);


/* short * get_AFE_parameters()
 *
 * This function returns the current AFE offset and gain settings of the spectrometer's camera.
 *
 * Input: None.
 *
 * Return value: an short array of size 2 which contains the following inormation:
 *               index = 0: offset - Specifies the current dark current
 *                          Possible values: 0 - 1024.
 *               index = 1; gain - specifies the current input sensitivity to the full scale output of the CCD.
 *                          Possible values: 0 - 63.
 *               error is NULL is returned
 *
 */
extern void matrix_get_AFE_parameters(short *params);

#endif
