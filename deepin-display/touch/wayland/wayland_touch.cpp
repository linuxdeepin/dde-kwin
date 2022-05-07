#include "wayland_touch.h"
#include "touchadaptor.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusInterface>
#include <QProcess>
#include <QStringList>
#include <QDebug>

WaylandTouch::WaylandTouch(QObject *parent)
    : AbstractTouch(parent)
{
    new TouchAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Touch"), this, QDBusConnection::ExportAllContents);

    getTouchPadInterface();
#if 0
    m_connectionThread = new QThread(this);
    m_connectionThreadObject = new ConnectionThread();

    init();
#endif
}
WaylandTouch::~WaylandTouch()
{
}

void WaylandTouch::init()
{
    connect(m_connectionThreadObject, &ConnectionThread::connected, this,
        [this] {
            m_eventQueue = new EventQueue(this);
            m_eventQueue->setup(m_connectionThreadObject);

            Registry *registry = new Registry(this);
            setupRegistry(registry);
        },
        Qt::QueuedConnection
    );
    m_connectionThreadObject->moveToThread(m_connectionThread);
    m_connectionThread->start();

    m_connectionThreadObject->initConnection();
}

void WaylandTouch::setupRegistry(Registry *registry)
{
    connect(registry, &Registry::ddeSeatAnnounced, this,
            [this, registry](quint32 name, quint32 version) {
                m_ddeSeat = registry->createDDESeat(name, version);
        }
    );

    if (m_ddeSeat) {
        m_ddeTouch = m_ddeSeat->createDDETouch(this);
        connect(m_ddeTouch, &DDETouch::touchUp, this,
                [this] (int32_t id) {
            qDebug() << "ddeseat touch up" << id;
            }
        );

        connect(m_ddeTouch, &DDETouch::touchDown, this,
                [this] (int32_t id, const QPointF &pos) {
            qDebug() << "ddeseat touch down" << id << pos;
            }
        );

        connect(m_ddeTouch, &DDETouch::touchMotion, this,
                [this] (int32_t id, const QPointF &pos) {
            qDebug() << "ddeseat touch motion" << id << pos;
            }
        );
    }


    registry->setEventQueue(m_eventQueue);
    registry->create(m_connectionThreadObject);
    registry->setup();
}

void WaylandTouch::getTouchPadInterface()
{
    QDBusInterface interface("org.kde.KWin", "/org/kde/KWin/InputDevice", "org.kde.KWin.InputDeviceManager");
    QStringList listEvents =  interface.property("devicesSysNames").toStringList();
    for (auto eventName : listEvents) {
        QString path = QString("/org/kde/KWin/InputDevice/%1").arg(eventName);
        QDBusInterface interfaceEvent("org.kde.KWin", path, "org.kde.KWin.InputDevice");
        if (interfaceEvent.property("touchpad").toBool()) {
            m_touchpadPath = path;
            m_eventId = eventName.replace("event", "").toInt();
            break;
        }
    }
}

bool WaylandTouch::getDisableIfTypeing()
{
    QDBusInterface interface("org.kde.KWin", m_touchpadPath, "org.kde.KWin.InputDevice");
    return interface.property("disableWhileTyping").toBool();
}

void WaylandTouch::setDisableIfTypeing(bool DisableIfTypeing)
{
    setProperty("DisableIfTypeing", DisableIfTypeing);
}

bool WaylandTouch::getEdgeScroll()
{
    QDBusInterface interface("org.kde.KWin", m_touchpadPath, "org.kde.KWin.InputDevice");
    return interface.property("supportsScrollEdge").toBool();
}

void WaylandTouch::setEdgeScroll(bool EdgeScroll)
{
    setProperty("EdgeScroll", EdgeScroll);
}

bool WaylandTouch::getExist()
{
    bool exist = true;
    QDBusInterface interface("org.kde.KWin", m_touchpadPath, "org.kde.KWin.InputDevice");
    QString name =  interface.property("name").toString();
    if (name.isEmpty()) {
        exist = false;
    }
    return exist;
}

bool WaylandTouch::getHorizScroll()
{
    return true;
}

