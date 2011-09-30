/*
 * Driver routines for old Aspect computer slow speed serial bus.
 * For old Bruker ENDOR equipment.
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
 * This uses the lpt-ttl routines in libmeas.
 *
 * The following units are assumed to be present in the bus:
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
 */

#include <stdio.h>
#include <math.h>
#include <meas/meas.h>
#include <string.h>

main() {

  meas_sl_init();
  meas_sl_wtk(50.0);
  meas_sl_wtk_mod(0);
  meas_sl_pts(15.0);
  meas_sl_attn1(0); /* wtk */ 
  meas_sl_attn2(63); /* pts */
  meas_sl_attn3(0);  /* sum */
  meas_sl_relays(7);  /* 0 or 7 */
  meas_sl_close();
}
