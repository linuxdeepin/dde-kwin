#ifndef WaylandSCREEN_H
#define WaylandSCREEN_H

#include "abstract_screen.h"

#include <QObject>
#include <QDBusContext>
#include <QVariant>

class WaylandScreen : public AbstractScreen
{
    Q_OBJECT

public:
    explicit WaylandScreen(QObject *parent = nullptr);
    ~WaylandScreen();

public :
    virtual QVariantList Modes();
    virtual QVariantList PreferredModes();
    virtual QVariantList Reflects();
    virtual QVariantList Rotations();
    virtual bool Connected();
    virtual bool Enabled();
    virtual uchar CurrentRotateMode();
    virtual double Brightness();
    virtual double RefreshRate();
    virtual int X();
    virtual int Y();
    virtual QString Manufacturer();
    virtual QString Model();
    virtual QString Name();
    virtual QVariantList BestMode();
    virtual QVariantList CurrentMode();
    virtual int Height();
    virtual int Reflect();
    virtual int Rotation();
    virtual int Width();
    virtual int ID();
    virtual int MmHeight();
    virtual int MmWidth();

public slots:
    virtual void Enable(bool isEnable);
    virtual void SetMode(int nMode);
    virtual void SetModeBySize(int nWidth, int nHeight);
    virtual void SetPosition(int x,int y);
    virtual void SetReflect(int nReflect);
    virtual void SetRefreshRate(double dRefreshRate);
    virtual void SetRotation(int nRotation);


};

#endif // WaylandSCREEN_H
