### MS5611-01BA03 Barometric Pressure Sensor Driver

MS5611-01BA03 Barometric Pressure Sensor, with statinless steel cap.

#### Building the source

1. Clone driver code
```bash
git clone https://github.com/Sarlor/ms5611-driver.git
cd ms5611-driver/src
```

2. Edit Makefile file, set the correct kernel directory 'KERN_DIR'
```bash
vim Makefile
```

3. Copy the driver files to the 'hwmon' directory
```bash
make cp
```

4. Add board-level support code
```bash
vim arch/arm/plat-s5p4418/realarm/device.c
```

5. Add the code in the appropriate place
```c
	#if defined(CONFIG_SENSORS_MS561101BA) || defined(CONFIG_SENSORS_MS561101BA_MODULE)
        #include <linux/i2c.h>

        /* CODEC */
        static struct i2c_board_info __initdata ms561101ba_i2c_bdi = {
        	.type   = "ms561101ba",
                .addr   = 0x77
        };
        #endif

        #if defined(CONFIG_SENSORS_MS561101BA) || defined(CONFIG_SENSORS_MS561101BA_MODULE)
        	printk("plat: add pressure MS5611-01BA03\n");
                i2c_register_board_info(1, &ms561101ba_i2c_bdi, 1);
        #endif
```

6. Modify the kernel configuration file for ms5611
```bash
vim drivers/hwmon/Kconfig
```

7.  Add the code in the appropriate place
```c
	config SENSORS_MS561101BA
        	tristate "MS5611-01BA03 Barometric Pressure Sensor"
                depends on I2C
                default n
                help
                	Say Y here to enable the pressure sensor.

                        If unsure, say N.
```

8. Edit Makefile
```bash
 vim drivers/hwmon/Makefile
```

9. Add ms5611 driver module
	`obj-$(CONFIG_SENSORS_MS561101BA)        += ms561101ba.o`

8. Configure the kernel to use this driver
    1. Open config menu
         ```bash
		 make menuconfig
	 ```

    2. Open the required submodule
       ```
	   Device Drivers -> Hardware Monitoring support -> MS5611-01BA03 Barometric Pressure Sensor
       ```
    3. Save config
