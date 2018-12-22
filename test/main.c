#include <stdio.h>
#include "ms5611.h"

int main(void)
{
	int temperature, pressure;
	struct ms5611_calibration cali;

	ms5611_read_calibration(&cali);
	ms5611_read_pressure_and_temperature(&cali, &pressure, &temperature);

	return 0;
}
