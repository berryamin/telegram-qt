/*
   Copyright (C) 2017 Alexandr Akulich <akulichalexander@gmail.com>

   This file is a part of TelegramQt library.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

 */

#include <QObject>

#include "AccountStorage.hpp"
#include "Client.hpp"
#include "ClientSettings.hpp"
#include "DataStorage.hpp"
#include "Utils.hpp"
#include "TelegramNamespace.hpp"
#include "CAppInformation.hpp"
#include "CRawStream.hpp"

#include "Operations/ClientAuthOperation.hpp"

#include "TelegramServer.hpp"
#include "TelegramServerClient.hpp"
#include "TelegramServerUser.hpp"
#include "ServerRpcLayer.hpp"
#include "DcConfiguration.hpp"
#include "LocalCluster.hpp"

#include <QTest>
#include <QSignalSpy>
#include <QDebug>
#include <QRegularExpression>
#include <QTemporaryFile>

#include "keys_data.hpp"

#define CLIENT_WORKS

using namespace Telegram;

constexpr int c_timeout = 500;

struct UserData
{
    quint32 dcId;
    QString phoneNumber;
    QString firstName;
    QString lastName;
    QString password;
    QByteArray passwordSalt;
    QByteArray passwordHash;

    void setName(const QString &first, const QString &last)
    {
        firstName = first;
        lastName = last;
    }

    void unsetPassword()
    {
        setPassword(QString(), QByteArray());
    }

    void setPassword(const QString &pass, const QByteArray &salt)
    {
        password = pass;
        setPasswordSalt(salt);
    }

    void setPasswordSalt(const QByteArray &salt)
    {
        if (salt.isEmpty()) {
            passwordSalt.clear();
            passwordHash.clear();
            return;
        }
        QByteArray pwdData = salt + password.toUtf8() + salt;
        passwordSalt = salt;
        passwordHash = Utils::sha256(pwdData);
    }
};

Q_DECLARE_METATYPE(UserData);

CAppInformation *getAppInfo()
{
    static CAppInformation *appInfo = nullptr;
    if (!appInfo) {
        appInfo = new CAppInformation();
        appInfo->setAppId(14617);
        appInfo->setAppHash(QLatin1String("e17ac360fd072f83d5d08db45ce9a121"));
        appInfo->setAppVersion(QLatin1String("0.1"));
        appInfo->setDeviceInfo(QLatin1String("pc"));
        appInfo->setOsInfo(QLatin1String("GNU/Linux"));
        appInfo->setLanguageCode(QLatin1String("en"));
    }
    return appInfo;
}

Telegram::Server::User *tryAddUser(Telegram::Server::LocalCluster *cluster, const UserData &data)
{
    Telegram::Server::User *u = cluster->addUser(data.phoneNumber, data.dcId);
    if (u) {
        u->setFirstName(data.firstName);
        u->setLastName(data.lastName);
        if (!data.passwordHash.isEmpty()) {
            u->setPassword(data.passwordSalt, data.passwordHash);
        } else {
            u->setPlainPassword(data.password);
        }
    } else {
        qCritical() << "Unable to add a user";
    }
    return u;
}

class TestServer : public Server::Server
{
    Q_OBJECT
public:
    explicit TestServer(QObject *parent = nullptr)
        : Server(parent)
    {
    }

    QByteArray sendAppCode(const QString &identifier) override
    {
        const QByteArray hash = Server::Server::sendAppCode(identifier);
        const Telegram::Server::AuthCode c = m_sentCodeMap.value(identifier);
        emit authCodeSent(identifier, c.code);
        return hash;
    }

signals:
    void authCodeSent(const QString &identifier, const QString &code);
};

static const UserData c_userWithPassword = []() {
    UserData userData;
    userData.dcId = 1;
    userData.setName(QStringLiteral("First"), QStringLiteral("Last"));
    userData.phoneNumber = QStringLiteral("123456");
    userData.password = QByteArrayLiteral("mypassword");
    userData.setPasswordSalt(QByteArrayLiteral("mysalt"));
    return userData;
}();

static const QVector<Telegram::DcOption> c_localDcOptions = {
    Telegram::DcOption(QStringLiteral("127.0.0.1"), 11441, 1),
    Telegram::DcOption(QStringLiteral("127.0.0.2"), 11442, 2),
    Telegram::DcOption(QStringLiteral("127.0.0.3"), 11443, 3),
};

