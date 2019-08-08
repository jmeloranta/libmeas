/*
 * Raspberry PI 2, 3, 4 GPIO routines.
 *
 * Adapted from source code written by "petzval" at Raspberry Pi forum.
 *
 * Note: The system will not respond to anything else when interrupts are disabled.
 * Typical GPIO line time jitter when interrupts are off is +- 100 ns.
 *
 */

#ifdef RPI

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "misc.h"

#define MAX_GPIO 35

static char mode[MAX_GPIO];

#define GPIO_BASE 0x00200000
#define TIMER_BASE 0x00003000
#define INT_BASE 0x0000B000

volatile static unsigned *gpio, *gpset, *gpclr, *gpin, *timer, *intrupt, *intquad;
enum pitypes {NOTSET, ARM6, ARM7, PI4};
static enum pitypes pitype = NOTSET;
static unsigned int timend;

#define TIMERLOOP  while((((*timer)-timend) & 0x80000000) != 0)
#define TIMEOUT           (((*timer-timend) & 0x80000000) == 0)
#define NOTIMEOUT         (((*timer-timend) & 0x80000000) != 0)

#define GP_HI(x) *gpset = (1 << (x))
#define GP_LO(x) *gpclr = (1 << (x))

#define GP_IN(x) (*gpin & (1 << (x)))   // GPIO 3 input - zero or non-zero

#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7 << (((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1 << (((g)%10)*3))

/*
 * Initialize GPIO driver.
 *
 */

EXPORT int meas_gpio_open() {

  int memfd, i;
  void *gpio_map, *timer_map, *int_map, *quad_map;
  unsigned int baseadd;
  FILE *stream;
  int n, c, getout;

  for(i = 0; i < MAX_GPIO; i++) mode[i] = -1;  // Unknown

  // read file /proc/cpuinfo to determine PI type
  // look for "ARMv6" or "ARMv7"

  pitype = NOTSET;  // in case it fails
          
  baseadd = 0;
  
  stream = fopen("/proc/device-tree/soc/ranges","rb");
            
  if(stream == NULL) {
    fprintf(stderr, "libmeas: Failed to open /proc/device-tree/soc/ranges\n");
    exit(1);
  }
   
  getout = 0;
  n = 0;
  do {
    c = getc(stream);
    if(c == EOF) {
      baseadd = 0;
      getout = 1;    
    } else if(n > 3) baseadd = (baseadd << 8) + c;
    ++n;
    if(n == 8 && baseadd == 0) n = 4;  // read third 4 bytes 
  } while(getout == 0 && n < 8);  
  fclose(stream);

  if(baseadd == 0x20000000) {
    pitype = ARM6;   // Pi2
    fprintf(stderr, "libmeas: Pi type = ARMv6\n");
  } else if(baseadd == 0x3F000000) {
    pitype = ARM7;  // 3B+
    fprintf(stderr, "libmeas: Pi type = ARMv7\n");
  } else if(baseadd == 0xFE000000) {
    pitype = PI4;
    fprintf(stderr, "libmeas: Pi type = Pi4\n");
  } else {
    fprintf(stderr,"libmeas: Failed to determine Pi type\n");
    exit(1);
  }

  memfd = open("/dev/mem",O_RDWR|O_SYNC);
  if(memfd < 0) {
    fprintf(stderr, "libmeas: Error opening /dev/mem (root access needed).\n");
    pitype = NOTSET;
    exit(1);
  }

  gpio_map = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, baseadd+GPIO_BASE);

  timer_map = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, baseadd+TIMER_BASE);

  if(pitype == PI4) int_map = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, 0xFF841000);  // GIC distributor
  else int_map = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, baseadd+INT_BASE);
  
  if(pitype == ARM7)  // Pi3
    quad_map = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, memfd, 0x40000000);
  else quad_map = MAP_FAILED;
 
  close(memfd);

  if(gpio_map == MAP_FAILED || timer_map == MAP_FAILED || int_map == MAP_FAILED || (pitype == ARM7 && quad_map == MAP_FAILED)) {
    fprintf(stderr, "Map failed\n");
    pitype = NOTSET;
    exit(1);
  }

  if(pitype == ARM7) intquad = (volatile unsigned *) quad_map;  // interrupt pointer
  intrupt = (volatile unsigned *) int_map;     // timer pointer
  timer = (volatile unsigned *) timer_map;
  ++timer;    // timer lo 4 bytes
              // timer hi 4 bytes available via *(timer+1)

  // GPIO pointers
  gpio = (volatile unsigned *) gpio_map;
  gpset = gpio + 7;     // set bit register offset 28
  gpclr = gpio + 10;    // clr bit register
  gpin = gpio + 13;     // read all bits register

  return 0;
}

