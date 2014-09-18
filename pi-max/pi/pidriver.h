#ifndef PIDRIVER_H
#define PIDRIVER_H
	
#define TABLE_ORDER 	4    	/* get 16 pages for scatter gather table 	*/
#define TABLE_PAGES 	16  	/* 2 ^ TABLE_ORDER 							*/
#define TABLE_SIZE		8192		/* scatter gather table for 128 meg images  */
						/* (TABLE_PAGES * PAGE_SIZE) / 8 			*/

#define ERROR_NONE		0x00
#define ERROR_IRQ		0x01
#define ERROR_ALLOCATE		0x02

#define PI_PCI_VENDOR		0x10e8
#define PI_PCI_DEVICE		0x801d
#define PI_MAX_CARDS 		0x0c

#define MAJOR_NUM 		177
#define DEVICE_FILE_NAME	"rspipci"
#define DEVICE_NAME		"pipci"

typedef unsigned char uns8, *uns8_ptr;
typedef short int16, *int16_ptr;
typedef unsigned short uint16, *uint16_ptr;
typedef unsigned long uns32, *uns32_ptr;
typedef unsigned long DWORD;
	
struct pi_dma_node {
  void *virtaddr;
  void *physaddr;
  DWORD physsize;
  struct pi_dma_node *next;
};
	
struct pi_dma_frames {
  long nodecount;
  long startindex;
  long offset;
  struct pi_dma_node *node;
  long framenumber;
  struct pi_dma_frames *next;
};

struct pi_userdma_info {
  DWORD numberofentries;
  DWORD size;
  struct pi_dma_node nodes[1024];
};

struct pi_userptr {
  void *address;
  DWORD size;
  DWORD nodecount;
  DWORD nodestart;
  DWORD offset;
  struct pi_dma_frames *xfernodes;
  DWORD sizeofnodes;
};

struct pi_irqs {
  DWORD triggers;
  DWORD eofs;
  DWORD bofs;
  DWORD interrupt_counter;
  DWORD avail;
  DWORD nframe_count;
  DWORD error_occurred;
  DWORD violations;
  DWORD fifo_full;
};

struct extension {
  unsigned long base_address0;
  unsigned long base_address1;
  unsigned long base_address2;
  unsigned int irq;		
  unsigned char state;
  
  /* Dma Buffer Information */
  struct pi_userdma_info dmainfo;	
  unsigned int bufferflag;
  struct pi_irqs irqs;
  unsigned int mem_mapped;
};
	
struct pi_pci_info {
	
  unsigned long base_address0;
  unsigned long base_address1;
  unsigned long base_address2;
  unsigned irq;	
  unsigned short number_of_cards;	
};
	
struct pi_pci_io {

  unsigned long port;
  union {
    unsigned long  dword_data;
    unsigned short word_data;
    unsigned char  byte_data;
  } data;			
};

#define STATE_CLOSED 0
#define STATE_OPEN 1
	
#define PIDD_SUCCESS 0
#define PIDD_FAILURE 1
	
/* Get General Info about the Interface Card... */
#define IOCTL_PCI_GET_PI_INFO  _IOR(MAJOR_NUM, 1, int)
	
/* Reading And Writing Out Ports */
#define IOCTL_PCI_READ_BYTE	    _IOR(MAJOR_NUM, 2, int)
#define IOCTL_PCI_READ_WORD		_IOR(MAJOR_NUM, 3, int)
#define IOCTL_PCI_READ_DWORD	_IOR(MAJOR_NUM, 4, int)
#define IOCTL_PCI_WRITE_BYTE	_IOW(MAJOR_NUM, 5, int)
#define IOCTL_PCI_WRITE_WORD	_IOW(MAJOR_NUM, 6, int)
#define IOCTL_PCI_WRITE_DWORD	_IOW(MAJOR_NUM, 7, int)

/* Get the Dma Information */
#define IOCTL_PCI_ALLOCATE_SG_TABLE _IOWR(MAJOR_NUM, 8, int)
#define IOCTL_PCI_TRANSFER_DATA     _IOWR(MAJOR_NUM, 9, int)
#define IOCTL_PCI_GET_IRQS          _IOWR(MAJOR_NUM, 10, int)	

#define  INTCR     0x38  /* interrupt control register          */

#define CTRL_WR_PCI           0x0         /* taxi ctrl reg; bit defs follow */
#define RESET              0x1
#define FF_SEL0            0x2
#define FF_SEL1            0x4
#define AUTO_INC_RD        0x8
#define RCV_CLR            0x10
#define FF_TEST            0x20
#define IRQ_TEST           0x40
#define IRQ_EN             0x80
#define A_RADR_CLR         0x100       /* self clr'ing bit! don't reset  */
#define FF_RESET           0x200       /* self clr'ing bit! don't reset  */
#define CRCEN_FIBER		  0x2
#define POSSIDLE_FIBER     0x4
#define LOSCONF_FIBER      0x20

#define RID_RD_PCI            0x8   /* controller int reg - bit defs follow */
#define RID_RD_FIBER		  0x8
#define I_VLTN_C           0x1
#define I_EOL              0x2
#define I_EOF              0x4
#define I_TRIG			  0x8
#define I_SCAN             0x40   

#define RCD_RD_PCI            0x10
#define IRQ_CLR_WR_PCI        0x4        /* interrupt control register */
#define IRQ_RD_PCI            0x4        /* local int reg - bit defs follow */
#define I_VLTN             0x1
#define I_LCMD_FIBER		  0x1
#define I_DMA_TC           0x2
#define I_RID1             0x4
#define I_RCD1             0x8
#define I_FF_FULL          0x10
#define BM_ERROR           0x20
#define I_IRQ_SPARE        0x40
#define I_IRQ_TEST         0x80

#define MAX_VIOLATIONS 10

static int __init initialize(void);
static void __exit cleanup(void);
static int princeton_open(struct inode *inode, struct file *fp);
static int princeton_release(struct inode *inode, struct file *fp);
static ssize_t princeton_read(struct file *fp, char *buffer, size_t length, loff_t *offset);
static ssize_t princeton_write(struct file *fp, const char *buffer, size_t length, loff_t *offset);		 		
static long princeton_ioctl(struct file *fp, unsigned int ioctl_command, unsigned long ioctl_param);

int princeton_find_devices(void);
int princeton_output(void *io_object, struct extension *devicex, unsigned int type);
int princeton_input(void *io_object, struct extension *devicex, unsigned int type);
int princeton_get_info(void *info_object, struct extension *devicex);					
int princeton_do_scatter(void *dma_object, struct extension *devicex);
int princeton_do_scatter_boot(long size, struct extension *devicex);
void princeton_release_scatter(struct extension *devicex);
int princeton_transfer_to_user(void *user_object, struct extension *devicex);
irqreturn_t princeton_handle_irq(int irq, void *devicex);
int princeton_get_irqs(void *user_object, struct extension *devicex);
int princeton_clear_counters(struct extension *devicex);
	
#endif /* PIDRIVER_H */
