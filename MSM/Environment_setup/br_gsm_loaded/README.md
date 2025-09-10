ON these CONFIGs

QT Examples are loading
GSM Module is able to ping



1. mmcli -m 0 --enable
2. qmicli -d /dev/cdc-wdm0 --device-open-proxy \
  --wds-start-network="apn='internet',ip-type=4" --client-no-release-cid
3. udhcpc -i wwan0


===============================================
generate uboot.scr

mkimage -C none -A arm -T script -d boot.cmd boot.scr

===============================================

Mostly Uboot does not require building
only buildroot for packages and kernel for features to use those packages