#include "ClientMessagesOperation.hpp"

#include "DataStorage.hpp"
#include "ClientBackend.hpp"
#include "ClientConnection.hpp"
#include "ClientRpcMessagesLayer.hpp"
#include "PendingRpcOperation.hpp"

#include "RpcError.hpp"

#ifdef DEVELOPER_BUILD
#include "TLTypesDebug.hpp"
#endif

#include <QLoggingCategory>

namespace Telegram {

namespace Client {

MessagesOperation::MessagesOperation(QObject *parent) :
    PendingOperation(parent)
{
}

void MessagesOperation::setBackend(Backend *backend)
{
    m_backend = backend;
}

MessagesOperation *MessagesOperation::getDialogs(Backend *backend)
{
    MessagesOperation *dialogsOperation = new MessagesOperation(backend);
    dialogsOperation->setBackend(backend);
    dialogsOperation->setRunMethod(&MessagesOperation::getDialogs);
    return dialogsOperation;
}

void MessagesOperation::setRunMethod(MessagesOperation::RunMethod method)
{
    m_runMethod = method;
}

void MessagesOperation::start()
{
    if (m_runMethod) {
        callMember<>(this, m_runMethod);
    }
}

void MessagesOperation::getDialogs()
{
    MessagesRpcLayer::PendingMessagesDialogs *requestDialogsOperation = messagesLayer()->getDialogs(0, 0, 0, TLInputPeer(), 32);
    connect(requestDialogsOperation, &PendingOperation::finished, this, [this, requestDialogsOperation] {
       this->onGetDialogsFinished(requestDialogsOperation);
    });
}

MessagesRpcLayer *MessagesOperation::messagesLayer() const
{
    return m_backend->messagesLayer();
}

void MessagesOperation::onGetDialogsFinished(MessagesRpcLayer::PendingMessagesDialogs *operation)
{
    TLMessagesDialogs dialogs;
    operation->getResult(&dialogs);
    m_backend->dataStorage()->internalApi()->processData(dialogs);
    setFinished();
}

} // Client

} // Telegram namespace
