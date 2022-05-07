#include "abstract_screen.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QDBusReply>
#include <QJsonParseError>
#include <QProcessEnvironment>
#include <QDBusInterface>

AbstractScreen::AbstractScreen(QObject *parent) : QObject(parent)
{

}

AbstractScreen::~AbstractScreen()
{

}
