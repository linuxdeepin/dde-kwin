#include "deepinwmfaker.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <KF5/KConfigCore/KConfig>
#include <KF5/KConfigCore/KConfigGroup>
#include <KF5/KWindowSystem/KWindowSystem>
#include <KF5/KGlobalAccel/KGlobalAccel>

#ifndef DISABLE_DEEPIN_WM
#include <QGSettings>
Q_GLOBAL_STATIC_WITH_ARGS(QGSettings, _gsettings_dde_appearance, ("com.deepin.dde.appearance"))
Q_GLOBAL_STATIC_WITH_ARGS(QGSettings, _gsettings_dde_zone, ("com.deepin.dde.zone"))
#define GsettingsBackgroundUri "backgroundUris"
#define GsettingsZoneRightUp "rightUp"
#define GsettingsZoneRightDown "rightDown"
#define GsettingsZoneLeftDown "leftDown"
#define GsettingsZoneLeftUp "leftUp"
#endif // DISABLE_DEEPIN_WM

#define DeepinWMConfigName "deepinwmrc"
#define DeepinWMGeneralGroupName "General"
#define DeepinWMWorkspaceBackgroundGroupName "WorkspaceBackground"

#define KWinConfigName "kwinrc"
#define KWinCloseWindowGroupName "Script-closewindowaction"
#define KWinRunCommandGroupName "Script-runcommandaction"

#define GlobalAccelComponentName "kwin"
#define GlobalAccelComponentDisplayName "KWin"

#define KWinUtilsDbusService "org.kde.KWin"
#define KWinUtilsDbusPath "/dde"

// kwin dbus
#define KWinDBusService "org.kde.KWin"
#define KWinDBusPath "/KWin"
#define KWinDBusCompositorInterface "org.kde.kwin.Compositing"
#define KWinDBusCompositorPath "/Compositor"

using org::kde::KWin;

// deepin-wm's accel as Key
static const QMap<QString, QString> AllDeepinWMKWinAccelsMap {
    { "switch-to-workspace-1", "Switch to Desktop 1" },
    { "switch-to-workspace-2", "Switch to Desktop 2" },
    { "switch-to-workspace-3", "Switch to Desktop 3" },
    { "switch-to-workspace-4", "Switch to Desktop 4" },
    { "switch-to-workspace-5", "Switch to Desktop 5" },
    { "switch-to-workspace-6", "Switch to Desktop 6" },
    { "switch-to-workspace-7", "Switch to Desktop 7" },
    { "switch-to-workspace-8", "Switch to Desktop 8" },
    { "switch-to-workspace-9", "Switch to Desktop 9" },
    { "switch-to-workspace-10", "Switch to Desktop 10" },
    { "switch-to-workspace-11", "Switch to Desktop 11" },
    { "switch-to-workspace-12", "Switch to Desktop 12" },
    { "switch-to-workspace-left", "Switch to Previous Desktop" },
    { "switch-to-workspace-right", "Switch to Next Desktop" },
    { "switch-group", "Walk Through Windows of Current Application" },
    { "switch-group-backward", "Walk Through Windows of Current Application (Reverse)" },
    { "switch-applications", "Walk Through Windows" },
    { "switch-applications-backward", "Walk Through Windows (Reverse)" },
    { "show-desktop", "Show Desktop" },
    { "activate-window-menu", "Window Operations Menu" },
    { "toggle-fullscreen", "Window Fullscreen" },
    { "toggle-maximized", "Window Maximize" },
    { "toggle-above", "Toggle Window Raise/Lower" },
    { "maximize", "Window Absolute Maximize" },
    { "unmaximize", "Window Unmaximize" },
    { "minimize", "Window Minimize" },
    { "close", "Window Close" },
    { "begin-move", "Window Move" },
    { "begin-resize", "Window Resize" },
    { "move-to-workspace-1", "Window to Desktop 1" },
    { "move-to-workspace-2", "Window to Desktop 2" },
    { "move-to-workspace-3", "Window to Desktop 3" },
    { "move-to-workspace-4", "Window to Desktop 4" },
    { "move-to-workspace-5", "Window to Desktop 5" },
    { "move-to-workspace-6", "Window to Desktop 6" },
    { "move-to-workspace-7", "Window to Desktop 7" },
    { "move-to-workspace-8", "Window to Desktop 8" },
    { "move-to-workspace-9", "Window to Desktop 9" },
    { "move-to-workspace-10", "Window to Desktop 10" },
    { "move-to-workspace-11", "Window to Desktop 11" },
    { "move-to-workspace-12", "Window to Desktop 12" },
    { "move-to-workspace-left", "Window to Previous Desktop" },
    { "move-to-workspace-right", "Window to Next Desktop" },
    { "maximize-vertically", "Window Maximize Vertical" },
    { "maximize-horizontally", "Window Maximize Horizontal" },
    { "expose-all-windows", "ExposeAll" },
    { "expose-windows", "Expose" },
    { "preview-workspace", "ShowDesktopGrid" },
};

