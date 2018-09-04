#include "TelegramServerUser.hpp"

#include "CTelegramStream.hpp"
#include "CTelegramStreamExtraOperators.hpp"
#include "ServerRpcLayer.hpp"
#include "TelegramServerClient.hpp"
#include "Utils.hpp"

#include <QDateTime>
#include <QLoggingCategory>

namespace Telegram {

namespace Server {

User::User(QObject *parent) :
    QObject(parent)
{
}

void User::setPhoneNumber(const QString &phoneNumber)
{
    m_phoneNumber = phoneNumber;
    m_id = qHash(m_phoneNumber);
}

void User::setFirstName(const QString &firstName)
{
    m_firstName = firstName;
}

void User::setLastName(const QString &lastName)
{
    m_lastName = lastName;
}

bool User::isOnline() const
{
    return true;
}

void User::setDcId(quint32 id)
{
    m_dcId = id;
}

Session *User::getSession(quint64 authId) const
{
    for (Session *s : m_sessions) {
        if (s->authId == authId) {
            return s;
        }
    }
    return nullptr;
}

QVector<Session *> User::activeSessions() const
{
    QVector<Session *> result;
    for (Session *s : m_sessions) {
        if (s->getConnection()) {
            result.append(s);
        }
    }
    return result;
}

void User::addSession(Session *session)
{
    m_sessions.append(session);
    session->setUser(this);
    emit sessionAdded(m_sessions.last());
}

void User::setPlainPassword(const QString &password)
{
    if (password.isEmpty()) {
        m_passwordSalt.clear();
        m_passwordHash.clear();
        return;
    }
    QByteArray pwdSalt(8, Qt::Uninitialized);
    Utils::randomBytes(&pwdSalt);
    const QByteArray pwdData = pwdSalt + password.toUtf8() + pwdSalt;
    const QByteArray pwdHash = Utils::sha256(pwdData);
    setPassword(pwdSalt, pwdHash);
}

void User::setPassword(const QByteArray &salt, const QByteArray &hash)
{
    qDebug() << Q_FUNC_INFO << "salt:" << salt.toHex();
    qDebug() << Q_FUNC_INFO << "hash:" << hash.toHex();

    m_passwordSalt = salt;
    m_passwordHash = hash;
}

TLPeer User::toPeer() const
{
    TLPeer p;
    p.tlType = TLValue::PeerUser;
    p.userId = id();
    return p;
}

quint32 User::addMessage(RemoteUser *sender, const QString &text)
{
    ++m_pts;
    TLMessage newMessage;
    newMessage.tlType = TLValue::Message;
    newMessage.id = m_pts;
    newMessage.message = text;
    newMessage.fromId = sender->id();
    newMessage.toId = toPeer();
    newMessage.date = static_cast<quint32>(QDateTime::currentSecsSinceEpoch());
    m_messages.append(newMessage);

    const auto sessions = activeSessions();

    TLUpdates updates;
    updates.tlType = TLValue::UpdateShortMessage;
    updates.message = newMessage.message;
    updates.fromId = newMessage.fromId;
    updates.date = newMessage.date;
    updates.pts = m_pts;
    updates.ptsCount = m_pts;

    CTelegramStream stream(CTelegramStream::WriteOnly);
    stream << updates;
    const QByteArray payload = stream.getData();

    for (const Session *session : sessions) {
        session->getConnection()->rpcLayer()->sendRpcMessage(payload);
    }

    return m_pts;
}

quint32 User::sendMessage(RemoteUser *recipient, const QString &text)
{
    return recipient->addMessage(this, text);
}

} // Server

} // Telegram
