/*
 * Newport Diode array + monochromator (IS minispectrometer).
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "newport_is.h"
#include <usb.h>
#include "misc.h"

/* Only one unit supported / machine (TODO) */

#define EP1 1 /* out to instrument */
#define EP2 2 /* data from instrument */

static usb_dev_handle *udev = NULL;

static void err_handler(int x) {

  int meas_newport_is_close();

  meas_misc_root_on();
  meas_newport_is_close();
  fprintf(stderr, "newport_is: Program interrupted.\n");
  exit(1);
}

static void setup_signals() {

  signal(SIGINT, &err_handler);
  signal(SIGSTOP, &err_handler);
  signal(SIGQUIT, &err_handler);
}

static void disable_signals() {

  signal(SIGINT, SIG_IGN);
  signal(SIGSTOP, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
}

static void enable_signals() {

  signal(SIGINT, &err_handler);
  signal(SIGSTOP, &err_handler);
  signal(SIGQUIT, &err_handler);
}

/* Byte order is different from intel */

static void swap_bytes(unsigned short *v) {

  unsigned char *p, tmp;

  p = (unsigned char *) v;
  tmp = *p;
  *p = *(p + 1);
  *(p + 1) = tmp;
}

static unsigned char lowbyte(unsigned short v) {

  unsigned char *p;

  p = (unsigned char *) &v;
  return *(p+1);
}

static unsigned char highbyte(unsigned short v) {

  unsigned char *p;

  p = (unsigned char *) &v;
  return *p;
}

/*
 * Return the number of points in spectrum.
 *
 */

int meas_newport_is_size() {

  return 1024;
}

/* 
 * Initialize the device
 * 
 */

int meas_newport_is_init() {

  struct usb_bus *busses, *bus;
  struct usb_device *dev;
  char buf[4096];

  /* libusb requires root (we need setuid root executable) */
  meas_misc_root_on();
  if(udev) meas_err("meas_newport_is_init: device already open.");

  usb_init();
  usb_find_busses();
  usb_find_devices();
    
  busses = usb_get_busses();
    
  for (bus = busses; bus; bus = bus->next) {
    
    for (dev = bus->devices; dev; dev = dev->next)
      if(dev->descriptor.idVendor == 0x03eb && dev->descriptor.idProduct == 0x6124) break;

    if(dev) break;
  }
  if(!dev) meas_err("meas_newport_is_init: Instrument not found.");

  if(!(udev = usb_open(dev))) 
    meas_err("meas_newport_is_init: Can't open instrument.");

  fprintf(stderr, "meas_newport_is_init: Instrument found.\n");

  /* This instrument is basically an implementation of USB/serial interface. */
  /* One could use usbserial vendor=0x03eb product=0x6124  to talk to it. */
  /* Here I decided to use libusb instead. */
  /* EP1 = Bulk out (to instrument), EP2 = Bulk input (from instrument). */

  if(usb_set_configuration(udev, 1) < 0)
    meas_err("meas_newport_is_init: Can't set active configuration."); /* 1st config. */
  if(usb_claim_interface(udev, 1) < 0)
    meas_err("meas_newport_is_init: Can't claim interface."); /* 1st interface */
  if(usb_set_altinterface(udev, 0) < 0)
    meas_err("meas_newport_is_init: Can't set alternate interface."); /* 0th alt */

  meas_misc_root_off();
  return 0;
}

/*
 * Take a scan.
 * 
 * exp = Exposure time in s. Note that valid times for this instrument are in ms range!
 * ext = External trigger (0 = internal, > 0 external with the value giving
 *       an estimate for trigger delay time in ms - minimum 1 ms). For
 *       delay times less than 1 ms, no need to worry about this.
 * ave = Number of averages to take.
 * dst = Where to write the spectrum (short wl first, long wl last).
 *
 * The instrument goes into the measurement mode when it receives the
 * command. Unfortunately, there seems to be no mechanism to query
 * the instrument if the measurement is done. One could send another command
 * and see if the response is OK but in practice this seems to initiate
 * another measurement cycle. This leads to build up of buffered measurement
 * requests in the instrument; causing lagging response and ultimately crash
 * of the instrument when it runs out of memory... 
 * So, we send the request and make sure that we wait for long enough before
 * reading anything back. Not a good solution but can't really do anything else...
 *
 */

int meas_newport_is_read(double exp, int ext, int ave, double *dst) {

  unsigned short exposure;
  unsigned short buf2[1024];
  unsigned char buf[2 * 1024];
  int i, j;

  meas_misc_root_on();
  if(!udev) meas_newport_is_init();

  /* Prepare scan request */
  memset(buf, 0x61, sizeof(buf));
  exposure = (unsigned short) (exp * 1E3); /* s -> ms */
  buf[0] = 0x01;
  buf[2] = lowbyte(exposure);
  buf[3] = highbyte(exposure);
  buf[4] = ext?1:0;
  buf[5] = 0;

  for (i = 0; i < 1024; i++) dst[i] = 0.0;

  for (i = 0; i < ave; i++) {
    long secs, nsecs;
    fprintf(stderr, "meas_newport_is_read: Measurement cycle: %d\n", i+1);
    disable_signals();
    for (j = 0; ; j++) {
      if(usb_bulk_write(udev, EP1, (char *) buf, sizeof(buf), 0) < 0)
	meas_err("meas_newport_is_read: USB write failed.");
      secs = 0;
      nsecs = MEAS_NEWPORT_IS_DELAY * 1000000;
      nsecs += exp * 1000000;
      if(nsecs > 999999999) {
	secs += nsecs / 999999999;
	nsecs = nsecs % 999999999;
      }
      nsecs += ext * 1000000;
      if(nsecs > 999999999) {
	secs += nsecs / 999999999;
	nsecs = nsecs % 999999999;
      }
      meas_misc_nsleep(secs, nsecs);
      if(usb_bulk_read(udev, EP2, (char *) buf2, sizeof(buf2), 0) < 0)
	meas_err("meas_newport_is_read: USB read failed.");
      /* early read? */
      if(highbyte(buf2[0]) != 0xF5 || lowbyte(buf2[0]) != 0xFA) break;
      fprintf(stderr, "meas_newport_is_read: Early read - increase MEAS_NEWPORT_IS_DELAY!\n");
    }
    enable_signals();
    for (j = 0; j < 1024; j++) swap_bytes(&buf2[j]);
    for (j = 0; j < 1024; j++) dst[j] += buf2[1024 - j - 1];
  }

  for (i = 0; i < 1024; i++) dst[i] /= (double) ave;

  meas_misc_root_off();
  return 0;
}

/*
 * Close the spectrometer.
 *
 */

int meas_newport_is_close() {

  meas_misc_root_on();
  if(udev) usb_close(udev);
  meas_misc_root_off();
  return 0;
}

/*
 * Wavelength calibration.
 *
 * pixel = Pixel number input.
 *
 * Returns the corresponding wavelength in nm.
 *
 * TODO: The calibration constants are hardwired.
 *
 */


double meas_newport_is_calib(int pixel) {

  return (MEAS_NEWPORT_IS_A + MEAS_NEWPORT_IS_B * ((double) pixel));
}
