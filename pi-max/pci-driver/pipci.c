/*
 * pipci.c
 *
 * Copyright (C) 2002, 2008 Princeton Instruments
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2 of the License
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* 
 * NOTE: This is modified from the original Princeton pipci driver!
 *       Use at your own risk!
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <generated/autoconf.h>
#include <linux/pci.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <asm/uaccess.h>	
#include <linux/ioctl.h>
#include "pidriver.h"

/* #define DEBUG   /* Enable command debugging */

/* Global structure holds state of card for all devices */
static struct extension device[PI_MAX_CARDS];
static int cards_found = 0;

int DMA_MB      = 1;       /* (was 8) */
int IMAGE_ORDER	= 1;       /* get 4 pages per block (was 2) */
int IMAGE_PAGES	= 2;       /* 2 ^ IMAGE_ORDER	  (was 4) */
int IRQ         = 99;      /* we won't be using 99, just a placeholder for now */
int SHARE       = 1;
#define BYTES_MB 1048576
		
static struct pi_dma_node *dmanodeshead;

MODULE_AUTHOR("Princeton Instruments");
MODULE_DESCRIPTION("PCI Device Driver for TAXI board");
MODULE_ALIAS("PIPCI");	
module_param(DMA_MB, int, 0);
module_param(IMAGE_ORDER, int, 0);
module_param(IMAGE_PAGES, int, 0);
module_param(IRQ, int, 0);
module_param(SHARE, int, 0);
MODULE_PARM_DESC(DMA_MB, "Memory Buffer Size (MB)");
MODULE_PARM_DESC(IMAGE_ORDER, "2 ^ IMAGE_ORDER = IMAGE_PAGES");
MODULE_PARM_DESC(IMAGE_PAGES, "IMAGE_PAGES = 2 ^ IMAGE_ORDER");
MODULE_PARM_DESC(IRQ, "Specify IRQ to use for pipci");
MODULE_PARM_DESC(SHARE, "1 Enables Irq Sharing, 0 Disables Irq Sharing");
MODULE_LICENSE("GPL v2");

/* Global driver entry points */
	
module_init(initialize);
module_exit(cleanup);
	
struct file_operations functions = {	
  .owner   = THIS_MODULE,
  .read    = princeton_read,
  .write   = princeton_write,
  .unlocked_ioctl   = princeton_ioctl,
  .open    = princeton_open,
  .release = princeton_release,
};			
	
static int initialize(void) {

  int devices, err;
  
  if((err = register_chrdev(MAJOR_NUM, DEVICE_NAME, &functions)) < 0) {
    printk("pipci: Failed to register character driver.\n");
    return err;
  }
  
  if (!(devices = princeton_find_devices())) {
    printk("pipci: No devices found.\n");
    return -ENODEV;
  } else printk("pipci: %d devices found.\n", devices);
		
  if(!(dmanodeshead = (struct pi_dma_node *) (__get_free_pages(GFP_KERNEL | GFP_DMA, 1)))) {
    printk("pipci: Failed to allocate DMA memory.\n");
    return 1;
  }

  return 0;
}
	
