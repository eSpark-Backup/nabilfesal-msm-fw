#ifndef HTTPSMANAGER_H
#define HTTPSMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QSslKey>

class HttpsManager : public QObject
{
    Q_OBJECT
public:
    explicit HttpsManager(QObject *parent = nullptr);
    ~HttpsManager();

    void connect_https(QWidget *parent, const QString &uid, const QString &pw);
    void setUrl(const QString &url);
    void setClientCertificates(const QString &caPath,
        const QString &clientCertPath = QString(), const QString &clientKeyPath = QString());
    void postJson(const QJsonObject &payload);
    void send_Payload(const QString &uid, const QString &pw);

signals:
    void requestFinished(const QJsonObject &response, int httpCode);
    void requestFailed(const QString &errorString, int httpCode);

private:
    QNetworkAccessManager *manager;
    QSslConfiguration sslConfig;
    QString endpointUrl;
    QWidget *uiMain = nullptr;
};

#endif // HTTPSMANAGER_H
