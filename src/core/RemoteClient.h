#pragma once

#include <QString>
#include <QList>
#include <QByteArray>
#include <QDateTime>

namespace UberPad {

struct RemoteConfig {
    enum Protocol {
        SFTP,
        FTP,
        FTPS
    };

    Protocol protocol = SFTP;
    QString host;
    int port = 22;
    QString username;
    QString password;
    QString privateKeyPath;
    QString rootDir = QStringLiteral("/");

    QString protocolString() const {
        if (protocol == FTP) return QStringLiteral("ftp");
        if (protocol == FTPS) return QStringLiteral("ftps");
        return QStringLiteral("sftp");
    }
};

struct RemoteItem {
    QString name;
    QString fullPath;
    bool isDir = false;
    qint64 size = 0;
};

class RemoteClient {
public:
    RemoteClient();
    explicit RemoteClient(const RemoteConfig &config);
    ~RemoteClient() = default;

    void setConfig(const RemoteConfig &config) { m_config = config; }
    RemoteConfig config() const { return m_config; }

    bool listDirectory(const QString &remotePath, QList<RemoteItem> &items, QString &errorMsg);
    bool downloadFile(const QString &remotePath, QByteArray &data, QString &errorMsg);
    bool uploadFile(const QString &remotePath, const QByteArray &data, QString &errorMsg);
    bool createDirectory(const QString &remotePath, QString &errorMsg);
    bool removeFile(const QString &remotePath, QString &errorMsg);
    bool removeDirectory(const QString &remotePath, QString &errorMsg);

    QString buildUrl(const QString &remotePath) const;

private:
    void parseDirectoryListing(const QString &rawListing, const QString &parentPath, QList<RemoteItem> &items);

    RemoteConfig m_config;
};

} // namespace UberPad
