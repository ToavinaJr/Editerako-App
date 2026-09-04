#include "debug/DapClient.h"

#include "core/Logging.h"
#include "lsp/JsonRpcTransport.h"

#include <QJsonArray>
#include <QJsonValue>

namespace {

QJsonObject dapRequest(int seq, const QString &command, const QJsonObject &arguments)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("seq"), seq);
    obj.insert(QStringLiteral("type"), QStringLiteral("request"));
    obj.insert(QStringLiteral("command"), command);
    if (!arguments.isEmpty()) {
        obj.insert(QStringLiteral("arguments"), arguments);
    }
    return obj;
}

QJsonObject dapErrorObject(const QString &message)
{
    return QJsonObject{{QStringLiteral("message"), message}};
}

} // namespace

DapClient::DapClient(JsonRpcTransport *transport, QObject *parent)
    : QObject(parent)
    , m_transport(transport)
{
    if (m_transport) {
        connect(m_transport, &JsonRpcTransport::messageReceived, this, &DapClient::onMessage);
        connect(m_transport, &JsonRpcTransport::errorOccurred, this, &DapClient::protocolError);
    }
}

void DapClient::sendObject(const QJsonObject &object)
{
    if (!m_transport) {
        emit protocolError(QStringLiteral("DAP transport is missing"));
        return;
    }
    m_transport->send(object);
}

int DapClient::sendRequest(const QString &command, const QJsonObject &arguments,
                           const ResponseCallback &callback)
{
    const int seq = ++m_nextSeq;
    if (callback) {
        m_pending.insert(seq, callback);
    }
    sendObject(dapRequest(seq, command, arguments));
    return seq;
}

void DapClient::initialize(const QString &adapterId, const ResponseCallback &callback)
{
    QJsonObject args;
    args.insert(QStringLiteral("clientID"), QStringLiteral("editerako"));
    args.insert(QStringLiteral("clientName"), QStringLiteral("Editerako"));
    args.insert(QStringLiteral("adapterID"), adapterId.isEmpty() ? QStringLiteral("cppdbg") : adapterId);
    args.insert(QStringLiteral("pathFormat"), QStringLiteral("path"));
    args.insert(QStringLiteral("linesStartAt1"), true);
    args.insert(QStringLiteral("columnsStartAt1"), true);
    args.insert(QStringLiteral("supportsVariableType"), true);
    args.insert(QStringLiteral("supportsRunInTerminalRequest"), false);
    sendRequest(QStringLiteral("initialize"), args,
                [this, callback](const QJsonObject &body, const QJsonObject &error) {
                    if (error.isEmpty()) {
                        QJsonObject caps = body;
                        if (body.contains(QStringLiteral("capabilities"))) {
                            caps = body.value(QStringLiteral("capabilities")).toObject();
                        }
                        m_supportsConfigurationDone =
                            caps.value(QStringLiteral("supportsConfigurationDoneRequest")).toBool();
                    }
                    if (callback) {
                        callback(body, error);
                    }
                });
}

void DapClient::launch(const QJsonObject &arguments, const ResponseCallback &callback)
{
    sendRequest(QStringLiteral("launch"), arguments, callback);
}

void DapClient::attach(const QJsonObject &arguments, const ResponseCallback &callback)
{
    sendRequest(QStringLiteral("attach"), arguments, callback);
}

void DapClient::configurationDone(const ResponseCallback &callback)
{
    sendRequest(QStringLiteral("configurationDone"), QJsonObject{}, callback);
}

void DapClient::setBreakpoints(const QString &path, const QList<int> &lines0,
                               const ResponseCallback &callback)
{
    QJsonArray breakpoints;
    for (int line0 : lines0) {
        breakpoints.append(QJsonObject{{QStringLiteral("line"), line0 + 1}});
    }
    QJsonObject args;
    args.insert(QStringLiteral("source"), QJsonObject{{QStringLiteral("path"), path}});
    args.insert(QStringLiteral("breakpoints"), breakpoints);
    args.insert(QStringLiteral("sourceModified"), false);
    sendRequest(QStringLiteral("setBreakpoints"), args, callback);
}

void DapClient::continueDebug(int threadId, const ResponseCallback &callback)
{
    sendRequest(QStringLiteral("continue"),
                QJsonObject{{QStringLiteral("threadId"), threadId}}, callback);
}

void DapClient::pause(int threadId, const ResponseCallback &callback)
{
    sendRequest(QStringLiteral("pause"),
                QJsonObject{{QStringLiteral("threadId"), threadId}}, callback);
}

void DapClient::next(int threadId, const ResponseCallback &callback)
{
    sendRequest(QStringLiteral("next"), QJsonObject{{QStringLiteral("threadId"), threadId}}, callback);
}

