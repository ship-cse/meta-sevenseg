#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/io.h>

#define EOK  0

volatile static int is_open = 0;

/** buffers for reading / writing data */
//TODO using global variables should be avoided - make them static in 
//  their local functions
static char msg_out[1024];
static int len_out = 0;

static char msg_in[1024];
static int len_in = 0;

/* defines for the structure of character driver */
#define MAJOR_NUM  202
#define CHRDEV_NAME "seven"

/* base address of the seven segment display */
//TODO move this to be a default, and then add a module argument
#define BASE 0x43c00000

/* ioctl for setting the mode */
//TODO this needs to move to a .h that is shared with user space code
#define IOCTL_SET_MODE _IOW(MAJOR_NUM, 0, uint32_t)
#define IOCTL_SET_TMPD _IOW(MAJOR_NUM, 1, uint32_t)
#define IOCTL_SET_COLPD _IOW(MAJOR_NUM, 2, uint32_t)

/* pointer to the memory mapped address for the device */
static void __iomem *base = NULL;



ssize_t seven_read (struct file * filep, char __user * outb, size_t nbytes, loff_t *offset)
{

  uint32_t bytes_read = 0;
  uint32_t ak = 0;
  uint32_t bcd = 0;
  uint32_t time = 0;
  uint32_t period = 0;
  uint32_t mode = 0;

  if (offset == NULL) return -EINVAL;
 
  if ((*offset == 0) || (len_out == 0)) {
    mode = readl(base) & 0x3;

    switch(mode) {
    case 0:
       sprintf(msg_out,"mode=%d,off\n",mode);
       break;

    case 1:
       ak = readl(base) >> 17;
       sprintf(msg_out, "mode=%d,kathodes=%2x,anodes=%3x\n",
	  mode,ak & 0x1f, (ak >> 6 ) & 0x1ff);
       break;

    case 2:
      bcd = readl(base+4);
      sprintf(msg_out,"mode=%d,bcd=%4x\n", mode, bcd);
      break;

    case 3:
      time = readl(base+4);
      period= readl(base+8);
      sprintf(msg_out,"mode=%d,time=%2x:%2x:%x,period=%d\n",
	mode, (time >> 16) & 0xff, (time >> 8) & 0xff, (time & 0xff), period);
      break;
   }
   len_out = strlen(msg_out);     
  }
 
  while ((bytes_read < nbytes) && (*offset < len_out)) {

    put_user( msg_out[ *offset ], &outb[bytes_read]);
    *offset = *offset + 1;
    bytes_read++; 
  }

  return bytes_read;
}

ssize_t seven_write (struct file * filep, const char __user * inpb, size_t nbytes, loff_t * offset)
{

  int i = 0;

  // line oriented input... read until we see a new-line.
  // handle that line, and then reset the msg_in / len_in buffers
  while ((len_in < 1023) && (*offset < 1023)  && (i < nbytes)) {
    char ch;
    uint32_t reg;

    get_user(ch, &inpb[i]);

    msg_in[len_in] = ch;
    msg_in[len_in +1] = '\0';

    //TODO refactor into a new function
    // detected the new-line, handle that line
    if (msg_in[len_in] == '\n') {

        msg_in[len_in] = 0x00;  // chomp off the new-line

	long ptr;
	uint32_t mode = readl(base) & 0x3;
	uint32_t value = simple_strtol(msg_in, NULL, 0); 

	switch(mode) {
	case 1:
		printk(KERN_INFO "Changing A/K values: %07x\n", value);
		reg = readl(base) & 0x1ffff;
		reg = reg | value << 17;
		writel(reg, base);
		break;	
	case 2:
		value = value & 0xffff;
		printk(KERN_INFO "Changing BCD values: %04x\n", value);
		writel(value & 0xffff, base+4);
		break;
	case 3:
		value = value & 0xffffff;
		printk(KERN_INFO "Changing time: %06x\n", value);
		writel(value, base+4);
		break;
	default:
		printk(KERN_INFO "Notice: mode %d unsupported write\n", mode);
		return -1;
	} // end case
 	len_in = 0; // restart processing input
   } // end if new line
   else {
	len_in++;
   } 
   *offset = *offset + 1;
   i = i + 1;  
  } // end while loop

  return i;
}


int seven_open (struct inode * inodep, struct file * filep)
{
   if (is_open == 1) {
     printk(KERN_INFO "Error - hello device already open\n");
     return -EBUSY;
   }
   is_open  = 1;
   try_module_get(THIS_MODULE);
   return EOK;
}


long seven_ioctl(struct file *filep, unsigned int ioctl_num, unsigned long ioctl_param)
{
  uint32_t mode;
 
   switch(ioctl_num) {
   case IOCTL_SET_MODE:
   
      switch(ioctl_param) {
        case 0:
	case 1:
	case 2:
	case 3:

	    mode = ioctl_param;
            writel(mode, base);
    	break;
        }

      mode = readl(base);
      printk(KERN_INFO "IOCTL set mode %d\n", mode);
      break;

    case IOCTL_SET_TMPD:
       writel(ioctl_param, base+8);
       printk(KERN_INFO "IOCTL set timepd %d\n", ioctl_param);
       break;

    case IOCTL_SET_COLPD:
       writel(ioctl_param, base+12);
       printk(KERN_INFO "IOCTL set colpd %d\n", ioctl_param);
       break;

    }

   return 0;
}

int seven_release (struct inode * inodep, struct file * filep)
{
  if (is_open == 0) {
    printk(KERN_INFO "Error - device wasn't opened\n");
    return -EBUSY;
  }
  is_open = 0;

  module_put(THIS_MODULE);
  return EOK;
}

struct file_operations fops = {
	.read = seven_read,
 	.write = seven_write,
	.open  = seven_open,
        .unlocked_ioctl = seven_ioctl,
	.release = seven_release
};

static int __init seven_start(void)
{
   int rc;
   
   printk(KERN_INFO "Seven segment driver\n");

   base = ioremap(BASE, SZ_4K);
 
   rc = register_chrdev(MAJOR_NUM, CHRDEV_NAME, &fops);


   return 0;
}

static void __exit seven_end(void)
{
  unregister_chrdev(MAJOR_NUM, CHRDEV_NAME);
}

module_init(seven_start);
module_exit(seven_end);

