#ifndef EDITERAKO_IPLUGIN_H
#define EDITERAKO_IPLUGIN_H

#include <QObject>
#include <QString>

class PluginContext;

class IPlugin
{
public:
    virtual ~IPlugin() = default;

    [[nodiscard]] virtual QString id() const = 0;
    virtual bool activate(PluginContext &context) = 0;
    virtual void deactivate() = 0;
};

#define EditerakoPluginIid "org.editerako.IPlugin/1.0"
Q_DECLARE_INTERFACE(IPlugin, EditerakoPluginIid)

#endif
