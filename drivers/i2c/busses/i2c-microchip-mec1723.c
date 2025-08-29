// Citations - Authorship
// 
// Derived from dt_i2c.c
// *Author: Johannes 4GNU_Linux

// Includes
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/slab.h>

struct mec1723_data {
	struct i2c_client	*client;
	struct mutex		lock;
};

static void mec1723_init_client(struct i2c_client *client)
{
	// No initialization needed for MEC1723
	printk("dt_i2c - Now I am in the Init function!\n");
	return;
}

static const struct i2c_device_id lenovo_mec1723_i2c[]

/**
 * @brief This function is called on loading the driver 
 */
static int mec1723_probe(struct i2c_client *client) {
	printk("dt_i2c - Now I am in the Probe function!\n");
	struct device *dev = &client->dev;
	struct i2c_adapter *adapter = client->adapter;
	struct mec1723_data *data;
	struct device *hwmon_dev;

	/* Initialize the AD7418 chip */
	mec1723_init_client(client);

	hwmon_dev = devm_hwmon_device_register_with_groups(dev,
							   client->name,
							   data, attr_groups);
	return PTR_ERR_OR_ZERO(hwmon_dev);
}

// What is this doing?
static const struct i2c_device_id lenovo_mec1723_i2c[] = {
	{"lenovo_mec1723_i2c", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, lenovo_mec1723_i2c);

static const struct of_device_id mec1723_driver_ids[] = {
	{.compatible = "microchip,lenovo-mec1723-i2c", .data = (void*)0},
	{ }
};
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

/* This will create the init and exit function automatically */
module_i2c_driver(mec1723_driver);

// Meta Information
MODULE_DESCRIPTION("Slave Driver for MCHP MEC1723");
MODULE_AUTHOR("Johannes 4 GNU/Linux");
MODULE_AUTHOR("Stephen Pupa <spupa@lenovo.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);