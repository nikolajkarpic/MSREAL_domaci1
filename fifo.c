#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/slab.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("nikolajkarpic");
MODULE_DESCRIPTION("Fifo");

int fifo_buffer[16];
int position = 0;
int temp_vrednost = 1;

struct semaphore sem;

dev_t dev_id;
static struct class *dev_class;
static struct device *fifo_device;
static struct cdev *fifo_cdev;

DECLARE_WAIT_QUEUE_HEAD(readQueue);
DECLARE_WAIT_QUEUE_HEAD(writeQueue);

int file_open(struct inode *pinode, struct file *pfile);
int file_close(struct inode *pinode, struct file *pfile);
ssize_t fifo_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
ssize_t fifo_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);

struct file_operations fifo_ops = {
    .owner = THIS_MODULE,
    .open = file_open,
    .read = fifo_read,
    .write = fifo_write,
    .release = file_close,
};

static int __init fifo_init(void)
{

    sema_init(&sem, 1); // pokusaj odma semafore.

    if (alloc_chrdev_region(&dev_id, 0, 1, "fifo"))
    {
        printk(KERN_ERR "Ne moze da registruje device!\n");
        return -1;
    }
    printk(KERN_INFO "Character device number registrovan!\n");

    dev_class = class_create(THIS_MODULE, "fifo_class");
    if (dev_class == NULL)
    {
        printk(KERN_ERR "Ne moze da narpavi klasu!\n");
        goto fail_0;
    }
    printk(KERN_INFO "Klasa napravljena!\n");

    fifo_device = device_create(dev_class, NULL, dev_id, NULL, "fifo");
    if (fifo_device == NULL)
    {
        printk(KERN_ERR "Ne moze da napravi device!\n");
        goto fail_1;
    }
    printk(KERN_INFO "Device napravljen!\n");

    fifo_cdev = cdev_alloc();
    fifo_cdev->ops = &fifo_ops;
    fifo_cdev->owner = THIS_MODULE;
    if (cdev_add(fifo_cdev, dev_id, 1))
    {
        printk(KERN_ERR "Ne moze da doda cdev!\n");
        goto fail_2;
    }
    printk(KERN_INFO "Character device fifo dodan!\n");

    printk(KERN_INFO "Fifo ucitan!\n");

    return 0;

fail_0:
    unregister_chrdev_region(dev_id, 1);
fail_1:
    class_destroy(dev_class);
fail_2:
    device_destroy(dev_class, dev_id);

    return -1;
}

int file_open(struct inode *pinode, struct file *pfile)
{
    printk(KERN_INFO "Otvorio fajl\n");
    return 0;
}

int file_close(struct inode *pinode, struct file *pfile)
{
    printk(KERN_INFO "Zatvorio fajl\n");
    return 0;
}

ssize_t fifo_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
    long int len = 0;
    char output[100];
    int ret;
    int i;
    int j;
    output[0] = '\0';
    if (position > (temp_vrednost - 1))
    {

        position = position - temp_vrednost;
        for (i = 0; i < temp_vrednost; i++)
        {
            len = snprintf(output + strlen(output), 100, "%#04x ", fifo_buffer[0]);

            for (j = 0; j < 15; j++)
            {
                fifo_buffer[i] = fifo_buffer[i + 1];
            }
            fifo_buffer[15] = -1;
        }

        ret = copy_to_user(buffer, output, len * temp_vrednost);
        if (ret)
            return -EFAULT;
        printk(KERN_INFO "Citaj iz fifo!\n");
    }
    return len * temp_vrednost;
}