#ifndef _SENSOR_MS5611_H
#define _SENSOR_MS5611_H

/*
 * MS5611-01BA03 Barometric Pressure Sensor
 *
 * Interface		RW status	Description	 					I/O farmat
 *    sens		Read Only	pressure sensitivity					"%d"
 *    off		Read Only	pressure offset						"%d"
 *    tcs		Read Only	temperature coefficient of pressure sensitivity		"%d"
 *    tco		Read Only	temperature coefficient of pressure offset		"%d"
 *    tref		Read Only	reference temperature					"%d"
 *    tempsens		Read Only	temperature coefficient of the temperature		"%d"
 *    oversampling_temp RW		oversampling of temperature				"%d"
 *    oversampling_pres RW		oversampling of pressure				"%d"
 *    temp_and_pressure	Read Only	digital pressure and digital temperature value		"%d %d"
 *  */

/* struct ms5611_calibration for calibration data */
struct ms5611_calibration {
	unsigned short c1, c2, c3;
	unsigned short c4, c5, c6;
};

int ms5611_read_calibration(struct ms5611_calibration *);
int ms5611_read_pressure_and_temperature(struct ms5611_calibration *, int *, int *);
int ms5611_get_oversampling_temeprature(unsigned short *);
int ms5611_get_oversampling_pressure(unsigned short *);
int ms5611_set_oversampling_temeprature(unsigned short);
int ms5611_set_oversampling_pressure(unsigned short);

#endif	/* _SENSOR_MS5611_H */