/*
 * Turn on/off interrupts. Turning interrupts off will improve the time accuracy but the
 * system will not respond to anything while they are off.
 *
 * flag = 0 to set interrupts off, 1 to turn on interrupts (normal operation) (char).
 *
 * Return 0 for success or -1 for error.
 *
 */

EXPORT int meas_gpio_interrupt(char flag) {

  int n;
  unsigned int temp131;
  static unsigned int sav132 = 0;
  static unsigned int sav133 = 0;
  static unsigned int sav134 = 0;
  static unsigned int sav4 = 0;
  static unsigned int sav16 = 0;
  static unsigned int sav17 = 0;
  static unsigned int sav18 = 0;
  static unsigned int sav19 = 0;
  static unsigned int sav20 = 0;
  static unsigned int sav21 = 0;
  static unsigned int sav22 = 0;
  static unsigned int sav23 = 0;
  static int disflag = 0;
  	
  if(pitype == NOTSET) {
    fprintf(stderr, "libmeas: meas_gpio_interrupt() called before opening gpio.\n");
    exit(1);
  }
 
  if(flag == 0) {    // disable interrupts
    if(disflag != 0) {
      // Interrupts already disabled so avoid printf
      return 0;
    }

    // Note on register offsets
    // If a register is described in the documentation as offset 0x20C = 524 bytes
    // The pointers such as intrupt are 4 bytes long,
    // so intrupt+131 points to intrupt + 4x131 = offset 0x20C 

    // save current interrupt settings  

    if(pitype == ARM7) {  // Pi3 only
      sav4 = *(intquad+4);     // Performance Monitor Interrupts set  register 0x0010
      sav16 = *(intquad+16);   // Core0 timers Interrupt control  register 0x0040 
      sav17 = *(intquad+17);   // Core1 timers Interrupt control  register 0x0044
      sav18 = *(intquad+18);   // Core2 timers Interrupt control  register 0x0048
      sav19 = *(intquad+19);   // Core3 timers Interrupt control  register 0x004C
             // the Mailbox interrupts are probably disabled anyway - but to be safe:
      sav20 = *(intquad+20);   // Core0 Mailbox Interrupt control  register 0x0050 
      sav21 = *(intquad+21);   // Core1 Mailbox Interrupt control  register 0x0054
      sav22 = *(intquad+22);   // Core2 Mailbox Interrupt control  register 0x0058
      sav23 = *(intquad+23);   // Core3 Mailbox Interrupt control  register 0x005C
    }
  
    if(pitype == PI4) {
      sav4 = *(intrupt);               // GICD_CTLR register
      *(intrupt) = sav4 & 0xFFFFFFFE;  // clear bit 0 enable forwarding to CPU
    } else {                      
      sav134 = *(intrupt+134);  // Enable basic IRQs register 0x218
      sav132 = *(intrupt+132);  // Enable IRQs 1 register 0x210
      sav133 = *(intrupt+133);  // Enable IRQs 2 register 0x214

      // Wait for pending interrupts to clear
      // Seems to work OK without this, but it does no harm
      // Limit to 100 tries to avoid infinite loop
      n = 0;
      while((*(intrupt+128) | *(intrupt+129) | *(intrupt+130)) != 0 && n < 100) ++n;
    }

    // disable all interrupts

    if(pitype == ARM7) {  // Pi3 only
      *(intquad+5) = sav4;   // disable via Performance Monitor Interrupts clear  register 0x0014
      *(intquad+16) = 0;     // disable by direct write
      *(intquad+17) = 0;
      *(intquad+18) = 0;
      *(intquad+19) = 0;
      *(intquad+20) = 0;
      *(intquad+21) = 0;
      *(intquad+22) = 0;
      *(intquad+23) = 0;
    }
       
    if(pitype != PI4) {
      temp131 = *(intrupt+131);  // read FIQ control register 0x20C 
      temp131 &= ~(1 << 7);      // zero FIQ enable bit 7
      *(intrupt+131) = temp131;  // write back to register
                                 // attempting to clear bit 7 of *(intrupt+131) directly
                                 // will crash the system 
      *(intrupt+135) = sav132;   // disable by writing to Disable IRQs 1 register 0x21C
      *(intrupt+136) = sav133;   // disable by writing to Disable IRQs 2 register 0x220
      *(intrupt+137) = sav134;   // disable by writing to Disable basic IRQs register 0x224
    }
            
    disflag = 1;   // interrupts disabled
  } else {          // flag = 1 enable
    if(disflag == 0) {
      // Interrupts are enabled
      return 0;
    }

    // restore all saved interrupts
    if(pitype == PI4) {
      *(intrupt) = sav4;  // set bit 0
    } else {
      *(intrupt+134) = sav134;
      *(intrupt+133) = sav133;
      *(intrupt+132) = sav132;    
      temp131 = *(intrupt+131);     // read FIQ control register 0x20C
      temp131 |= (1 << 7);          // set FIQ enable bit
      *(intrupt+131) = temp131;     // write back to register
    }
      
    if(pitype == ARM7) { // Pi3 only
      *(intquad+4) = sav4;
      *(intquad+16) = sav16;
      *(intquad+17) = sav17;
      *(intquad+18) = sav18;
      *(intquad+19) = sav19;
      *(intquad+20) = sav20;
      *(intquad+21) = sav21;
      *(intquad+22) = sav22;
      *(intquad+23) = sav23;
      }
    
    disflag = 0;         // indicates interrupts enabled
  }
  return 0;
}

