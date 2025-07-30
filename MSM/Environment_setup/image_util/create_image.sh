#!/bin/bash
set -e

# ==== Configuration ====
IMG_NAME="sdcard.img"
IMG_SIZE_MB=512
BOOT_SIZE_MB=64
LOOP_DEV=""
MOUNT_DIR="/tmp/sd_mount"
BOOT_DIR="$MOUNT_DIR/boot"
ROOT_DIR="$MOUNT_DIR/rootfs"
MODULES_DIR="../output_files/lib/modules/5.2.0-licheepi-zero+"

# ==== Input files ====
BOOTSCR="boot.scr"
DTB="sun8i-v3s-licheepi-zero-with-800x480-lcd.dtb"
UBOOT="u-boot-sunxi-with-spl.bin"
KERNEL="zImage"
ROOTFS_TAR="rootfs.tar"

# ==== Check for required files ====
for f in "$BOOTSCR" "$DTB" "$UBOOT" "$KERNEL" "$ROOTFS_TAR"; do
    [ -f "$f" ] || { echo "Missing file: $f"; exit 1; }
done

# ==== Cleanup any previous mounts ====
umount "$BOOT_DIR" 2>/dev/null || true
umount "$ROOT_DIR" 2>/dev/null || true
rm -rf "$MOUNT_DIR"
mkdir -p "$BOOT_DIR" "$ROOT_DIR"

# ==== Create empty image ====
echo "Creating empty image..."
dd if=/dev/zero of="$IMG_NAME" bs=1M count="$IMG_SIZE_MB"

# ==== Create partition table ====
echo "Creating partition table..."
parted -s "$IMG_NAME" mklabel msdos
parted -s "$IMG_NAME" mkpart primary fat32 1MiB "$((BOOT_SIZE_MB+1))"MiB
parted -s "$IMG_NAME" mkpart primary ext4 "$((BOOT_SIZE_MB+1))"MiB 100%

# ==== Map image to loop device ====
echo "Setting up loop device..."
LOOP_DEV=$(losetup -f --show "$IMG_NAME")
partprobe "$LOOP_DEV"

# Create loop partitions (requires kpartx or use loopXpY directly)
LOOP_BOOT="${LOOP_DEV}p1"
LOOP_ROOT="${LOOP_DEV}p2"

# Wait for /dev/loopXpY to appear
sleep 1

# ==== Format partitions ====
echo "Formatting boot partition as FAT32..."
mkfs.vfat "$LOOP_BOOT"

echo "Formatting root partition as ext4..."
mkfs.ext4 "$LOOP_ROOT"

# ==== Mount and populate partitions ====
echo "Mounting partitions..."
mount "$LOOP_BOOT" "$BOOT_DIR"
mount "$LOOP_ROOT" "$ROOT_DIR"

echo "Copying boot files..."
cp "$BOOTSCR" "$DTB" "$KERNEL" "$BOOT_DIR"

echo "Extracting rootfs..."
tar -xf "$ROOTFS_TAR" -C "$ROOT_DIR"

# Copy kernel modules into rootfs
if [ -d "$MODULES_DIR" ]; then
    echo "Copying kernel modules to rootfs..."
    mkdir -p "$ROOT_DIR/lib/modules"
    cp -r "$MODULES_DIR" "$ROOT_DIR/lib/modules/"
else
    echo "Warning: MODULES_DIR not found: $MODULES_DIR"
fi

# ==== Install U-Boot ====
echo "Writing U-Boot SPL to image..."
dd if="$UBOOT" of="$LOOP_DEV" bs=1024 seek=8 conv=notrunc

# ==== Cleanup ====
echo "Cleaning up..."
sync
umount "$BOOT_DIR"
umount "$ROOT_DIR"
losetup -d "$LOOP_DEV"
rm -rf "$MOUNT_DIR"

echo "Bootable image created: $IMG_NAME"