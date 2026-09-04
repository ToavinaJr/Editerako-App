#ifndef EDITERAKO_DAPTYPES_H
#define EDITERAKO_DAPTYPES_H

#include <QJsonObject>
#include <QMetaType>
#include <QString>
#include <QVector>

struct DapStoppedEvent {
    QString reason;
    QString description;
    QString text;
    int threadId = 1;
    bool allThreadsStopped = false;
};

struct DapStackFrame {
    int id = 0;
    QString name;
    QString sourcePath;
    int line = 0;
    int column = 0;
};

struct DapScope {
    QString name;
    int variablesReference = 0;
    bool expensive = false;
};

struct DapVariable {
    QString name;
    QString value;
    QString type;
    int variablesReference = 0;
};

[[nodiscard]] QString dapSourcePath(const QJsonObject &source);
[[nodiscard]] DapStoppedEvent dapStoppedFromJson(const QJsonObject &body);
[[nodiscard]] QVector<DapStackFrame> dapStackFramesFromJson(const QJsonObject &body);
[[nodiscard]] QVector<DapScope> dapScopesFromJson(const QJsonObject &body);
[[nodiscard]] QVector<DapVariable> dapVariablesFromJson(const QJsonObject &body);
[[nodiscard]] QString dapNormalizePath(const QString &path);

Q_DECLARE_METATYPE(DapStoppedEvent)

#endif
