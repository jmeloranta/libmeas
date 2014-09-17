/*************************************************************
**************************************************************
***     Added version info which is available              ***
***     from modinfo.  Also removed the printing           ***
***     of "princeton_open".                               ***
***     DTrent 25 April 2002                               ***
***     --------------------------------------------       ***
***	Made change to always include modversions.h        ***
***     Eliminating unresolved symbols on insmod	   ***
***     DTrent 8 May 2002				   ***
***     --------------------------------------------       ***
***     Added command line parameter to set IRQ            ***
***     DTrent 25 Nov 2003                                 ***
***     --------------------------------------------       ***
***     Many changes for 2.6 kernel                        ***
***     DTrent 16 Jan 2004                                 ***
***     --------------------------------------------       ***
***     More changes for 2.6 Kernel.  Officially           ***
***     releasing the driver.                              ***
***     DTrent 27 August 2004                              ***
***     --------------------------------------------       ***
***
***     Modified to work with 3.6 series kernels           ***
***     ----------------------------------------           ***
**************************************************************
*************************************************************/
	
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

#define DRV_VERSION "2.0.1 (jme)"
#define DRV_RELDATE "17 September 2014"

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) ((a)*65536+(b)*256+(c))
#endif	

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
MODULE_DESCRIPTION("PCI Device Driver for Linux\n\t\tVERSION "DRV_VERSION " \t" DRV_RELDATE);
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
MODULE_LICENSE("Proprietary");

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

  int devices;
  
  if(register_chrdev(MAJOR_NUM, DEVICE_NAME, &functions) < 0) {
    printk("pipci: Failed to register character driver.\n");
    return 1;
  }
  
  if (!(devices = princeton_find_devices())) {
    printk("pipci: No devices found.\n");
    return 1;
  } else printk("pipci: %d devices found.\n", devices);
		
  // TODO: GFP_KERNEL -> GFP_DMA?
  if(!(dmanodeshead = (struct pi_dma_node *) (__get_free_pages(GFP_KERNEL, 1)))) {
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

  free_pages((unsigned long) dmanodeshead, 1);						
  
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
  
  return PIDD_SUCCESS;
}					
  
static ssize_t  princeton_write(struct file *fp, const char *buffer, size_t length, loff_t *offset) {

  return PIDD_SUCCESS;
}		
	
static int princeton_open(struct inode *inode, struct file *fp) {

  int card;
  
  card = inode->i_rdev & 0x0f;

  if (device[card].state == STATE_OPEN)
    return -EBUSY;

  fp->private_data = (void *) (&device[card]);
  device[card].state = STATE_OPEN;

  return PIDD_SUCCESS;
}
	
static int princeton_release(struct inode *inode, struct file *fp) {

  int card;

  card = inode->i_rdev & 0x0f;
  device[card].state = STATE_CLOSED;

  return PIDD_SUCCESS;
}
	
static long princeton_ioctl(struct file *fp, unsigned int ioctl_command, unsigned long ioctl_param) {

  int status;
  struct extension *devicex;
  status = PIDD_SUCCESS;						
  devicex = (struct extension *) (fp->private_data);
  
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
  }
  
  return status;
}
	
int princeton_output(void *io_object, struct extension *devicex, unsigned int type) {

  struct pi_pci_io output;
  int status = 0;
  
  // TODO: check return value
  (void) copy_from_user(&output, io_object, sizeof(struct pi_pci_io));
  
  if (devicex->mem_mapped == 0) {
    switch (type) {
    case IOCTL_PCI_WRITE_BYTE:
      outb(output.data.byte_data, output.port);
      break;
    case IOCTL_PCI_WRITE_WORD:
      outw(output.data.word_data, output.port);
      break;
    case IOCTL_PCI_WRITE_DWORD:
      outl(output.data.dword_data, output.port);
      break;
    }
  } else {
    switch(type) {		
    case IOCTL_PCI_WRITE_BYTE:
      writeb(output.data.byte_data, (void *) output.port);
      break;
    case IOCTL_PCI_WRITE_WORD:
      writew(output.data.word_data, (void *) output.port);
      break;
    case IOCTL_PCI_WRITE_DWORD:
      writel(output.data.dword_data, (void *) output.port);
      break;		
    }
  }		
  return status;
}

