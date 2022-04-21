#ifndef ABSTRACTEFFECTS_H
#define ABSTRACTEFFECTS_H

#include "../display_utils.h"

#include <QObject>

class AbstractEffects : public QObject
{
    Q_OBJECT

public:

    explicit AbstractEffects(QObject *parent = nullptr);
    ~AbstractEffects();
signals:
    void WMChanged(QString);

public Q_SLOTS:
    virtual bool AllowSwitch(void)  = 0;
    virtual QString CurrentWM(void) = 0;
    virtual void RequestSwitchWM(void) = 0;

};

#endif

