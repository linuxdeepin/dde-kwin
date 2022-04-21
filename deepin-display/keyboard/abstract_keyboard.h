#ifndef ABSTRACTKEYBOARD_H
#define ABSTRACTKEYBOARD_H

#include "../display_utils.h"

#include <QObject>
#include <QMap>

class AbstractKeyboard : public QObject
{
    Q_OBJECT

public:

    ADD_PROPERTY(bool, CapslockToggle)
    ADD_PROPERTY(QString, CurrentLayout)
    ADD_PROPERTY(int, CursorBlink)
    ADD_PROPERTY(int, LayoutScope)
    ADD_PROPERTY(int, RepeatDelay)
    ADD_PROPERTY(bool, RepeatEnabled)
    ADD_PROPERTY(unsigned int, RepeatInterval)
    ADD_PROPERTY(QList<QString>, UserLayoutList)
    ADD_PROPERTY(QList<QString>, UserOptionList)

    explicit AbstractKeyboard(QObject *parent = nullptr);
    ~AbstractKeyboard();


signals:
    void KeyEvent(unsigned int, bool, bool, bool, bool, bool);

public Q_SLOTS:

    virtual void AddLayoutOption(QString) = 0;
    virtual void AddUserLayout(QString) = 0;
    virtual void ClearLayoutOption() = 0;
    virtual void DeleteLayoutOption(QString) = 0;
    virtual void DeleteUserLayout(QString) = 0;
    virtual QString GetLayoutDesc(QString) = 0;
    virtual QMap<QString,QString> LayoutList() = 0;
    virtual void Reset() = 0;

};

#endif