int princeton_input(void *io_object, struct extension *devicex,	unsigned int type) {

  struct pi_pci_io input;
  int status = 0;
  
  // TODO: check return value
  (void) copy_from_user(&input, io_object, sizeof(struct pi_pci_io));
  
  if (devicex->mem_mapped == 0) {
    switch (type) {
    case IOCTL_PCI_READ_BYTE:
      input.data.byte_data  = inb(input.port);
      break;
    case IOCTL_PCI_READ_WORD:
      input.data.word_data  = inw(input.port);
      break;
    case IOCTL_PCI_READ_DWORD:
      input.data.dword_data = inl(input.port);
      break;		
    }
  } else {
    switch (type) {
    case IOCTL_PCI_READ_BYTE:
      input.data.byte_data  = readb((void *) input.port);
      break;
    case IOCTL_PCI_READ_WORD:
      input.data.word_data  = readw((void *) input.port);
      break;
    case IOCTL_PCI_READ_DWORD:
      input.data.dword_data = readl((void *) input.port);
      break;		
    }
  }
  
  // TODO: check return value
  (void) __copy_to_user(io_object, &input, sizeof(struct pi_pci_io));
  
  return status;
}

int princeton_get_info(void *info_object, struct extension *devicex) {

  struct pi_pci_info local_info;
				
  local_info.base_address0   = devicex->base_address0;
  local_info.base_address1   = devicex->base_address1;
  local_info.base_address2   = devicex->base_address2;
  local_info.irq	     = devicex->irq;
  local_info.number_of_cards = cards_found;
		
  // TODO: check return value
  (void) __copy_to_user(info_object, &local_info, sizeof(struct pi_pci_info));
  
  return PIDD_SUCCESS;
}
	