DeepinWMFaker::DeepinWMFaker(QObject *parent)
    : QObject(parent)
    , m_windowSystem(KWindowSystem::self())
    , m_deepinWMConfig(new KConfig(DeepinWMConfigName, KConfig::SimpleConfig))
    , m_deepinWMGeneralGroup(new KConfigGroup(m_deepinWMConfig->group(DeepinWMGeneralGroupName)))
    , m_deepinWMWorkspaceBackgroundGroup(new KConfigGroup(m_deepinWMConfig->group(DeepinWMWorkspaceBackgroundGroupName)))
    , m_kwinConfig(new KConfig(KWinConfigName, KConfig::SimpleConfig))
    , m_kwinCloseWindowGroup(new KConfigGroup(m_kwinConfig->group(KWinCloseWindowGroupName)))
    , m_kwinRunCommandGroup(new KConfigGroup(m_kwinConfig->group(KWinRunCommandGroupName)))
    , m_globalAccel(KGlobalAccel::self())
    , m_kwinUtilsInter(new KWin(KWinUtilsDbusService, KWinUtilsDbusPath, QDBusConnection::sessionBus(), this))
    , m_previewWinMiniPair(QPair<uint, bool>(-1, false))
{
#ifndef DISABLE_DEEPIN_WM
    m_currentDesktop = m_windowSystem->currentDesktop();

    connect(m_windowSystem, &KWindowSystem::currentDesktopChanged, this, [this] (int to) {
        Q_EMIT WorkspaceSwitched(m_currentDesktop, to);
        m_currentDesktop = to;
    });
    connect(_gsettings_dde_appearance, &QGSettings::changed, this, &DeepinWMFaker::onGsettingsDDEAppearanceChanged);
    connect(_gsettings_dde_zone, &QGSettings::changed, this, &DeepinWMFaker::onGsettingsDDEZoneChanged);

    // 启动后先将所有热区设置同步一遍
    const QStringList zoneKeyList = {GsettingsZoneRightUp, GsettingsZoneRightDown,
                                    GsettingsZoneLeftUp, GsettingsZoneLeftDown};

    // 清理旧数据
    m_kwinCloseWindowGroup->deleteGroup();
    m_kwinRunCommandGroup->deleteGroup();

    // 设置新数据
    for (const QString &key : zoneKeyList) {
        onGsettingsDDEZoneChanged(key);
    }
#endif // DISABLE_DEEPIN_WM
}

DeepinWMFaker::~DeepinWMFaker()
{
    delete m_deepinWMConfig;
    delete m_deepinWMGeneralGroup;
    delete m_deepinWMWorkspaceBackgroundGroup;
    delete m_kwinConfig;
    delete m_kwinCloseWindowGroup;
    delete m_kwinRunCommandGroup;
}

bool DeepinWMFaker::compositingEnabled() const
{
    return QDBusInterface(KWinDBusService, KWinDBusCompositorPath, KWinDBusCompositorInterface).property("active").toBool();
}

#ifndef DISABLE_DEEPIN_WM
static QString getWorkspaceBackgroundOfDeepinWM(const int index)
{
    return _gsettings_dde_appearance->get(GsettingsBackgroundUri).toStringList().value(index - 1);
}