void DapClient::stepIn(int threadId, const ResponseCallback &callback)
{
    sendRequest(QStringLiteral("stepIn"),
                QJsonObject{{QStringLiteral("threadId"), threadId}}, callback);
}

void DapClient::stepOut(int threadId, const ResponseCallback &callback)
{
    sendRequest(QStringLiteral("stepOut"),
                QJsonObject{{QStringLiteral("threadId"), threadId}}, callback);
}

void DapClient::stackTrace(int threadId, const ResponseCallback &callback)
{
    QJsonObject args;
    args.insert(QStringLiteral("threadId"), threadId);
    args.insert(QStringLiteral("startFrame"), 0);
    args.insert(QStringLiteral("levels"), 50);
    sendRequest(QStringLiteral("stackTrace"), args, callback);
}

void DapClient::scopes(int frameId, const ResponseCallback &callback)
{
    sendRequest(QStringLiteral("scopes"), QJsonObject{{QStringLiteral("frameId"), frameId}}, callback);
}

void DapClient::variables(int variablesReference, const ResponseCallback &callback)
{
    sendRequest(QStringLiteral("variables"),
                QJsonObject{{QStringLiteral("variablesReference"), variablesReference}}, callback);
}

void DapClient::evaluate(const QString &expression, int frameId, const ResponseCallback &callback)
{
    QJsonObject args;
    args.insert(QStringLiteral("expression"), expression);
    args.insert(QStringLiteral("context"), QStringLiteral("repl"));
    if (frameId > 0) {
        args.insert(QStringLiteral("frameId"), frameId);
    }
    sendRequest(QStringLiteral("evaluate"), args, callback);
}

void DapClient::disconnectSession(bool terminateDebuggee, const ResponseCallback &callback)
{
    QJsonObject args;
    args.insert(QStringLiteral("restart"), false);
    args.insert(QStringLiteral("terminateDebuggee"), terminateDebuggee);
    sendRequest(QStringLiteral("disconnect"), args, callback);
}

void DapClient::replyToRequest(int requestSeq, const QString &command, bool success,
                               const QString &message)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("seq"), ++m_nextSeq);
    obj.insert(QStringLiteral("type"), QStringLiteral("response"));
    obj.insert(QStringLiteral("request_seq"), requestSeq);
    obj.insert(QStringLiteral("success"), success);
    obj.insert(QStringLiteral("command"), command);
    if (!success) {
        obj.insert(QStringLiteral("message"),
                   message.isEmpty() ? QStringLiteral("not supported") : message);
    }
    sendObject(obj);
}

void DapClient::handleEvent(const QString &event, const QJsonObject &body)
{
    if (event == QLatin1String("initialized")) {
        emit initializedEvent();
        return;
    }
    if (event == QLatin1String("stopped")) {
        emit stopped(dapStoppedFromJson(body));
        return;
    }
    if (event == QLatin1String("continued")) {
        emit continued();
        return;
    }
    if (event == QLatin1String("terminated")) {
        emit terminated();
        return;
    }
    if (event == QLatin1String("exited")) {
        emit exited(body.value(QStringLiteral("exitCode")).toInt());
        return;
    }
    if (event == QLatin1String("output")) {
        emit output(body.value(QStringLiteral("category")).toString(),
                    body.value(QStringLiteral("output")).toString());
    }
}

void DapClient::onMessage(const QJsonObject &message)
{
    const QString type = message.value(QStringLiteral("type")).toString();
    if (type == QLatin1String("response")) {
        const int requestSeq = message.value(QStringLiteral("request_seq")).toInt();
        const ResponseCallback cb = m_pending.take(requestSeq);
        if (!cb) {
            return;
        }
        if (!message.value(QStringLiteral("success")).toBool(true)) {
            QString text = message.value(QStringLiteral("message")).toString();
            if (text.isEmpty()) {
                text = QStringLiteral("DAP request failed");
            }
            cb(QJsonObject{}, dapErrorObject(text));
            return;
        }
        cb(message.value(QStringLiteral("body")).toObject(), QJsonObject{});
        return;
    }

    if (type == QLatin1String("event")) {
        handleEvent(message.value(QStringLiteral("event")).toString(),
                    message.value(QStringLiteral("body")).toObject());
        return;
    }

    if (type == QLatin1String("request")) {
        const QString command = message.value(QStringLiteral("command")).toString();
        const int seq = message.value(QStringLiteral("seq")).toInt();
        qCInfo(lcDap) << "DAP reverse request" << command;
        replyToRequest(seq, command, false, QStringLiteral("not supported"));
        return;
    }

    qCWarning(lcDap) << "Unknown DAP message type" << type;
}
