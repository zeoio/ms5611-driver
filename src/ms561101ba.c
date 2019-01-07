/*
 * @file ms561101ba.c
 * driver for the MS5611-01BA03 Barometric Pressure Sensor connected via I2C
 *
 * write by Sarlor, 01/19/2017
 *  */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define MS5611_ADDRESS 		 	0X77 	/* Slave Address */

#define CMD_ADC_READ			0x00	/* ADC Read 	 */
#define CMD_RST     	 		0x1E	/* Reset 	 */
#define CALI_DATA_START			0xA0	/* PROM Read 	 */

#define CALI_DATA_LEN			8	/* Length of PROM */
#define SENSOR_NAME			"ms561101ba"

#define MS5611_INIT_OSR(_cmd, _conv_usec, _rate)		\
		{ .cmd = _cmd, .conv_usec = _conv_usec, .rate = _rate  }

/*
*   OverSampling Rate descriptor.
*   Warning: cmd MUST be kept aligned on a word boundary
* */
struct ms5611_osr {
	unsigned long conv_usec;	/* The time takes for the conversion */
	u8 cmd;				/* Convert sent commands 	     */
	unsigned short rate;		/* The frequency of conversion 	     */
};

/* Each client has this additional data */
struct ms5611_data {
	u16 calibration[CALI_DATA_LEN];	/* Calivbration data from PROM */
	u32 raw_pressure;
	u32 raw_temperature;
	const struct ms5611_osr *temp_osr;
	const struct ms5611_osr *pressure_osr;
	struct i2c_client *ms5611_client;
	struct input_dev *input;
	struct mutex lock;
};

/* This OSR array is for pressure. */
static const struct ms5611_osr ms5611_avail_pressure_osr[] = {
	MS5611_INIT_OSR(0x40, 600,  256),
	MS5611_INIT_OSR(0x42, 1170, 512),
	MS5611_INIT_OSR(0x44, 2280, 1024),
	MS5611_INIT_OSR(0x46, 4540, 2048),
	MS5611_INIT_OSR(0x48, 9040, 4096)
};

/* This OSR array is for temperature. */
static const struct ms5611_osr ms5611_avail_temp_osr[] = {
	MS5611_INIT_OSR(0x50, 600,  256),
	MS5611_INIT_OSR(0x52, 1170, 512),
	MS5611_INIT_OSR(0x54, 2280, 1024),
	MS5611_INIT_OSR(0x56, 4540, 2048),
	MS5611_INIT_OSR(0x58, 9040, 4096)
};

/*
 * 4-bit CRC to check the data validity in array.
 * @prom: The name of the array being checked.
 * @len: The length of the array.
 *
 * Returning 1 on success else 0 on failure.
 *  */
static int ms5611_prom_is_valid(u16 *prom, size_t len)
{
	int i, j;
	uint16_t crc = 0, crc_orig = prom[7] & 0x000F;

	prom[7] &= 0xFF00;

	for (i=0; i<len*2; i++) {
		if (i % 2 == 1)
			crc ^= prom[i >> 1] & 0x00FF;
		else
			crc ^= prom[i >> 1] >> 8;

		for (j = 0; j < 8; j++) {
			if (crc & 0x8000)
			crc = (crc << 1) ^ 0x3000;
			else
			crc <<= 1;
		}
	}

	crc = (crc >> 12) & 0x000F;
	return crc_orig != 0x0000 && crc == crc_orig;
}

/*
 * Read a PROM memory with 128-bit of MS5611-01BA, and verify.
 * @client: Handle to slave device.
 *
 * A 4-bit CRC has been implemented to check the data validity in memory.
 * 16 bit reserved for manufacturer of CALI_DATA_START. The next 12 bytes
 * are the coefficient values, and a coefficient value of 2 bytes.
 * The fourth bit of the last two bytes is the CRC check value. Returning
 * negative errno else zero on success.
 *    */
