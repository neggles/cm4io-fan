/*
 * Driver for the Microchip/SMSC EMC181x Temperature monitor
 *
 * Copyright (C) 2018-2019 Traverse Technologies
 * Author: Mathew McBride <matt@traverse.com.au>
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>

/* High side only for now */
#define EMC181X_INTERNAL_TEMP 0x60
#define EMC181X_INTERNAL_TEMP_FRAC 0x61
#define EMC181X_EXTERNAL1_TEMP 0x62
#define EMC181X_EXTERNAL1_TEMP_LOW 0x63
#define EMC181X_EXTERNAL2_TEMP 0x65
#define EMC181X_EXTERNAL2_TEMP_LOW 0x66
#define EMC181X_EXTERNAL3_TEMP 0x66
#define EMC181X_EXTERNAL3_TEMP_LOW 0x67
#define EMC181X_EXTERNAL4_TEMP 0x68
#define EMC181X_EXTERNAL4_TEMP_FRAC 0x69

struct emc181x_data {
	struct i2c_client *i2c;
	bool is_apd; /* Anti-parallel diode mode, when enabled there are two
		diodes on each external channel */
};

#if 0
static ssize_t emc1704_show_value(struct device *dev,
				  struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	int value;
	int ret;

	ret = emc1704_get_value(dev_get_drvdata(dev), attr->index, &value);
	if (unlikely(ret < 0))
		return ret;

	return snprintf(buf, PAGE_SIZE, "%d\n", value);
}
#endif

static int emc181x_read(struct device *dev, enum hwmon_sensor_types type,
	u32 attr, int channel, long *val) {
	struct emc181x_data *data = dev_get_drvdata(dev);
	struct i2c_client *i2c = data->i2c;
	long temperature_val = 0;

	int8_t channel_reg = 0;
	int8_t channel_deg = 0;
	uint8_t channel_frac = 0;

	if (type != hwmon_temp) {
		return -EOPNOTSUPP;
	}
	if (channel > 4) {
		return -ENOTSUPP;
	}
	channel_reg = 0x60 + (channel * 0x02);
	printk(KERN_DEBUG "Reading channel %d register %X\n", channel, channel_reg);
	channel_deg = i2c_smbus_read_byte_data(i2c, channel_reg);

	channel_frac = i2c_smbus_read_byte_data(i2c, channel_reg + 0x01);
	channel_frac = channel_frac >> 5;

	printk(KERN_DEBUG "Got values %02X,%02X for channel %d\n", channel_deg, channel_frac, channel);

	temperature_val = channel_deg * 1000 + (channel_frac * 125);
	printk(KERN_DEBUG "Final temperature value: %ld\n", temperature_val);
	*val = temperature_val;

	return 0;
}

static u32 emc181x_chip_register_config[] = {
	HWMON_C_REGISTER_TZ,
	0
};

static const struct hwmon_channel_info emc181x_chip_register_info = {
	.type = hwmon_chip,
	.config = emc181x_chip_register_config,
};

static const u32 emc181x_temp_config[] = {
		HWMON_T_INPUT,
		HWMON_T_INPUT,
		HWMON_T_INPUT,
#if 0 /* TODO: EMC1814 support */
		HWMON_T_INPUT,
		HWMON_T_INPUT,
#endif
		0
};

static const struct hwmon_channel_info emc181x_temp_info = {
	.type = hwmon_temp,
	.config = emc181x_temp_config,
};

static const struct hwmon_channel_info *emc181x_info[] = {
	&emc181x_chip_register_info,
	&emc181x_temp_info,
	NULL
};

static umode_t emc181x_is_visible(const void *drvdata, enum hwmon_sensor_types type, u32 attr, int channel) {
	//const struct emc181x_data *data = drvdata;

	if (type != hwmon_temp)
		return 0;

	switch(attr) {
		case hwmon_temp_input:
		#if 0 /* TODO: EMC1814 supoort */
			/* Hide channel 2 and 4 if not in anti-paralllel diode mode */
			if (data->is_apd == 0 && (channel == 2 || channel == 4))
				return 0;
		#endif
			return S_IRUGO;
		default:
			return 0;
	}
}

static const struct hwmon_channel_info emc181x_temp = {
	.type = hwmon_temp,
	.config = emc181x_temp_config,
};


static const struct hwmon_ops emc181x_ops = {
	.is_visible = emc181x_is_visible,
	.read = emc181x_read,
};

static const struct hwmon_chip_info emc181x_chip_info = {
	.ops = &emc181x_ops,
	.info = emc181x_info,
};

static int emc181x_i2c_probe (struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	//int ret;
	struct device *hwmon_dev;
	struct device_node *of_node = i2c->dev.of_node;
	struct emc181x_data *data;
	int8_t regval;

	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_SMBUS_BYTE_DATA |
				     I2C_FUNC_SMBUS_WORD_DATA))
		return -ENODEV;

	data = devm_kzalloc(&i2c->dev, sizeof(struct emc181x_data), GFP_KERNEL);
	if (unlikely(!data))
		return -ENODEV;

	data->i2c = i2c;
	if (of_node) {
		data->is_apd = of_property_read_bool(of_node, "emc181x,apd");
		/* By default, APD is enabled in the EMC181X, if disabled we
			need to set this in the CONFIG register */
		pr_debug("EMC181X is_apd: %d\n", data->is_apd);
		if (!data->is_apd) {
			regval = i2c_smbus_read_byte_data(i2c, 0x03);
			regval &= ~(0xFE);
			regval = regval | 0x01;
			pr_debug("EMC181X not apd, setting CONFIG to %d\n", regval);
			i2c_smbus_write_byte_data(i2c, 0x03, regval);
		}
	}

	hwmon_dev = devm_hwmon_device_register_with_info(&i2c->dev,
		i2c->name,
		data,
		&emc181x_chip_info,
		NULL
	);

	return PTR_ERR_OR_ZERO(hwmon_dev);
}

static const struct i2c_device_id emc181x_i2c_id[] = {
	{ "emc1812", 0 },
	{ "emc1813", 0 },
	{ "emc1814", 0 },
	{ "emc1815", 0 },
	{ "emc1833", 0 },
	{}
};

MODULE_DEVICE_TABLE(i2c, emc181x_i2c_id);

static struct i2c_driver emc181x_i2c_driver = {
	.driver = {
		.name = "emc181x",
	},
	.probe    = emc181x_i2c_probe,
	.id_table = emc181x_i2c_id,
};

module_i2c_driver(emc181x_i2c_driver);

MODULE_DESCRIPTION("EMC181X Sensor Driver");
MODULE_AUTHOR("Mathew McBride <matt@traverse.com.au>");
MODULE_LICENSE("GPL v2");
