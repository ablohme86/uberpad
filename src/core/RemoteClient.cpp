#include "RemoteClient.h"
#include <curl/curl.h>
#include <QStringList>
#include <algorithm>

namespace UberPad {

static size_t stringWriteCallback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t totalBytes = size * nmemb;
    QString *str = static_cast<QString*>(userdata);
    str->append(QString::fromUtf8(static_cast<const char*>(ptr), totalBytes));
    return totalBytes;
}

static size_t byteArrayWriteCallback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t totalBytes = size * nmemb;
    QByteArray *buf = static_cast<QByteArray*>(userdata);
    buf->append(static_cast<const char*>(ptr), totalBytes);
    return totalBytes;
}

struct UploadBuffer {
    const char *data = nullptr;
    size_t remaining = 0;
};

static size_t byteArrayReadCallback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t maxBytes = size * nmemb;
    UploadBuffer *ub = static_cast<UploadBuffer*>(userdata);
    if (!ub || ub->remaining == 0) return 0;

    size_t toCopy = std::min(maxBytes, ub->remaining);
    std::memcpy(ptr, ub->data, toCopy);
    ub->data += toCopy;
    ub->remaining -= toCopy;
    return toCopy;
}

RemoteClient::RemoteClient() {
}

RemoteClient::RemoteClient(const RemoteConfig &config)
    : m_config(config)
{
}

QString RemoteClient::buildUrl(const QString &remotePath) const {
    QString p = remotePath;
    while (p.startsWith(QLatin1Char('/'))) {
        p.remove(0, 1);
    }
    return QStringLiteral("%1://%2:%3/%4")
           .arg(m_config.protocolString())
           .arg(m_config.host)
           .arg(m_config.port)
           .arg(p);
}

bool RemoteClient::listDirectory(const QString &remotePath, QList<RemoteItem> &items, QString &errorMsg) {
    items.clear();
    errorMsg.clear();

    CURL *curl = curl_easy_init();
    if (!curl) {
        errorMsg = QStringLiteral("Failed to initialize cURL.");
        return false;
    }

    QString targetPath = remotePath;
    if (!targetPath.endsWith(QLatin1Char('/'))) {
        targetPath.append(QLatin1Char('/'));
    }

    QString url = buildUrl(targetPath);
    QString rawListing;

    curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, stringWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &rawListing);
    curl_easy_setopt(curl, CURLOPT_DIRLISTONLY, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    if (!m_config.username.isEmpty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, m_config.username.toUtf8().constData());
    }
    if (!m_config.password.isEmpty()) {
        curl_easy_setopt(curl, CURLOPT_PASSWORD, m_config.password.toUtf8().constData());
    }
    if (!m_config.privateKeyPath.isEmpty()) {
        curl_easy_setopt(curl, CURLOPT_SSH_PRIVATE_KEYFILE, m_config.privateKeyPath.toUtf8().constData());
    }

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        errorMsg = QString::fromUtf8(curl_easy_strerror(res));
        return false;
    }

    parseDirectoryListing(rawListing, targetPath, items);
    return true;
}

void RemoteClient::parseDirectoryListing(const QString &rawListing, const QString &parentPath, QList<RemoteItem> &items) {
    QString cleanParent = parentPath;
    if (!cleanParent.endsWith(QLatin1Char('/'))) {
        cleanParent.append(QLatin1Char('/'));
    }

    QStringList lines = rawListing.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        // Parse UNIX ls -l line:
        // drwxr-xr-x 2 root root 4096 Sep 20 12:00 dirName
        // -rw-r--r-- 1 root root  123 Sep 20 12:00 fileName
        bool isDir = trimmed.startsWith(QLatin1Char('d'));

        QStringList parts = trimmed.split(QChar::fromLatin1(' '), Qt::SkipEmptyParts);
        if (parts.size() >= 9) {
            QString name = parts.mid(8).join(QLatin1Char(' '));
            if (name == QStringLiteral(".") || name == QStringLiteral("..")) {
                continue;
            }

            RemoteItem item;
            item.name = name;
            item.fullPath = cleanParent + name;
            item.isDir = isDir;
            item.size = parts[4].toLongLong();
            items.append(item);
        } else {
            // Fallback for simple names
            if (trimmed == QStringLiteral(".") || trimmed == QStringLiteral("..")) {
                continue;
            }
            RemoteItem item;
            item.name = trimmed;
            item.fullPath = cleanParent + trimmed;
            item.isDir = false;
            items.append(item);
        }
    }

    // Sort directories first, then files alphabetically
    std::sort(items.begin(), items.end(), [](const RemoteItem &a, const RemoteItem &b) {
        if (a.isDir != b.isDir) return a.isDir;
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });
}

