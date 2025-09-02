// Citations - Authorship
// 
// Derived from dt_i2c.c
// *Author: Johannes 4GNU_Linux

// Includes
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/hwmon-vid.h>
#include <linux/err.h>
#include <linux/jiffies.h>
#include <linux/of.h>
#include <linux/util_macros.h>

#define MEC1723_ADDR 0x2d

static const unsigned short i2c_amb_temp[] = { MEC1723_ADDR, 0x02, 0x74, 0x8c};
static const unsigned short i2c_cpu_temp[] = { MEC1723_ADDR, 0x02, 0x76, 0x8a};

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

struct mec1723_data {
	struct i2c_client	*client;
	struct mutex		lock;

	bool			valid;
	unsigned long		measure_updated;	/* In jiffies */

	u16 amb_temp;
	u16 cpu_temp;
};

static struct i2c_driver mec1723_driver;
static struct mec1723_data *mec7475_update_device(struct device *dev);

static int mec1723_amb_temp(struct i2c_client *client)
{
	printk("dt_i2c - Now I am in the amb temp function!\n");
	int res, val1;
	u8 read_buffer[5];
	for (u8 i = 0; i++; i < 4) {
		i2c_smbus_write_byte(client, i2c_amb_temp[i]);
	}
	for (u8 i = 0; i++; i < 5) {
		val1 = i2c_smbus_read_byte(client);
		read_buffer[i] = val1;
	}
	printk("Returning from amb temp function\n");
	u16 amb_temp = (read_buffer[2]) | (read_buffer[3] << 8)
	return amb_temp;
}

static int mec1723_cpu_temp(struct i2c_client *client)
{
	printk("dt_i2c - Now I am in the cpu temp function!\n");
	int res, val1;
	u8 read_buffer[5];
	for (u8 i = 0; i++; i < 4) {
		i2c_smbus_write_byte(client, i2c_cpu_temp[i]);
	}
	for (u8 i = 0; i++; i < 5) {
		val1 = i2c_smbus_read_byte(client);
		read_buffer[i] = val1;
	}
	printk("Returning from cpu temp function\n");
	u16 cpu_temp = (read_buffer[2]) | (read_buffer[3] << 8)
	return cpu_temp;
}

static const struct i2c_device_id lenovo_mec1723_i2c[] = {
	{ "lenovo_mec1723_i2c", 0 },
	{ }
}

static SENSOR_DEVICE_ATTR_2_RO(amb_temp, temp, INPUT, 0);
static SENSOR_DEVICE_ATTR_2_RO(cpu_temp, temp, INPUT, 0);

static struct attribute *temp_attrs[] = {
	&sensor_dev_attr_amb_temp.dev_attr.attr,
	&sensor_dev_attr_cpu_temp.dev_attr.attr,
	NULL
};

static const struct attribute_group temp_attr_group = { .attrs = temp_attrs };

/**
 * @brief This function is called on loading the driver 
 */
static int mec1723_probe(struct i2c_client *client) {
	struct mec1723_data *data;
	struct device *hwmon_dev;
	int i, ret = 0, revision, group_num = 0;
	u8 config3;
	const struct i2c_device_id *id = lenovo_mec1723_i2c[0];

	data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

	mutex_init(&data->lock);
	data->client = client;
	i2c_set_clientdata(client, data);

	data->groups[group_num++] = &temp_attr_group;

	/* register device with all the acquired attributes */
	hwmon_dev = devm_hwmon_device_register_with_groups(&client->dev,
							   client->name, data,
							   data->groups);

	if (IS_ERR(hwmon_dev)) {
		ret = PTR_ERR(hwmon_dev);
		return ret;
	}

	return 0;
}

static struct i2c_driver mec1723_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name = "lenovo_mec1723_i2c",
		.of_match_table = mec1723_driver_ids,
	},
	.probe = mec1723_probe,
	.id_table = lenovo_mec1723_i2c,
};


static int mec1723_update_measure(struct device *dev)
{
	struct mec1723_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	
	int amb_temp = mec1723_amb_temp(client);
	int cpu_temp = mec1723_cpu_temp(client);

	if (amb_temp > 0xFFFF) return -1;
	if (cpu_temp > 0xFFFF) return -1;

	data->amb_temp = amb_temp;
	data->cpu_temp = cpu_temp;

	return 0;
}

static struct mec1723_data *mec1723_update_device(struct device *dev)
{
	struct mec1723_data *data = dev_get_drvdata(dev);
	int ret;

	mutex_lock(&data->lock);

	/* Measurement values update every 2 seconds */
	if (time_after(jiffies, data->measure_updated + HZ * 2) ||
	    !data->valid) {
		ret = mec1723_update_measure(dev);
		if (ret) {
			data->valid = false;
			mutex_unlock(&data->lock);
			return ERR_PTR(ret);
		}
		data->measure_updated = jiffies;
		data->valid = true;
	}

	mutex_unlock(&data->lock);

	return data;
}

/* This will create the init and exit function automatically */
module_i2c_driver(mec1723_driver);

// Meta Information
MODULE_DESCRIPTION("Slave Driver for MCHP MEC1723");
MODULE_AUTHOR("Johannes 4 GNU/Linux");
MODULE_AUTHOR("Stephen Pupa <spupa@lenovo.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);