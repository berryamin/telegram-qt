#include "DeclarativeOperation.hpp"
#include "DeclarativeClient.hpp"

#include <QTimer>
#include <QDebug>

namespace Telegram {

using namespace Client;

DeclarativeOperation::DeclarativeOperation(QObject *parent) :
    QObject(parent)
{
}

DeclarativeClient *DeclarativeOperation::target() const
{
    return m_target;
}

bool DeclarativeOperation::isSucceeded() const
{
    return m_operation && m_operation->isSucceeded();
}

void DeclarativeOperation::start()
{
//    m_running = true;
    emit started();
    startEvent();
}

void DeclarativeOperation::setTarget(DeclarativeClient *target)
{
    m_target = target;
}

void DeclarativeOperation::startEvent()
{
}

void DeclarativeOperation::setPendingOperation(PendingOperation *op)
{
    m_operation = op;
    connect(op, &PendingOperation::finished, this, &DeclarativeOperation::onOperationFinished);
}

void DeclarativeOperation::onOperationFinished(PendingOperation *operation)
{
    if (operation->isSucceeded()) {
        emit succeeded();
    } else {
        emit failed(operation->errorDetails());
    }
    emit finished();
}

} // Telegram
