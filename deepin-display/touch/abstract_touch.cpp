#include "abstract_touch.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusInterface>
#include <QProcess>
#include <QStringList>


AbstractTouch::AbstractTouch(QObject *parent)
    : QObject(parent)
{
}

AbstractTouch::~AbstractTouch()
{
}