static s32 ms5611_read_calibration_data(struct i2c_client *client)
{
	unsigned char i;
	int status;
	struct ms5611_data *data = i2c_get_clientdata(client);

	for (i=0; i<CALI_DATA_LEN; i++) {
		status = i2c_smbus_read_word_swapped(client,
				CALI_DATA_START+i*2);
		if (status < 0)
			return status;

		data->calibration[i] = (u16)(status & 0xffff);
	}

	if (!ms5611_prom_is_valid(data->calibration, CALI_DATA_LEN)) {
		printk(KERN_INFO "PROM integrity check failed\n");
		return -ENODEV;
	}

	return 0;
}

/*
 * Read the temperature or atmospheric pressure value.
 * @client: Handle to slave device.
 * @osr: Pointer to the osr structure.
 * @data: Stores the read data value.
 *
 * After the conversion command is sent, the value of the ADC can not be
 * read until the segment time has elapsed. Different sampling rates require
 * different periods of sleep. Returning negative errno else zero on success.
 *  */
static int ms5611_update_raw_data(struct i2c_client *client,
		const struct ms5611_osr *osr, u32 *data)
{
	unsigned char tmp[3] = {0};
	int status;

	status = i2c_smbus_write_byte(client, osr->cmd);
	if (status < 0) {
		dev_err(&client->dev, "Error while requesting measurement.\n");
		return status;
	}
	usleep_range(osr->conv_usec, osr->conv_usec + (osr->conv_usec / 10UL));

	status = i2c_smbus_read_i2c_block_data(client, CMD_ADC_READ, 3, tmp);
	if (status < 0) {
                printk(KERN_ERR "ERROR: i2c read adc\n");
		return status;
	}

	*data = (tmp[0] << 16) | (tmp[1] << 8) | tmp[2];
	return 0;
}

/*
 * Update the value of the sample rate.
 * @array: An array of pre-defined sample rate information.
 * @osr:   Save the array offset for the new specified sample rate.
 * @data:  The value of the stored sample rate.
 *
 * Because the temperature and atmospheric pressure sampling rate range
 * of the same value, so a common function. Returning negative errno
 * else zero on success.
 *  */
static int update_oversampling(const struct ms5611_osr *array,
		const struct ms5611_osr **osr, unsigned long data)
{
	switch (data) {
	case 256:
		*osr = &array[0];
		break;
	case 512:
		*osr = &array[1];
		break;
	case 1024:
		*osr = &array[2];
		break;
	case 2048:
		*osr = &array[3];
		break;
	case 4096:
		*osr = &array[4];
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
 * Displays the value of pressure sensitivity.
 *  */
static ssize_t ms5611_sens_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ms5611_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%u", data->calibration[1]);
}

/*
 * Displays the vaule of pressure offset.
 *  */
static ssize_t ms5611_off_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ms5611_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%u", data->calibration[2]);
}

/*
 * Displays the vaule of temperature coefficient of pressure sensitivity.
 *  */
static ssize_t ms5611_tcs_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ms5611_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%u", data->calibration[3]);
}

/*
 * Displays the value of temperature coefficient of pressure offset.
 *  */
static ssize_t ms5611_tco_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ms5611_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%u", data->calibration[4]);
}

/*
 * Displays the value of reference temperature.
 *  */
static ssize_t ms5611_tref_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ms5611_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%u", data->calibration[5]);
}

/*
 * Displays the value of temperature coefficient of the temperature.
 *  */
static ssize_t ms5611_tempsens_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ms5611_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%u", data->calibration[6]);
}

/*
 * Dispalys the value of temperature oversampling. The value of the sampling
 * rate defined in the 's5611_avail_temp_osr' of array, please direct the use
 * of assignment.
 *  */
static int ms5611_oversampling_temp_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ms5611_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%u", data->temp_osr->rate);
}

