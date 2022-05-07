#include "wayland/wayland_mouse.h"
#include "wayland/wayland_keyboard.h"
#include "wayland/wayland_trackpoint.h"
#include "wayland/wayland_effects.h"
#include "wayland/wayland_inputdevices.h"
#include "wayland/wayland_output.h"
#include "wayland/wayland_gesture.h"
#include "wayland/wayland_tablet.h"
#include "wayland/wayland_screen.h"

#include "x11/x11_mouse.h"
#include "x11/x11_keyboard.h"
#include "x11/x11_trackpoint.h"
#include "x11/x11_effects.h"
#include "x11/x11_inputdevices.h"
#include "x11/x11_output.h"
#include "x11/x11_gesture.h"
#include "x11/x11_tablet.h"
#include "x11/x11_screen.h"

#include "mouseadaptor.h"
#include "keyboardadaptor.h"
#include "trackpointadaptor.h"
#include "effectsadaptor.h"
#include "inputdevicesadaptor.h"
#include "outputadaptor.h"
#include "gestureadaptor.h"
#include "tabletadaptor.h"

#include "wayland/wayland_touch.h"
#include "x11/x11_touch.h"
#include "wayland/wayland_shortcut.h"
#include "x11/x11_shortcut.h"
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

#define GesturePath "/com/deepin/kwin/Display/Gesture"
#define GestureInterface "com.deepin.kwin.Display.Gesture"

#define TabletPath "/com/deepin/kwin/Display/Tablet"
#define TabletInterface "com.deepin.kwin.Display.Tablet"

#define outputService "com.deepin.kwin.Display.output"
#define outputPath "/com/deepin/kwin/Display/output"
#define outputInterface "com.deepin.kwin.Display.output"

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
    AbstractOutput *output;
    AbstractGesture *gesture;
    AbstractTablet *tablet;

    if (isX11Platform()) {
        amouse = new X11Mouse();
        akeyboard = new X11Keyboard();
        atrackpoint = new X11Trackpoint();
        aeffects = new X11Effects();
        ainputdevices = new X11InputDevices();
        output = new X11Output();
        new X11Touch();
        new X11Shortcut();
        gesture = new X11Gesture();
        tablet = new X11Tablet();
        new X11Screen();
    } else {
        amouse = new WaylandMouse();
        akeyboard = new WaylandKeyboard();
        atrackpoint = new WaylandTrackpoint();
        aeffects = new WaylandEffects();
        ainputdevices = new WaylandInputDevices();
        output = new WaylandOutput();
        new WaylandTouch();
        new WaylandShortcut();
        gesture = new WaylandGesture();
        tablet = new WaylandTablet();
        new WaylandScreen;
    }

    MouseAdaptor mouseadadaptor(amouse);
    KeyboardAdaptor keyboardadaptor(akeyboard);
    TrackpointAdaptor trackpointadaptor(atrackpoint);
    EffectsAdaptor effectsadaptor(aeffects);
    InputDevicesAdaptor inputdevicesadaptor(ainputdevices);
    OutputAdaptor outputAdaptor(output);
    GestureAdaptor gestureAdaptor(gesture);
    TabletAdaptor tabletAdaptor(tablet);

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

    if (!QDBusConnection::sessionBus().registerObject(outputPath, outputInterface, output)) {
        return -2;
    }

    if (!QDBusConnection::sessionBus().registerObject(GesturePath, GestureInterface, gesture)) {
        return -2;
    }

    if (!QDBusConnection::sessionBus().registerObject(TabletPath, TabletInterface, tablet)) {
        return -2;
    }

    return app.exec();
}
