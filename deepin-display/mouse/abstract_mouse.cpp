#include "abstract_mouse.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusInterface>
#include <QProcess>
#include <QStringList>


AbstractMouse::AbstractMouse(QObject *parent)
    : QObject(parent)
{
}

AbstractMouse::~AbstractMouse()
{
}