static void cleanup(void) {

  int i;				

  if (cards_found > 0) {
    for (i = 0; i < cards_found; i++) {
      free_irq(device[i].irq, (struct extension *) &device[i]);
      princeton_release_scatter(&device[i]);
    }
  }

  if(dmanodeshead) free_pages((unsigned long) dmanodeshead, 1);
  
  unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

int princeton_find_devices(void) {				

  int status = 0;
  struct pci_dev *dev = NULL;
  unsigned short command;	
  unsigned long flags;	
		
  while ((dev = pci_get_device(PI_PCI_VENDOR, PI_PCI_DEVICE, dev))) { 
    pci_read_config_word(dev, PCI_COMMAND, &command);
    pci_read_config_word(dev, PCI_BASE_ADDRESS_0, (unsigned short *) &device[cards_found].base_address0);
    pci_read_config_word(dev, PCI_BASE_ADDRESS_1, (unsigned short *) &device[cards_found].base_address1);
    pci_read_config_word(dev, PCI_BASE_ADDRESS_2, (unsigned short *) &device[cards_found].base_address2);
    if (device[0].base_address0 & 1) {
      device[cards_found].base_address0 = device[cards_found].base_address0 - 1;
      device[cards_found].base_address1 = device[cards_found].base_address1 - 1;
      device[cards_found].base_address2 = device[cards_found].base_address2 - 1;
      command |= PCI_COMMAND_IO;
      device[cards_found].mem_mapped = 0;
    } else {
      device[cards_found].mem_mapped = 1;
      command |= PCI_COMMAND_MEMORY;		
    }

    if(IRQ != 99) dev->irq = IRQ;
    device[cards_found].irq = dev->irq;		
    printk("pipci: Using IRQ %d\n", dev->irq);
    device[cards_found].bufferflag = 0;
    device[cards_found].dmainfo.numberofentries = 0;
    
    printk("pipci: Base address 0 0x%x\n", (unsigned int) device[0].base_address0);
    printk("pipci: Base address 1 0x%x\n", (unsigned int) device[0].base_address1);
    printk("pipci: Base address 2 0x%x\n", (unsigned int) device[0].base_address2);
    
    // TODO: should we remove IRQF_DISABLED ?
    flags = IRQF_DISABLED | ((SHARE)?IRQF_SHARED:0);
    status = request_irq(dev->irq, princeton_handle_irq, flags, DEVICE_NAME, &device[cards_found]);
    
    command |= PCI_COMMAND_MASTER;	
    pci_write_config_word(dev, PCI_COMMAND, command);
    pci_set_master(dev);
    
    princeton_do_scatter_boot(BYTES_MB * DMA_MB, &device[cards_found]);
    cards_found++;
  }			
  
  return cards_found;
}

static ssize_t princeton_read(struct file *fp, char *buffer, size_t length, loff_t *offset) {

#ifdef DEBUG
  printk("pipci: call to read() - this is a null function.\n");
#endif
  return PIDD_SUCCESS;
}					
  
static ssize_t  princeton_write(struct file *fp, const char *buffer, size_t length, loff_t *offset) {

#ifdef DEBUG
  printk("pipci: call to write() - this is a null function.\n");
#endif
  return PIDD_SUCCESS;
}		
	
static int princeton_open(struct inode *inode, struct file *fp) {

  int card;
  
  card = inode->i_rdev & 0x0f;

  if (device[card].state == STATE_OPEN)
    return -EBUSY;

  fp->private_data = (void *) (&device[card]);
  mutex_init(&device[card].mutex);
  device[card].state = STATE_OPEN;

#ifdef DEBUG
  printk("pipci: open device.\n");
#endif

  return PIDD_SUCCESS;
}
	
static int princeton_release(struct inode *inode, struct file *fp) {

  int card;

  card = inode->i_rdev & 0x0f;
  device[card].state = STATE_CLOSED;

#ifdef DEBUG
  printk("pipci: release device.\n");
#endif

  return PIDD_SUCCESS;
}
	
static long princeton_ioctl(struct file *fp, unsigned int ioctl_command, unsigned long ioctl_param) {

  int status;
  struct extension *devicex;

  status = PIDD_SUCCESS;						
  devicex = (struct extension *) (fp->private_data);
  mutex_lock(&devicex->mutex);
  
  switch (ioctl_command) {
  case IOCTL_PCI_GET_PI_INFO:
    princeton_get_info((void *) ioctl_param, devicex);
    break;
  case IOCTL_PCI_READ_BYTE:
  case IOCTL_PCI_READ_WORD:
  case IOCTL_PCI_READ_DWORD:
    princeton_input((void *) ioctl_param, devicex, ioctl_command);
    break;
  case IOCTL_PCI_WRITE_BYTE:
  case IOCTL_PCI_WRITE_WORD:
  case IOCTL_PCI_WRITE_DWORD:
    princeton_output((void *) ioctl_param, devicex, ioctl_command);
    break;
  case IOCTL_PCI_ALLOCATE_SG_TABLE:
    princeton_clear_counters(devicex);
    princeton_do_scatter((void *) ioctl_param, devicex);
    break;
  case IOCTL_PCI_TRANSFER_DATA:
    princeton_transfer_to_user((void *) ioctl_param, devicex);
    break;
  case IOCTL_PCI_GET_IRQS:
    princeton_get_irqs((void *) ioctl_param, devicex);
    break;
  default:
    status = -ENOTTY;
  }
  
  mutex_unlock(&devicex->mutex);
  return status;
}
	
int princeton_output(void *io_object, struct extension *devicex, unsigned int type) {

  struct pi_pci_io output;
  int status = 0;
  
  if(copy_from_user(&output, io_object, sizeof(struct pi_pci_io))) return -EACCES;
  
  if (devicex->mem_mapped == 0) {
    switch (type) {
    case IOCTL_PCI_WRITE_BYTE:
#ifdef DEBUG
      printk("pipci: IOCTL_PCI_WRITE_BYTE - data = %x, port = %x.\n", 
	     (unsigned int) output.data.byte_data, (unsigned int) output.port);
#endif
      outb_p(output.data.byte_data, output.port);
      break;
    case IOCTL_PCI_WRITE_WORD:
#ifdef DEBUG
      printk("pipci: IOCTL_PCI_WRITE_WORD - data = %x, port = %x.\n", 
	     (unsigned int) output.data.word_data, (unsigned int) output.port);
#endif
      outw_p(output.data.word_data, output.port);
      break;
    case IOCTL_PCI_WRITE_DWORD:
#ifdef DEBUG
      printk("pipci: IOCTL_PCI_WRITE_DWORD - data = %x, port = %x.\n",
	     (unsigned int) output.data.dword_data, (unsigned int) output.port);
#endif
      outl_p(output.data.dword_data, output.port);
      break;
    }
  } else {
    switch(type) {		
    case IOCTL_PCI_WRITE_BYTE:
#ifdef DEBUG
      printk("pipci: IOCTL_PCI_WRITE_BYTE - data = %x, port = %x.\n",
	     (unsigned int) output.data.byte_data, (unsigned int) output.port);
#endif
      writeb(output.data.byte_data, (void *) output.port);
      break;
    case IOCTL_PCI_WRITE_WORD:
#ifdef DEBUG
      printk("pipci: IOCTL_PCI_WRITE_WORD - data = %x, port = %x.\n",
	     (unsigned int) output.data.word_data, (unsigned int) output.port);
#endif
      writew(output.data.word_data, (void *) output.port);
      break;
    case IOCTL_PCI_WRITE_DWORD:
#ifdef DEBUG
      printk("pipci: IOCTL_PCI_WRITE_DWORD - data = %x, port = %x.\n",
	     (unsigned int) output.data.dword_data, (unsigned int) output.port);
#endif
      writel(output.data.dword_data, (void *) output.port);
      break;		
    }
  }		
  return status;
}

int princeton_input(void *io_object, struct extension *devicex,	unsigned int type) {

  struct pi_pci_io input;
  int status = 0;
  
  if(copy_from_user(&input, io_object, sizeof(struct pi_pci_io))) return -EACCES;
  
  if (devicex->mem_mapped == 0) {
    switch (type) {
    case IOCTL_PCI_READ_BYTE:
      input.data.byte_data  = inb_p(input.port);
#ifdef DEBUG
      printk("pipci: IOCTL_PCI_READ_BYTE - data = %x, port = %x.\n", 
	     (unsigned int) input.data.byte_data, (unsigned int) input.port);
#endif
      break;
    case IOCTL_PCI_READ_WORD:
      input.data.word_data  = inw_p(input.port);
#ifdef DEBUG
      printk("pipci: IOCTL_PCI_READ_WORD - data = %x, port = %x.\n", 
	     (unsigned int) input.data.word_data, (unsigned int) input.port);
#endif
      break;
    case IOCTL_PCI_READ_DWORD:
      input.data.dword_data = inl_p(input.port);
#ifdef DEBUG
      printk("pipci: IOCTL_PCI_READ_DWORD - data = %x, port = %x.\n", 
	     (unsigned int) input.data.dword_data, (unsigned int) input.port);
#endif
      break;		
    }
  } else {
    switch (type) {
    case IOCTL_PCI_READ_BYTE:
      input.data.byte_data  = readb((void *) input.port);
#ifdef DEBUG
      printk("pipci: IOCTL_PCI_READ_BYTE - data = %x, port = %x.\n", 
	     (unsigned int) input.data.byte_data, (unsigned int) input.port);
#endif
      break;
    case IOCTL_PCI_READ_WORD:
      input.data.word_data  = readw((void *) input.port);
#ifdef DEBUG
      printk("pipci: IOCTL_PCI_READ_WORD - data = %x, port = %x.\n", 
	     (unsigned int) input.data.word_data, (unsigned int) input.port);
#endif
      break;
    case IOCTL_PCI_READ_DWORD:
      input.data.dword_data = readl((void *) input.port);
#ifdef DEBUG
      printk("pipci: IOCTL_PCI_READ_DWORD - data = %x, port = %x.\n", 
	     (unsigned int) input.data.dword_data, (unsigned int) input.port);
#endif
      break;		
    }
  }
  
  if(__copy_to_user(io_object, &input, sizeof(struct pi_pci_io))) return -EACCES;
  
  return status;
}

int princeton_get_info(void *info_object, struct extension *devicex) {

  struct pi_pci_info local_info;
				
  local_info.base_address0   = devicex->base_address0;
  local_info.base_address1   = devicex->base_address1;
  local_info.base_address2   = devicex->base_address2;
  local_info.irq	     = devicex->irq;
  local_info.number_of_cards = cards_found;
		
#ifdef DEBUG
  printk("pipci: IOCTL_PCI_GET_PI_INFO - Request struct pi_pci_info.\n");
#endif
  if(__copy_to_user(info_object, &local_info, sizeof(struct pi_pci_info))) return -EACCES;
  
  return PIDD_SUCCESS;
}
	
int princeton_do_scatter(void *dma_object, struct extension *devicex) {    /* Allocate DMA buffer space */

  int nblocks, i;
  unsigned long bytes_remaining, bsize;

#ifdef DEBUG
  printk("pipci: IOCTL_PCI_ALLOCATE_SG_TABLE (struct pi_usedma_info).\n");
#endif

  /* Buffer Exists Return Its Dma Information */
  if (devicex->dmainfo.numberofentries != 0) {
    if(__copy_to_user(dma_object, &devicex->dmainfo, sizeof(struct pi_userdma_info))) return -EACCES;
    return PIDD_SUCCESS;
  }
  
  /* Free Last Buffer */
  princeton_release_scatter(devicex);
		
  if (devicex == NULL) return -EINVAL;
			
  if (copy_from_user(&devicex->dmainfo, dma_object, sizeof(struct pi_userdma_info))) return -EACCES;
		
  if (devicex->dmainfo.size == 0) return -EINVAL;

  /* allocate memory in blocks */
  bsize = PAGE_SIZE * IMAGE_PAGES;		
  nblocks = (devicex->dmainfo.size / bsize);
  
  if ((nblocks * bsize) < devicex->dmainfo.size) nblocks++;
			
  if (nblocks >= TABLE_SIZE) return -ENOMEM;
		
  bytes_remaining = devicex->dmainfo.size;
  for (i = 0; i < nblocks; i++) {
    /* TODO: Something fishy here: virtual <-> physical */
    devicex->dmainfo.nodes[i].virtaddr = (void *) (__get_free_pages(GFP_KERNEL, IMAGE_ORDER));
    devicex->dmainfo.nodes[i].physaddr = virt_to_phys(devicex->dmainfo.nodes[i].virtaddr);
    if (devicex->dmainfo.nodes[i].physaddr != 0) {
      if (bytes_remaining < bsize) bsize = bytes_remaining;
      devicex->dmainfo.nodes[i].physsize = bsize;
      bytes_remaining = bytes_remaining - bsize;
    } else {				
      printk("pipci: Image allocation failed\n");
      return -ENOMEM;
    }
  }
  
  devicex->dmainfo.numberofentries = nblocks;
  
  if(__copy_to_user(dma_object, &devicex->dmainfo, sizeof(struct pi_userdma_info))) return -EACCES;
  
  devicex->bufferflag = 1;
  
  return PIDD_SUCCESS;
}

void princeton_release_scatter(struct extension *devicex) {    /* Release DMA buffer space */

  int i;
  
  if (devicex == NULL || devicex->dmainfo.numberofentries == 0) return;  
			
  for (i = 0; i < devicex->dmainfo.numberofentries; i++) {
    if (devicex->dmainfo.nodes[i].physaddr != 0) 
      free_pages((unsigned long) devicex->dmainfo.nodes[i].virtaddr, IMAGE_ORDER);
    devicex->dmainfo.nodes[i].physaddr = 0;
    devicex->dmainfo.nodes[i].physsize = 0;
  }
  
  devicex->dmainfo.numberofentries = 0;
}

int princeton_transfer_to_user(void *user_object, struct extension *devicex) {  /* Transfer DMA data to user */

  struct pi_userptr userbuffer;
  struct pi_dma_node *dmanodes;
  void *virtual;
  long bytesleft;

  if(copy_from_user(&userbuffer, user_object, sizeof(struct pi_userptr))) return -EACCES;
  
#ifdef DEBUG
  printk("pipci: IOCTL_PCI_TRANSFER_DATA (struct pi_userptr) - address = %x.\n", (unsigned int) userbuffer.address);
  printk("pipci: IOCTL_PCI_TRANSFER_DATA - size = %x, nodecount = %x, nodestart = %x, offset = %x.\n",
	 (unsigned int) userbuffer.size, (unsigned int) userbuffer.nodecount,
	 (unsigned int) userbuffer.nodestart, (unsigned int) userbuffer.offset);
  printk("pipci: IOCTL_PCI_TRANSFER_DATA - xfernodes = %x, sizeofnodes = %x.\n", 
	 (unsigned int) userbuffer.xfernodes, (unsigned int) userbuffer.sizeofnodes);
#endif
		
  bytesleft = userbuffer.size;

  if(copy_from_user(dmanodeshead, userbuffer.xfernodes, userbuffer.sizeofnodes)) return -EACCES;
  dmanodes = dmanodeshead;
  
  while (dmanodes) {
    virtual = phys_to_virt(dmanodes->physaddr);
    if(__copy_to_user((caddr_t) userbuffer.address, (caddr_t) virtual, dmanodes->physsize)) return -EACCES;
    
    userbuffer.address += dmanodes->physsize;	
    dmanodes = dmanodes->next;
  }

  return PIDD_SUCCESS;
}
	
int princeton_do_scatter_boot(long size, struct extension *devicex) {

  int breakout = 0;
  int nblocks, i;
  unsigned long bytes_remaining, bsize;
		
  /* allocate memory in blocks */
  bsize = PAGE_SIZE * IMAGE_PAGES;		
  nblocks = (size / bsize);
  
  if ((nblocks * bsize) < size) nblocks++;
  
  if (nblocks >= TABLE_SIZE) return -ENOMEM;
  
  bytes_remaining = size;
  for (i = 0; i < nblocks; i++) {
    /* TODO: Something fishy here: virtual <-> physical */
    devicex->dmainfo.nodes[i].virtaddr = (void *) (__get_free_pages(GFP_KERNEL, IMAGE_ORDER));
    devicex->dmainfo.nodes[i].physaddr = virt_to_phys(devicex->dmainfo.nodes[i].virtaddr);
    if (devicex->dmainfo.nodes[i].physaddr) {
      if (bytes_remaining < bsize) bsize = bytes_remaining;
      devicex->dmainfo.nodes[i].physsize = bsize;
      bytes_remaining = bytes_remaining - bsize;
    } else {
      printk("pipci: Image allocation failed\n");
      breakout = 1;
    }
    if (breakout) break;
  }
  
  devicex->dmainfo.numberofentries = i;
  printk("pipci: Nodes allocated %i\n", i);
  devicex->bufferflag = 1;
  
  return PIDD_SUCCESS;
}	
	
int princeton_get_irqs(void *user_object, struct extension *devicex) {

  if(__copy_to_user(user_object, &devicex->irqs, sizeof(struct pi_irqs))) return -EACCES;
  devicex->irqs.interrupt_counter = 0;
  return 1;
}

int princeton_clear_counters(struct extension *devicex) {

  struct extension *ext = devicex;
		
  ext->irqs.interrupt_counter = 0;
  ext->irqs.triggers = 0;
  ext->irqs.eofs = 0;
  ext->irqs.bofs = 0;
  ext->irqs.violations = 0;
  ext->irqs.fifo_full = 0;
  ext->irqs.error_occurred = 0;
  ext->irqs.avail = 0;
  ext->irqs.nframe_count = 0;
  
  return 1;
}		

irqreturn_t princeton_handle_irq(int irq, void *devicex) {

  unsigned long tmp_stat;
  unsigned short rid_stat, rcd_stat, ctrl_reg;
  unsigned char status;
  DECLARE_WAIT_QUEUE_HEAD(wq);
  struct extension *driverx = (struct extension *) devicex;
  
  if (!driverx || driverx->irq != irq) return IRQ_NONE;
  
  /* Clear AMCC IRQ source and disable AMCC Interrupts */
  if (driverx->mem_mapped == 1)
    tmp_stat = readl((void *) (driverx->base_address0 + INTCR));
  else		
    tmp_stat = inl_p(driverx->base_address0 + INTCR);
  
  while (tmp_stat & 0xffff0000L) {		
    if (driverx->mem_mapped == 1)
      writel(tmp_stat, (void *) (driverx->base_address0 + INTCR));
    else		
      outl_p(tmp_stat, driverx->base_address0 + INTCR);
    /* Read Taxi EPLD IRQ Status */
    if (driverx->mem_mapped == 1)
      status = (unsigned char) readl((void *) (driverx->base_address2 + IRQ_RD_PCI));
    else	
      status = (unsigned char) inl_p(driverx->base_address2 + IRQ_RD_PCI);
    
    while (status) { /* stay in loop until all ints serviced */
      if (driverx->mem_mapped == 1)
	writel(status, (void *) (driverx->base_address2 + IRQ_CLR_WR_PCI));
      else		
	outl_p(status, driverx->base_address2 + IRQ_CLR_WR_PCI);
      
      if (status & I_RID1) {             /* controller interrupt data received*/
	                                 /* read data from TAXI EPLD RID regs */
	if (driverx->mem_mapped == 1)
	  rid_stat = (unsigned short) readl((void *) (driverx->base_address2 + RID_RD_PCI));
	else		
	  rid_stat = (unsigned short) inl_p(driverx->base_address2 + RID_RD_PCI);
	
	if (rid_stat & I_TRIG) driverx->irqs.triggers++;
	
	if (rid_stat & I_SCAN)  {
	  driverx->irqs.bofs++;
	  driverx->irqs.nframe_count++;
	} else if (rid_stat & I_EOF) {
	  driverx->irqs.eofs++;		  
	  driverx->irqs.nframe_count++;
	}
      }

      if (status & I_DMA_TC) {      /* Update DMA Controller Equivalent   */
	if (!driverx->irqs.error_occurred) {
	  driverx->irqs.interrupt_counter++; /* Yikes! You are hosed by a Faux OS! */
	  driverx->irqs.avail++;
	  driverx->irqs.nframe_count++;
	}
      }
      
      if (status & I_RCD1) {          /* controller register data received */
	if (driverx->mem_mapped == 1)
	  rcd_stat = (unsigned short) readl((void *) (driverx->base_address2 + RCD_RD_PCI));
	else		
	  rcd_stat = (unsigned short) inl_p(driverx->base_address2 + RCD_RD_PCI);
      }
      
      if(status & I_VLTN) {              /* Taxi Violation has occured       */
	if (driverx->mem_mapped == 1) {
	  ctrl_reg = readl((void *) (driverx->base_address2)); /* get taxi ctrl reg val */
	  writel(ctrl_reg & (~RCV_CLR), (void *) (driverx->base_address2));
	  writel(ctrl_reg | RCV_CLR, (void *) (driverx->base_address2));
	} else {
	  ctrl_reg = inl_p(driverx->base_address2); /* get taxi ctrl reg val */
	  outl_p(ctrl_reg & (~RCV_CLR), driverx->base_address2);
	  outl_p(ctrl_reg | RCV_CLR, driverx->base_address2);
	}
		  
	driverx->irqs.violations++;
	if (driverx->irqs.violations > MAX_VIOLATIONS) {
	  if (driverx->mem_mapped == 1)
	    writel(ctrl_reg & (~IRQ_EN), (void *) (driverx->base_address2));
	  else				
	    outl_p(ctrl_reg & (~IRQ_EN), driverx->base_address2);
	  driverx->irqs.error_occurred = 1;
	}
      }

      if(status & I_FF_FULL) {          /* Fifo Full - scrolling is imminent */
	driverx->irqs.error_occurred = 1;
	driverx->irqs.fifo_full++;
      }

      if (driverx->mem_mapped == 1)
	status = (unsigned char) readl((void *) (driverx->base_address2 + IRQ_RD_PCI));
      else		
	status = (unsigned char) inl_p(driverx->base_address2 + IRQ_RD_PCI);

    } /* end while */
	
    if (driverx->mem_mapped == 1)
      tmp_stat = readl((void *) (driverx->base_address0 + INTCR));
    else		
      tmp_stat = inl_p(driverx->base_address0 + INTCR);
    
  } /*end tmp_stat */ 
  
  /* Re-Write INTCR interrupt mask */ 
  wake_up_interruptible(&wq);
  
  return IRQ_HANDLED;
}
