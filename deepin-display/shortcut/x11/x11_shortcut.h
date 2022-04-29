#ifndef X11SHORTCUT_H
#define X11SHORTCUT_H

#include "abstract_shortcut.h"

#include <QObject>
#include <QDBusContext>

class X11Shortcut : public AbstractShortcut
{
    Q_OBJECT

public:
    explicit X11Shortcut(QObject *parent = nullptr);
    ~X11Shortcut();

public:
    int getNumLockState() override;
    int getShortcutSwitchLayout() override;
    void setShortcutSwitchLayout(int ShortcutSwitchLayout) override;

Q_SIGNALS:
    void Added(QString, int);
    void Changed(QString, int);
    void Deleted(QString, int);
    void KeyEvent(bool, QString);

public Q_SLOTS:
    QVariantList Add(QString, QString, QString) override;//string,bool
    QVariantList AddCustomShortcut(QString, QString, QString) override;//string,int
    void AddShortcutKeystroke(QString, int, QString) override;
    QVariantList CheckAvaliable(QString) override;//bool,QString
    void ClearShortcutKeystrokes(QString, int) override;
    void Delete(QString, int) override;
    void DeleteCustomShortcut(QString) override;
    void DeleteShortcutKeystroke(QString, int, QString) override;
    void Disable (QString, int) override;
    int GetCapsLockState() override;
    QString GetShortcut(QString, int) override;
    void GrabScreen() override;
    QString List() override;
    QString ListAllShortcuts() override;
    QString ListAllShortcutsByType(int) override;
    QString LookupConflictingShortcut(QString) override;
    QVariantList ModifiedAccel(QString, int, QString, bool) override;//bool,QString
    void ModifyCustomShortcut(QString, QString, QString, QString) override;
    QString Query(QString, int) override;
    void Reset(void) override;
    QString SearchShortcuts(QString) override;
    void SelectKeystroke() override;
    void SetCapsLockState(int) override;
    void SetNumLockState(int) override;
};

#endif
