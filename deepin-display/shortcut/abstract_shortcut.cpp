#include "abstract_shortcut.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusInterface>
#include <QProcess>
#include <QStringList>

AbstractShortcut::AbstractShortcut(QObject *parent)
    : QObject(parent)
{
    qDBusRegisterMetaType<add>();
}

AbstractShortcut::~AbstractShortcut()
{
}