static void setWorkspaceBackgroundForDeepinWM(const int index, const QString &uri)
{
    QStringList all_wallpaper = _gsettings_dde_appearance->get(GsettingsBackgroundUri).toStringList();

    if (index <= all_wallpaper.size()) {
        all_wallpaper[index - 1] = uri;
        // 将壁纸设置同步到 deepin-wm
        _gsettings_dde_appearance->set(GsettingsBackgroundUri, all_wallpaper);
    }
}
#endif // DISABLE_DEEPIN_WM

QString DeepinWMFaker::GetWorkspaceBackground(const int index) const
{
    if (!m_transientBackgroundUri.isEmpty() && index == m_windowSystem->currentDesktop()) {
        return m_transientBackgroundUri;
    }

    const QString &uri = getWorkspaceBackground(index);

#ifndef DISABLE_DEEPIN_WM
    // fellback
    if (uri.isEmpty()) {
        return getWorkspaceBackgroundOfDeepinWM(index);
    }
#endif // DISABLE_DEEPIN_WM

    return uri;
}

void DeepinWMFaker::SetWorkspaceBackground(const int index, const QString &uri)
{
    m_transientBackgroundUri.clear();
    setWorkspaceBackground(index, uri);
#ifndef DISABLE_DEEPIN_WM
    m_deepinWMBackgroundUri.clear();
    setWorkspaceBackgroundForDeepinWM(index, uri);
#endif // DISABLE_DEEPIN_WM
}

QString DeepinWMFaker::GetCurrentWorkspaceBackground() const
{
    return GetWorkspaceBackground(m_windowSystem->currentDesktop());
}

void DeepinWMFaker::SetCurrentWorkspaceBackground(const QString &uri)
{
    SetWorkspaceBackground(m_windowSystem->currentDesktop(), uri);
}

void DeepinWMFaker::SetTransientBackground(const QString &uri)
{
    int current = m_windowSystem->currentDesktop();

    m_transientBackgroundUri = uri;
#ifndef DISABLE_DEEPIN_WM
    if (m_transientBackgroundUri.isEmpty()) {
        quitTransientBackground();
    } else {
        m_deepinWMBackgroundUri = getWorkspaceBackgroundOfDeepinWM(current);
        setWorkspaceBackgroundForDeepinWM(current, uri);
    }
#endif // DISABLE_DEEPIN_WM

    Q_EMIT WorkspaceBackgroundChanged(current, uri);
}

#ifndef DISABLE_DEEPIN_WM
void DeepinWMFaker::ChangeCurrentWorkspaceBackground(const QString &uri)
{
    SetCurrentWorkspaceBackground(uri);
}
#endif // DISABLE_DEEPIN_WM

int DeepinWMFaker::GetCurrentWorkspace() const
{
    return m_windowSystem->currentDesktop();
}

void DeepinWMFaker::SetCurrentWorkspace(const int index)
{
    // 切换工作区时关闭壁纸预览
    quitTransientBackground();
    m_windowSystem->setCurrentDesktop(index);
}

void DeepinWMFaker::NextWorkspace()
{
    // loopback support
//    int current = m_windowSystem->currentDesktop();
//    ++current < m_windowSystem->numberOfDesktops() ? current : loopback ? 0 : --current;
//    SetCurrentWorkspace(current);

   SetCurrentWorkspace(m_windowSystem->currentDesktop() + 1);
}

void DeepinWMFaker::PreviousWorkspace()
{
    // loopback support
//    int current = m_windowSystem->currentDesktop();
//    --current >= 0 ? current : loopback ? --(m_windowSystem->numberOfDesktops()) : 0;
//    SetCurrentWorkspace(current);

    SetCurrentWorkspace(m_windowSystem->currentDesktop() - 1);
}

/*!
 * [ { "Id":"...", "Accels":["...", "..."] }, {...} ]
 */
QString DeepinWMFaker::GetAllAccels() const
{
    QJsonArray allAccelsArray;
    for (auto it = AllDeepinWMKWinAccelsMap.constBegin(); it != AllDeepinWMKWinAccelsMap.constEnd(); ++it) {
        QJsonObject accelObj;
        accelObj.insert("Id", it.key());
        accelObj.insert("Accels", QJsonArray::fromStringList(GetAccel(it.key())));
        allAccelsArray.append(accelObj);
    }

    return QJsonDocument(allAccelsArray).toJson(QJsonDocument::Compact);
}

/*!
 * \brief DeepinWMFaker::GetAccel
 * \param id The deepin wm accel name
 * \return
 */
