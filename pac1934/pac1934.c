/*
 * Hwmon driver for the Microchip/SMSC PAC1934 Voltage/Current monitor
 *
 * Copyright (C) 2018 Traverse Technologies
 * Author: Mathew McBride <matt@traverse.com.au>
 *
 * Note: Microchip has posted an IIO subsystem driver on the website, that 
 * might be better suited to some applications
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define PAC1934_REFRESH_REGISTER 0x00

/* LSB step size for unipolar voltages is 488uV */
#define PAC1934_VBUS0_REGISTER 0x07
#define PAC1934_VBUS1_REGISTER 0x08
#define PAC1934_VBUS2_REGISTER 0x09
#define PAC1934_VBUS3_REGISTER 0x0A

#define PAC1934_VBUS0_AVG_REGISTER 0x0F
#define PAC1934_VBUS1_AVG_REGISTER 0x10
#define PAC1934_VBUS2_AVG_REGISTER 0x11
#define PAC1934_VBUS3_AVG_REGISTER 0x12

/* LSB step size for Vsense is 1.8uV */
#define PAC1934_VSENSE0_REGISTER 0x0B
#define PAC1934_VSENSE1_REGISTER 0x0C
#define PAC1934_VSENSE2_REGISTER 0x0D
#define PAC1934_VSENSE3_REGISTER 0x0E

#define PAC1934_VSENSE0_AVG_REGISTER 0x13
#define PAC1934_VSENSE1_AVG_REGISTER 0x14
#define PAC1934_VSENSE2_AVG_REGISTER 0x15
#define PAC1934_VSENSE3_AVG_REGISTER 0x16

struct pac1934_data {
	struct i2c_client *i2c;
	u32 shunt_resistor[4]; /* Shunt resistor value, in uOhms */
};

/* Return the converted value from the given register in uV or mC */
static int pac1934_get_value(struct pac1934_data *data, u8 reg, int *result)
{
	int val;
	struct i2c_client *i2c = data->i2c;
	u32 nvsense, shunt_res;
	u64 cur_reading;
	int bus = PAC1934_VSENSE0_REGISTER;

	val = i2c_smbus_read_word_swapped(i2c,reg);
	
	switch(reg) {
	case PAC1934_VBUS0_REGISTER:
	case PAC1934_VBUS1_REGISTER:
	case PAC1934_VBUS2_REGISTER:
	case PAC1934_VBUS3_REGISTER:
	case PAC1934_VBUS0_AVG_REGISTER:
	case PAC1934_VBUS1_AVG_REGISTER:
	case PAC1934_VBUS2_AVG_REGISTER:
	case PAC1934_VBUS3_AVG_REGISTER:
		*result = val * 488;
		break;
	case PAC1934_VSENSE0_REGISTER:
	case PAC1934_VSENSE1_REGISTER:
	case PAC1934_VSENSE2_REGISTER:
	case PAC1934_VSENSE3_REGISTER:
	case PAC1934_VSENSE0_AVG_REGISTER:
	case PAC1934_VSENSE1_AVG_REGISTER:
	case PAC1934_VSENSE2_AVG_REGISTER:
	case PAC1934_VSENSE3_AVG_REGISTER:
		/* The LSB value for Vsense is 1.5uV (0xFFFF ~ 100mV), to avoid floating point,
		 * make this into nV and multiply by 1500nV  */
		 #ifdef DEBUG
		 printk(KERN_INFO "Read value register %x=%d\n", reg, val);
		 #endif
		 nvsense = val * 1500;
		 if (reg >= PAC1934_VSENSE0_AVG_REGISTER) {
		 	 bus = reg - PAC1934_VSENSE0_AVG_REGISTER;
		 } else if (reg >= PAC1934_VSENSE0_REGISTER) {
		 	 bus = reg - PAC1934_VSENSE0_REGISTER;
		 }
		 /* If there is no shunt resistor (i.e only sensing voltage), stop here */
		 if (data->shunt_resistor[bus] == 0) {
		 	 *result = 0;
		 	 break;
		 }
		 shunt_res = data->shunt_resistor[bus]; /* nV / uOhms = mA */
		 cur_reading = (u64)nvsense / (u64)shunt_res;
		 *result = cur_reading;
		break;
	default:
		*result = 0;
		return -EINVAL;
	}

	return 0;
}


static ssize_t pac1934_show_value(struct device *dev,
				  struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct pac1934_data *data = dev_get_drvdata(dev);
	int value;
	int ret;

	/* Send a refresh command */
	i2c_smbus_write_byte_data(data->i2c, 0x00, 0x00);
	
	ret = pac1934_get_value(data, attr->index, &value);
	if (unlikely(ret < 0))
		return ret;

	return snprintf(buf, PAGE_SIZE, "%d\n", value);
}

static SENSOR_DEVICE_ATTR(vbus0, S_IRUGO, pac1934_show_value, NULL,
			  PAC1934_VBUS0_REGISTER);
static SENSOR_DEVICE_ATTR(vbus1, S_IRUGO, pac1934_show_value, NULL,
			  PAC1934_VBUS1_REGISTER);
static SENSOR_DEVICE_ATTR(vbus2, S_IRUGO, pac1934_show_value, NULL,
			  PAC1934_VBUS2_REGISTER);
