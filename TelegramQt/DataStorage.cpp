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

#include "DataStorage_p.hpp"

namespace Telegram {

namespace Client {

/*!
    \class Telegram::Client::DataStorage
    \brief The DataStorage class provides a basic interface for session
           data management
    \inmodule TelegramQt
    \ingroup Client

    \sa AccountStorage
*/

DataStorage::DataStorage(QObject *parent) :
    DataStorage(new DataStoragePrivate(), parent)
{
    Q_D(DataStorage);
    d->m_api = new DataInternalApi(this);
}

DataInternalApi *DataStorage::internalApi()
{
    Q_D(DataStorage);
    return d->m_api;
}

DcConfiguration DataStorage::serverConfiguration() const
{
    Q_D(const DataStorage);
    return d->m_serverConfig;
}

void DataStorage::setServerConfiguration(const DcConfiguration &configuration)
{
    Q_D(DataStorage);
    d->m_serverConfig = configuration;
}

QVector<Peer> DataStorage::dialogs() const
{
    Q_D(const DataStorage);
    const auto dialogs = d->m_api->m_dialogs;
    QVector<Peer> result;
    result.reserve(dialogs.count);
    for (const TLDialog &dialog : dialogs.dialogs) {
        result.append(DataInternalApi::toPublicPeer(dialog.peer));
    }
    return result;
}

DataStorage::DataStorage(DataStoragePrivate *d, QObject *parent)
    : QObject(parent),
      d_ptr(d)
{
}

InMemoryDataStorage::InMemoryDataStorage(QObject *parent) :
    DataStorage(parent)
{
}

DataInternalApi::DataInternalApi(QObject *parent) :
    QObject(parent)
{
}

void DataInternalApi::processDialogs(const TLMessagesDialogs &dialogs)
{
    m_dialogs = dialogs;
}

Peer DataInternalApi::toPublicPeer(const TLPeer &peer)
{
    switch (peer.tlType) {
    case TLValue::PeerChat:
        return Telegram::Peer(peer.chatId, Telegram::Peer::Chat);
    case TLValue::PeerChannel:
        return Telegram::Peer(peer.channelId, Telegram::Peer::Channel);
    case TLValue::PeerUser:
        return Telegram::Peer(peer.userId);
    default:
        return Telegram::Peer();
    }
}

//QVector<Peer> InMemoryDataStorage::dialogs() const
//{
//    return {};
//}

} // Client namespace

} // Telegram namespace