static const Telegram::DcConfiguration c_localDcConfiguration = []() {
    Telegram::DcConfiguration configuration;
    configuration.dcOptions = c_localDcOptions;
    return configuration;
}();

static const Telegram::Server::LocalCluster::ServerConstructor c_testServerConstructor = [](QObject *parent) -> Telegram::Server::Server* {
    return new TestServer(parent);
};

class tst_all : public QObject
{
    Q_OBJECT
public:
    explicit tst_all(QObject *parent = nullptr);

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testClientConnection_data();
    void testClientConnection();

    void testAccountStorage();
};

tst_all::tst_all(QObject *parent) :
    QObject(parent)
{
}

void tst_all::initTestCase()
{
    qRegisterMetaType<UserData>();
    Telegram::initialize();
    QVERIFY(TestKeyData::initKeyFiles());
}

void tst_all::cleanupTestCase()
{
    QVERIFY(TestKeyData::cleanupKeyFiles());
}

void tst_all::testClientConnection_data()
{
    QTest::addColumn<Telegram::Client::Settings::SessionType>("sessionType");
    QTest::addColumn<UserData>("userData");
    QTest::addColumn<DcOption>("clientDcOption");
    UserData userOnDc1 = c_userWithPassword;
    userOnDc1.dcId = 1;
    UserData userOnDc2 = c_userWithPassword;
    userOnDc2.dcId = 2;
    UserData user2OnDc2 = c_userWithPassword;
    user2OnDc2.unsetPassword();
    user2OnDc2.dcId = 2;

    DcOption opt = c_localDcOptions.first();

    QTest::newRow("Abridged")   << Client::Settings::SessionType::Abridged
                                << userOnDc1
                                << opt;
    QTest::newRow("Obfuscated") << Client::Settings::SessionType::Obfuscated
                                << userOnDc1
                                << opt;
    QTest::newRow("Abridged with migration")   << Client::Settings::SessionType::Abridged
                                               << userOnDc2
                                               << opt;
    QTest::newRow("Obfuscated with migration") << Client::Settings::SessionType::Obfuscated
                                               << userOnDc2
                                               << opt;

    opt.id = 0;
    QTest::newRow("Migration from unknown dc (with password)") << Client::Settings::SessionType::Obfuscated
                                               << userOnDc2
                                               << opt;
    QTest::newRow("Migration from unknown dc, no password") << Client::Settings::SessionType::Obfuscated
                                               << user2OnDc2
                                               << opt;
}

