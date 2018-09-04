/*
   Copyright (C) 2018 Alexander Akulich <akulichalexander@gmail.com>

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

#ifndef TELEGRAMQT_DATA_STORAGE_HPP
#define TELEGRAMQT_DATA_STORAGE_HPP

#include <QObject>
#include <QHash>

#include "DcConfiguration.hpp"
#include "TelegramNamespace.hpp"

#include "TLTypes.hpp"


namespace Telegram {

class PendingOperation;

namespace Client {

class DataStoragePrivate;

class DataInternalApi : public QObject
{
    Q_OBJECT
public:
    explicit DataInternalApi(QObject *parent = nullptr);
    ~DataInternalApi() override;

    const TLUser *getSelfUser() const;

    void processData(const TLUser &user);
    void processData(const TLAuthAuthorization &authorization);
    void processData(const TLMessagesDialogs &dialogs);

    quint32 selfUserId() const { return m_selfUserId; }

    // Getters
    // const TLUser *getUser(quint32 userId) const;
    // const TLChat *getChat(const Telegram::Peer &peer) const;
    // const TLMessage *getMessage(quint32 messageId, const Telegram::Peer &peer) const;

    // TLInputPeer toInputPeer(const Telegram::Peer &peer) const;
    // Telegram::Peer toPublicPeer(const TLInputPeer &inputPeer) const;
    static Telegram::Peer toPublicPeer(const TLPeer &peer);
    // Telegram::Peer toPublicPeer(const TLUser &user) const;
    // Telegram::Peer toPublicPeer(const TLChat *chat) const;
    // TLPeer toTLPeer(const Telegram::Peer &peer) const;
    TLInputUser toInputUser(quint32 userId) const;
    // TLInputChannel toInputChannel(const Telegram::Peer &peer);
    // TLInputChannel toInputChannel(const TLChat *chat);
    // TLInputChannel toInputChannel(const TLDialog &dialog);

    quint32 m_selfUserId = 0;
    QHash<quint32, TLUser*> m_users;
    TLMessagesDialogs m_dialogs;
};

class DataStorage : public QObject
{
    Q_OBJECT
public:
    explicit DataStorage(QObject *parent = nullptr);

    DataInternalApi *internalApi();

    DcConfiguration serverConfiguration() const;
    void setServerConfiguration(const DcConfiguration &configuration);

    QVector<Telegram::Peer> dialogs() const;

    //bool getDialogInfo(DialogInfo *info, const Peer &peer) const;
//    bool getUserInfo(UserInfo *info, quint32 userId) const;
//    bool getChatInfo(ChatInfo *info, const Peer peer) const;
//    bool getChatParticipants(QVector<quint32> *participants, quint32 chatId);

protected:
    DataStorage(DataStoragePrivate *d, QObject *parent);
    DataStoragePrivate *d_ptr;
    Q_DECLARE_PRIVATE(DataStorage)

};

class InMemoryDataStorage : public DataStorage
{
    Q_OBJECT
public:
    explicit InMemoryDataStorage(QObject *parent = nullptr);

//    QVector<Telegram::Peer> dialogs() const override;

};

} // Client namespace

} // Telegram namespace

#endif // TELEGRAMQT_DATA_STORAGE_HPP