/*
 * Set the value of temperature oversampling. The value entered must be a
 * pre-specified valid value. Valid values are predefined in the
 * 's5611_avail_temp_osr' array.
 *  */
static int ms5611_oversampling_temp_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int err;
	unsigned long data;
	struct i2c_client *client = to_i2c_client(dev);
	struct ms5611_data *ms5611 = i2c_get_clientdata(client);

	err = strict_strtoul(buf, 10, &data);
	if (err)
		return err;

	update_oversampling(ms5611_avail_temp_osr, &ms5611->temp_osr, data);
	return count;
}

/*
 * Dispalys the value of pressure oversampling. The value of the sampling
 * rate defined in the 's5611_avail_pressure_osr' of array, please direct
 * the use of assignment.
 *   */
static int ms5611_oversampling_pres_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ms5611_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%u", data->pressure_osr->rate);
}

/*
 * Set the value of pressure oversampling. The value entered must be a
 * pre-specified valid value. Valid values are predefined in the
 * 's5611_avail_pressure_osr' array.
 *  */
static int ms5611_oversampling_pres_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int err;
	unsigned long data;
	struct i2c_client *client = to_i2c_client(dev);
	struct ms5611_data *ms5611 = i2c_get_clientdata(client);

	err = strict_strtoul(buf, 10, &data);
	if (err)
		return err;

	update_oversampling(ms5611_avail_pressure_osr,
			&ms5611->pressure_osr, data);
	return count;
}

/*
 * Read temperature and atmospheric pressure values.
 *  */
static int ms5611_read_temp_and_pressure(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 pressure = 0, temperature = 0;
	s32 status;
	struct i2c_client *client = to_i2c_client(dev);
	struct ms5611_data *ms5611 = i2c_get_clientdata(client);

	status = ms5611_update_raw_data(ms5611->ms5611_client,
			ms5611->pressure_osr, &pressure);
	if (status != 0)
		return status;

	status = ms5611_update_raw_data(ms5611->ms5611_client,
			ms5611->temp_osr, &temperature);
	if (status != 0)
		return status;

	return sprintf(buf, "%u %u", temperature, pressure);
}

static DEVICE_ATTR(sens, S_IRUGO|S_IWUSR|S_IWGRP,
		ms5611_sens_show, NULL);
static DEVICE_ATTR(off, S_IRUGO|S_IWUSR|S_IWGRP,
		ms5611_off_show, NULL);
static DEVICE_ATTR(tcs, S_IRUGO|S_IWUSR|S_IWGRP,
		ms5611_tcs_show, NULL);
static DEVICE_ATTR(tco, S_IRUGO|S_IWUSR|S_IWGRP,
		ms5611_tco_show, NULL);
static DEVICE_ATTR(tref, S_IRUGO|S_IWUSR|S_IWGRP,
		ms5611_tref_show, NULL);
static DEVICE_ATTR(tempsens, S_IRUGO|S_IWUSR|S_IWGRP,
		ms5611_tempsens_show, NULL);
static DEVICE_ATTR(oversampling_temp, S_IRUGO|S_IWUSR|S_IWGRP,
                ms5611_oversampling_temp_show, ms5611_oversampling_temp_store);
static DEVICE_ATTR(oversampling_pres, S_IRUGO|S_IWUSR|S_IWGRP,
		ms5611_oversampling_pres_show, ms5611_oversampling_pres_store);
static DEVICE_ATTR(temp_and_pressure, S_IRUGO|S_IWUSR|S_IWGRP,
		ms5611_read_temp_and_pressure, NULL);

static struct attribute *ms5611_attributes[] = {
	&dev_attr_sens.attr,
	&dev_attr_off.attr,
	&dev_attr_tcs.attr,
	&dev_attr_tco.attr,
	&dev_attr_tref.attr,
	&dev_attr_tempsens.attr,
	&dev_attr_oversampling_temp.attr,
	&dev_attr_oversampling_pres.attr,
	&dev_attr_temp_and_pressure.attr,
	NULL
};

