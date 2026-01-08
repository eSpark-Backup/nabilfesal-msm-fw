#ifndef ENCRYPTIONHELPER_H
#define ENCRYPTIONHELPER_H


#include <QDir>
#include <QFile>
#include <QString>
#include <QByteArray>
#include <QDataStream>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QCryptographicHash>


class EncryptionHelper
{
public:
    static bool isEncrypted(QFile &file);
    static QByteArray encrypt(const QByteArray &plainText, const QString &password);
    static QByteArray decrypt(const QByteArray &cipherText, const QString &password);
    static QByteArray readAndDecryptFile(const QString &path,const QString &password);
    static void check_and_encrypt_files(const QString &file_path,
                                const QString &arg, bool showMessages, QWidget* parent);

private:
    static bool deriveKey(
        const QString &password,
        const QByteArray &salt,
        QByteArray &key
    );
};


#endif // ENCRYPTIONHELPER_H
