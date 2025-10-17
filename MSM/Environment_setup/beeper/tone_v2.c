#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BRIGHTNESS_PATH "/sys/class/backlight/backlight/brightness"
#define POWER_PATH      "/sys/class/backlight/backlight/bl_power"

// Helper: write integer to sysfs file
static void write_sysfs(const char *path, int value) {
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "%d", value);
        fclose(f);
    } else {
        perror(path);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <delay_us> <brightness pattern>\n", argv[0]);
        fprintf(stderr, "Example: %s 150000 0 2 4 6 4 2 0\n", argv[0]);
        return 1;
    }

    int delay_us = atoi(argv[1]);
    if (delay_us <= 0)
        delay_us = 150000; // fallback default

    printf("Playing tone (delay: %d µs)...\n", delay_us);

    // Enable backlight output
    write_sysfs(POWER_PATH, 0);

    // Loop through brightness pattern
    for (int i = 2; i < argc; i++) {
        int level = atoi(argv[i]);
        write_sysfs(BRIGHTNESS_PATH, level);
        usleep(delay_us);
    }

    // Restore default brightness
    write_sysfs(BRIGHTNESS_PATH, 3);

    // Disable backlight after tone
    write_sysfs(POWER_PATH, 1);

    printf("Done.\n");
    return 0;
}
