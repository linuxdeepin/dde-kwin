#ifndef X11EFFECTS_H
#define X11EFFECTS_H

#include "abstract_effects.h"

#include <QObject>

class X11Effects : public AbstractEffects
{
    Q_OBJECT

public:
    explicit X11Effects(QObject *parent = nullptr);
    ~X11Effects();

signals:
    void WMChanged(QString);

public Q_SLOTS:

    bool AllowSwitch(void)  override;
    QString CurrentWM(void) override;
    void RequestSwitchWM(void) override;

};

#endif 