/*
 * Set GPIO port mode (input/output).
 *
 * port = GPIO port (char).
 * mo   = 1 for input or 2 = for output (char).
 *
 * Return 0 for success or -1 for error.
 *
 */

EXPORT int meas_gpio_mode(char port, char mo) {

  if(port < 0 || port > MAX_GPIO) {
    fprintf(stderr, "libmeas: Invalid GPIO pin.\n");
    return -1;
  }
  if(mo == 1) { // input
    INP_GPIO(port);
    mode[port] = 1;
  } else if(mo == 2) {  // output
    INP_GPIO(port);
    OUT_GPIO(port);
    mode[port] = 2;
  } else {
    fprintf(stderr, "libmeas: Invalid GPIO pin mode.\n");
    return -1;
  }
  return 0;
}

/*
 * Read GPIO port value.
 *
 * port = GPIO port (char).
 *
 * Returns 0 or 1 depending on the port value
 * and -1 for error.
 *
 */

EXPORT int meas_gpio_read(char port) {

  int value;

  if(port < 0 || port > MAX_GPIO) {
    fprintf(stderr, "libmeas: Invalid GPIO pin.\n");
    return -1;
  }
  if(mode[port] != 1) {
    fprintf(stderr, "libmeas: Wrong GPIO mode for read.\n");
    return -1;
  }
  value = GP_IN(port);
  if(value) return 1;
  else return 0;
}

/*
 * Write to GPIO port.
 *
 * port  = GPIO port (char).
 * value = Value (1 or 0) (char).
 *
 * Return 0 for success or -1 for error.
 *
 */

EXPORT int meas_gpio_write(char port, char value) {

  if(port < 0 || port > MAX_GPIO) {
    fprintf(stderr, "libmeas: Invalid GPIO pin.\n");
    return -1;
  }
  if(mode[port] != 2) {
    fprintf(stderr, "libmeas: Wrong GPIO mode for write.\n");
    return -1;
  }
  if(value) GP_HI(port);
  else GP_LO(port);

  return 0;  
}

/*
 * Timer function (microsecond resolution).
 *
 * dtim = Delay time in microseconds (unsigned int).
 *
 * No return value.
 *
 */

EXPORT void meas_gpio_timer(unsigned int dtim) {

  timend = *timer + dtim;
  TIMERLOOP;
}

#endif /* RPI */
