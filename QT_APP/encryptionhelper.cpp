#include "utils.h"
#include "toast.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include "encryptionhelper.h"


static constexpr int SALT_SIZE    = 16;
static constexpr int KEY_SIZE     = 32; // AES-256
static constexpr int IV_SIZE      = 12; // GCM recommended
static constexpr int TAG_SIZE     = 16;
static constexpr int PBKDF2_ITERS = 100000;


static const QString ENC_KEY_PATH = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                                    + "/encryption_key.bin";
                                    
bool EncryptionHelper::deriveKey(
    const QString &password,
    const QByteArray &salt,
    QByteArray &key)
{
    key.resize(KEY_SIZE);

    int result = PKCS5_PBKDF2_HMAC(
        password.toUtf8().constData(),
        password.toUtf8().size(),
        reinterpret_cast<const unsigned char*>(salt.constData()),
        salt.size(),
        PBKDF2_ITERS,
        EVP_sha256(),
        KEY_SIZE,
        reinterpret_cast<unsigned char*>(key.data())
    );

    return result == 1;
}


QByteArray EncryptionHelper::encrypt(const QByteArray &plainText, const QString &password)
{
    if (plainText.isEmpty() || password.isEmpty()) return {};

    QByteArray salt(SALT_SIZE, 0);
    QByteArray iv(IV_SIZE, 0);

    RAND_bytes(reinterpret_cast<unsigned char*>(salt.data()), SALT_SIZE);
    RAND_bytes(reinterpret_cast<unsigned char*>(iv.data()), IV_SIZE);

    QByteArray key;
    if (!deriveKey(password, salt, key)) return {};

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return {};

    QByteArray cipherText(plainText.size(), 0);
    QByteArray tag(TAG_SIZE, 0);

    int len = 0;
    int cipherLen = 0;

    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, nullptr);
    EVP_EncryptInit_ex(ctx, nullptr, nullptr,
                       reinterpret_cast<unsigned char*>(key.data()),
                       reinterpret_cast<unsigned char*>(iv.data()));

    EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(cipherText.data()),
                      &len, reinterpret_cast<const unsigned char*>(plainText.constData()),
                      plainText.size());
    cipherLen = len;

    EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(cipherText.data()) + len, &len);
    cipherLen += len;

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE,
                        reinterpret_cast<unsigned char*>(tag.data()));

    EVP_CIPHER_CTX_free(ctx);

    cipherText.resize(cipherLen);

    QByteArray out;
    out.append(salt);
    out.append(iv);
    out.append(cipherText);
    out.append(tag);

    return out.toBase64();
}


QByteArray EncryptionHelper::decrypt(const QByteArray &cipherText, const QString &password)
{
    if (cipherText.isEmpty() || password.isEmpty()) return {};

    QByteArray decoded = QByteArray::fromBase64(cipherText);
    if (decoded.size() < SALT_SIZE + IV_SIZE + TAG_SIZE) return {};

    QByteArray salt = decoded.left(SALT_SIZE);
    QByteArray iv   = decoded.mid(SALT_SIZE, IV_SIZE);
    QByteArray tag  = decoded.right(TAG_SIZE);
    QByteArray encrypted = decoded.mid(SALT_SIZE + IV_SIZE,
                           decoded.size() - SALT_SIZE - IV_SIZE - TAG_SIZE);

    QByteArray key;
    if (!deriveKey(password, salt, key)) return {};

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return {};

    QByteArray plainText(encrypted.size(), 0);
    int len = 0;
    int plainLen = 0;

    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, nullptr);
    EVP_DecryptInit_ex(ctx, nullptr, nullptr,
                       reinterpret_cast<unsigned char*>(key.data()),
                       reinterpret_cast<unsigned char*>(iv.data()));

    EVP_DecryptUpdate(ctx,
                      reinterpret_cast<unsigned char*>(plainText.data()),
                      &len,
                      reinterpret_cast<const unsigned char*>(encrypted.constData()),
                      encrypted.size());
    plainLen = len;

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE,
                        reinterpret_cast<unsigned char*>(tag.data()));

    int ret = EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plainText.data()) + len, &len);

    EVP_CIPHER_CTX_free(ctx);

    if (ret <= 0) return {}; // wrong password or tampered data

    plainLen += len;
    plainText.resize(plainLen);
    return plainText;
}


bool EncryptionHelper::isEncrypted(QFile &file)
{
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
        return false;

    QByteArray firstBytes = file.peek(64);
    file.close();

    if (firstBytes.startsWith('{') || firstBytes.startsWith("\n{"))
        return false; // plain JSON
    return true; // assume encrypted
}


void EncryptionHelper::check_and_encrypt_files(const QString &file_path, const QString &arg, bool showMessages, QWidget* parent)
{
    QFile file(file_path);
    bool isEncryptedFile = isEncrypted(file);

    if (!isEncryptedFile) {
        if (!file.open(QIODevice::ReadOnly)) {
            if (showMessages) Toast::show(QString("Failed to read unencrypted %1 file!").arg(arg), parent, Toast::Error);
            return;
        }
        QByteArray plain = file.readAll();
        file.close();

        QByteArray encrypted = encrypt(plain, Utils::getEncryptionKey());
        if (encrypted.isEmpty()) {
            if (showMessages) Toast::show(QString("Failed to encrypt existing %1 file!").arg(arg), parent, Toast::Error);
            return;
        }

        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            if (showMessages) Toast::show(QString("Failed to write encrypted %1 file!").arg(arg), parent, Toast::Error);
            return;
        }
        
        qint64 written = file.write(encrypted);
        if (written != encrypted.size()) {
            if (showMessages) Toast::show(QString("Failed to write full encrypted %1 file!").arg(arg), parent, Toast::Error);
            file.close();
            return;
        }

        if (!file.flush()) {
            qWarning() << "Failed to flush encrypted file to disk";
        }
        file.close();

        qDebug() << QString("%1 file was unencrypted, auto-encrypted successfully.").arg(arg);
    }
}
