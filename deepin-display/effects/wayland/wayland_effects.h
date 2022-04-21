#ifndef WAYLANDEFFECTS_H
#define WAYLANDEFFECTS_H

#include "abstract_effects.h"

#include <QObject>

class WaylandEffects : public AbstractEffects
{
    Q_OBJECT

public:
    explicit WaylandEffects(QObject *parent = nullptr);
    ~WaylandEffects();
    
signals:
    void WMChanged(QString);

public Q_SLOTS:
    bool AllowSwitch(void)  override;
    QString CurrentWM(void) override;
    void RequestSwitchWM(void) override;
};

#endif 
