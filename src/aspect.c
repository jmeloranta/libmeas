/*
 * Driver routines for old Aspect computer slow speed serial bus.
 * Used, for example, in some old Bruker ENDOR equipment.
 * The interface has Bruker type code EN-003.
 *
 * Due to the limited number of TTL outputs in the required two
 * parallel ports, we cannot specify all the address lines. In
 * most systems, they are not necessary. Currently the routines
 * are hardwired to keep address lines #3, #5 and #6 as zero.
 * Aspect uses negative TTL everywhere (driven by grounded ANDs).
 *
 * This implementation requires two parallel ports that are both
 * connected to the slow speed serial bus as follows (there is only
 * one Aspect connector, which is connected to two LPT cables):
 *
 * LPT1 (25 pin male)     Aspect (25 pin female)    Comment
 * 1                      25                        Strobe (normal TTL)
 * 2                      1                         Data #0 (LSB)
 * 3                      2                         Data #1 
 * 4                      3                         Data #2
 * 5                      4                         Data #3
 * 6                      5                         Data #4
 * 7                      6                         Data #5
 * 8                      7                         Data #6
 * 9                      8                         Data #7
 * 18                     20                        Ground
 *
 * LPT2 (25 pin male)     Aspect (25 pin female)    Comment
 * 1                      16, 18, 19                Strobe (keep high)
 *                                                  Hardwired address lines
 *                                                  #3, #5, and #6.
 * 2                      9                         Data #8
 * 3                      10                        Data #9
 * 4                      11                        Data #10
 * 5                      12                        Data #11
 * 6                      13                        Address #0
 * 7                      14                        Address #1
 * 8                      15                        Address #2
 * 9                      17                        Address #4
 *
 * The data lines #4, #5, and #6 should be kept at TTL high, which in the Aspect
 * bus means that they are zero. Remember that the strobe signal is also
 * negative TTL.
 *
 * Additionally, the following units are assumed to be present on the bus:
 *
 *  address(hex)   address(bin)    reduced address(bin)    function
 *                 654 3210
 *  -----------------------------------------------------------------
 *  0x2            000 0010        0010 (0x2)              PTS fine
 *  0x3            000 0011        0011 (0x3)              PTS mid
 *  0x4            000 0100        0100 (0x4)              PTS coarse
 *  0x10           001 0000        1000 (0x8)              WTK fine
 *  0x11           001 0001        1001 (0x9)              WTK coarse
 *  0x12           001 0010        1010 (0xa)              attn 1
 *  0x13           001 0011        1011 (0xb)              attn 2
 *  0x14           001 0100        1100 (0xc)              attn 3
 *  0x15           001 0101        1101 (0xd)              relays
 *  0x16           001 0110        1110 (0xe)              modulation
 *  0x17           001 0111        1111 (0xf)              modulation
 *
 * At the moment we only implement writing to the bus as none of the
 * above devices can reply.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "lpt-ttl.h"

/*
 * Initialize the serial bus.
 *
 */

EXPORT int meas_sl_init() {

  meas_lpt_init(0);
  meas_lpt_init(1);
  meas_lpt_strobe(0, 1);      /* LPT1 strobe used to trigger the bus */
  meas_lpt_strobe(1, 0);      /* LPT2 strobe permanently high */
  return 0;
}

/*
 * Write to the serial bus.
 *
 * address = Serial bus destination address.
 * data    = Data to be sent.
 * 
 */

EXPORT int meas_sl_write(unsigned int address, unsigned int data) {

  unsigned char p1, p2;

  /* partition for the two LPTs */
  p1 = data & 0xff;  /* for LPT1 */
  p2 = ((address & 0x10)<<3) + ((address & 0x07)<<4) + ((data & 0xf00)>>8); /* for LPT2 */

  meas_lpt_strobe(1, 0);      /* LPT2 strobe permanently high */  
  meas_lpt_write(0, ~p1);     /* aspect is negative TTL logic */
  meas_lpt_write(1, ~p2);

  meas_lpt_strobe(0, 0);      /* pull the TTL line up (strobe for aspect) */
  meas_misc_nsleep(0, 10000);  /* keep up for 1 microsec */
  meas_lpt_strobe(0, 1);      /* pull the TTL line down */
  return 0;
}

/*
 * Close serial bus.
 *
 */

EXPORT int meas_sl_close() {

  meas_lpt_strobe(0, 1);      /* LPT1 low */
  meas_lpt_strobe(1, 1);      /* LPT2 low */
  meas_lpt_write(0, 0);
  meas_lpt_write(1, 0);
  meas_lpt_close(0);
  meas_lpt_close(1);
  return 0;
}

