#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QHBoxLayout>
#include <QGuiApplication>
#include <QScreen>
#include <QPropertyAnimation>
#include <QVector>

class Toast : public QWidget
{
    Q_OBJECT
public:
    enum Level {
        Info,
        Warning,
        Error
    };

    static void show(const QString &message, QWidget *parent = nullptr, Level level = Info, int timeoutMs = 3000)
    {
        auto *toast = new Toast(message, parent, level, timeoutMs);
        activeToasts.append(toast);
        toast->repositionAllToasts();
    }

private:
    explicit Toast(const QString &message, QWidget *parent = nullptr, Level level = Info, int timeoutMs = 3000)
        : QWidget(parent)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip);
        setAttribute(Qt::WA_ShowWithoutActivating);

        // --- Colors ---
        QString bgColor = "rgba(255, 255, 255, 255)";
        QString borderColor;
        QString textColor = "#000000";
        QString iconPath;

        switch (level) {
            case Info:
                iconPath = ":/icons/information.png";
                borderColor = "#2E8B57";   // green
                break;
            case Warning:
                iconPath = ":/icons/warning.png";
                borderColor = "#DAA520";   // yellow
                break;
            case Error:
                iconPath = ":/icons/error.png";
                borderColor = "#B22222";   // red
                break;
        }

        // --- UI ---
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 8, 10, 8);
        layout->setSpacing(10);

        QLabel *iconLabel = new QLabel(this);
        QPixmap icon(iconPath);
        iconLabel->setPixmap(icon.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        layout->addWidget(iconLabel);

        QLabel *textLabel = new QLabel(message, this);
        textLabel->setStyleSheet(QString("QLabel { color: %1; font-size: 14px; }").arg(textColor));
        textLabel->setWordWrap(true);
        layout->addWidget(textLabel);

        setLayout(layout);
        setObjectName("ToastWidget");
        setAttribute(Qt::WA_StyledBackground, true);
        setStyleSheet(QString(
            "#ToastWidget { "
            "background-color: %1; "
            "border: 2px solid %2; "
            "border-radius: 5px; "
            "}"
        ).arg(bgColor, borderColor));

        adjustSize();

        // --- Position ---
        QRect geom = parent ? parent->geometry()
                            : QGuiApplication::primaryScreen()->availableGeometry();

        int x = geom.x() + geom.width() - width() - 10;
        int y = geom.y() + 20;
        move(x, y);

        QWidget::show();

        // --- Auto close ---
        QTimer::singleShot(timeoutMs, this, [this]() {
            activeToasts.removeOne(this);
            close();
            repositionAllToasts();
            deleteLater();
        });
    }

    // 🧭 Helper: reposition all toasts dynamically
    void repositionAllToasts()
    {
        int margin = 10;
        int offset = 20;
        QRect geom = parentWidget()
            ? parentWidget()->geometry()
            : QGuiApplication::primaryScreen()->availableGeometry();

        int maxWidth = 0;
        for (auto *toast : activeToasts)
            maxWidth = qMax(maxWidth, toast->width());

        int anchorX = geom.x() + geom.width() - maxWidth - margin;

        int currentY = geom.y() + offset;
        for (Toast *toast : activeToasts) {
            int toastX = anchorX + (maxWidth - toast->width());
            QPropertyAnimation *anim = new QPropertyAnimation(toast, "pos");
            anim->setDuration(200);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->setStartValue(toast->pos());
            anim->setEndValue(QPoint(toastX, currentY));
            anim->start(QAbstractAnimation::DeleteWhenStopped);

            currentY += toast->height() + margin;
        }

    }

    static inline QVector<Toast*> activeToasts;
};