bool RemoteClient::downloadFile(const QString &remotePath, QByteArray &data, QString &errorMsg) {
    data.clear();
    errorMsg.clear();

    CURL *curl = curl_easy_init();
    if (!curl) {
        errorMsg = QStringLiteral("Failed to initialize cURL.");
        return false;
    }

    QString url = buildUrl(remotePath);

    curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, byteArrayWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    if (!m_config.username.isEmpty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, m_config.username.toUtf8().constData());
    }
    if (!m_config.password.isEmpty()) {
        curl_easy_setopt(curl, CURLOPT_PASSWORD, m_config.password.toUtf8().constData());
    }
    if (!m_config.privateKeyPath.isEmpty()) {
        curl_easy_setopt(curl, CURLOPT_SSH_PRIVATE_KEYFILE, m_config.privateKeyPath.toUtf8().constData());
    }

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        errorMsg = QString::fromUtf8(curl_easy_strerror(res));
        return false;
    }

    return true;
}

bool RemoteClient::uploadFile(const QString &remotePath, const QByteArray &data, QString &errorMsg) {
    errorMsg.clear();

    CURL *curl = curl_easy_init();
    if (!curl) {
        errorMsg = QStringLiteral("Failed to initialize cURL.");
        return false;
    }

    QString url = buildUrl(remotePath);

    UploadBuffer ub;
    ub.data = data.constData();
    ub.remaining = (size_t)data.size();

    curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, byteArrayReadCallback);
    curl_easy_setopt(curl, CURLOPT_READDATA, &ub);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)data.size());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    if (!m_config.username.isEmpty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, m_config.username.toUtf8().constData());
    }
    if (!m_config.password.isEmpty()) {
        curl_easy_setopt(curl, CURLOPT_PASSWORD, m_config.password.toUtf8().constData());
    }
    if (!m_config.privateKeyPath.isEmpty()) {
        curl_easy_setopt(curl, CURLOPT_SSH_PRIVATE_KEYFILE, m_config.privateKeyPath.toUtf8().constData());
    }

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        errorMsg = QString::fromUtf8(curl_easy_strerror(res));
        return false;
    }

    return true;
}

bool RemoteClient::createDirectory(const QString &remotePath, QString &errorMsg) {
    errorMsg.clear();

    CURL *curl = curl_easy_init();
    if (!curl) return false;

    QString url = buildUrl(QStringLiteral("/"));
    struct curl_slist *headerList = nullptr;

    QString cmd;
    if (m_config.protocol == RemoteConfig::SFTP) {
        cmd = QStringLiteral("mkdir \"%1\"").arg(remotePath);
    } else {
        cmd = QStringLiteral("MKD %1").arg(remotePath);
    }

    headerList = curl_slist_append(headerList, cmd.toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_QUOTE, headerList);

    if (!m_config.username.isEmpty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, m_config.username.toUtf8().constData());
    }
    if (!m_config.password.isEmpty()) {
        curl_easy_setopt(curl, CURLOPT_PASSWORD, m_config.password.toUtf8().constData());
    }

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headerList);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        errorMsg = QString::fromUtf8(curl_easy_strerror(res));
        return false;
    }
    return true;
}

bool RemoteClient::removeFile(const QString &remotePath, QString &errorMsg) {
    errorMsg.clear();

    CURL *curl = curl_easy_init();
    if (!curl) return false;

    QString url = buildUrl(QStringLiteral("/"));
    struct curl_slist *headerList = nullptr;

    QString cmd;
    if (m_config.protocol == RemoteConfig::SFTP) {
        cmd = QStringLiteral("rm \"%1\"").arg(remotePath);
    } else {
        cmd = QStringLiteral("DELE %1").arg(remotePath);
    }

    headerList = curl_slist_append(headerList, cmd.toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_QUOTE, headerList);

    if (!m_config.username.isEmpty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, m_config.username.toUtf8().constData());
    }
    if (!m_config.password.isEmpty()) {
        curl_easy_setopt(curl, CURLOPT_PASSWORD, m_config.password.toUtf8().constData());
    }

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headerList);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        errorMsg = QString::fromUtf8(curl_easy_strerror(res));
        return false;
    }
    return true;
}

bool RemoteClient::removeDirectory(const QString &remotePath, QString &errorMsg) {
    errorMsg.clear();

    CURL *curl = curl_easy_init();
    if (!curl) return false;

    QString url = buildUrl(QStringLiteral("/"));
    struct curl_slist *headerList = nullptr;

    QString cmd;
    if (m_config.protocol == RemoteConfig::SFTP) {
        cmd = QStringLiteral("rmdir \"%1\"").arg(remotePath);
    } else {
        cmd = QStringLiteral("RMD %1").arg(remotePath);
    }

    headerList = curl_slist_append(headerList, cmd.toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
    curl_easy_setopt(curl, CURLOPT_QUOTE, headerList);

    if (!m_config.username.isEmpty()) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, m_config.username.toUtf8().constData());
    }
    if (!m_config.password.isEmpty()) {
        curl_easy_setopt(curl, CURLOPT_PASSWORD, m_config.password.toUtf8().constData());
    }

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headerList);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        errorMsg = QString::fromUtf8(curl_easy_strerror(res));
        return false;
    }
    return true;
}

} // namespace UberPad
