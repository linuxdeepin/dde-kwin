#include "x11_shortcut.h"
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


X11Shortcut:: X11Shortcut(QObject *parent)
    : AbstractShortcut(parent)
{
    new ShortcutAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Shortcut"), this, QDBusConnection::ExportAllContents);
}

X11Shortcut::~ X11Shortcut()
{
}

int X11Shortcut::getNumLockState()
{
    return 1;
}

int X11Shortcut::getShortcutSwitchLayout()
{
    return 1;
}

void X11Shortcut::setShortcutSwitchLayout(int ShortcutSwitchLayout)
{

}

QVariantList X11Shortcut::Add(QString, QString, QString)
{
    QString str = "";
    bool b = true;
    QVariantList valist{str, b};
    return valist;
}

QVariantList X11Shortcut::AddCustomShortcut(QString, QString, QString)
{
    QString str = "";
    int n = true;
    QVariantList valist{str, n};
    return valist;
}

void X11Shortcut::AddShortcutKeystroke(QString, int, QString)
{

}

QVariantList X11Shortcut::CheckAvaliable(QString)
{
    bool b = true;
    QString str = "";
    QVariantList valist{b, str};
    return valist;
}

void X11Shortcut::ClearShortcutKeystrokes(QString, int)
{

}

void X11Shortcut::Delete(QString, int)
{

}

void X11Shortcut::DeleteCustomShortcut(QString)
{

}

void X11Shortcut::DeleteShortcutKeystroke(QString, int, QString)
{

}

void X11Shortcut::Disable(QString, int)
{

}

int X11Shortcut::GetCapsLockState()
{
    return 1;
}

QString X11Shortcut::GetShortcut(QString, int)
{
    return QString{""};
}

void X11Shortcut::GrabScreen()
{

}

QString X11Shortcut::List()
{
    return "";
}

QString X11Shortcut::ListAllShortcuts()
{
    return "";
}

QString X11Shortcut::ListAllShortcutsByType(int)
{
    return "";
}

QString X11Shortcut::LookupConflictingShortcut(QString)
{
    return "";
}

QVariantList X11Shortcut::ModifiedAccel(QString, int, QString, bool)
{
    bool b = true;
    QString str = "";
    QVariantList varlist{b, str};
    return varlist;
}

void X11Shortcut::ModifyCustomShortcut(QString, QString, QString, QString)
{

}

QString X11Shortcut::Query(QString, int)
{
    return "";
}

void X11Shortcut::Reset()
{

}

QString X11Shortcut::SearchShortcuts(QString)
{
    return "";
}

void X11Shortcut::SelectKeystroke()
{

}

void X11Shortcut::SetCapsLockState(int)
{

}

void X11Shortcut::SetNumLockState(int)
{

}
