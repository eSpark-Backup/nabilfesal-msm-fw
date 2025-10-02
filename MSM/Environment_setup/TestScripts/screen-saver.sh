#!/bin/bash
#
# fb_idle_blank.sh
# Blank framebuffer after 2 min of inactivity, restore on touch/key event

FBDEV="/dev/fb0"
BACKUP="/tmp/backup.raw"
INPUT_DEV="/dev/input/event0"   # adjust for your touchscreen/keyboard
IDLE_TIME=10                   # 2 minutes in seconds

# Get framebuffer size in bytes
XRES=$(cat /sys/class/graphics/fb0/virtual_size | cut -d, -f1)
YRES=$(cat /sys/class/graphics/fb0/virtual_size | cut -d, -f2)
BPP=$(cat /sys/class/graphics/fb0/bits_per_pixel)
FB_SIZE=$((XRES * YRES * BPP / 8))

echo "Framebuffer size: $FB_SIZE bytes"
echo "Monitoring $INPUT_DEV for activity..."

last_event=$(date +%s)
blanked=0

# Monitor input events
evtest --grab "$INPUT_DEV" 2>/dev/null | while read line; do
    now=$(date +%s)
    idle=$((now - last_event))

    # If screen is blanked and any input occurs → restore
    if [ $blanked -eq 1 ]; then
        echo "Activity detected → restoring framebuffer..."
        kill -CONT"$(cat /var/run/msm_run.pid)"
        sleep 1
        dd if="$BACKUP" of="$FBDEV" bs=$FB_SIZE count=1 status=none
        # cat /tmp/backup.raw > /dev/fb0
        blanked=0
    fi

    # Update last event time
    last_event=$now
done &

# Background loop checks idle time
while true; do
    now=$(date +%s)
    idle=$((now - last_event))

    if [ $blanked -eq 0 ] && [ $idle -ge $IDLE_TIME ]; then
        echo "Idle for $IDLE_TIME seconds → blanking framebuffer..."
        kill -STOP "$(cat /var/run/msm_run.pid)"
        sleep 1
        dd if="$FBDEV" of="$BACKUP" bs=$FB_SIZE count=1 status=none
        cat /dev/zero | dd of="$FBDEV" bs=$FB_SIZE count=1 status=none
        blanked=1
    fi

    sleep 5
done
