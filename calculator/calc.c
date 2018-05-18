#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include <linux/string.h> /* for memset() and memcpy() */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artur Fogiel");
MODULE_DESCRIPTION("A simple Linux char driver that act as calcualtor");
MODULE_VERSION("0.1");

#define  DEVICE_NAME "calcchar"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "calc"        ///< The device class -- this is a character device driver
#define  BUFF_SIZE 256
static int    majorNumber;                  ///< Stores the device number -- determined automatically
static char   message[BUFF_SIZE] = {0};           ///< Memory for the string that is passed from userspace
static short  size_of_message;              ///< Used to remember the size of the string stored
static struct class*  ebbcharClass  = NULL; ///< The device-driver class struct pointer
static struct device* ebbcharDevice = NULL; ///< The device-driver device struct pointer
static char*  msg_Ptr = NULL;
static int    result = 0;

typedef enum OPERATIONS_E {
    ADD = 0,
    SUB,
    MULT,
    DIV,
    POW,
    NONE
} OPERATIONS;

// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

void prepare_calc(void);
void calc(int first, int second, OPERATIONS op);

static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};

static int dev_open(struct inode *inodep, struct file *filep){
   msg_Ptr = message;
   return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    int error_count = 0;

    if(*msg_Ptr == 0) {
        return 0;
    }

    memset(message, 0, sizeof(char) * BUFF_SIZE);

    snprintf(message, 64, "%d\n", result); 
    size_of_message = strlen(message); 
    
    error_count = copy_to_user(buffer, message, size_of_message); 
    if (error_count == 0) {
        printk(KERN_INFO "CALC: size: %d result: %d\n", size_of_message, result);
      while (len && *msg_Ptr) {
        error_count = put_user(*(msg_Ptr++), buffer++);
        len--;
      }
      printk(KERN_INFO "CALC: error_count %d\n", error_count);
      if(error_count == 0) {
         return (size_of_message);
      } else {
          return -EFAULT;
      }
   }
   else {
      printk(KERN_INFO "CALC: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;
   }
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
   size_of_message = 0;
   memset(message, 0, sizeof(char) * BUFF_SIZE);

   snprintf(message, BUFF_SIZE, "%s", buffer);  
   size_of_message = strlen(message);              
   printk(KERN_INFO "CALC: Received %d -> %s\n", size_of_message, message);

   if (size_of_message >= 3) {
       prepare_calc();
   }
   return len;
}

void calc(int first, int second, OPERATIONS op) {
    switch(op) {
        case ADD: {
            result = first + second;
            break;
        }
        case SUB: {
            result = first - second;
            break;
        }
        case MULT: {
            result = first * second;
            break;
        }
        case DIV: {
            result = first / second;
            break;
        }
        case POW: {
            result = first ^ second;
            break;
        }
        default:
            result = 0;
            break;
    }
}

void handle_case(int* first_num_stop, int* sec_num_start, int i) {
    if(first_num_stop && *first_num_stop == 0) {
        *first_num_stop = i - 1;
    }

    if(sec_num_start && *sec_num_start == 0) {
        *sec_num_start = i + 1;
    }
}

void prepare_calc(void) {
    int i = 0;
    int first_num_stop = 0;
    int sec_num_start = 0;
    int op_idx = 0;
    OPERATIONS op = NONE;

    for(i = 0; i < BUFF_SIZE; i++) {
        if (message[i] == '\0') {
            break;
        }

        switch(message[i]) {
            case '*': {
                handle_case(&first_num_stop, &sec_num_start, i);
                op_idx = i;
                op = MULT;
                break;
            }
            case '/': {
                handle_case(&first_num_stop, &sec_num_start, i);
                op_idx = i;
                op = DIV;
                break;
            }
            case '+': {
                handle_case(&first_num_stop, &sec_num_start, i);
                op_idx = i;
                op = ADD;
                break;
            }
            case '-': {
                handle_case(&first_num_stop, &sec_num_start, i);
                op_idx = i;
                op = SUB;
                break;
            }
            case '^': {
                handle_case(&first_num_stop, &sec_num_start, i);
                op_idx = i;
                op = POW;
                break;
            }
            case ' ': {
                handle_case(&first_num_stop, &sec_num_start, i);
                op_idx = i;
                break;
            }
            default:
                break;
        }
    }

    if(sec_num_start > 0 && op_idx > 0) {
        char** endp = NULL;
        int first_num = simple_strtol(message, endp, 10);
        int sec_num = simple_strtol((message + sec_num_start), endp, 10);

        printk(KERN_INFO "input is valid !\n");

        calc(first_num, sec_num, op);

        printk(KERN_INFO "first num: %d\n", first_num);
        printk(KERN_INFO "sec num: %d\n", sec_num);
    }
}

static int dev_release(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "Calc: Device successfully closed\n");
   return 0;
}

static int __init calc_init(void)
{
   printk(KERN_INFO "CALC: Initializing the CALC LKM\n");
 
   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "CALC failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "CALC: registered correctly with major number %d\n", majorNumber);
 
   // Register the device class
   ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(ebbcharClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(ebbcharClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "CALC: device class registered correctly\n");
 
   // Register the device driver
   ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(ebbcharDevice)){               // Clean up if there is an error
      class_destroy(ebbcharClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(ebbcharDevice);
   }
   printk(KERN_INFO "CALC: device class created correctly\n"); // Made it! device was initialized
   return 0;
}

static void __exit calc_exit(void)
{
   device_destroy(ebbcharClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(ebbcharClass);                          // unregister the device class
   class_destroy(ebbcharClass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "CALC: Goodbye from the LKM!\n");
}

module_init(calc_init);
module_exit(calc_exit);