QStringList DeepinWMFaker::GetAccel(const QString &id) const
{
    if (id.isEmpty()) {
        return QStringList();
    }

    const QString &kId = AllDeepinWMKWinAccelsMap.value(id);
    if (kId.isEmpty()) {
        return QStringList();
    }

    const QList<QKeySequence> &seqList = m_globalAccel->globalShortcut(GlobalAccelComponentName, kId);
    if (seqList.isEmpty()) {
        return QStringList();
    }

    QStringList accelList;
    for (const QKeySequence &seq : seqList) {
        accelList.append(transToDaemonAccelStr(seq.toString()));
    }

    return accelList;
}

/*!
 * { "Id":"...", "Accels":["...", "..."] }
 */
bool DeepinWMFaker::SetAccel(const QString &data)
{
    if (data.isEmpty()) {
        return false;
    }

    const QJsonDocument &jsonDoc = QJsonDocument::fromJson(data.toUtf8());
    if (jsonDoc.isEmpty()) {
        return false;
    }

    bool result = true;

    const QJsonObject &jsonObj = jsonDoc.object();
    const QString &kId = AllDeepinWMKWinAccelsMap.value(jsonObj.value("Id").toString());
    if (kId.isEmpty()) {
        return false;
    }
    QAction *action = accelAction(kId);
    if (!action) {
        return false;
    }

    QList<QKeySequence> accelList;
    const QJsonArray &accelArray = jsonObj.value("Accels").toArray();
    for (const QJsonValue &jsonValue : accelArray) {
        const QString &accelStr = jsonValue.toString();
        QKeySequence seq(transFromDaemonAccelStr(accelStr));
        if (seq.isEmpty()) {
            qDebug() << "WARNING: got an empty key sequence for accel string:" << accelStr;
        }
        accelList.append(seq);
    }

    // using setGlobalShortcat() only can set a new accel,
    // it will not override the exist global accel just change the default accel
    if (!m_globalAccel->setShortcut(action, accelList, KGlobalAccel::NoAutoloading)) {
        qDebug() << "WARNING: set accel failed for" << kId << "with accels:" << accelList;
        result = false;
    }

    m_accelIdActionMap.insert(kId, action);

    return result;
}

/*!
 * \brief DeepinWMFaker::RemoveAccel
 * \param id The deepin wm accel name
 * \return
 */
void DeepinWMFaker::RemoveAccel(const QString &id)
{
    if (id.isEmpty()) {
        return;
    }

    const QString &kId = AllDeepinWMKWinAccelsMap.value(id);
    if (kId.isEmpty()) {
        return;
    }

    const bool contains = m_accelIdActionMap.contains(kId);

    QAction *action = accelAction(kId);
    if (!action) {
        return;
    }

    // remove will failed if the action is not handled by KGlobalAccel
    if (!contains) {
        m_globalAccel->setShortcut(
                    action, m_globalAccel->globalShortcut(GlobalAccelComponentName, kId));
    }

    m_globalAccel->removeAllShortcuts(action);

    m_accelIdActionMap.remove(kId);
    action->deleteLater();
}

void DeepinWMFaker::PreviewWindow(uint xid)
{
    // FIXME: preview window should not change the order of windows

    qDebug() << "winid" << xid;
    qDebug() << "windows" << m_windowSystem->windows();
    qDebug() << "order" << m_windowSystem->stackingOrder();
    qDebug() << "contains" << m_windowSystem->hasWId(xid);

    m_windowSystem->forceActiveWindow(xid);
    m_previewWinMiniPair.first = xid;
    m_previewWinMiniPair.second = false;

    KWindowInfo info(xid, NET::WMState | NET::XAWMState);
    if (info.valid()) {
        m_previewWinMiniPair.second = info.isMinimized();
    }

    qDebug() << "preview" << m_previewWinMiniPair;
}

void DeepinWMFaker::CancelPreviewWindow()
{
    // FIXME: same as above
    if (m_windowSystem->windows().contains(m_previewWinMiniPair.first)) {
        if (m_previewWinMiniPair.second) {
//            m_windowSystem->minimizeWindow(m_previewWinMiniPair.first);
            // using this way to minimize a window without animation
            m_windowSystem->setState(m_previewWinMiniPair.first, NET::Hidden);
            return;
        }
        m_windowSystem->lowerWindow(m_previewWinMiniPair.first);
    }
}

