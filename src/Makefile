obj-m = ms561101ba.o
KERN_DIR = ~/WorkDir/Sensor/kernel-3.4.39
PWD = $(shell pwd)

all:
	make -C $(KERN_DIR) SUBDIRS=$(PWD) modules

.PHONY: clean cp
clean:
	rm *.o *.ko *.symvers *.order *.mod.c
cp:
	cp ms561101ba.c $(KERN_DIR)/drivers/hwmon
