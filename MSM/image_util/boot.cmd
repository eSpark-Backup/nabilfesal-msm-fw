setenv bootargs console=ttyS0,115200 panic=5 console=ttyS0 rootwait rw init=/sbin/init root=/dev/mmcblk0p2 earlyprintk rw
load mmc 0:1 0x41000000 zImage
load mmc 0:1 0x41800000 sun8i-v3s-licheepi-zero-with-800x480-lcd.dtb
bootz 0x41000000 - 0x41800000