void tst_all::testClientConnection()
{
    return;
    QFETCH(Telegram::Client::Settings::SessionType, sessionType);
    QFETCH(UserData, userData);
    QFETCH(DcOption, clientDcOption);

    const RsaKey publicKey = Utils::loadRsaKeyFromFile(TestKeyData::publicKeyFileName());
    QVERIFY2(publicKey.isValid(), "Unable to read public RSA key");
    const RsaKey privateKey = Utils::loadRsaPrivateKeyFromFile(TestKeyData::privateKeyFileName());
    QVERIFY2(privateKey.isValid(), "Unable to read private RSA key");

    Telegram::Server::LocalCluster cluster;
    cluster.setServerContructor(c_testServerConstructor);
    cluster.setServerPrivateRsaKey(privateKey);
    cluster.setServerConfiguration(c_localDcConfiguration);
    QVERIFY(cluster.start());

    Server::User *user = tryAddUser(&cluster, userData);
    QVERIFY(user);

    Client::Client client;
    Client::AccountStorage accountStorage;
    Client::Settings clientSettings;
    Client::InMemoryDataStorage dataStorage;
    client.setAppInformation(getAppInfo());
    client.setSettings(&clientSettings);
    client.setAccountStorage(&accountStorage);
    client.setDataStorage(&dataStorage);
    accountStorage.setPhoneNumber(userData.phoneNumber);
    QVERIFY(clientSettings.setServerConfiguration({clientDcOption}));
    QVERIFY(clientSettings.setServerRsaKey(publicKey));
    clientSettings.setPreferedSessionType(sessionType);

    // --- Connect ---
    PendingOperation *connectOperation = client.connectToServer();
    QTest::ignoreMessage(QtDebugMsg, QRegularExpression(QStringLiteral("%1 \\d* \"%2\"")
                                                        .arg(QString::fromLatin1(Server::RpcLayer::gzipPackMessage()))
                                                        .arg(TLValue(TLValue::Config).toString())));
    QTRY_VERIFY(connectOperation->isSucceeded());

    TestServer *server = qobject_cast<TestServer*>(cluster.getServerInstance(userData.dcId));
    QVERIFY(server);

    QCOMPARE(dataStorage.serverConfiguration().dcOptions, cluster.serverConfiguration().dcOptions);

    // --- Sign in ---
    QSignalSpy accountStorageSynced(&accountStorage, &Client::AccountStorage::synced);
    Client::AuthOperation *signInOperation = client.signIn();
    QSignalSpy serverAuthCodeSpy(server, &TestServer::authCodeSent);

    QSignalSpy authCodeSpy(signInOperation, &Client::AuthOperation::authCodeRequired);
    QTRY_VERIFY(!authCodeSpy.isEmpty());
    QCOMPARE(authCodeSpy.count(), 1);
    QCOMPARE(serverAuthCodeSpy.count(), 1);
    QList<QVariant> authCodeSentArguments = serverAuthCodeSpy.takeFirst();
    QCOMPARE(authCodeSentArguments.count(), 2);
    const QString authCode = authCodeSentArguments.at(1).toString();

    signInOperation->submitAuthCode(authCode);

    if (!userData.password.isEmpty()) {
        QSignalSpy authPasswordSpy(signInOperation, &Client::AuthOperation::passwordRequired);
        QSignalSpy passwordCheckFailedSpy(signInOperation, &Client::AuthOperation::passwordCheckFailed);
        QTRY_VERIFY2(!authPasswordSpy.isEmpty(), "The user has a password-protection, "
                                                 "but there are no passwordRequired signals on the client side");
        QCOMPARE(authPasswordSpy.count(), 1);
        QVERIFY(passwordCheckFailedSpy.isEmpty());

        PendingOperation *op = signInOperation->getPassword();
        QTRY_VERIFY(op->isFinished());

        signInOperation->submitPassword(userData.password + QStringLiteral("invalid"));
        QTRY_VERIFY2(!passwordCheckFailedSpy.isEmpty(), "The submitted password is not valid, "
                                                        "but there are not signals on the client side");
        QVERIFY(!signInOperation->isFinished());
        QCOMPARE(passwordCheckFailedSpy.count(), 1);

        signInOperation->submitPassword(userData.password);
    }
    QTRY_VERIFY2(signInOperation->isSucceeded(), "Unexpected sign in fail");
    QTRY_VERIFY_WITH_TIMEOUT(!accountStorageSynced.isEmpty(), c_timeout);

    quint64 clientAuthId = accountStorage.authId();
    QVERIFY(clientAuthId);
    QSet<Server::RemoteClientConnection*> clientConnections = server->getConnections();
    QCOMPARE(clientConnections.count(), 1);
    Server::RemoteClientConnection *remoteClientConnection = *clientConnections.cbegin();
    QCOMPARE(remoteClientConnection->authId(), clientAuthId);
    QCOMPARE(accountStorage.phoneNumber(), userData.phoneNumber);
    QCOMPARE(accountStorage.dcInfo().id, server->dcId());
}

