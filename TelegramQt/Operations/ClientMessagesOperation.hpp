#ifndef TELEGRAMQT_CLIENT_MESSAGES_OPERATION
#define TELEGRAMQT_CLIENT_MESSAGES_OPERATION

#include "PendingOperation.hpp"
#include "TelegramNamespace.hpp"

#include "RpcLayers/ClientRpcMessagesLayer.hpp"

namespace Telegram {

class PendingRpcOperation;

namespace Client {

class Backend;
class MessagesRpcLayer;

class MessagesOperation : public PendingOperation
{
    Q_OBJECT
public:
    explicit MessagesOperation(QObject *parent = nullptr);
    void setBackend(Backend *backend);

    static MessagesOperation *getDialogs(Backend *backend);

    using RunMethod = void(MessagesOperation::*)();

    void setRunMethod(RunMethod method);

public slots:
    void start() override;

    void getDialogs();

protected:
    MessagesRpcLayer *messagesLayer() const;

    Backend *m_backend = nullptr;
    RunMethod m_runMethod = nullptr;

protected:
    // Implementation:
    void onGetDialogsFinished(MessagesRpcLayer::PendingMessagesDialogs *operation);
};

} // Client

} // Telegram

#endif // TELEGRAMQT_CLIENT_MESSAGES_OPERATION
