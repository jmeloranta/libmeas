/**************************************************************************
 *                      gpib_bitbang.c  -  description                    *
 *                           -------------------                          *
 *  This code has been developed at the Institute of Sensor and Actuator  *
 *  Systems (Technical University of Vienna, Austria) to enable the GPIO  *
 *  lines (e.g. of a raspberry pi) to function as a GPIO master device    *
 *                                                                        *
 *  begin                : March 2016                                     *
 *  copyright            : (C) 2016 Thomas Klima                          *
 *  email                : elektronomikon@gmail.com                       *
 *                                                                        *
 *************************************************************************/

/**************************************************************************
 *                                                                        *
 *  January 2017: widely modified to add SRQ and use interrupts for the   *
 *  long waits by Marcello Carla' (carla@fi.infn.it) at Department of     *
 *  Physics (University of Florence, Italy)                               *
 *                                                                        *
 **************************************************************************/

/**************************************************************************
 *                                                                        *
 *   This program is free software; you can redistribute it and/or modify *
 *   it under the terms of the GNU General Public License as published by *
 *   the Free Software Foundation; either version 2 of the License, or    *
 *   (at your option) any later version.                                  *
 *                                                                        *
 *************************************************************************/

/*
  not implemented:
	parallel/serial polling
	return2local?
*/

#define DEBUG 0
#define TIMEOUT_US 1000000
#define IRQ_DEBOUNCE_US 1000
#define DELAY 10
#define JIFFY (1000000 / HZ)

#define NAME "gpib_bitbang"
#define HERE  NAME, (char *) __FUNCTION__

