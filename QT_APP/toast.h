#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QHBoxLayout>
#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QGuiApplication>
#include <QScreen>

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
        new Toast(message, parent, level, timeoutMs);
    }

    Toast(const QString &message, QWidget *parent = nullptr, Level level = Info, int timeoutMs = 3000)
        : QWidget(parent)
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip);
        setAttribute(Qt::WA_ShowWithoutActivating);

        // --- Color based on level ---
        QString bgColor = "rgba(255, 255, 255, 255)";
        QString borderColor;
        QString textColor = "#000000";
        QString iconPath;

        switch (level) {
            case Info:
                iconPath = ":/icons/information.png";
                borderColor = "#2E8B57";   // green
                textColor = "#000000";
                break;
            case Warning:
                iconPath = ":/icons/warning.png";
                borderColor = "#DAA520";   // yellow-ish
                textColor = "#000000";
                break;
            case Error:
                iconPath = ":/icons/error.png";
                borderColor = "#B22222";   // red
                textColor = "#000000";
                break;
            }

        // --- UI ---
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 8, 10, 8);
        layout->setSpacing(10);

        // Icon
        QLabel *iconLabel = new QLabel(this);
        QPixmap icon(iconPath);
        iconLabel->setPixmap(icon.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        layout->addWidget(iconLabel);

        // Text
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
        QRect geom;
        if (parent) {
            geom = parent->geometry();
        } else {
            QScreen *screen = QGuiApplication::primaryScreen();
            if (screen)
                geom = screen->availableGeometry();
        }

        int x = geom.x() + geom.width() - width() - 10;
        int y = geom.y() + 20;
        move(x, y);

        QWidget::show();

        QTimer::singleShot(timeoutMs, this, &QWidget::close);
    }
};