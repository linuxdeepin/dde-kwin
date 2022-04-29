#ifndef ABSTRACTSHORTCUT_H
#define ABSTRACTSHORTCUT_H


#include <QObject>
#include <QDBusContext>
#include <QDBusMetaType>

struct add {
    QString str;
    bool b;

    friend QDBusArgument &operator<<(QDBusArgument &argument, const add& message)
    {
        argument.beginStructure();
        argument << message.str;
        argument << message.b;
        argument.endStructure();
        return argument;
    }
    friend const QDBusArgument &operator>>(const QDBusArgument &argument, add &message)
    {
        argument.beginStructure();
        argument >> message.str;
        argument >> message.b;
        argument.endStructure();
        return argument;
    }
    static void registerMeateType() {
        qDBusRegisterMetaType<add>();
    }
};
Q_DECLARE_METATYPE(add)

class AbstractShortcut : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.KWin.Display.Shortcut")

    Q_PROPERTY(int NumLockState READ getNumLockState)
    Q_PROPERTY(int ShortcutSwitchLayout READ getShortcutSwitchLayout WRITE setShortcutSwitchLayout)

public:
    explicit AbstractShortcut(QObject *parent = nullptr);
    virtual ~AbstractShortcut();

public:
    virtual int getNumLockState() = 0;
    virtual int getShortcutSwitchLayout() = 0;
    virtual void setShortcutSwitchLayout(int ShortcutSwitchLayout) = 0;

Q_SIGNALS:
    void Added(QString, int);
    void Changed(QString, int);
    void Deleted(QString, int);
    void KeyEvent(bool, QString);

public Q_SLOTS:
    virtual QVariantList Add(QString, QString, QString) = 0;//string,bool
    virtual QVariantList AddCustomShortcut(QString, QString, QString) = 0;//string,int
    virtual void AddShortcutKeystroke(QString, int, QString) = 0;
    virtual QVariantList CheckAvaliable(QString) = 0;//bool,QString
    virtual void ClearShortcutKeystrokes(QString, int) = 0;
    virtual void Delete(QString, int) = 0;
    virtual void DeleteCustomShortcut(QString) = 0;
    virtual void DeleteShortcutKeystroke(QString, int, QString) = 0;
    virtual void Disable (QString, int) = 0;
    virtual int GetCapsLockState() = 0;
    virtual QString GetShortcut(QString, int) = 0;
    virtual void GrabScreen() = 0;
    virtual QString List() = 0;
    virtual QString ListAllShortcuts() = 0;
    virtual QString ListAllShortcutsByType(int) = 0;;
    virtual QString LookupConflictingShortcut(QString) = 0;
    virtual QVariantList ModifiedAccel(QString, int, QString, bool) = 0;//bool,QString
    virtual void ModifyCustomShortcut(QString, QString, QString, QString) = 0;
    virtual QString Query(QString, int) = 0;
    virtual void Reset(void) = 0;
    virtual QString SearchShortcuts(QString) = 0;
    virtual void SelectKeystroke() = 0;
    virtual void SetCapsLockState(int) = 0;
    virtual void SetNumLockState(int) = 0;
};

#endif
