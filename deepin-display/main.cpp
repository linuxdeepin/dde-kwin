#include "wayland/wayland_mouse.h"
#include "wayland/wayland_keyboard.h"
#include "wayland/wayland_trackpoint.h"
#include "wayland/wayland_effects.h"
#include "wayland/wayland_inputdevices.h"

#include "x11/x11_mouse.h"
#include "x11/x11_keyboard.h"
#include "x11/x11_trackpoint.h"
#include "x11/x11_effects.h"
#include "x11/x11_inputdevices.h"


#include "mouseadaptor.h"
#include "keyboardadaptor.h"
#include "trackpointadaptor.h"
#include "effectsadaptor.h"
#include "inputdevicesadaptor.h"

#include <QGuiApplication>
#include <QtCore/QObject>
#include <QtDBus/QtDBus>
#include <DLog>

#define Service "com.deepin.kwin.Display"
#define Path "/com/deepin/kwin/Display"
#define Interface "com.deepin.kwin.Display"

#define MousePath "/com/deepin/kwin/Display/Mouse"
#define MouseInterface "com.deepin.kwin.Display.Mouse"

#define KeyboardPath "/com/deepin/kwin/Display/Keyboard"
#define KeyboardInterface "com.deepin.kwin.Display.Keyboard"

#define TrackpointPath "/com/deepin/kwin/Display/Trackpoint"
#define TrackpointInterface "com.deepin.kwin.Display.Trackpoint"

#define EffectsPath "/com/deepin/kwin/Display/Effects"
#define EffectsInterface "com.deepin.kwin.Display.Effects"

#define InputDevicesPath "/com/deepin/kwin/Display/InputDevices"
#define InputDevicesInterface "com.deepin.kwin.Display.InputDevices"

DCORE_USE_NAMESPACE

//在wayland下，kwindowsystem默认使用wayland平台的接口，但目前wayland的接口currentDesktop()未实现，所以设置环境变量转为使用xcb平台的接口
__attribute__((constructor))void init()
{
    qputenv("QT_QPA_PLATFORM", "xcb");
}

bool isX11Platform()
{
    auto e = QProcessEnvironment::systemEnvironment();
    QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));

    if (XDG_SESSION_TYPE != QLatin1String("x11")) {
        return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setApplicationName("DeepinDisplay");

    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();

    AbstractMouse* amouse;
    AbstractKeyboard* akeyboard;
    AbstractTrackpoint* atrackpoint;
    AbstractEffects* aeffects;
    AbstractInputDevices* ainputdevices;

    if (isX11Platform()) {
        amouse = new X11Mouse();
        akeyboard = new X11Keyboard();
        atrackpoint = new X11Trackpoint();
        aeffects = new X11Effects();
        ainputdevices = new X11InputDevices();
    } else {
        amouse = new WaylandMouse();
        akeyboard = new WaylandKeyboard();
        atrackpoint = new WaylandTrackpoint();
        aeffects = new WaylandEffects();
        ainputdevices = new WaylandInputDevices();
    }

    MouseAdaptor mouseadadaptor(amouse);
    KeyboardAdaptor keyboardadaptor(akeyboard);
    TrackpointAdaptor trackpointadaptor(atrackpoint);
    EffectsAdaptor effectsadaptor(aeffects);
    InputDevicesAdaptor inputdevicesadaptor(ainputdevices);


    if (!QDBusConnection::sessionBus().registerService(Service)) {
        return -1;
    }

    if (!QDBusConnection::sessionBus().registerObject(MousePath, MouseInterface, amouse)) {
        return -2;
    }
    if (!QDBusConnection::sessionBus().registerObject(KeyboardPath, KeyboardInterface, akeyboard)) {
        return -2;
    }
    if (!QDBusConnection::sessionBus().registerObject(EffectsPath, EffectsInterface, aeffects)) {
        return -2;
    }

    if (!QDBusConnection::sessionBus().registerObject(InputDevicesPath, InputDevicesInterface, ainputdevices)) {
        return -2;
    }

    return app.exec();
}