void tst_all::testAccountStorage()
{
    const Telegram::Client::Settings::SessionType sessionType = Telegram::Client::Settings::SessionType::Obfuscated;
    const UserData userData = c_userWithPassword;
    const DcOption clientDcOption = c_localDcOptions.first();

    const RsaKey publicKey = Utils::loadRsaKeyFromFile(TestKeyData::publicKeyFileName());
    QVERIFY2(publicKey.isValid(), "Unable to read public RSA key");
    const RsaKey privateKey = Utils::loadRsaPrivateKeyFromFile(TestKeyData::privateKeyFileName());
    QVERIFY2(privateKey.isValid(), "Unable to read private RSA key");

    QTemporaryFile accountV1File;
    accountV1File.setFileTemplate(accountV1File.fileTemplate() + QStringLiteral(".secret"));
    QVERIFY2(accountV1File.open(), "Unable to create a temporary file"); // Touch the file

//    {
//        const quint32 secretFormatVersion = 4;
//        const qint32 deltaTime = 0;

//        CRawStreamEx outputStream(&accountV1File);
//        outputStream << secretFormatVersion;
//        outputStream << deltaTime;
//        outputStream << clientDcOption.id;
//        outputStream << clientDcOption.address.toLatin1();
//        outputStream << clientDcOption.port;
//        //outputStream << authKey;
//        //outputStream << authId;
//    }
    accountV1File.close();

    Telegram::Server::LocalCluster cluster;
    cluster.setServerContructor(c_testServerConstructor);
    cluster.setServerPrivateRsaKey(privateKey);
    cluster.setServerConfiguration(c_localDcConfiguration);
    QVERIFY(cluster.start());

    Server::User *user = tryAddUser(&cluster, userData);
    QVERIFY(user);

    Client::Client client;
    Client::FileAccountStorage accountStorage;
    Client::Settings clientSettings;
    Client::InMemoryDataStorage dataStorage;
    client.setAppInformation(getAppInfo());
    client.setSettings(&clientSettings);
    client.setAccountStorage(&accountStorage);
    client.setDataStorage(&dataStorage);
    accountStorage.setFileName(accountV1File.fileName());
    accountV1File.remove();

    QVERIFY(clientSettings.setServerConfiguration({clientDcOption}));
    QVERIFY(clientSettings.setServerRsaKey(publicKey));
    clientSettings.setPreferedSessionType(sessionType);

    // --- Connect ---
    PendingOperation *connectOperation = client.connectToServer();
    QTest::ignoreMessage(QtDebugMsg, QRegularExpression(QStringLiteral("%1 \\d* \"%2\"")
                                                        .arg(QString::fromLatin1(Server::RpcLayer::gzipPackMessage()))
                                                        .arg(TLValue(TLValue::Config).toString())));
    QTRY_VERIFY(connectOperation->isSucceeded());

    TestServer *server = qobject_cast<TestServer*>(cluster.getServerInstance(userData.dcId));
    QVERIFY(server);

    QCOMPARE(dataStorage.serverConfiguration().dcOptions, cluster.serverConfiguration().dcOptions);

    // --- Sign in ---
    QSignalSpy accountStorageSynced(&accountStorage, &Client::AccountStorage::synced);
    Client::AuthOperation *signInOperation = client.signIn();
    signInOperation->setPhoneNumber(userData.phoneNumber);
    QSignalSpy serverAuthCodeSpy(server, &TestServer::authCodeSent);

    QSignalSpy authCodeSpy(signInOperation, &Client::AuthOperation::authCodeRequired);
    QTRY_VERIFY(!authCodeSpy.isEmpty());
    QCOMPARE(authCodeSpy.count(), 1);
    QCOMPARE(serverAuthCodeSpy.count(), 1);
    QList<QVariant> authCodeSentArguments = serverAuthCodeSpy.takeFirst();
    QCOMPARE(authCodeSentArguments.count(), 2);
    const QString authCode = authCodeSentArguments.at(1).toString();

    signInOperation->submitAuthCode(authCode);

    if (!userData.password.isEmpty()) {
        QSignalSpy authPasswordSpy(signInOperation, &Client::AuthOperation::passwordRequired);
        QSignalSpy passwordCheckFailedSpy(signInOperation, &Client::AuthOperation::passwordCheckFailed);
        QTRY_VERIFY2(!authPasswordSpy.isEmpty(), "The user has a password-protection, "
                                                 "but there are no passwordRequired signals on the client side");
        QCOMPARE(authPasswordSpy.count(), 1);
        QVERIFY(passwordCheckFailedSpy.isEmpty());

        PendingOperation *op = signInOperation->getPassword();
        QTRY_VERIFY(op->isFinished());

        signInOperation->submitPassword(userData.password + QStringLiteral("invalid"));
        QTRY_VERIFY2(!passwordCheckFailedSpy.isEmpty(), "The submitted password is not valid, "
                                                        "but there are not signals on the client side");
        QVERIFY(!signInOperation->isFinished());
        QCOMPARE(passwordCheckFailedSpy.count(), 1);

        signInOperation->submitPassword(userData.password);
    }
    QTRY_VERIFY2(signInOperation->isSucceeded(), "Unexpected sign in fail");
    QTRY_VERIFY_WITH_TIMEOUT(!accountStorageSynced.isEmpty(), c_timeout);

    quint64 clientAuthId = accountStorage.authId();
    QVERIFY(clientAuthId);
    QSet<Server::RemoteClientConnection*> clientConnections = server->getConnections();
    QCOMPARE(clientConnections.count(), 1);
    Server::RemoteClientConnection *remoteClientConnection = *clientConnections.cbegin();
    QCOMPARE(remoteClientConnection->authId(), clientAuthId);
    QCOMPARE(accountStorage.phoneNumber(), userData.phoneNumber);
    QCOMPARE(accountStorage.dcInfo().id, server->dcId());

    accountV1File.open();
}

QTEST_GUILESS_MAIN(tst_all)

#include "tst_all.moc"