static struct attribute_group ms5611_attr_group = {
	.attrs = ms5611_attributes,
};

/*
 * ms5611 initialization.
 * @client: Handle to slave device.
 *
 * The CMD_RST command is sent first. And then reads the check value.
 * Set the temperature and atmospheric pressure sampling rate to 4096.
 * Returning negative errno else zero on success.
 *   */
static int ms5611_init_client(struct i2c_client *client)
{
	int status;
	struct ms5611_data *data = i2c_get_clientdata(client);

	status = i2c_smbus_write_byte(client, CMD_RST);
	if (status < 0) {
		printk(KERN_INFO "Error: ms561101ba reset\n");
		goto exit;
	}
	usleep_range(3000, 4000);

	status = ms5611_read_calibration_data(client);
	if (status < 0)
		goto exit;

	data->temp_osr = &ms5611_avail_temp_osr[4];
	data->pressure_osr = &ms5611_avail_pressure_osr[4];

exit:
	return status;
}

static int __devinit ms5611_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int err = 0;
	struct ms5611_data *data;
	struct input_dev *dev;

	/* Check whether the client's adapter supports the I2C interface */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_WRITE_BYTE
					| I2C_FUNC_SMBUS_READ_WORD_DATA
					| I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
		printk(KERN_INFO "i2c_check_functionality error\n");
		err = -EOPNOTSUPP;
		goto exit;
	}

	data = kzalloc(sizeof(struct ms5611_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}

	i2c_set_clientdata(client, data);
	data->ms5611_client = client;
	mutex_init(&data->lock);

	err = ms5611_init_client(data->ms5611_client);
	if (err != 0)
		goto exit_free;

	dev = input_allocate_device();
	if (!dev) {
		printk(KERN_ERR "ms5611 input_allocate_device fail\n");
		err = -ENOMEM;
		goto exit_free;
	}
	dev->name = SENSOR_NAME;
	dev->id.bustype = BUS_I2C;

	input_set_drvdata(dev, data);

	err = input_register_device(dev);
	if (err < 0) {
		printk(KERN_INFO "ms5611 input_register_device fail\n");
		input_free_device(dev);
		goto exit_free;
	}

	data->input = dev;
	err = sysfs_create_group(&data->input->dev.kobj, &ms5611_attr_group);
	if (err < 0)
		goto error_sysfs;

	dev_info(&data->ms5611_client->dev,
			"Successfully initialized ms561101ba!\n");
	return 0;

error_sysfs:
	input_unregister_device(data->input);
exit_free:
	kfree(data);
exit:
	return err;
}

static int __devexit ms5611_remove(struct i2c_client *client)
{
	struct ms5611_data *data = i2c_get_clientdata(client);

	sysfs_remove_group(&client->dev.kobj, &ms5611_attr_group);
	input_unregister_device(data->input);
	kfree(data);

	return 0;
}

static const struct i2c_device_id ms5611_id[] = {
	{SENSOR_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, ms5611_id);

static struct i2c_driver ms561101ba_driver = {
	.driver = {
		.owner 	= THIS_MODULE,
		.name	= SENSOR_NAME
	},
	.class		= I2C_CLASS_HWMON,
	.id_table	= ms5611_id,
	.probe		= ms5611_probe,
	.remove		= __devexit_p(ms5611_remove),
};

static int __init ms561101ba_init(void)
{
	return i2c_add_driver(&ms561101ba_driver);
}

static void __exit ms561101ba_exit(void)
{
	i2c_del_driver(&ms561101ba_driver);
}

MODULE_AUTHOR("Sarlor <zoreorg@gmail.com>");
MODULE_DESCRIPTION("MS561101BA driver");
MODULE_LICENSE("GPL");

module_init(ms561101ba_init);
module_exit(ms561101ba_exit);
