MS5611-01BA03 Barometric Pressure Sensor, with statinless steel cap.

Directory introduction:
        doc:    development documentation
        src:    driver source code
        test:   test program source code
        tmp:    temporary intermediate files during development


MS5611-01BA03 Brarometric Pressure Sensor Develop documentation. Development
this driver is divided into three parts. First write the device driver file,
Then add the board support code. Third, modify the kernel configuration.

1. Frist enter the 'src' directory.
        ===> $: cd src
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



test data

365.57 20.04 63023
365.50 20.07 63049
365.44 20.10 63074
365.38 20.14 63099
365.33 20.16 63124
365.28 20.19 63150
365.25 20.21 63175
365.21 20.23 63201
365.17 20.25 63226
365.14 20.26 63251
365.11 20.28 63276
365.08 20.29 63301
365.05 20.31 63326
365.04 20.32 63352
365.02 20.33 63378
365.00 20.34 63403
364.98 20.35 63428
364.97 20.36 63454
364.95 20.36 63479
364.93 20.37 63504
364.92 20.38 63529
364.91 20.38 63554
364.90 20.39 63580
364.89 20.39 63605
364.88 20.40 63630
364.87 20.40 63655
364.87 20.41 63681
364.86 20.41 63706
364.86 20.41 63732
364.85 20.42 63757
364.84 20.42 63782
364.83 20.43 63808
364.82 20.43 63833
364.82 20.43 63858
364.81 20.44 63883
364.81 20.44 63909
364.80 20.44 63935
364.79 20.44 63960
364.80 20.44 63985
364.80 20.44 64011
364.79 20.45 64035
364.79 20.45 64060
364.78 20.45 64086
364.78 20.45 64111
364.79 20.45 64136
364.78 20.45 64162
364.78 20.45 64187
364.79 20.45 64213
364.79 20.45 64238
364.79 20.45 64263
364.78 20.45 64288
364.79 20.45 64313
364.79 20.45 64338
364.78 20.45 64364
364.79 20.45 64390
364.79 20.45 64415
364.79 20.45 64441