/*
 * digit parsing.
 *
 * digit > 0 left of decimal point, < 0 right of decimal point.
 *
 */

static inline unsigned int digits(double value, int digit) {

  char buf[128], *str;
  int len;

  sprintf(buf, "%lf", value);
  str = strchr(buf, '.');
  len = strlen(buf);
  if(str == NULL) str = buf + len;
  if(str - digit < buf || str - digit > buf + len) return 0;
  return (unsigned int) (*(str - digit) - '0');
}

/*
 * Set frequency to PTS160 NMR frequency generator.
 *
 * value = Frequency in MHz.
 *
 */

EXPORT int meas_sl_pts(double value) {
  /* set freq (MHz) (PTS160) */

  unsigned int word;
  
  meas_sl_write(0x2, 0); /* fine = 0 (must go 1st) */
  word = 0;
  word |= digits(value, -2) << 8;
  word |= digits(value, -3) << 4;
  word |= digits(value, -4);
  meas_sl_write(0x3, word); /* PTS mid (2nd) */
  word = 0;
  word |= digits(value, 2) << 8;
  word |= digits(value, 1) << 4;
  word |= digits(value, -1);
  meas_sl_write(0x4, word); /* PTS coarse (last) */
  return 0;
}

/*
 * Set frequency to Wavetek 3000 NMR frequency generator.
 *
 * value = Frequency in MHz.
 *
 */

EXPORT int meas_sl_wtk(double value) {
  /* freq (MHz) (Wavetek 3000) */

  unsigned int word;

  word = 0;
  word |= digits(value,-1) << 8;
  word |= digits(value,-2) << 4;
  word |= digits(value,-3);
  meas_sl_write(0x10, word); /* WTK fine (1st) */
  word = 0;
  word |= digits(value,3) << 8;
  word |= digits(value,2) << 4;
  word |= digits(value,1);
  meas_sl_write(0x11, word); /* WTK coarse (2nd) */
  return 0;
}

/*
 * Set attenuation for Wavetek 3000.
 *
 * value = Attenuation in dB.
 *
 */

EXPORT int meas_sl_attn1(unsigned int value) {
  /* attenuator 1 (dB) (wtk) */

  meas_sl_write(0x12, ~value);
  return 0;
}

/*
 * Set attenuation for PTS160.
 *
 * value = Attenuation in dB.
 *
 */

EXPORT int meas_sl_attn2(unsigned int value) {
  /* attenuator 1 (dB) (pts) */

  meas_sl_write(0x13, ~value);
  return 0;
}

/*
 * Set common attenuation.
 *
 * value = Attenuation in dB.
 *
 */

EXPORT int meas_sl_attn3(unsigned int value) {
  /* attenuator 1 (dB) (common) */

  meas_sl_write(0x14, ~value);
  return 0;
}

/*
 * Set the experiment type.
 *
 * value = Set as follows (TODO):
 *
 * OLD: ENDOR/TRIPLE GEN = 0, TRIPLE SPEC = 7)
 * It looks like we need 7 for endor/triple gen and possibly 0 for 
 * triple/spec
 * Bit 1: WTK relay (R1), Bit 2: PTS (R2), Bit 3: common (R3)
 *
 */

EXPORT int meas_sl_relays(unsigned int value) {
  /* relays */

  meas_sl_write(0x15, value); /* three bits (1 on, 0 off) */
  return 0;
}

/*
 * Set NMR modulation.
 *
 * value = Modulation in kHz.
 *
 */

EXPORT int meas_sl_wtk_mod(unsigned int value) {
  /* modulation (kHz) */

  double tmp;
  unsigned int value2;

  /* modulation is expressed as dB from the maximum of 500 kHz */
  /* 10 V peak-to-peak input (mod. amp.) to Wavetek corresponds to 500 kHz */
  /* The maximum modulatin freq is 25 kHz (in practice 12.5 kHz) */
  tmp = -10.0 * log10(value / 500.0);
  if(tmp > 10.0) {
    value2 = 0x2; /* 10 db coarse (not) */
    tmp -= 10.0;
  } else value2 = 0x3; /* 0 db coarse (not) */
  value = 4095 * pow(10.0, -tmp/10.0);
  meas_sl_write(0x16, (~value) & 0x0fff);
  meas_sl_write(0x17, value2 & 0x3);
  return 0;
}