#define dbg_printk(frm,...) if (DEBUG) \
               printk(KERN_INFO "%s:%s - " frm, HERE, ## __VA_ARGS__ )
#define LINVAL gpio_get_value(DAV),\
               gpio_get_value(NRFD),\
               gpio_get_value(SRQ)
#define LINFMT "DAV: %d  NRFD:%d  SRQ: %d"

#include "gpibP.h"
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>

//  GPIB signal to GPIO pin-number mappings; fedora 4.13.5 needs ARCH_NR_GPIOS - NGPIOS offset!

/* For raspi 3 */
#define NGPIOS 54

typedef enum {
	D01 = ARCH_NR_GPIOS - NGPIOS + 20,
	D02 = ARCH_NR_GPIOS - NGPIOS + 26,
	D03 = ARCH_NR_GPIOS - NGPIOS + 16,
	D04 = ARCH_NR_GPIOS - NGPIOS + 19,
	D05 = ARCH_NR_GPIOS - NGPIOS + 13,
	D06 = ARCH_NR_GPIOS - NGPIOS + 12,
	D07 = ARCH_NR_GPIOS - NGPIOS + 6,
	D08 = ARCH_NR_GPIOS - NGPIOS + 5,
	EOI = ARCH_NR_GPIOS - NGPIOS + 9,
	NRFD = ARCH_NR_GPIOS - NGPIOS + 24,
	IFC = ARCH_NR_GPIOS - NGPIOS + 22,
	_ATN = ARCH_NR_GPIOS - NGPIOS + 25,
	REN = ARCH_NR_GPIOS - NGPIOS + 27,
	DAV = ARCH_NR_GPIOS - NGPIOS + 10,
	NDAC = ARCH_NR_GPIOS - NGPIOS + 23,
	SRQ = ARCH_NR_GPIOS - NGPIOS + 11,
	PE = ARCH_NR_GPIOS - NGPIOS + 7,
	DC = ARCH_NR_GPIOS - NGPIOS + 8,
	TE = ARCH_NR_GPIOS - NGPIOS + 18,
	ACT_LED = ARCH_NR_GPIOS - NGPIOS + 4,
} lines_t;

static struct gpio gpios[] = {
	{D01, GPIOF_IN, "D01" },
	{D02, GPIOF_IN, "D02" },
	{D03, GPIOF_IN, "D03" },
	{D04, GPIOF_IN, "D04" },
	{D05, GPIOF_IN, "D05" },
	{D06, GPIOF_IN, "D06" },
	{D07, GPIOF_IN, "D07" },
	{D08, GPIOF_IN, "D08" },
	{EOI, GPIOF_OUT_INIT_HIGH, "EOI" },
	{NRFD, GPIOF_IN, "NRFD" },
	{IFC, GPIOF_OUT_INIT_HIGH, "IFC" },
	{_ATN, GPIOF_OUT_INIT_HIGH, "ATN" },
	{REN, GPIOF_OUT_INIT_HIGH, "REN" },
	{DAV, GPIOF_OUT_INIT_HIGH, "DAV" },
	{NDAC, GPIOF_IN, "NDAC" },
	{SRQ, GPIOF_IN, "SRQ" },
	{PE, GPIOF_OUT_INIT_LOW, "PE" },
	{DC, GPIOF_OUT_INIT_LOW, "DC" },
	{TE, GPIOF_OUT_INIT_LOW, "TE" },
	{ACT_LED, GPIOF_OUT_INIT_LOW, "ACT_LED" },
};

// struct which defines private_data for gpio driver
typedef struct {
    int irq_DAV;
    int irq_NRFD;
    int irq_SRQ;
    uint8_t eos;	// eos character
    short eos_flags; // eos mode
    int go_read;
    int go_write;
    short int end;
    int request;
    int count;
    uint8_t *rbuf;
    uint8_t *wbuf;
} bb_private_t;

void set_data_lines(uint8_t byte);
uint8_t get_data_lines(void);
void set_data_lines_input(void);
int check_for_eos(bb_private_t *priv, uint8_t byte);
irqreturn_t bb_DAV_interrupt(int, void *);
irqreturn_t bb_NRFD_interrupt(int, void *);

inline int SET_DIR_WRITE(gpib_board_t *);
inline int SET_DIR_READ(gpib_board_t *);

MODULE_LICENSE("GPL");

/****  global variables  ****/

static int debug=1;

module_param (debug, int, S_IRUGO | S_IWUSR);

char printable(char x) {

  if (x < 32 || x > 126) return ' ';
  return x;
}

/* Set up DAV as input line (interrupt) */
int input_DAV(gpib_board_t *board) {

  bb_private_t *priv = board->private_data;

  /* Set up DAV interrupt */
  gpio_direction_input(DAV);
  if(priv->irq_DAV == 0) {
    priv->irq_DAV = gpio_to_irq(DAV);
    if (request_irq(priv->irq_DAV, bb_DAV_interrupt, IRQF_TRIGGER_FALLING, "gpib_bitbang", board)) {
      printk("gpib: can't request IRQ for DAV %d\n", board->ibirq);
      return -1;
    }
  }
  return 0;
}

/* Set up DAV as output line */
int output_DAV(gpib_board_t *board, int value) {

  bb_private_t *priv = board->private_data;

  /* unregister DAV interrupt */
  if(priv->irq_DAV) {
    free_irq(priv->irq_DAV, board);
    priv->irq_DAV = 0;
  }
  gpio_direction_output(DAV, value);
  return 0;
}

/* Set up NRFD as input line (interrupt) */
int input_NRFD(gpib_board_t *board) {

  bb_private_t *priv = board->private_data;

  /* Set up NRFD interrupt */
  gpio_direction_input(NRFD);
  if(priv->irq_NRFD == 0) {
    priv->irq_NRFD = gpio_to_irq(NRFD);
    if (request_irq(priv->irq_NRFD, bb_NRFD_interrupt, IRQF_TRIGGER_RISING, "gpib_bitbang", board)) {
      printk("gpib: can't request IRQ for NRFD %d\n", board->ibirq);
      return -1;
    }
  }
  return 0;
}

/* Set up NRFD as output line */
int output_NRFD(gpib_board_t *board, int value) {

  bb_private_t *priv = board->private_data;

  /* unregister NRFD interrupt */
  if(priv->irq_NRFD) {
    free_irq(priv->irq_NRFD, board);
    priv->irq_NRFD = 0;
  }
  gpio_direction_output(NRFD, value);
  return 0;
}

/***************************************************************************
 *                                                                         *
 * READ                                                                    *
 *                                                                         *
 ***************************************************************************/

int bb_read(gpib_board_t *board, uint8_t *buffer, size_t length, int *end, size_t *bytes_read) {

  bb_private_t *priv = board->private_data;
  int retval;

  dbg_printk("board: %p  lock %d  length: %d\n", board, mutex_is_locked(&board->user_mutex), length);

  priv->end = 0;
  priv->count = 0;
  priv->rbuf = buffer;

  if (length == 0) goto read_end;

  priv->request = length;

  SET_DIR_READ(board);
  udelay(DELAY);

  dbg_printk (".........." LINFMT "\n",LINVAL);

  /* interrupt wait for first DAV valid */

  priv->go_read = 1;

  input_NRFD(board);

  retval = wait_event_interruptible(board->wait, board->status & TIMO || priv->go_read == 0);

  if (board->status & TIMO) {
    retval = -ETIMEDOUT;
    dbg_printk("timeout - " LINFMT "\n", LINVAL);
    goto read_end;
  }

  if(retval) goto read_end;  /* -ERESTARTSYS */

  /* poll loop for data read */

  while (1) {
    while (gpio_get_value(DAV) && !(board->status & TIMO));

    if (board->status & TIMO) {
      retval = -ETIMEDOUT;
      dbg_printk ("timeout + " LINFMT "\n", LINVAL);
      goto read_end;
    }

    output_NRFD(board, 0);

    priv->rbuf[priv->count++] = get_data_lines();
    priv->end = !gpio_get_value(EOI);

    dbg_printk(LINFMT "count: %3d eoi: %d  val: %2x -> %c\n", LINVAL, priv->count-1, priv->end, priv->rbuf[priv->count-1],
               printable(priv->rbuf[priv->count-1]));

    gpio_direction_input(NDAC);

    if ((priv->count >= priv->request) || priv->end || check_for_eos(priv, priv->rbuf[priv->count-1])) {
      dbg_printk ("wake_up with %d %d %x\n", priv->count, priv->end, priv->rbuf[priv->count-1]);
      goto read_end;
    }

    while (!gpio_get_value(DAV) && !(board->status & TIMO));

    if (board->status & TIMO) {
      retval = -ETIMEDOUT;
      dbg_printk("timeout - " LINFMT "\n", LINVAL);
      goto read_end;
    }

    gpio_direction_output(NDAC, 0);
    udelay(DELAY);
    input_NRFD(board);
  }

read_end:
  dbg_printk("got %d bytes.\n", priv->count);
  *bytes_read = priv->count;
  *end = priv->end;

  priv->rbuf[priv->count] = 0;

  dbg_printk("return: %d  eoi: %d\n\n", retval, priv->end);

  return retval;
}

/***************************************************************************
 *                                                                         *
 *      READ interrupt routine (DAV line)                                  *
 *                                                                         *
 ***************************************************************************/

irqreturn_t bb_DAV_interrupt(int irq, void *arg) {

  gpib_board_t *board = arg;
  bb_private_t *priv = board->private_data;

  if (!priv->go_read) return IRQ_HANDLED;
  priv->go_read = 0;

  dbg_printk ("n: %-3d"  LINFMT " val: %2x\n", priv->count, LINVAL, get_data_lines());

  wake_up_interruptible(&board->wait);

  return IRQ_HANDLED;
}

/***************************************************************************
 *                                                                         *
 * WRITE                                                                   *
 *                                                                         *
 ***************************************************************************/

int bb_write(gpib_board_t *board, uint8_t *buffer, size_t length, int send_eoi, size_t *bytes_written) {

  size_t i = 0;
  int retval = 0;
  bb_private_t *priv = board->private_data;

  dbg_printk("board %p  lock %d  length: %d\n", board, mutex_is_locked(&board->user_mutex), length);

  if (debug) {
    dbg_printk("<%d %s>\n", length, (send_eoi)?"w.EOI":" ");
    for (i = 0; i < length; i++) {
      dbg_printk("%3d  0x%x->%c\n", i, buffer[i], printable(buffer[i]));
    }
  }

  priv->count = 0;
  if (length == 0) goto write_end;
  priv->request = length;
  priv->end = send_eoi;

  /* interrupt wait for first NRFD valid */

  priv->go_write = 1;
  SET_DIR_WRITE(board);

  retval = wait_event_interruptible (board->wait, gpio_get_value(NRFD) || priv->go_write == 0 || board->status & TIMO);

  if (board->status & TIMO) {
    retval = -ETIMEDOUT;
    dbg_printk ("timeout - " LINFMT "\n", LINVAL);
    goto write_end;
  }
  if (retval) goto write_end;  /* -ERESTARTSYS */

  dbg_printk("NRFD: %d   NDAC: %d\n", gpio_get_value(NRFD), gpio_get_value(NDAC));

  /*  poll loop for data write */

  for (i = 0; i < length; i++) {
    while (!gpio_get_value(NRFD) && !(board->status & TIMO));
    if(board->status & TIMO) {
      retval = -ETIMEDOUT;
      dbg_printk ("timeout - " LINFMT "\n", LINVAL);
      goto write_end;
    }

    if ((i >= length-1) && send_eoi) gpio_direction_output(EOI, 0);
    else gpio_direction_output(EOI, 1);

    dbg_printk("sending %d\n", i);

    set_data_lines(buffer[i]);
    output_DAV(board, 0);

    while (!gpio_get_value(NDAC) && !(board->status & TIMO));

    dbg_printk("accepted %d\n", i);

    udelay(DELAY);
    gpio_direction_output(DAV, 1);
  }
  retval = i;

write_end:
  *bytes_written = i;

  dbg_printk("sent %d bytes.\r\n\r\n", *bytes_written);

  return retval;
}

/***************************************************************************
 *                                                                         *
 *      WRITE interrupt routine (NRFD line)                                *
 *                                                                         *
 ***************************************************************************/

irqreturn_t bb_NRFD_interrupt(int irq, void *arg) {

  gpib_board_t *board = arg;
  bb_private_t *priv = board->private_data;

  if (!priv->go_write) return IRQ_HANDLED;
  priv->go_write = 0;

  dbg_printk (" %-3d " LINFMT " val: %2x\n", priv->count, LINVAL, get_data_lines());

  wake_up_interruptible(&board->wait);

 return IRQ_HANDLED;
}

/***************************************************************************
 *                                                                         *
 *      interrupt routine for SRQ line                                     *
 *                                                                         *
 ***************************************************************************/

irqreturn_t bb_SRQ_interrupt(int irq, void * arg) {

  gpib_board_t  * board = arg;
  int val = gpio_get_value(SRQ);

  dbg_printk ("   " LINFMT " -> %d   st: %4lx\n", LINVAL, val, board->status);

  if (!val) set_bit(SRQI_NUM, &board->status);  /* set_bit() is atomic */

  wake_up_interruptible(&board->wait);

  return IRQ_HANDLED;
}

ssize_t bb_command(gpib_board_t *board, uint8_t *buffer, size_t length, size_t *bytes_written) {

  size_t ret, i;

  dbg_printk("%p  %p\n", buffer, board->buffer);

  SET_DIR_WRITE(board);
  gpio_direction_output(_ATN, 0);
  output_NRFD(board, 0);

  if (debug) {
    dbg_printk("CMD(%d):\n", length);
    for (i = 0; i < length; i++) {
      if (buffer[i] & 0x40) {
        dbg_printk("0x%x=TLK%d\n", buffer[i], buffer[i]&0x1F);
      } else {
        dbg_printk("0x%x=LSN%d\n", buffer[i], buffer[i]&0x1F);
      }
    }
  }

  ret = bb_write(board, buffer, length, 0, bytes_written);
  gpio_direction_output(_ATN, 1);

  return ret;
}


/***************************************************************************
 *                                                                         *
 * STATUS Management                                                       *
 *                                                                         *
 ***************************************************************************/


int bb_take_control(gpib_board_t *board, int synchronous) {

  udelay(DELAY);
  dbg_printk("%d\n", synchronous);
  gpio_direction_output(_ATN, 0);
  set_bit(CIC_NUM, &board->status);
  return 0;
}

int bb_go_to_standby(gpib_board_t *board) {

  dbg_printk("\n");
  udelay(DELAY);
  gpio_direction_output(_ATN, 1);
  return 0;
}

void bb_request_system_control(gpib_board_t *board, int request_control ) {

  dbg_printk("%d\n", request_control);
  udelay(DELAY);
  if (request_control)
    set_bit(CIC_NUM, &board->status);
  else
    clear_bit(CIC_NUM, &board->status);
}

void bb_interface_clear(gpib_board_t *board, int assert) {

  udelay(DELAY);
  dbg_printk("%d\n", assert);
  if (assert)
    gpio_direction_output(IFC, 0);
  else
    gpio_direction_output(IFC, 1);
}

void bb_remote_enable(gpib_board_t *board, int enable) {

  dbg_printk("%d\n", enable);
  udelay(DELAY);
  if (enable) {
    set_bit(REM_NUM, &board->status);
    gpio_direction_output(REN, 0);
  } else {
    clear_bit(REM_NUM, &board->status);
    gpio_direction_output(REN, 1);
  }
}

int bb_enable_eos(gpib_board_t *board, uint8_t eos_byte, int compare_8_bits) {

  bb_private_t *priv = board->private_data;

  dbg_printk("%s\n", "EOS_en");
  priv->eos = eos_byte;
  priv->eos_flags = REOS;
  if (compare_8_bits) priv->eos_flags |= BIN;

  return 0;
}

void bb_disable_eos(gpib_board_t *board) {

  bb_private_t *priv = board->private_data;
  dbg_printk("\n");
  priv->eos_flags &= ~REOS;
}

unsigned int bb_update_status(gpib_board_t *board, unsigned int clear_mask) {

  dbg_printk("0x%lx mask 0x%x\n",board->status, clear_mask);
  board->status &= ~clear_mask;

 if (gpio_get_value(SRQ)) {                    /* SRQ asserted low */
   clear_bit (SRQI_NUM, &board->status);
 } else {
   set_bit (SRQI_NUM, &board->status);
 }

 return board->status;
}

void bb_primary_address(gpib_board_t *board, unsigned int address) {

  dbg_printk("%d\n", address);
  board->pad = address;
}

void bb_secondary_address(gpib_board_t *board, unsigned int address, int enable) {

  dbg_printk("%d %d\n", address, enable);
  if (enable) board->sad = address;
}

int bb_parallel_poll(gpib_board_t *board, uint8_t *result) {

  dbg_printk("%s\n", "not implemented");
  return 0;
}

void bb_parallel_poll_configure(gpib_board_t *board, uint8_t config) {

  dbg_printk("%s\n", "not implemented");
}

void bb_parallel_poll_response(gpib_board_t *board, int ist) {

}

void bb_serial_poll_response(gpib_board_t *board, uint8_t status) {

  dbg_printk("%s\n", "not implemented");
}

uint8_t bb_serial_poll_status(gpib_board_t *board) {

  dbg_printk("%s\n", "not implemented");
  return 0;
}

unsigned int bb_t1_delay(gpib_board_t *board, unsigned int nano_sec) {

  udelay(nano_sec/1000 + 1);

  return 0;
}

void bb_return_to_local(gpib_board_t *board) {

  dbg_printk("%s\n", "not implemented");
}

int bb_line_status(const gpib_board_t *board ) {

  int line_status = 0x00;

  if (gpio_get_value(REN) == 1) line_status |= BusREN;
  if (gpio_get_value(IFC) == 1) line_status |= BusIFC;
  if (gpio_get_value(NDAC) == 0) line_status |= BusNDAC;
  if (gpio_get_value(NRFD) == 0) line_status |= BusNRFD;
  if (gpio_get_value(DAV) == 0) line_status |= BusDAV;
  if (gpio_get_value(EOI) == 0) line_status |= BusEOI;
  if (gpio_get_value(_ATN) == 0) line_status |= BusATN;
  if (gpio_get_value(SRQ) == 0) line_status |= BusSRQ;

  dbg_printk("status lines: %4x\n", line_status);

  return line_status;
}


/***************************************************************************
 *                                                                         *
 * Module Management                                                       *
 *                                                                         *
 ***************************************************************************/

static int allocate_private(gpib_board_t *board) {

  board->private_data = kmalloc(sizeof(bb_private_t), GFP_KERNEL);
  if (board->private_data == NULL) return -1;
  memset(board->private_data, 0, sizeof(bb_private_t));
  return 0;
}

static void free_private(gpib_board_t *board) {

  if (board->private_data) {
    kfree(board->private_data);
    board->private_data = NULL;
  }
}

int bb_attach(gpib_board_t *board, gpib_board_config_t config) {

  bb_private_t *priv;

  board->status = 0;
  if (allocate_private(board)) return -ENOMEM;
  priv = board->private_data;

  if(SET_DIR_WRITE(board) < 0) return -1;

  /* request SRQ interrupt for Service Request */
  gpio_direction_input(SRQ);
  priv->irq_SRQ = gpio_to_irq(SRQ);
  if (request_irq(priv->irq_SRQ, bb_SRQ_interrupt, IRQF_TRIGGER_FALLING, "gpib_bitbang", board)) {
    printk("gpib: can't request IRQ for SRQ %d\n", board->ibirq);
    return -1;
  }

  printk("Registered bitbang GPIB board.\n");

  return 0;
}

void bb_detach(gpib_board_t *board) {        

  bb_private_t *priv = board->private_data;

  if (priv->irq_DAV) free_irq(priv->irq_DAV, board);
  if (priv->irq_NRFD) free_irq(priv->irq_NRFD, board);
  if (priv->irq_SRQ) free_irq(priv->irq_SRQ, board);
  free_private(board);
}

gpib_interface_t bb_interface = {
  name:			"gpib_bitbang",
  attach:		bb_attach,
  detach:		bb_detach,
  read:			bb_read,
  write:		bb_write,
  command:		bb_command,
  take_control:		bb_take_control,
  go_to_standby:	bb_go_to_standby,
  request_system_control:	bb_request_system_control,
  interface_clear:	bb_interface_clear,
  remote_enable:	bb_remote_enable,
  enable_eos:		bb_enable_eos,
  disable_eos:		bb_disable_eos,
  parallel_poll:	bb_parallel_poll,
  parallel_poll_configure:	bb_parallel_poll_configure,
  parallel_poll_response:	bb_parallel_poll_response,
  line_status:		bb_line_status,
  update_status:	bb_update_status,
  primary_address:	bb_primary_address,
  secondary_address:	bb_secondary_address,
  serial_poll_response:	bb_serial_poll_response,
  serial_poll_status:	bb_serial_poll_status,
  t1_delay: 		bb_t1_delay,
  return_to_local: 	bb_return_to_local,
};

static int __init bb_init_module(void) {

  int ret;

  ret = gpio_request_array(gpios, ARRAY_SIZE(gpios));

  if (ret) {
    printk("Unable to request GPIOs: %d\n", ret);
    return ret;
  }

//  SET_DIR_WRITE();
  gpio_direction_output(ACT_LED, 1);

  dbg_printk("%s\n", "module loaded!");

  gpib_register_driver(&bb_interface, THIS_MODULE);

  return ret;
}

static void __exit bb_exit_module(void) {

  int i;

  /* all to low inputs is the safe default */
  for(i = 0; i < ARRAY_SIZE(gpios); i++) {
    gpio_set_value(gpios[i].gpio, 0);
    gpio_direction_output(i, 0);
  }

  gpio_free_array(gpios, ARRAY_SIZE(gpios));

  dbg_printk("%s\n", "module unloaded!");

  gpib_unregister_driver(&bb_interface);
}

module_init(bb_init_module);
module_exit(bb_exit_module);

/***************************************************************************
 *                                                                         *
 * UTILITY Functions                                                       *
 *                                                                         *
 ***************************************************************************/

int check_for_eos(bb_private_t *priv, uint8_t byte) {

  static const uint8_t sevenBitCompareMask = 0x7f;

  if ((priv->eos_flags & REOS) == 0) return 0;

  if (priv->eos_flags & BIN) {
    if (priv->eos == byte) return 1;
  } else {
    if ((priv->eos & sevenBitCompareMask) == (byte & sevenBitCompareMask)) return 1;
  }
  return 0;
}

void set_data_lines(uint8_t byte) {

  gpio_direction_output(D01, !(byte & 0x01));
  gpio_direction_output(D02, !(byte & 0x02));
  gpio_direction_output(D03, !(byte & 0x04));
  gpio_direction_output(D04, !(byte & 0x08));
  gpio_direction_output(D05, !(byte & 0x10));
  gpio_direction_output(D06, !(byte & 0x20));
  gpio_direction_output(D07, !(byte & 0x40));
  gpio_direction_output(D08, !(byte & 0x80));
}

uint8_t get_data_lines(void) {

  uint8_t ret = 0;
//  set_data_lines_input();
  ret += gpio_get_value(D01) * 0x01;
  ret += gpio_get_value(D02) * 0x02;
  ret += gpio_get_value(D03) * 0x04;
  ret += gpio_get_value(D04) * 0x08;
  ret += gpio_get_value(D05) * 0x10;
  ret += gpio_get_value(D06) * 0x20;
  ret += gpio_get_value(D07) * 0x40;
  ret += gpio_get_value(D08) * 0x80;
  return ~ret;
}

void set_data_lines_input(void) {

  gpio_direction_input(D01);
  gpio_direction_input(D02);
  gpio_direction_input(D03);
  gpio_direction_input(D04);
  gpio_direction_input(D05);
  gpio_direction_input(D06);
  gpio_direction_input(D07);
  gpio_direction_input(D08);
}

inline int SET_DIR_WRITE(gpib_board_t *board) {

  udelay(DELAY);
  gpio_set_value(DC, 0);
  gpio_set_value(PE, 1);
  gpio_set_value(TE, 1);

  gpio_direction_input(NDAC);
  gpio_direction_output(EOI, 1);

  output_DAV(board, 1);

  if(input_NRFD(board) < 0) return -1;

  set_data_lines(0);

  return 0;
}

inline int SET_DIR_READ(gpib_board_t *board) {

  udelay(DELAY);
  gpio_set_value(DC, 1);
  gpio_set_value(PE, 0);
  gpio_set_value(TE, 0);

  gpio_direction_output(NDAC, 0); // Assert NDAC, informing the talker we have not yet accepted the byte
  gpio_direction_input(EOI); 

  output_NRFD(board, 0);

  if(input_DAV(board) < 0) return -1;

  set_data_lines_input();
  return 0;
}
