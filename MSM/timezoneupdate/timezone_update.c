#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
    const char *resp_file = "responses.txt";
    FILE *fp = fopen(resp_file, "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    char line[256];
    char tz_str[8] = {0};

    // Find the +CCLK line
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "+CCLK:", 6) == 0) {
            // Skip until the first quote after the colon
            char *p = strchr(line, ':');
            if (!p) continue;
            p++; // move past ':'

            // Find the first quote after ':'
            while (*p && *p != '"') p++;
            if (!*p) continue;
            p++; // move past opening quote

            // Find closing quote
            char *q = strchr(p, '"');
            if (!q) continue;

            // p..q-1 is the date-time string, e.g., 25/12/09,14:18:24+20
            // Timezone is last 3 characters: +20
            if (q - p >= 3) {
                char sign = *(q - 3);
                if (sign == '+' || sign == '-') {
                    tz_str[0] = sign;
                    tz_str[1] = *(q - 2);
                    tz_str[2] = *(q - 1);
                    tz_str[3] = '\0';
                }
            }
            break;
        }
    }
    fclose(fp);

    if (tz_str[0] == '\0') {
        fprintf(stderr, "No timezone found in %s\n", resp_file);
        return 1;
    }

    int tz_val = atoi(tz_str); // GSM timezone value
    printf("Parsed Tz_val %s\n", tz_str);
    // GSM timezone is in 15-minute increments
    int tz_minutes = tz_val * 15;
    int tz_hours = tz_minutes / 60;
    int tz_rem_minutes = tz_minutes % 60;

    printf("Parsed timezone: +%d:%02d\n", tz_hours, tz_rem_minutes);

    // Map to /usr/share/zoneinfo/Etc/GMT
    char zoneinfo[128] = {0};
    if (tz_hours == 0 && tz_rem_minutes == 0) {
        snprintf(zoneinfo, sizeof(zoneinfo), "/usr/share/zoneinfo/UTC");
    } else {
        // Etc/GMT has reversed sign
        int etc_val = -tz_hours;
        snprintf(zoneinfo, sizeof(zoneinfo), "/usr/share/zoneinfo/Etc/GMT%+d", etc_val);
    }

    printf("Linking %s -> /etc/localtime\n", zoneinfo);

    // Update /etc/localtime
    if (unlink("/etc/localtime") != 0) {
        perror("unlink /etc/localtime");
    }

    if (symlink(zoneinfo, "/etc/localtime") != 0) {
        perror("symlink");
        return 1;
    }

    // Optional: update /etc/timezone
    FILE *tzfp = fopen("/etc/timezone", "w");
    if (tzfp) {
        fprintf(tzfp, "%s\n", zoneinfo);
        fclose(tzfp);
    } else {
        perror("fopen /etc/timezone");
    }

    printf("Timezone updated successfully.\n");
    return 0;
}
