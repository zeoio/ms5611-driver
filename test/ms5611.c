#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ops.h"
#include "ms5611.h"

#define MS561101BA_PATH_BASE		"/sys/devices/virtual/input/input4"

/* Read calibration data from PROM */
int ms5611_read_calibration(struct ms5611_calibration *cali)
{
	if (!cali)
		return -1;

	char ret = 0;

	ret = read_data(MS561101BA_PATH_BASE, "sens", &cali->c1);
	ret += read_data(MS561101BA_PATH_BASE, "off", &cali->c2);
	ret += read_data(MS561101BA_PATH_BASE, "tcs", &cali->c3);
	ret += read_data(MS561101BA_PATH_BASE, "tco", &cali->c4);
	ret += read_data(MS561101BA_PATH_BASE, "tref", &cali->c5);
	ret += read_data(MS561101BA_PATH_BASE, "tempsens", &cali->c6);

	return ret != 6;
}

static int read_adc_pressure_and_temperature(unsigned int *pressure,
		unsigned int *temperature)
{
	char *buf = NULL;
	int ret;

	ret = read_data_block(MS561101BA_PATH_BASE, "temp_and_pressure", &buf);
	if (ret < 0)
		return ret;

	if (buf) {
		*temperature = (unsigned int)strtoul(strtok(buf, " "),NULL, 10);
		*pressure = (unsigned int)strtoul(strtok(NULL, " "), NULL, 10);
	}

	free(buf);
	return 0;
}

static int cal_temp_and_pressure(struct ms5611_calibration *cali,
		int *pressure, int *temperature)
{
	int t = *temperature, p = *pressure;
	long off, sens, dt;

	dt = t - (cali->c5 << 8);
	off = ((long)cali->c2<<16) + ((cali->c4 * dt) >> 7);
	sens = ((long)cali->c1<<15) + ((cali->c3 * dt) >> 8);

	t = 2000 + ((cali->c6 * dt) >> 23);

	if (t < 2000) {
		long off2, sens2, t2;

		t2 = (dt * dt) >> 31;
		off2 = (5 * (t - 2000) * (t - 2000)) >> 1;
		sens2 = off2 >> 1;

		if (t < -1500) {
			long tmp = (t + 1500) * (t + 1500);
			off2 += 7 * tmp;
			sens2 += (11 * tmp) >> 1;
		}

		t -= t2;
		off -= off2;
		sens -= sens2;
	}

	*temperature = t;
	*pressure = (((p * sens) >> 21) - off) >> 15;

	return 0;
}

int ms5611_read_pressure_and_temperature(struct ms5611_calibration *cali,
		int *pressure, int *temperature)
{
	int status;

	if (read_adc_pressure_and_temperature((unsigned int *)pressure,
				(unsigned int *)temperature) < 0) {
		printf("Error: Read raw data\n");
		return -1;
	}

	if (cal_temp_and_pressure(cali, pressure, temperature) < 0) {
		printf("Error: Compensate temperature and pressure\n");
		return -1;
	}

	return 0;
}

int ms5611_get_oversampling_temperature(unsigned short *sample)
{
	int ret;

	ret = read_data(MS561101BA_PATH_BASE, "oversampling_temp", sample);
	if (ret < 0) {
		printf("Error: Read oversampling of temperature\n");
		return -1;
	}

	return 0;
}

int ms5611_get_oversampling_pressure(unsigned short *sample)
{
	int ret;

	ret = read_data(MS561101BA_PATH_BASE, "oversampling_pres", sample);
	if (ret < 0) {
		printf("Error: Read oversampling of pressure\n");
		return -1;
	}

	return 0;
}

int ms5611_set_oversampling_temperature(unsigned short sample)
{
	int ret;

	ret = write_data(MS561101BA_PATH_BASE, "oversampling_temp", sample);
	if (ret < 0) {
		printf("Error: Write oversampling of temperature\n");
		return -1;
	}

	return 0;
}

int ms5611_set_oversampling_pressure(unsigned short sample)
{
	int ret;

	ret = write_data(MS561101BA_PATH_BASE, "oversampling_pres", sample);
	if (ret < 0) {
		printf("Error: Write oversampling of pressure\n");
		return -1;
	}

	return 0;
}
