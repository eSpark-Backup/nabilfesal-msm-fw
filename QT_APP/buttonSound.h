#ifndef BUTTONSOUND_H
#define BUTTONSOUND_H

#include <QObject>
#include <QEvent>
#include <QPushButton>
#include "utils.h"

class ButtonSound : public QObject {
    Q_OBJECT
public:
    explicit ButtonSound(QObject *parent = nullptr)
        : QObject(parent) {}

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if ((event->type() == QEvent::MouseButtonPress) ||
            (event->type() == QEvent::TouchBegin)) {
            if (qobject_cast<QPushButton*>(obj)) {
                Utils::keySound();
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

#endif // BUTTONSOUND_H
