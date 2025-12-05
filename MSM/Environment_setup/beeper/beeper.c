#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>

#define EVENT_DEV "/dev/input/event0"

void beep(void) {
    system("sh -c 'echo 0 > /sys/class/backlight/backlight/bl_power; \
                   usleep 25000; \
                   echo 1 > /sys/class/backlight/backlight/bl_power' &");
}

int main(void) {
    int fd = open(EVENT_DEV, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct pollfd pfd = {
        .fd = fd,
        .events = POLLIN,
    };

    struct input_event ev;

    while (1) {
        int ret = poll(&pfd, 1, -1); // block until event
        if (ret > 0 && (pfd.revents & POLLIN)) {
            if (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type == EV_KEY && ev.value == 1) {
                    beep();
                }
            }
        }
    }

    close(fd);
    return 0;
}
