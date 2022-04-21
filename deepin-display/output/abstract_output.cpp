#include "abstract_output.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusInterface>
#include <QProcess>
#include <QStringList>

AbstractOutput::AbstractOutput(QObject *parent) : QObject(parent)
{

}

AbstractOutput::~AbstractOutput()
{

}
