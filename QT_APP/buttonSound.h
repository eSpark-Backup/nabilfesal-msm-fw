#ifndef BUTTONSOUND_H
#define BUTTONSOUND_H

#include <QObject>
#include <QEvent>
#include <QAbstractButton>
#include <QFile>
#include <QTimer>
#include <QKeyEvent>
#include <QApplication>
#include <QThread>
#include <chrono>

class ButtonSound : public QObject {
    Q_OBJECT
public:
    explicit ButtonSound(QObject *parent = nullptr)
        : QObject(parent)
    {
        // one reusable timer for debouncing the sound
        soundTimer = new QTimer(this);
        soundTimer->setSingleShot(true);
        connect(soundTimer, &QTimer::timeout, this, &ButtonSound::stopBeep);
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        // 🔊 Handle button/touch beep
        if (event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::TouchBegin) {
            if (qobject_cast<QAbstractButton*>(obj)) {
                playBeep();
            }
        }

        // ⌨️ Handle key remapping
        if (event->type() == QEvent::KeyPress) {
            auto *keyEvent = static_cast<QKeyEvent*>(event);

            if (keyEvent->key() == Qt::Key_Left) {
                // simulate Shift+Tab
                sendKeyEvent(Qt::Key_Tab, Qt::ShiftModifier);
                return true; // swallow Left Arrow
            }
            else if (keyEvent->key() == Qt::Key_Right) {
                // simulate Tab
                sendKeyEvent(Qt::Key_Tab, Qt::NoModifier);
                return true; // swallow Right Arrow
            }
        }

        return QObject::eventFilter(obj, event);
    }

private:
    QTimer *soundTimer;
    std::atomic<bool> m_beeping{false};


    // void playBeep() {
    //     if (!soundTimer->isActive()) {
    //         #if defined(BUILD_V3S)
    //         writeSysfs("/sys/class/backlight/backlight/bl_power", "0");
    //         #endif
    //         soundTimer->start(200); // beep for 50ms
    //     }
    // }
    void playBeep() {
        if (m_beeping.exchange(true)) return; // already beeping, skip

        #if defined(BUILD_V3S)
        writeSysfs("/sys/class/backlight/backlight/bl_power", "0");
        #endif

        // Run stop on a separate thread — NOT subject to event loop delays
        QThread::create([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // strict 50
            stopBeep();
            m_beeping.store(false);
        })->start();
    }

    void stopBeep() {
        #if defined(BUILD_V3S)
        writeSysfs("/sys/class/backlight/backlight/bl_power", "1");
        #endif
    }

    void writeSysfs(const QString &path, const QString &value) {
        QFile f(path);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(value.toUtf8());
            f.close();
        }
    }

    void sendKeyEvent(int key, Qt::KeyboardModifiers mods) {
        QWidget *target = QApplication::focusWidget();
        if (!target) return;

        // KeyPress
        QKeyEvent *pressEvent = new QKeyEvent(
            QEvent::KeyPress, key, mods
            );
        QCoreApplication::postEvent(target, pressEvent);

        // KeyRelease
        QKeyEvent *releaseEvent = new QKeyEvent(
            QEvent::KeyRelease, key, mods
            );
        QCoreApplication::postEvent(target, releaseEvent);
    }
};

#endif // BUTTONSOUND_H
