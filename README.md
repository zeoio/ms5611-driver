### MS5611-01BA03 Barometric Pressure Sensor Driver

MS5611-01BA03 Barometric Pressure Sensor, with statinless steel cap. 

#### Building the source
git clone https://github.com/Sarlor/ms5611-driver.git

1. Frist enter the 'src' directory
```bash
cd src
```


2. Edit the 'Makefile' file, set the correct kernel directory 'KERN_DIR'.
        ===> $: vim Makefile
3. Copy the driver files to the 'hwmon' directory.
        ===> $: make cp
4. Add board-level support code.
        ===> $: vim arch/arm/plat-s5p4418/realarm/device.c
5. Add the code in the appropriate place.
        CODE 1:

                #if defined(CONFIG_SENSORS_MS561101BA) || defined(CONFIG_SENSORS_MS561101BA_MODULE)
                #include <linux/i2c.h>

                /* CODEC */
                static struct i2c_board_info __initdata ms561101ba_i2c_bdi = {
                        .type   = "ms561101ba",
                        .addr   = 0x77
                };

                #endif

        CODE 2:

                #if defined(CONFIG_SENSORS_MS561101BA) || defined(CONFIG_SENSORS_MS561101BA_MODULE)
                        printk("plat: add pressure MS5611-01BA03\n");
                        i2c_register_board_info(1, &ms561101ba_i2c_bdi, 1);
                #endif

7. Modify the kernel configuration file for ms5611.
        ===> $: vim drivers/hwmon/Kconfig
        ADD CODE:

                config SENSORS_MS561101BA
                        tristate "MS5611-01BA03 Barometric Pressure Sensor"
                        depends on I2C
                        default n
                        help
                                Say Y here to enable the pressure sensor.

                                If unsure, say N.
        ===> $: vim drivers/hwmon/Makefile
        ADD CODE:

                obj-$(CONFIG_SENSORS_MS561101BA)        += ms561101ba.o

8. Configure the kernel to use this driver.
        ===> $: make menuconfig
                Device Drivers -> Hardware Monitoring support -> MS5611-01BA03 Barometric Pressure Sensor

        Check, save.