int princeton_do_scatter(void *dma_object, struct extension *devicex) {

  int nblocks, i;
  unsigned long bytes_remaining, bsize;

  /* Buffer Exists Return Its Dma Information */
  if (devicex->dmainfo.numberofentries != 0) {
    // TODO: check return value
    (void) __copy_to_user(dma_object, &devicex->dmainfo, sizeof(struct pi_userdma_info));
    return PIDD_SUCCESS;
  }
  
  /* Free Last Buffer */
  princeton_release_scatter(devicex);
		
  if (devicex == NULL) return -EINVAL;
			
  // TODO: check return value
  copy_from_user(&devicex->dmainfo, dma_object, sizeof(struct pi_userdma_info));
		
  if (devicex->dmainfo.size == 0) return -EINVAL;

  /* allocate memory in blocks */
  bsize = PAGE_SIZE * IMAGE_PAGES;		
  nblocks = (devicex->dmainfo.size / bsize);
  
  if ((nblocks * bsize) < devicex->dmainfo.size) nblocks++;
			
  if (nblocks >= TABLE_SIZE) return -ENOMEM;
		
  bytes_remaining = devicex->dmainfo.size;
  for (i = 0; i < nblocks; i++) {
    // FIXME: GFP and virt_to_bus() not protable
    devicex->dmainfo.nodes[i].virtaddr = (void *) (__get_free_pages(GFP_KERNEL, IMAGE_ORDER));
    devicex->dmainfo.nodes[i].physaddr = (void *) virt_to_bus(devicex->dmainfo.nodes[i].virtaddr);
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
  
  // TODO: check return value
  (void) __copy_to_user(dma_object, &devicex->dmainfo, sizeof(struct pi_userdma_info));
  
  devicex->bufferflag = 1;
  
  return PIDD_SUCCESS;
}

void princeton_release_scatter(struct extension *devicex) {

  int i;
  
  if (devicex == NULL) return;		
  if (devicex->dmainfo.numberofentries == 0) return;  
			
  for (i=0; i< devicex->dmainfo.numberofentries; i++) {
    if (devicex->dmainfo.nodes[i].physaddr != 0) 
      /*      free_pages((int) devicex->dmainfo.nodes[i].virtaddr, IMAGE_ORDER); (JME CHANGE) */
      free_pages((unsigned long) devicex->dmainfo.nodes[i].virtaddr, IMAGE_ORDER);
    devicex->dmainfo.nodes[i].physaddr = 0;
    devicex->dmainfo.nodes[i].physsize = 0;
  }
  
  devicex->dmainfo.numberofentries = 0;
}

int princeton_transfer_to_user(void *user_object, struct extension *devicex) {

  struct pi_userptr userbuffer;
  struct pi_dma_node *dmanodes;
  void *virtual;
  long bytesleft;
		
  // TODO: check return value
  (void) copy_from_user(&userbuffer, user_object, sizeof(struct pi_userptr));
  
  bytesleft = userbuffer.size;

  // TODO: check return value
  (void) copy_from_user(dmanodeshead, userbuffer.xfernodes, userbuffer.sizeofnodes);
  dmanodes = dmanodeshead;
  
  while (dmanodes) {
    virtual = bus_to_virt((unsigned long) dmanodes->physaddr);
    // TODO: check return value
    (void) __copy_to_user((caddr_t) userbuffer.address, (caddr_t) virtual, dmanodes->physsize);
    
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
  
  if ((nblocks * bsize) < size) 
    nblocks++;
  
  if (nblocks >= TABLE_SIZE) 
    return -ENOMEM;
  
  bytes_remaining = size;
  for (i = 0; i < nblocks; i++) {
    // FIXME: GFP and virt_to_bus() not protable
      devicex->dmainfo.nodes[i].virtaddr = (void *) (__get_free_pages(GFP_KERNEL, IMAGE_ORDER));
      devicex->dmainfo.nodes[i].physaddr = (void *) virt_to_bus(devicex->dmainfo.nodes[i].virtaddr);
      if (devicex->dmainfo.nodes[i].physaddr != 0) {
	if (bytes_remaining < bsize) 
	  bsize = bytes_remaining;
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

  // TODO: check the return value
  (void) __copy_to_user(user_object, &devicex->irqs, sizeof(struct pi_irqs));
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
  struct extension *driverx = (struct extension *)devicex;

  if (!driverx || driverx->irq != irq) return IRQ_NONE;

  /* Clear AMCC IRQ source and disable AMCC Interrupts */
  if (driverx->mem_mapped == 1)
    tmp_stat = readl((void *) (driverx->base_address0 + INTCR));
  else		
    tmp_stat = inl(driverx->base_address0 + INTCR);
	
  while (tmp_stat & 0xffff0000L) {		
    if (driverx->mem_mapped == 1)
      writel(tmp_stat, (void *) (driverx->base_address0 + INTCR));
    else		
      outl(tmp_stat, driverx->base_address0 + INTCR);
    /* Read Taxi EPLD IRQ Status */
    if (driverx->mem_mapped == 1)
      status = (unsigned char) readl((void *) (driverx->base_address2 + IRQ_RD_PCI));
    else	
      status = (unsigned char) inl(driverx->base_address2 + IRQ_RD_PCI);
    
    while (status) { /* stay in loop until all ints serviced */
      if (driverx->mem_mapped == 1)
	writel(status, (void *) (driverx->base_address2 + IRQ_CLR_WR_PCI));
      else		
	outl(status, driverx->base_address2 + IRQ_CLR_WR_PCI);
      
      if (status & I_RID1) {             /* controller interrupt data received*/
	                                 /* read data from TAXI EPLD RID regs */
	if (driverx->mem_mapped == 1)
	  rid_stat = (unsigned short) readl((void *) (driverx->base_address2 + RID_RD_PCI));
	else		
	  rid_stat = (unsigned short) inl(driverx->base_address2 + RID_RD_PCI);

	if (rid_stat & I_TRIG)
	  driverx->irqs.triggers++;

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
	  rcd_stat = (unsigned short) inl(driverx->base_address2 + RCD_RD_PCI);
      }

      if(status & I_VLTN) {              /* Taxi Violation has occured       */
	if (driverx->mem_mapped == 1) {
	  ctrl_reg = readl((void *) (driverx->base_address2)); /* get taxi ctrl reg val */			
	  writel(ctrl_reg & (~RCV_CLR), (void *) (driverx->base_address2));
	  writel(ctrl_reg | RCV_CLR, (void *) (driverx->base_address2));
	} else {
	  ctrl_reg = inl(driverx->base_address2); /* get taxi ctrl reg val */			
	  outl(ctrl_reg & (~RCV_CLR), driverx->base_address2);
	  outl(ctrl_reg | RCV_CLR, driverx->base_address2);
	}
		  
	driverx->irqs.violations++;
	if (driverx->irqs.violations > MAX_VIOLATIONS) {
	  if (driverx->mem_mapped == 1)
	    writel(ctrl_reg & (~IRQ_EN), (void *) (driverx->base_address2));
	  else				
	    outl(ctrl_reg & (~IRQ_EN), driverx->base_address2);
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
	status = (unsigned char) inl(driverx->base_address2 + IRQ_RD_PCI);

    } /* end while */
	
    if (driverx->mem_mapped == 1)
      tmp_stat = readl((void *) (driverx->base_address0 + INTCR));
    else		
      tmp_stat = inl(driverx->base_address0 + INTCR);

  } /*end tmp_stat */ 

  /* Re-Write INTCR interrupt mask */ 
  wake_up_interruptible(&wq);

  return IRQ_HANDLED;
}