void DeepinWMFaker::PerformAction(int type)
{
    switch (type) {
    case wmActionShowWorkspace:
        ShowWorkspace();
        break;
    case wmActionToggleMaximize:
        ToggleActiveWindowMaximize();
        break;
    case wmActionMinimize:
        MinimizeActiveWindow();
        break;
    case wmActionShowWindow:
        ShowWindow();
        break;
    case wmActionShowAllWindow:
        ShowAllWindow();
        break;
    default:
        break;
    }
}

void DeepinWMFaker::BeginToMoveActiveWindow()
{
    m_kwinUtilsInter->WindowMove();
}

void DeepinWMFaker::SwitchApplication(bool backward)
{
    if (!m_kwinUtilsInter->isValid()) {
        return;
    }

    backward ? m_kwinUtilsInter->WalkBackThroughWindows()
             : m_kwinUtilsInter->WalkThroughWindows();
}

void DeepinWMFaker::TileActiveWindow(uint side)
{
    m_kwinUtilsInter->QuickTileWindow(side);
}

void DeepinWMFaker::ToggleActiveWindowMaximize()
{
    m_kwinUtilsInter->WindowMaximize();
}

void DeepinWMFaker::MinimizeActiveWindow()
{
    m_windowSystem->minimizeWindow(m_windowSystem->activeWindow());
}

void DeepinWMFaker::SetDecorationTheme(const QString &name)
{
    m_kwinConfig->group("org.kde.kdecoration2").writeEntry("theme", name);
    syncConfigForKWin();
}

void DeepinWMFaker::SetDecorationDeepinTheme(const QString &name)
{
    if (name == "light") {
        SetDecorationTheme("__aurorae__svg__deepin");
    } else if (name == "dark") {
        SetDecorationTheme("__aurorae__svg__deepin-dark");
    }
}

void DeepinWMFaker::setCompositingEnabled(bool on)
{
    m_kwinConfig->group("Compositing").writeEntry("Enabled", on);
    syncConfigForKWin();

    QDBusInterface compositor(KWinDBusService, KWinDBusCompositorPath, KWinDBusCompositorInterface);

    if (on) {
        compositor.call("resume");
    } else {
        compositor.call("suspend");
    }
}

void DeepinWMFaker::ShowAllWindow()
{
    m_kwinUtilsInter->ShowAllWindowsView();
}

void DeepinWMFaker::ShowWindow()
{
    m_kwinUtilsInter->ShowWindowsView();
}

void DeepinWMFaker::ShowWorkspace()
{
    m_kwinUtilsInter->ShowWorkspacesView();
}

#ifndef DISABLE_DEEPIN_WM
void DeepinWMFaker::SwitchToWorkspace(bool backward)
{
    backward ? PreviousWorkspace() : NextWorkspace();
}
#endif

/*!
 * \brief DeepinWMFaker::accelAction
 * \param accelKid The KWin accel name
 * \return
 */
QAction *DeepinWMFaker::accelAction(const QString accelKid) const
{
    if (accelKid.isEmpty()) {
        qDebug() << "ERROR: obtain action from an empty accel id";
        return nullptr;
    }

    QAction *action = m_accelIdActionMap.value(accelKid, nullptr);
    if (action) {
        return action;
    }

    // pass empty string to the constructor means to not change the Accel Friendly Name
    action = new QAction("");
    action->setObjectName(accelKid);
    action->setProperty("componentName", GlobalAccelComponentName);
    action->setProperty("componentDisplayName", GlobalAccelComponentDisplayName);

    return action;
}

QString DeepinWMFaker::transFromDaemonAccelStr(const QString &accelStr) const
{
    return QString(accelStr).remove("<")
            .replace(">", "+")
            .replace("Control", "Ctrl")
            .replace("Super", "Meta");
}

QString DeepinWMFaker::transToDaemonAccelStr(const QString &accelStr) const
{
    return QString(accelStr).replace("Shift+", "<Shift>")
            .replace("Ctrl+", "<Control>")
            .replace("Alt+", "<Alt>")
            .replace("Meta+", "<Super>");
}