void WaylandTouch::setHorizScroll(bool HorizScroll)
{
    setProperty("HorizScroll", HorizScroll);
}

bool WaylandTouch::getLeftHanded()
{
    QDBusInterface interface("org.kde.KWin", m_touchpadPath, "org.kde.KWin.InputDevice");
    return interface.property("leftHanded").toBool();
}

void WaylandTouch::setLeftHanded(bool LeftHanded)
{
    setProperty("LeftHanded", LeftHanded);
}

bool WaylandTouch::getNaturalScroll()
{
    QDBusInterface interface("org.kde.KWin", m_touchpadPath, "org.kde.KWin.InputDevice");
    return interface.property("naturalScroll").toBool();
}

void WaylandTouch::setNaturalScroll(bool NaturalScroll)
{
    setProperty("NaturalScroll", NaturalScroll);
}

bool WaylandTouch::getPalmDetect()
{
    return false;
}

void WaylandTouch::setPalmDetect(bool PalmDetect)
{
    setProperty("PalmDetect", PalmDetect);
}

bool WaylandTouch::getTPadEnable()
{
    QDBusInterface interface("org.kde.KWin", m_touchpadPath, "org.kde.KWin.InputDevice");
    return interface.property("enabled").toBool();
}

void WaylandTouch::setTPadEnable(bool TPadEnable)
{
    setProperty("TPadEnable", TPadEnable);
}

bool WaylandTouch::getTapClick()
{
    QDBusInterface interface("org.kde.KWin", m_touchpadPath, "org.kde.KWin.InputDevice");
    return interface.property("tapToClick").toBool();
}

void WaylandTouch::setTapClick(bool TapClick)
{
    setProperty("TapClick", TapClick);
}

bool WaylandTouch::getVertScroll()
{
    return true;
}

void WaylandTouch::setVertScroll(bool VertScroll)
{
    setProperty("VertScroll", VertScroll);
}

double WaylandTouch::getMotionAcceleration()
{
    return 1.0;
}

void WaylandTouch::setMotionAcceleration(double MotionAcceleration)
{
    setProperty("MotionAcceleration", MotionAcceleration);
}

double WaylandTouch::getMotionScaling()
{
    return 10.0;
}

void WaylandTouch::setMotionScaling(double MotionScaling)
{
    setProperty("MotionScaling", MotionScaling);
}

double WaylandTouch::getMotionThreshold()
{
    return 1.0;
}

void WaylandTouch::setMotionThreshold(double MotionThreshold)
{
    setProperty("MotionThreshold", MotionThreshold);
}

int WaylandTouch::getDeltaScroll()
{
    return 30;
}

void WaylandTouch::setDeltaScroll(int DeltaScroll)
{
    setProperty("DeltaScroll", DeltaScroll);
}

int WaylandTouch::getDoubleClick()
{
    return 500;
}

void WaylandTouch::setDoubleClick(int DoubleClick)
{
    setProperty("DoubleClick", DoubleClick);
}

int WaylandTouch::getDragThreshold()
{
    return 8;
}

void WaylandTouch::setDragThreshold(int DragThreshold)
{
    setProperty("DragThreshold", DragThreshold);
}

int WaylandTouch::getPalmMinWidth()
{
    return 6;
}

void WaylandTouch::setPalmMinWidth(int PalmMinWidth)
{
    setProperty("PalmMinWidth", PalmMinWidth);
}

int WaylandTouch::getPalmMinZ()
{
    return 50;
}

void WaylandTouch::setPalmMinZ(int PalmMinZ)
{
    setProperty("PalmMinZ", PalmMinZ);
}

QString WaylandTouch::getDeviceList()
{
    QJsonArray array;
    QJsonObject obj;
    QDBusInterface interface("org.kde.KWin", m_touchpadPath, "org.kde.KWin.InputDevice");
    QString name =  interface.property("name").toString();
    if (name.isEmpty()) {
        return "";
    }
    obj.insert("Id", m_eventId);
    obj.insert("Name", name);
    array.insert(0, obj);
    QJsonDocument doc;
    doc.setArray(array);
    QByteArray byteArray = doc.toJson(QJsonDocument::Compact);
    return byteArray;
}

void WaylandTouch::Reset()
{

}
