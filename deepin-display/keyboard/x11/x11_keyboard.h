#ifndef X11KEYBOARD_H
#define X11KEYBOARD_H

#include "abstract_keyboard.h"

#include <QObject>
#include <QDBusContext>
#include <QMap>

class X11Keyboard : public AbstractKeyboard
{
    Q_OBJECT

public:
    explicit X11Keyboard(QObject *parent = nullptr);
    ~X11Keyboard();

    bool CapslockToggle() override; 
    void setCapslockToggle(bool) override;

    QString CurrentLayout() override; 
    void setCurrentLayout(QString) override;

    int CursorBlink() override; 
    void setCursorBlink(int) override;

    int LayoutScope() override; 
    void setLayoutScope(int) override;

    int RepeatDelay() override; 
    void setRepeatDelay(int) override;

    bool RepeatEnabled() override; 
    void setRepeatEnabled(bool) override;

    unsigned int RepeatInterval() override; 
    void setRepeatInterval(unsigned int) override;

    QList<QString> UserLayoutList() override; 
    void setUserLayoutList(QList<QString>) override;

    QList<QString> UserOptionList() override; 
    void setUserOptionList(QList<QString>) override;

public Q_SLOTS:

    void AddLayoutOption(QString) override;
    void AddUserLayout(QString) override;
    void ClearLayoutOption() override;
    void DeleteLayoutOption(QString) override;
    void DeleteUserLayout(QString) override;
    QString GetLayoutDesc(QString) override;
    QMap<QString,QString> LayoutList() override;
    void Reset() override;
};

#endif 