QString DeepinWMFaker::getWorkspaceBackground(const int index) const
{
    return m_deepinWMWorkspaceBackgroundGroup->readEntry(QString::number(index));
}

void DeepinWMFaker::setWorkspaceBackground(const int index, const QString &uri)
{
    m_deepinWMWorkspaceBackgroundGroup->writeEntry(QString::number(index), uri);

    Q_EMIT WorkspaceBackgroundChanged(index, uri);
}

void DeepinWMFaker::quitTransientBackground()
{
    if (!m_transientBackgroundUri.isEmpty()) {
        m_transientBackgroundUri.clear();

        Q_EMIT WorkspaceBackgroundChanged(m_windowSystem->currentDesktop(), GetCurrentWorkspaceBackground());
    }

#ifndef DISABLE_DEEPIN_WM
    if (!m_deepinWMBackgroundUri.isEmpty()) {
        // 在退出预览时不同步deepin-wm的设置
        QSignalBlocker blocker(_gsettings_dde_appearance);
        Q_UNUSED(blocker)
        setWorkspaceBackgroundForDeepinWM(m_windowSystem->currentDesktop(), m_deepinWMBackgroundUri);
        m_deepinWMBackgroundUri.clear();
    }
#endif // DISABLE_DEEPIN_WM
}

#ifndef DISABLE_DEEPIN_WM
void DeepinWMFaker::onGsettingsDDEAppearanceChanged(const QString &key)
{
    if (QLatin1String(GsettingsBackgroundUri) == key) {
        const QStringList &uris = _gsettings_dde_appearance->get(GsettingsBackgroundUri).toStringList();

        for (int i = 0; i < uris.count(); ++i) {
            const QString &uri = uris.at(i);

            // 从 deepin-wm 中同步壁纸设置
            if (uri != getWorkspaceBackground(i + 1)) {
                setWorkspaceBackground(i + 1, uri);
            }
        }

        // 更新值
        if (!m_deepinWMBackgroundUri.isEmpty()) {
            m_deepinWMBackgroundUri = uris.value(m_windowSystem->currentDesktop());
        }
    }
}

static void setBorderActivate(KConfigGroup *group, int value, bool remove)
{
    const QString &activate = "BorderActivate";
    QStringList list = group->readEntry(activate).split(",");
    const QString &v = QString::number(value);

    if (remove) {
        list.removeAll(v);
    } else if (!list.contains(v)) {
        list.append(v);
    } else {
        return;
    }

    group->writeEntry(activate, list.join(","));
}

void DeepinWMFaker::onGsettingsDDEZoneChanged(const QString &key)
{
    ElectricBorder pos = ElectricNone;

    if (key == GsettingsZoneRightUp) {
        pos = ElectricTopRight;
    } else if (key == GsettingsZoneRightDown) {
        pos = ElectricBottomRight;
    } else if (key == GsettingsZoneLeftDown) {
        pos = ElectricBottomLeft;
    } else if (key == GsettingsZoneLeftUp) {
        pos = ElectricTopLeft;
    }

    const QString &value = _gsettings_dde_zone->get(key).toString();

    if (value.isEmpty()) {
        setBorderActivate(m_kwinCloseWindowGroup, pos, true);
        setBorderActivate(m_kwinRunCommandGroup, pos, true);
    } else {
        if (value == "!wm:close") {
            // 移除这个区域设置的其它命令
            setBorderActivate(m_kwinRunCommandGroup, pos, true);
            setBorderActivate(m_kwinCloseWindowGroup, pos, false);
        } else {
            // 移除这个区域设置的关闭窗口命令
            setBorderActivate(m_kwinCloseWindowGroup, pos, true);
            setBorderActivate(m_kwinRunCommandGroup, pos, false);

            const QString &program = QString("Border%1Program").arg(pos);
            m_kwinRunCommandGroup->writeEntry(program, value);
        }
    }

    syncConfigForKWin();
}

void DeepinWMFaker::syncConfigForKWin()
{
    // 同步配置到文件
    m_kwinConfig->sync();
    // 通知kwin重新加载配置文件
    QDBusInterface(KWinDBusService, KWinDBusPath).call("reconfigure");
}
#endif // DISABLE_DEEPIN_WM
