#ifndef ABSTRACTTABLET_H
#define ABSTRACTTABLET_H

#include "../display_utils.h"

#include <QObject>

class AbstractTablet : public QObject
{
    Q_OBJECT

public:

    explicit AbstractTablet(QObject *parent = nullptr);
    ~AbstractTablet();

    ADD_PROPERTY(QStringList, ActionInfos);
    ADD_PROPERTY(bool, CursorMode);
    ADD_PROPERTY(bool, Exist);
    ADD_PROPERTY(bool, ForceProportions);
    ADD_PROPERTY(bool, LeftHanded);
    ADD_PROPERTY(bool, MouseEnterRemap);
    ADD_PROPERTY(QString, KeyDownAction);
    ADD_PROPERTY(QString, KeyUpAction);
    ADD_PROPERTY(QString, MapOutput);
    ADD_PROPERTY(int, EraserPressureSensitive);
    ADD_PROPERTY(int, EraserRawSample);
    ADD_PROPERTY(int, EraserThreshold);
    ADD_PROPERTY(int, StylusPressureSensitive);
    ADD_PROPERTY(int, StylusRawSample);
    ADD_PROPERTY(int, StylusThreshold);
    ADD_PROPERTY(int, Suppress);
    ADD_PROPERTY(QString, DeviceList);

signals:


public Q_SLOTS:

    virtual void Reset() = 0;    
};
#endif

