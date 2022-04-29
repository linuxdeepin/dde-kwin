#include "wayland_shortcut.h"
#include "shortcutadaptor.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusInterface>
#include <QProcess>
#include <QStringList>
#include <QDBusMetaType>

WaylandShortcut::WaylandShortcut(QObject *parent)
    : AbstractShortcut(parent)
{
    new ShortcutAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Shortcut"), this, QDBusConnection::ExportAllContents);
}
WaylandShortcut::~WaylandShortcut()
{
}

int WaylandShortcut::getNumLockState()
{
    return 1;
}

int WaylandShortcut::getShortcutSwitchLayout()
{
    return 1;
}

void WaylandShortcut::setShortcutSwitchLayout(int ShortcutSwitchLayout)
{

}

QVariantList WaylandShortcut::Add(QString, QString, QString)
{
    QString str = "";
    bool b = true;
    QVariantList valist{str, b};
    return valist;
}

QVariantList WaylandShortcut::AddCustomShortcut(QString, QString, QString)
{
    QString str = "";
    int n = true;
    QVariantList valist{str, n};
    return valist;
}

void WaylandShortcut::AddShortcutKeystroke(QString, int, QString)
{

}

QVariantList WaylandShortcut::CheckAvaliable(QString)
{
    bool b = true;
    QString str = "";
    QVariantList valist{b, str};
    return valist;
}

void WaylandShortcut::ClearShortcutKeystrokes(QString, int)
{

}

void WaylandShortcut::Delete(QString, int)
{

}

void WaylandShortcut::DeleteCustomShortcut(QString)
{

}

void WaylandShortcut::DeleteShortcutKeystroke(QString, int, QString)
{

}

void WaylandShortcut::Disable(QString, int)
{

}

int WaylandShortcut::GetCapsLockState()
{
    return 1;
}

QString WaylandShortcut::GetShortcut(QString, int)
{
    return QString{""};
}

void WaylandShortcut::GrabScreen()
{

}

QString WaylandShortcut::List()
{
    return "";
}

QString WaylandShortcut::ListAllShortcuts()
{
    return "";
}

QString WaylandShortcut::ListAllShortcutsByType(int)
{
    return "";
}

QString WaylandShortcut::LookupConflictingShortcut(QString)
{
    return "";
}

QVariantList WaylandShortcut::ModifiedAccel(QString, int, QString, bool)
{
    bool b = true;
    QString str = "";
    QVariantList varlist{b, str};
    return varlist;
}

void WaylandShortcut::ModifyCustomShortcut(QString, QString, QString, QString)
{

}

QString WaylandShortcut::Query(QString, int)
{
    return "";
}

void WaylandShortcut::Reset()
{

}

QString WaylandShortcut::SearchShortcuts(QString)
{
    return "";
}

void WaylandShortcut::SelectKeystroke()
{

}

void WaylandShortcut::SetCapsLockState(int)
{

}

void WaylandShortcut::SetNumLockState(int)
{

}