static SENSOR_DEVICE_ATTR(vbus3, S_IRUGO, pac1934_show_value, NULL,
			  PAC1934_VBUS3_REGISTER);
static SENSOR_DEVICE_ATTR(vbus0avg, S_IRUGO, pac1934_show_value, NULL,
			  PAC1934_VBUS0_AVG_REGISTER);
static SENSOR_DEVICE_ATTR(vbus1avg, S_IRUGO, pac1934_show_value, NULL,
			  PAC1934_VBUS1_AVG_REGISTER);
static SENSOR_DEVICE_ATTR(vbus2avg, S_IRUGO, pac1934_show_value, NULL,
			  PAC1934_VBUS2_AVG_REGISTER);
static SENSOR_DEVICE_ATTR(vbus3avg, S_IRUGO, pac1934_show_value, NULL,
			  PAC1934_VBUS3_AVG_REGISTER);

static SENSOR_DEVICE_ATTR(cur0, S_IRUGO, pac1934_show_value, NULL,
			  PAC1934_VSENSE0_REGISTER);
static SENSOR_DEVICE_ATTR(cur1, S_IRUGO, pac1934_show_value, NULL,
			  PAC1934_VSENSE1_REGISTER);
static SENSOR_DEVICE_ATTR(cur2, S_IRUGO, pac1934_show_value, NULL,
			  PAC1934_VSENSE2_REGISTER);
static SENSOR_DEVICE_ATTR(cur3, S_IRUGO, pac1934_show_value, NULL,
			  PAC1934_VSENSE3_REGISTER);

static SENSOR_DEVICE_ATTR(cur0avg, S_IRUGO, pac1934_show_value, NULL,
			  PAC1934_VSENSE0_AVG_REGISTER);
static SENSOR_DEVICE_ATTR(cur1avg, S_IRUGO, pac1934_show_value, NULL,
			  PAC1934_VSENSE1_AVG_REGISTER);
static SENSOR_DEVICE_ATTR(cur2avg, S_IRUGO, pac1934_show_value, NULL,
			  PAC1934_VSENSE2_AVG_REGISTER);
static SENSOR_DEVICE_ATTR(cur3avg, S_IRUGO, pac1934_show_value, NULL,
			  PAC1934_VSENSE3_AVG_REGISTER);




static struct attribute *pac1934_attrs[] = {
	&sensor_dev_attr_vbus0.dev_attr.attr,
	&sensor_dev_attr_vbus1.dev_attr.attr,
	&sensor_dev_attr_vbus2.dev_attr.attr,
	&sensor_dev_attr_vbus3.dev_attr.attr,
	&sensor_dev_attr_vbus0avg.dev_attr.attr,
	&sensor_dev_attr_vbus1avg.dev_attr.attr,
	&sensor_dev_attr_vbus2avg.dev_attr.attr,
	&sensor_dev_attr_vbus3avg.dev_attr.attr,
	&sensor_dev_attr_cur0.dev_attr.attr,
	&sensor_dev_attr_cur1.dev_attr.attr,
	&sensor_dev_attr_cur2.dev_attr.attr,
	&sensor_dev_attr_cur3.dev_attr.attr,
	&sensor_dev_attr_cur0avg.dev_attr.attr,
	&sensor_dev_attr_cur1avg.dev_attr.attr,
	&sensor_dev_attr_cur2avg.dev_attr.attr,
	&sensor_dev_attr_cur3avg.dev_attr.attr,
	NULL,
};

ATTRIBUTE_GROUPS(pac1934);

static int pac1934_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	int ret;
	struct device *hwmon_dev;
	struct pac1934_data *data;
	struct device_node *of_node = i2c->dev.of_node;

	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_SMBUS_BYTE_DATA |
				     I2C_FUNC_SMBUS_WORD_DATA))
		return -ENODEV;

	data = devm_kzalloc(&i2c->dev, sizeof(struct pac1934_data), GFP_KERNEL);
	if (unlikely(!data))
		return -ENODEV;
	
	data->i2c = i2c;
	
	if (of_node) {
		ret = of_property_read_u32_array(of_node, "shunt-resistors", data->shunt_resistor, 4);
		if (ret < 0) {
			memset(data->shunt_resistor, 0, 4*sizeof(u32));
			printk(KERN_INFO "No shunt resistors specified\n");
		} 
		#ifdef DEBUG
		else {
			for (i=0; i<4; i++) {
				printk(KERN_INFO "Bus %d shunt resistor value %d uOhms\n", i,data->shunt_resistor[i]);
			}
		}
		#endif
	}
	hwmon_dev = devm_hwmon_device_register_with_groups(&i2c->dev,
							   i2c->name,
							   data,
							   pac1934_groups);

	return PTR_ERR_OR_ZERO(hwmon_dev);
}

static const struct i2c_device_id pac1934_i2c_id[] = {
	{ "pac1934", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, pac1934_i2c_id);

static struct i2c_driver pac1934_i2c_driver = {
	.driver = {
		.name = "pac1934",
	},
	.probe    = pac1934_i2c_probe,
	.id_table = pac1934_i2c_id,
};

module_i2c_driver(pac1934_i2c_driver);

MODULE_DESCRIPTION("PAC1934 Sensor Driver");
MODULE_AUTHOR("Mathew McBride <matt@traverse.com.au>");
MODULE_LICENSE("GPL v2");
