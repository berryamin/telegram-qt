#include "AccountStorage.hpp"
#include "LegacySecretReader.hpp"
#include "ClientBackend.hpp"
#include "CRawStream.hpp"

#include <QFile>
#include <QLoggingCategory>
#include <QUrl>

namespace Telegram {

namespace Client {

class AccountStoragePrivate
{
public:
    explicit AccountStoragePrivate(AccountStorage *q) :
        m_parent(q)
    {
    }

    AccountStorage *m_parent = nullptr;
    QString m_accountIdentifier;
    QString m_phoneNumber;
    QByteArray m_authKey;
    quint64 m_authId = 0;
    qint32 m_deltaTime = 0;
    DcOption m_dcInfo;
};

/*!
    \class Telegram::Client::AccountStorage
    \brief The AccountStorage class provides a basic interface for account
           data management
    \inmodule TelegramQt
    \ingroup Client

    When subclassing AccountStorage you need to save at least authKey and
    dcInfo. It is highly recommended to also store deltaTime and authId.
    On authentication succeeded the storage can get phone number and some
    extra data from server to setup data storage and decrypt local cache.

    \sa DataStorage
*/

AccountStorage::AccountStorage(QObject *parent) :
    AccountStorage(new AccountStoragePrivate(this), parent)
{
}

bool AccountStorage::hasMinimalDataSet() const
{
    return !d->m_dcInfo.address.isEmpty() && !d->m_authKey.isEmpty();
}

QString AccountStorage::accountIdentifier() const
{
    return d->m_accountIdentifier;
}

void AccountStorage::setAccountIdentifier(const QString &account)
{
    d->m_accountIdentifier = account;
}

QString AccountStorage::phoneNumber() const
{
    return d->m_phoneNumber;
}

void AccountStorage::setPhoneNumber(const QString &phoneNumber) const
{
    d->m_phoneNumber = phoneNumber;
}

quint64 AccountStorage::authId() const
{
    return d->m_authId;
}

void AccountStorage::setAuthId(quint64 id)
{
    d->m_authId = id;
}

QByteArray AccountStorage::authKey() const
{
    return d->m_authKey;
}

void AccountStorage::setAuthKey(const QByteArray &newAuthKey)
{
    d->m_authKey = newAuthKey;
}

qint32 AccountStorage::deltaTime() const
{
    return d->m_deltaTime;
}

void AccountStorage::setDeltaTime(const qint32 newDt)
{
    d->m_deltaTime = newDt;
}

DcOption AccountStorage::dcInfo() const
{
    return d->m_dcInfo;
}

void AccountStorage::setDcInfo(const DcOption &newDcInfo)
{
    d->m_dcInfo = newDcInfo;
}

bool AccountStorage::sync()
{
    emit synced();
    return true;
}

AccountStorage::AccountStorage(AccountStoragePrivate *dd, QObject *parent) :
    QObject(parent),
    d(dd)
{
}

class FileAccountStoragePrivate : public AccountStoragePrivate
{
public:
    explicit FileAccountStoragePrivate(FileAccountStorage *q) :
        AccountStoragePrivate(q)
    {
    }

    QString m_fileName;
};

FileAccountStorage::FileAccountStorage(QObject *parent) :
    AccountStorage(new FileAccountStoragePrivate(this), parent)
{
}

QString FileAccountStorage::fileName() const
{
    TG_D(FileAccountStorage);
    return d->m_fileName;
}

bool FileAccountStorage::saveData() const
{
    TG_D(FileAccountStorage);
    if (d->m_fileName.isEmpty()) {
        return false;
    }

    QFile file(d->m_fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << Q_FUNC_INFO << "Unable to open file" << fileName();
        return false;
    }
    file.write("TelegramQt");
    CRawStreamEx outputStream(&file);
    outputStream << d->m_deltaTime;
    outputStream << d->m_dcInfo.id;
    outputStream << d->m_dcInfo.address.toLatin1();
    outputStream << d->m_dcInfo.port;
    outputStream << d->m_authKey;
    outputStream << d->m_authId;
    qDebug() << Q_FUNC_INFO << "Saved key" << QString::number(authId(), 0x10);
    return true;
}

bool FileAccountStorage::loadData()
{
    TG_D(FileAccountStorage);
    if (d->m_fileName.isEmpty()) {
        qDebug() << Q_FUNC_INFO << "File name is not set";
        return false;
    }
    QFile file(d->m_fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << Q_FUNC_INFO << "Unable to open file" << fileName();
        return false;
    }
    CRawStreamEx outputStream(&file);
    outputStream >> d->m_deltaTime;
    outputStream >> d->m_dcInfo.id;
    QByteArray address;
    outputStream >> address;
    d->m_dcInfo.address = QString::fromLatin1(address);
    outputStream >> d->m_dcInfo.port;
    outputStream >> d->m_authKey;
    outputStream >> d->m_authId;

    qDebug() << Q_FUNC_INFO << "Loaded key" << QString::number(authId(), 0x10);
    return outputStream.error();
}

bool FileAccountStorage::sync()
{
    saveData();
    return AccountStorage::sync();
}

void FileAccountStorage::setFileName(const QString &fileName)
{
    TG_D(FileAccountStorage);
    if (d->m_fileName == fileName) {
        return;
    }
    d->m_fileName = fileName;
    emit fileNameChanged(fileName);
}

} // Client

} // Telegram
