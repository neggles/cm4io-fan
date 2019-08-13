/*
 * Driver for the Microchip/SMSC EMC1704 Temperature and Voltage monitor
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
#define EMC1704_INTERNAL_TEMP 0x38
#define EMC1704_INTERNAL_TEMP_LOW 0x39
#define EMC1704_EXTERNAL1_TEMP 0x3A
#define EMC1704_EXTERNAL1_TEMP_LOW 0x3A
#define EMC1704_EXTERNAL2_TEMP 0x3C
#define EMC1704_EXTERNAL2_TEMP_LOW 0x3D
#define EMC1704_EXTERNAL3_TEMP 0x3E
#define EMC1704_EXTERNAL3_TEMP_LOW 0x3F

#define EMC1704_SOURCE_VOLT_REGISTER 0x58
#define EMC1704_SENSE_VOLT_REGISTER 0x55

/* As we can't do floating point, use the 'human' interpretation values given
 * in the EMC1704 datasheet, in uV (1e6).
 * The correct formula is Volts = 23.9883 * (Vsource/4094)
 */
static const int32_t emc1704_source_weights[] = {
	0,0,0,0,0,11700,23400,46900,93800,187500,375000,750000,1500000,3000000,6000000,12000000
};


/* Return the converted value from the given register in uV or mC */
static int emc1704_get_value(struct i2c_client *i2c, u8 reg, int *result)
{
	int mval,tempval,i;

	int8_t highval;
	uint8_t lowval;

	#ifdef DEBUG
	printk(KERN_INFO "Reading register %X\n",reg);
	#endif

	highval = i2c_smbus_read_byte_data(i2c, reg);
	#ifdef DEBUG
	printk(KERN_INFO "Got value %d\n",highval);
	#endif

	lowval = i2c_smbus_read_byte_data(i2c,reg+1);
	#ifdef DEBUG
	printk(KERN_INFO "Got low value %d\n",lowval);
	#endif

	if (reg >= EMC1704_INTERNAL_TEMP && reg <= EMC1704_EXTERNAL3_TEMP) {
		/* The low bits have a weight of .125C */
		lowval = lowval >> 5;
		mval = (highval * 1000) + (lowval * 125);
		#ifdef DEBUG
		printk(KERN_INFO "Calculated value %d\n",mval);
		#endif
		*result = mval;
	} else if (reg == EMC1704_SOURCE_VOLT_REGISTER) {
		tempval = (highval << 8) | lowval;
		mval = 0;
		for(i=15; i>0; i--) {
		if ((tempval & BIT(i)) == BIT(i)) {
			mval += emc1704_source_weights[i];
			#if DEBUG
			printk(KERN_INFO "Bit %d Value now %d\n",i,mval);
			#endif
		}
		*result = mval;
		}
	} else if (reg == EMC1704_SENSE_VOLT_REGISTER) {
		tempval = ((highval & 0x7F) >> 4) | (lowval >> 4);
		/* We masked out the sign bit above */
		if (highval < 0) {
			tempval = 0-tempval;
		}
		*result = tempval;
	} else {
		*result = -0;
		return -EINVAL;
	}

	return 0;
}


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

static SENSOR_DEVICE_ATTR(internal_temp, S_IRUGO, emc1704_show_value, NULL,
			  EMC1704_INTERNAL_TEMP);
static SENSOR_DEVICE_ATTR(external1_temp, S_IRUGO, emc1704_show_value, NULL,
			  EMC1704_EXTERNAL1_TEMP);
static SENSOR_DEVICE_ATTR(external2_temp, S_IRUGO, emc1704_show_value, NULL,
				EMC1704_EXTERNAL2_TEMP);
static SENSOR_DEVICE_ATTR(external3_temp, S_IRUGO, emc1704_show_value, NULL,
				EMC1704_EXTERNAL3_TEMP);
static SENSOR_DEVICE_ATTR(source_voltage, S_IRUGO, emc1704_show_value, NULL,
			  EMC1704_SOURCE_VOLT_REGISTER);
static SENSOR_DEVICE_ATTR(sense_voltage, S_IRUGO, emc1704_show_value, NULL,
			  EMC1704_SENSE_VOLT_REGISTER);

static struct attribute *emc1704_attrs[] = {
	&sensor_dev_attr_internal_temp.dev_attr.attr,
	&sensor_dev_attr_external1_temp.dev_attr.attr,
	&sensor_dev_attr_external2_temp.dev_attr.attr,
	&sensor_dev_attr_external3_temp.dev_attr.attr,
	&sensor_dev_attr_source_voltage.dev_attr.attr,
	&sensor_dev_attr_sense_voltage.dev_attr.attr,
	NULL,
};
ATTRIBUTE_GROUPS(emc1704);

static int emc1704_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	//int ret;
	struct device *hwmon_dev;

	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_SMBUS_BYTE_DATA |
				     I2C_FUNC_SMBUS_WORD_DATA))
		return -ENODEV;

	hwmon_dev = devm_hwmon_device_register_with_groups(&i2c->dev,
							   i2c->name,
							   i2c,
							   emc1704_groups);

	return PTR_ERR_OR_ZERO(hwmon_dev);
}

static const struct i2c_device_id emc1704_i2c_id[] = {
	{ "emc1704", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, emc1704_i2c_id);

static struct i2c_driver emc17xx_i2c_driver = {
	.driver = {
		.name = "emc1704",
	},
	.probe    = emc1704_i2c_probe,
	.id_table = emc1704_i2c_id,
};

module_i2c_driver(emc17xx_i2c_driver);

MODULE_DESCRIPTION("EMC1704 Sensor Driver");
MODULE_AUTHOR("Mathew McBride <matt@traverse.com.au>");
MODULE_LICENSE("GPL v2");
