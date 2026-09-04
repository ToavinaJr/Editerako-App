#ifndef EDITERAKO_TASKDEFINITION_H
#define EDITERAKO_TASKDEFINITION_H

#include <QString>
#include <QStringList>

enum class TaskKind {
    Custom,
    CMakeConfigure,
    CMakeBuild,
    CMakeClean,
    CMakeTest,
    CMakeRun,
};

struct TaskDefinition {
    QString id;
    QString label;
    QString command;
    QStringList args;
    QString workingDirectory;
    QString problemMatcher;
    TaskKind kind = TaskKind::Custom;
};

struct TaskProblem {
    QString path;
    int line = 0;
    int column = 0;
    enum class Severity { Error, Warning, Information } severity = Severity::Error;
    QString message;
};

struct ProcessSpec {
    QString title;
    QString program;
    QStringList arguments;
    QString workingDirectory;
    QString problemMatcher;
    bool detach = false;
};

#endif
