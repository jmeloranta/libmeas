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

#define MEAS_MATRIX_EP4 4  /* Endpoint 4 (instrument input) */
#define MEAS_MATRIX_EP6 6  /* Endpoint 6 (CCD image data stream) */
#define MEAS_MATRIX_EP8 8  /* Endpoint 8 (instrument output) */

/*
 * CCD element dimensions. Hard coded for now - see comments in
 * matrix_get_pixel_hw().
 *
 */
#define MEAS_MATRIX_WIDTH 512
#define MEAS_MATRIX_HEIGHT 128

#endif
