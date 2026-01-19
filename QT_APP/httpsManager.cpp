#include "httpsManager.h"
#include <QFile>
#include <QDebug>
#include "toast.h"
#include "utils.h"
#include <QProcessEnvironment>


HttpsManager::HttpsManager(QObject *parent) : QObject(parent)
{
    manager = new QNetworkAccessManager(this);
}


HttpsManager::~HttpsManager()
{
    delete manager;
}


void HttpsManager::setUrl(const QString &url)
{
    endpointUrl = url;
}


/* =========================
 * HTTPS CONFIG
 * ========================= */
void HttpsManager::setClientCertificates(const QString &caPath,
                                         const QString &clientCertPath,
                                         const QString &clientKeyPath)
{
    sslConfig = QSslConfiguration::defaultConfiguration();

    QFile caFile(caPath);
    if (caFile.open(QIODevice::ReadOnly)) {
        sslConfig.setCaCertificates(
            QSslCertificate::fromData(caFile.readAll(), QSsl::Pem));
        caFile.close();
    }

    if (!clientCertPath.isEmpty() && !clientKeyPath.isEmpty()) {
        QFile certFile(clientCertPath);
        QFile keyFile(clientKeyPath);

        if (certFile.open(QIODevice::ReadOnly) &&
            keyFile.open(QIODevice::ReadOnly)) {

            sslConfig.setLocalCertificate(
                QSslCertificate(certFile.readAll(), QSsl::Pem));
            sslConfig.setPrivateKey(
                QSslKey(keyFile.readAll(), QSsl::Rsa, QSsl::Pem));
        }
    }

    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
}


/* =========================
 * POST JSON
 * ========================= */
void HttpsManager::postJson(const QJsonObject &payload)
{
    if (endpointUrl.isEmpty()) {
        emit requestFailed("URL not set", 0);
        return;
    }

    QNetworkRequest request{ QUrl(endpointUrl) };
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/json; charset=utf-8");
    request.setRawHeader("Accept", "application/json");
    request.setSslConfiguration(sslConfig);

    QByteArray body = QJsonDocument(payload)
                          .toJson(QJsonDocument::Compact);

    QNetworkReply *reply = manager->post(request, body);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {

        int httpCode =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        QByteArray respData = reply->readAll();
        QJsonParseError err;
        QJsonDocument jsonResp = QJsonDocument::fromJson(respData, &err);

        if (err.error != QJsonParseError::NoError) {
            emit requestFailed("Invalid JSON response", httpCode);
            reply->deleteLater();
            return;
        }


        QJsonObject obj = jsonResp.object();

        qDebug() << "HTTP CODE:" << httpCode;
        qDebug().noquote() << QJsonDocument(obj).toJson(QJsonDocument::Indented);
        bool httpSuccess = false;
        /* ===== HTTPs API Responses ===== */
        switch (httpCode)
        {
        case 200:   // Registered
        case 201:   // UUID Created
        case 202:   // Device Assigned
            emit requestFinished(obj, httpCode);
            httpSuccess = true;
            break;

        case 400:   // Missing fields
        case 401:   // Unauthorized
        case 409:   // UUID conflict
        case 500:   // Server error
            emit requestFailed(obj["message"].toString(), httpCode);
            httpSuccess = false;
            break;

        default:
            emit requestFailed("Unknown HTTP response", httpCode);
            httpSuccess = true;
            break;
        }
        Toast::show(obj["message"].toString(),
                            uiMain, httpSuccess ? Toast::Info : Toast::Error);

        reply->deleteLater();
    });

    connect(reply, &QNetworkReply::sslErrors,
            this, [reply](const QList<QSslError> &errors) {
        for (const auto &e : errors)
            qDebug() << "[SSL ERROR]" << e.errorString();
        reply->ignoreSslErrors(); // for testing only
    });
}


/* =========================
 * PAYLOAD (DOC MATCH)
 * ========================= */
void HttpsManager::send_Payload()
{
    QJsonObject payload;

    payload["device_uuid"] = Utils::getUid() + "_factory";
    payload["device_imei"] = Utils::getDeviceImei();
    payload["device_pass"] = Utils::getDevPassword();

    qDebug().noquote() << "Sending payload:"
             << QJsonDocument(payload).toJson(QJsonDocument::Indented);

    postJson(payload);
}


/* =========================
 * CONNECT + START
 * ========================= */
void HttpsManager::connect_https(QWidget *parent)
{
    uiMain = parent;
    endpointUrl = "https://api.eincloud.ai/api/devices/verify";

    connect(this, &HttpsManager::requestFinished,
            this, [](const QJsonObject &resp, int code) {
        qDebug() << "HTTPs SUCCESS (" << code << ")";
    });

    connect(this, &HttpsManager::requestFailed,
            this, [](const QString &err, int code) {
        qDebug() << "HTTPs FAILED (" << code << "):" << err;
    });

    send_Payload();
}
