// Citations - Authorship
// 
// Derived from dt_i2c.c
// *Author: Johannes 4GNU_Linux

// Includes
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/types.h>

// Meta Information
MODULE_DESCRIPTION("Slave Driver for MCHP MEC1723");
MODULE_AUTHOR("Johannes 4 GNU/Linux");
MODULE_AUTHOR("Stephen Pupa <spupa@lenovo.com>");
MODULE_LICENSE("GPL");

static struct i2c_client *mec1723_client;

/* Declarations for probe and remove functions */
static int mec1723_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int mec1723_remove(struct i2c_client *client);
static int mec1723_command(struct i2c_client *client, unsigned int cmd, void *arg);

static struct of_device_id mec1723_driver_ids[] = {
	{
		.compatible = "microchip,lenovo-mec1723-i2c",
	}
};

// What is this doing?
static struct i2c_device_id lenovo_mec1723_i2c[] = {
	{"lenovo_mec1723_i2c", 0},
	{ },
};

// What does this function do? Is this adding the compatible string into metadata? Can this be removed?
MODULE_DEVICE_TABLE(of, mec1723_driver_ids);

static struct i2c_driver mec1723_driver = {
	.probe = mec1723_probe,
	.remove = mec1723_remove,
	.command = mec1723_command,
	.id_table = lenovo_mec1723_i2c,
	.driver = {
		.name = "lenovo_mec1723_i2c",
		.of_match_table = mec1723_driver_ids,
	},
};

// Will I need a proc file if i setup a hwmon and use mec1723_command?
static struct proc_dir_entry *proc_file;

/**
 * @brief Update timing between to ADC reads
 */
static ssize_t mec1723_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
	long temp;
	if(0 == kstrtol(user_buffer, 0, &temp)) 
		i2c_smbus_write_byte(mec1723_client, (u8)temp);
	return count;
}

/**
 * @brief Read ADC value
 */
static ssize_t mec1723_read(struct file *File, char *user_buffer, size_t count, loff_t *offs) {
	u8 result;
	result = i2c_smbus_read_byte(mec1723_client);
	return sprintf(user_buffer, "%d\n", result);
}

static struct proc_ops fops = {
	.proc_write = mec1723_write,
	.proc_read = mec1723_read,
};

/**
 * @brief This function is called on loading the driver 
 */
static int mec1723_probe(struct i2c_client *client, const struct i2c_device_id *id) {
	printk("dt_i2c - Now I am in the Probe function! 1\n");
	pr_err("dt_i2c - Now I am in the Probe function! 2\n");

	if(client->addr != 0x2d) {
		printk("dt_i2c - I2C Address must be = 0x2d\n");
		return -1;
	}

	mec1723_client = client;
		
	/* Creating procfs file */
	proc_file = proc_create("lenovo-mec1723-i2c", 0666, NULL, &fops);
	if(proc_file == NULL) {
		printk("dt_i2c - Error creating /proc/lenovo-mec1723-i2c 1\n");
		pr_err("dt_i2c - Error creating /proc/lenovo-mec1723-i2c 2\n");
		return -1; // was return -ENOMEM but that is undefined?
	}

	return 0;
}

/**
 * @brief This function is called on unloading the driver 
 */
static int mec1723_command(struct i2c_client *client, unsigned int cmd, void *arg) {
	printk("dt_i2c - Now I am in the Command function!\n");
	u8 ret_cnt = 0;
	switch (cmd){
		case 0x74: // Amb
			ret_cnt = 5;
			i2c_smbus_write_byte(mec1723_client, 0x74);
			i2c_smbus_write_byte(mec1723_client, 0x8c);
			break;
		case 0x76: // CPU
			ret_cnt = 5;
			i2c_smbus_write_byte(mec1723_client, 0x76);
			i2c_smbus_write_byte(mec1723_client, 0x8a);
			break;
		case 0x79: // GPU
			ret_cnt = 5;
			i2c_smbus_write_byte(mec1723_client, 0x79);
			i2c_smbus_write_byte(mec1723_client, 0x87);
			break;
		default:
			printk("Cmd invalid\n");
			return -1;
			break;
	}
	while (ret_cnt > 0) {
		u8 result;
		result = i2c_smbus_read_byte(mec1723_client);
		printk("{%x}",result);
	}
	printk("\n");
	return 0;
}

/**
 * @brief This function is called on unloading the driver 
 */
static int mec1723_remove(struct i2c_client *client) {
	printk("dt_i2c - Now I am in the Remove function!\n");
	proc_remove(proc_file);
	return 0;
}

/* This will create the init and exit function automatically */
module_i2c_driver(mec1723_driver);
