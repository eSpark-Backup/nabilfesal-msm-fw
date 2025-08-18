#include "utils.h"
#include <QProcess>
#include <QThread>

namespace Utils {
void keySound() {
    QProcess::execute("sh", {"-c", "echo 0 > /sys/class/backlight/backlight/bl_power"});
    QThread::usleep(50000); // 50ms
    QProcess::execute("sh", {"-c", "echo 1 > /sys/class/backlight/backlight/bl_power"});
}
}
