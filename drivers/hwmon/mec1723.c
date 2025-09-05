// Citations - Authorship

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

/* Indexes for the sysfs hooks */
/* Whatever that means */

// temp[nr][] categories
#define INPUT		0
#define MIN			1
#define MAX			2
#define CONTROL		3
#define OFFSET		3
#define AUTOMIN		4
#define THERM		5
#define HYSTERSIS	6

/* MEC1723 Settings */
#define MEC1723_TEMP_COUNT	2

// temp[][index] categories
#define AMBIENT			0
#define CPU				1

#define MEC1723_ADDR 0x2d

static const unsigned short i2c_amb_temp[] = { MEC1723_ADDR, 0x02, 0x74, 0x8c};
static const unsigned short i2c_cpu_temp[] = { MEC1723_ADDR, 0x02, 0x76, 0x8a};

// I don't think this is needed so long as we only use one address 0x2d
// I2C_CLIENT_END is an "Internal numbers to terminate lists"
static const unsigned short normal_i2c[] = { 0x2d, I2C_CLIENT_END };

enum chips { mec1723 };

// What is this doing?
static const struct i2c_device_id mec1723_id[] = {
	{"mec1723", mec1723},
	{ }
};
MODULE_DEVICE_TABLE(i2c, mec1723_id);

static const struct of_device_id __maybe_unused mec1723_of_match[] = {
	{
		.compatible = "microchip,mec1723", 
		.data = (void*)mec1723
	},
	{ },
};
MODULE_DEVICE_TABLE(of, mec1723_of_match);

struct mec1723_data {
	struct i2c_client	*client;
	struct mutex		lock;

	unsigned long measure_updated; /* In jiffies */
	bool valid;
	// temp[nr][index], no idea what index is for
	// nr = {INPUT = 0}
	// index = {AMBIENT = 0, CPU = 1}
	u16 temp[1][2]; 

	const struct attribute_group *groups[1]; // just temp group?
};

static struct i2c_driver mec1723_driver;
static struct mec1723_data *mec1723_update_device(struct device *dev);

static ssize_t temp_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	printk("dt_i2c - Now I am in the temp show function!\n");
	struct mec1723_data *data = mec1723_update_device(dev);
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int out;

	if (IS_ERR(data))
		return PTR_ERR(data);

	switch (sattr->nr) {
	case AMBIENT:
		// I think this is cased so that different conversion schemes can be
		// used for different sensors, not applicable in these cases.
		mutex_lock(&data->lock);
		out = data->temp[sattr->nr][sattr->index];
		mutex_unlock(&data->lock);
		break;

	case CPU:
		// Same as above
		mutex_lock(&data->lock);
		out = data->temp[sattr->nr][sattr->index];
		mutex_unlock(&data->lock);
		break;

	default:
		/* If this line is reached, something is wrong */
		out = -1;
		break;
	}

	return sprintf(buf, "%d\n", out);
}

static ssize_t temp_store(struct device *dev, struct device_attribute *attr,
			  const char *buf, size_t count)
{
	printk("dt_i2c - Now I am in the temp store function!\n");
	sprintf(buf, "Now I am in the temp store function!\n");
	// This entire function seems dedicated to writing a min/max setting
	// to the sensor, unneeded in our application
	return count;
}

// store and show functions for all exlcuded attributes removed

// Helper for facilitation amb temp get function
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

// Helper for facilitation cpu temp get function
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

// SENSOR_DEVICE_ATTR_2_RO(name, func, nr, index)
static SENSOR_DEVICE_ATTR_2_RO(amb_temp, temp, INPUT, 0);
static SENSOR_DEVICE_ATTR_2_RO(cpu_temp, temp, INPUT, 1);

// Format: &sensor_dev_attr_{name}.dev_attr.attr,
static struct attribute *mec1723_attrs[] = {
	&sensor_dev_attr_amb_temp.dev_attr.attr,
	&sensor_dev_attr_cpu_temp.dev_attr.attr,
	NULL
};

// The ADT example had some attributes in an all-encompassing table (above)
// and some split up based on sensor (showing 3-4 sub categories)
// I have no sub categories, and they can go in the above regardless, so I
// am assumming subtables are not needed.

// The others are not needed, they are separated because the different chip versions
// contain different hardware and to remove certain sensors the lists must be
// setup separately

static const struct attribute_group mec1723_attr_group = { .attrs = mec1723_attrs };

static int mec1723_detect(struct i2c_client *client,
			  struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int vendid, devid, devid2;
	const char *name;

	// Looks like an error checker, removing for now
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	// We only have one valid target, not going to bother checking for now
	name = "mec1723";

	strscpy(info->type, name, I2C_NAME_SIZE);

	return 0;
}

// We have no limits, skipping
// static int mec1723_update_limits(struct i2c_client *client)

// Skipping the following
// static int load_config3
// static int load_config4
// static int load_config
// set_property_bit
// static int load_attenuators
// static int adt7475_set_pwm_polarity

/**
 * @brief This function is called on loading the driver 
 */
static int mec1723_probe(struct i2c_client *client) {

	enum chips chip;
	static const char * const names[] = {
		[mec1723] = "MEC1723",
	};

	struct mec1723_data *data;
	struct device *hwmon_dev;
	int i, ret = 0, revision, group_num = 0;
	const struct i2c_device_id *id = i2c_match_id(mec1723_id, client);

	// From below here I've mostly just copy pasted so long as an
	// unneeded config is not used
	data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

	mutex_init(&data->lock);
	data->client = client;
	i2c_set_clientdata(client, data);

	if (client->dev.of_node)
		chip = (uintptr_t)of_device_get_match_data(&client->dev);
	else
		chip = id->driver_data;

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
		.name = "mec1723",
		.of_match_table = of_match_ptr(mec1723_of_match),
	},
	.probe = mec1723_probe,
	.id_table = mec1723_id,
	.detect		= mec1723_detect,
	.address_list	= normal_i2c,
};


static int mec1723_update_measure(struct device *dev)
{
	struct mec1723_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int i;
	int ret;

	for (i = 0; i < MEC1723_TEMP_COUNT; i++) {
		switch (i) {
			case AMBIENT:
				ret = mec1723_amb_temp(client);
				if (ret > 0xFFFF) return -1;
				break;
			case CPU:
				ret = mec1723_cpu_temp(client);
				if (ret > 0xFFFF) return -1;
				break
			data->temp[INPUT][i] = (u16)ret;
		}
	}

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
MODULE_AUTHOR("Stephen Pupa <spupa@lenovo.com>");
MODULE_AUTHOR("Advanced Micro Devices, Inc");
MODULE_DESCRIPTION("Slave Driver for MCHP MEC1723");
//MODULE_AUTHOR("Johannes 4 GNU/Linux");
MODULE_LICENSE("GPL");
//MODULE_VERSION(DRV_VERSION);