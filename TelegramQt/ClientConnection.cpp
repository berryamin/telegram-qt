#include "ClientConnection.hpp"
#include "ClientDhLayer.hpp"
#include "ClientRpcLayer.hpp"
#include "CTelegramTransport.hpp"
#include "ConnectionOperation.hpp"
#include "SendPackageHelper.hpp"
#include "TelegramUtils.hpp"
#include "Utils.hpp"
#include "Debug_p.hpp"

#include <QDateTime>
#include <QLoggingCategory>

namespace Telegram {

namespace Client {

class SendPackageHelper : public BaseSendPackageHelper
{
public:
    explicit SendPackageHelper(BaseConnection *connection) :
        BaseSendPackageHelper()
    {
        m_connection = connection;
    }

    quint64 newMessageId(SendMode mode) override
    {
        quint64 ts = TelegramUtils::formatTimeStamp(QDateTime::currentMSecsSinceEpoch() + deltaTime() * 1000);
        if (mode == SendMode::Client) {
            ts &= ~quint64(3);
        } else {
            qWarning() << Q_FUNC_INFO << "Invalid mode";
        }
        return m_connection->transport()->getNewMessageId(ts);
    }

    void sendPackage(const QByteArray &package) override
    {
        return m_connection->transport()->sendPackage(package);
    }
};

Connection::Connection(QObject *parent) :
    BaseConnection(parent)
{
    m_sendHelper = new SendPackageHelper(this);
    m_dhLayer = new DhLayer(this);
    m_dhLayer->setSendPackageHelper(m_sendHelper);
    connect(m_dhLayer, &BaseDhLayer::stateChanged, this, &Connection::onClientDhStateChanged);
    m_rpcLayer = new RpcLayer(this);
    m_rpcLayer->setSendPackageHelper(m_sendHelper);
}

void Connection::setDcOption(const DcOption &dcOption)
{
    m_dcOption = dcOption;
}

Connection *Connection::fromOperation(PendingRpcOperation *operation)
{
    return reinterpret_cast<Telegram::Client::Connection*>(operation->getConnection());
}

RpcLayer *Connection::rpcLayer()
{
    return reinterpret_cast<RpcLayer*>(m_rpcLayer);
}

ConnectOperation *Connection::connectToDc()
{
    if (m_status != Status::Disconnected) {
        const QString text = QStringLiteral("Connection is already in progress");
        return PendingOperation::failOperation<ConnectOperation>({
                                                                     { QStringLiteral("text"), text }
                                                                 },
                                                                 this);
    }

#ifdef DEVELOPER_BUILD
    qDebug() << Q_FUNC_INFO << m_dcOption.id << m_dcOption.address << m_dcOption.port;
#endif

    if (m_transport->state() != QAbstractSocket::UnconnectedState) {
        m_transport->disconnectFromHost(); // Ensure that there is no connection
    }

    ConnectOperation *op = new ConnectOperation(this);
    // TODO: Connect error to op->setFinishedWithError()

    setStatus(Status::Connecting, StatusReason::Local);
//    setAuthState(AuthStateNone);
    m_transport->connectToHost(m_dcOption.address, m_dcOption.port);

    connect(m_transport, &CTelegramTransport::errorOccurred, [op] (QAbstractSocket::SocketError error, const QString &text) {
        op->setFinishedWithError({
                                     { QStringLiteral("qtError"), error },
                                     { QStringLiteral("qtErrorText"), text },
                                 });
    });
    connect(this, &Connection::statusChanged, [op] (Status status, StatusReason reason) {
        Q_UNUSED(reason)

        if (status == Status::Authenticated) {
            op->setFinished();
        }
    });
//    connect(m_transport, &CTelegramTransport::stateChanged, [op] (QAbstractSocket::SocketState transportState) {
//        if (transportState == QAbstractSocket::ConnectedState) {
//            op->setFinished();
//        }
//    });

    return op;
}

void Connection::processSeeOthers(PendingRpcOperation *operation)
{
    if (m_status == Status::Disconnected) {
        connectToDc();
    }
    if (m_dhLayer->state() != DhLayer::State::HasKey) {
        qWarning() << Q_FUNC_INFO << "queue operation:" << TLValue::firstFromArray(operation->requestData());
        m_queuedOperations.append(operation);
        return;
    }
    qWarning() << Q_FUNC_INFO << "processSeeOthers:" << TLValue::firstFromArray(operation->requestData());
    rpcLayer()->sendRpc(operation);
}

void Connection::onClientDhStateChanged()
{
    qWarning() << Q_FUNC_INFO << m_dcOption.id << m_dcOption.address << "DH status:" << m_dhLayer->state();
    if (m_dhLayer->state() == BaseDhLayer::State::HasKey) {
        if (!m_rpcLayer->sessionId()) {
            rpcLayer()->setSessionId(Utils::randomBytes<quint64>());
        }
        if (!m_queuedOperations.isEmpty()) {
            for (PendingRpcOperation *operation : m_queuedOperations) {
                qWarning() << Q_FUNC_INFO << "Dequeue operation" << TLValue::firstFromArray(operation->requestData());
                rpcLayer()->sendRpc(operation);
            }
            m_queuedOperations.clear();
        }
    }
}

} // Client namespace

} // Telegram namespace
