#include "deepinwmfaker.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <KF5/KConfigCore/KConfig>
#include <KF5/KConfigCore/KConfigGroup>
#include <KF5/KConfigCore/KSharedConfig>
#include <KF5/KWindowSystem/KWindowSystem>
#include <KF5/KWindowSystem/KWindowEffects>
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
#define KWinUtilsDbusInterface "org.kde.KWin"
#define KWinDBusCompositorInterface "org.kde.kwin.Compositing"
#define KWinDBusCompositorPath "/Compositor"
const char defaultFirstBackgroundUri[] = "file:///usr/share/wallpapers/deepin/desktop.jpg";
#ifdef __mips__
    const char defaultSecondBackgroundUri[] = "file:///usr/share/wallpapers/deepin/francesco-ungaro-1fzbUyzsHV8-unsplash.bmp";
#else
    const char defaultSecondBackgroundUri[] = "file:///usr/share/wallpapers/deepin/francesco-ungaro-1fzbUyzsHV8-unsplash.jpg";
#endif

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
    { "preview-workspace", "ShowMultitasking" },
};

static const QMap<QString, QString> SpecialKeyMap = {
    {"minus", "-"}, {"equal", "="}, {"brackertleft", "["}, {"breckertright", "]"},
    {"backslash", "\\"}, {"semicolon", ";"}, {"apostrophe", "'"}, {"comma", ","},
    {"period", "."}, {"slash", "/"}, {"grave", "`"},
};

static const QMap<QString, QString> SpecialRequireShiftKeyMap = {
    {"exclam", "!"}, {"at", "@"}, {"numbersign", "#"}, {"dollar", "$"},
    {"percent", "%"}, {"asciicircum", "^"}, {"ampersand", "&"}, {"asterisk", "*"},
    {"parenleft", "("}, {"parenright", ")"}, {"underscore", "_"}, {"plus", "+"},
    {"braceleft", "{"}, {"braceright", "}"}, {"bar", "|"}, {"colon", ":"},
    {"quotedbl", "\""}, {"less", "<"}, {"greater", ">"}, {"question", "?"},
    {"asciitilde", "~"}
};

DeepinWMFaker::DeepinWMFaker(QObject *parent)
    : QObject(parent)
    , m_windowSystem(KWindowSystem::self())
    , m_deepinWMConfig(new KConfig(DeepinWMConfigName, KConfig::CascadeConfig))
    , m_deepinWMGeneralGroup(new KConfigGroup(m_deepinWMConfig->group(DeepinWMGeneralGroupName)))
    , m_deepinWMWorkspaceBackgroundGroup(new KConfigGroup(m_deepinWMConfig->group(DeepinWMWorkspaceBackgroundGroupName)))
    , m_kwinConfig(new KConfig(KWinConfigName, KConfig::CascadeConfig))
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
    connect(m_windowSystem, &KWindowSystem::numberOfDesktopsChanged, this, &DeepinWMFaker::workspaceCountChanged);
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

    QDBusConnection::sessionBus().connect(KWinDBusService, KWinDBusCompositorPath, KWinDBusCompositorInterface,
                                          "compositingToggled", "b", this, SLOT(wmCompositingEnabledChanged(bool)));

    // 迁移旧的标题栏主题插件配置
    KConfigGroup decoration_group(m_kwinConfig, "org.kde.kdecoration2");

    if (decoration_group.readEntry("library", QString()) == "org.kde.kwin.aurorae") {
        const QString &theme = decoration_group.readEntry("theme", QString());

        // 自动将旧的主题更新为新的插件配置项
        if (theme == "__aurorae__svg__deepin") {
            SetDecorationDeepinTheme("light");
        } else if (theme == "__aurorae__svg__deepin-dark") {
            SetDecorationDeepinTheme("dark");
        }
    }
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

bool DeepinWMFaker::compositingPossible() const
{
    return QDBusInterface(KWinDBusService, KWinDBusCompositorPath, KWinDBusCompositorInterface).property("compositingPossible").toBool();
}

bool DeepinWMFaker::compositingAllowSwitch() const
{
    if (qgetenv("KWIN_COMPOSE").startsWith("N"))
        return false;

    if (QDBusInterface(KWinDBusService, KWinDBusCompositorPath, KWinDBusCompositorInterface)
            .property("platformRequiresCompositing").toBool()) {
        return false;
    }

    return m_kwinConfig->group("Compositing").readEntry("AllowSwitch", true);
}

bool DeepinWMFaker::zoneEnabled() const
{
    bool enable_closewindow = m_kwinCloseWindowGroup->readEntry("Enabled", QVariant(true)).toBool();

    if (enable_closewindow)
        return true;

    return m_kwinRunCommandGroup->readEntry("Enabled", QVariant(true)).toBool();
}

QString DeepinWMFaker::cursorTheme() const
{
    KConfigGroup mousecfg(KSharedConfig::openConfig("kcminputrc", KConfig::NoGlobals), "Mouse");
    const QString themeName = mousecfg.readEntry("cursorTheme", "default");

    return themeName;
}

int DeepinWMFaker::cursorSize() const
{
    KConfigGroup mousecfg(KSharedConfig::openConfig("kcminputrc", KConfig::NoGlobals), "Mouse");
    bool ok = false;
    int themeSize = mousecfg.readEntry("cursorSize", QString("0")).toInt(&ok);

    return ok ? themeSize : -1;
}

#ifndef DISABLE_DEEPIN_WM
static QString getWorkspaceBackgroundOfDeepinWM(const int index)
{
    return _gsettings_dde_appearance->get(GsettingsBackgroundUri).toStringList().value(index - 1);
}

static void setWorkspaceBackgroundForDeepinWM(const int index, const QString &uri)
{
    QStringList all_wallpaper = _gsettings_dde_appearance->get(GsettingsBackgroundUri).toStringList();

    // 当设置的工作区编号大于列表长度时，先填充数据
    if (index > all_wallpaper.size()) {
        all_wallpaper.reserve(index);

        for (int i = all_wallpaper.size(); i < index; ++i) {
            all_wallpaper.append(QString());
        }
    }

    all_wallpaper[index - 1] = uri;
    // 将壁纸设置同步到 deepin-wm
    _gsettings_dde_appearance->set(GsettingsBackgroundUri, all_wallpaper);
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

QString DeepinWMFaker::GetWorkspaceBackgroundForMonitor(const int index,const QString &strMonitorName) const
{
    QUrl uri = getWorkspaceBackgroundForMonitor(index, strMonitorName);
    if (uri.isEmpty()) {
        uri = _gsettings_dde_appearance->get(GsettingsBackgroundUri).toStringList().value(index - 1);
        if (index == 1) {
            if(!QFileInfo(uri.path()).isFile()) {
                uri = defaultFirstBackgroundUri;
            }
        } else {
            if (!QFileInfo(uri.path()).isFile()) {
                uri = defaultSecondBackgroundUri;
            }
        }
        const QString &workSpaceBackgroundUri = uri.toString();
        setWorkspaceBackgroundForMonitor(index, strMonitorName, workSpaceBackgroundUri);
    }
    return uri.toString();
}

void DeepinWMFaker::SetWorkspaceBackgroundForMonitor(const int index, const QString &strMonitorName, const QString &uri)
{
    m_transientBackgroundUri.clear();
    setWorkspaceBackgroundForMonitor( index,strMonitorName,uri );
}

QString DeepinWMFaker::GetCurrentWorkspaceBackgroundForMonitor(const QString &strMonitorName)
{
    return GetWorkspaceBackgroundForMonitor( m_windowSystem->currentDesktop(), strMonitorName);
}
void DeepinWMFaker::SetCurrentWorkspaceBackgroundForMonitor(const QString &uri, const QString &strMonitorName)
{
    SetWorkspaceBackgroundForMonitor(  m_windowSystem->currentDesktop(), strMonitorName, uri );
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

void DeepinWMFaker::SetTransientBackgroundForMonitor(const QString &uri, const QString &strMonitorName)
{
     int current = m_windowSystem->currentDesktop();

     m_transientBackgroundUri = uri;
     Q_EMIT WorkspaceBackgroundChangedForMonitor( current,strMonitorName,uri );
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

int DeepinWMFaker::WorkspaceCount() const
{
    return m_windowSystem->numberOfDesktops();
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
        accelObj.insert("Default", QJsonArray::fromStringList(GetDefaultAccel(it.key())));
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

static QMap<QString, QList<QKeySequence>> getShoutcutListFromKDEConfigFile()
{
    // 认为系统配置文件中存储的快捷键为默认值
    KConfig kglobalshortcutsrc("/etc/xdg/kglobalshortcutsrc");
    KConfigGroup kwin(&kglobalshortcutsrc, "kwin");

    if (!kwin.isValid())
        return {};

    const QStringList key_list = kwin.keyList();
    QMap<QString, QList<QKeySequence>> result;

    for (const QString &str : key_list) {
        auto value_list = kwin.readEntry(str, QStringList());

        if (value_list.isEmpty())
            continue;

        QList<QKeySequence> ks_list;

        // 多个快捷键是以制表符为分隔符
        for (const QString &key : value_list.first().split("\t")) {
            QKeySequence ks(key);

            if (!ks.isEmpty()) {
                ks_list << ks;
            }
        }

        result[str] = ks_list;
    }

    return result;
}

QStringList DeepinWMFaker::GetDefaultAccel(const QString &id) const
{
    if (id.isEmpty()) {
        return QStringList();
    }

    const QString &kId = AllDeepinWMKWinAccelsMap.value(id);
    if (kId.isEmpty()) {
        return QStringList();
    }

    static auto shortcutMap = getShoutcutListFromKDEConfigFile();
    const QList<QKeySequence> &seqList = shortcutMap.value(kId);
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
            // qDebug() << "WARNING: got an empty key sequence for accel string:" << accelStr;
        }
        accelList.append(seq);
    }

    // using setGlobalShortcat() only can set a new accel,
    // it will not override the exist global accel just change the default accel
    if (!m_globalAccel->setShortcut(action, accelList, KGlobalAccel::NoAutoloading)) {
        // qDebug() << "WARNING: set accel failed for" << kId << "with accels:" << accelList;
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

static WId previewingController = 0;
void DeepinWMFaker::PreviewWindow(uint xid)
{
    QDBusInterface interface_kwin(KWinDBusService, KWinDBusPath);

    interface_kwin.call("previewWindows", QVariant::fromValue(QList<uint>({xid})));

    if (interface_kwin.lastError().type() == QDBusError::NoError) {
        return;
    } // else 兼容非deepin-kwin的环境

    // 只允许同时预览一个窗口
    if (previewingController) {
        return;
    }

    // 使用kwin自带的预览特效
    if (KWindowEffects::isEffectAvailable(KWindowEffects::HighlightWindows)) {
        // ###(zccrs): 按道理讲 previewingController 应该为dock的预览展示窗口（发起预览请求的窗口）
        // 不过，dde-dock中不支持此种用法，而且对kwin接口的调用仅仅是fallback，因此直接将xid作为预览请求的controller窗口
        previewingController = xid;
        KWindowEffects::highlightWindows(previewingController, {xid});
        return;
    }

    // FIXME: preview window should not change the order of windows

    // qDebug() << "winid" << xid;
    // qDebug() << "windows" << m_windowSystem->windows();
    // qDebug() << "order" << m_windowSystem->stackingOrder();
    // qDebug() << "contains" << m_windowSystem->hasWId(xid);

    m_windowSystem->forceActiveWindow(xid);
    m_previewWinMiniPair.first = xid;
    m_previewWinMiniPair.second = false;

    KWindowInfo info(xid, NET::WMState | NET::XAWMState);
    if (info.valid()) {
        m_previewWinMiniPair.second = info.isMinimized();
    }

    // qDebug() << "preview" << m_previewWinMiniPair;
}

void DeepinWMFaker::CancelPreviewWindow()
{
    QDBusInterface interface_kwin(KWinDBusService, KWinDBusPath);

    interface_kwin.call("quitPreviewWindows");

    if (interface_kwin.lastError().type() == QDBusError::NoError) {
        return;
    } // else 兼容非deepin-kwin的环境

    // 退出kwin自带的预览特效
    if (previewingController) {
        KWindowEffects::highlightWindows(previewingController, {});
        previewingController = 0;
        return;
    }

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

void DeepinWMFaker::TouchToMove(int x, int y)
{
#ifndef DISABLE_DEEPIN_WM
    m_kwinUtilsInter->TouchPadToMoveWindow(x,y);
#endif
}

void DeepinWMFaker::ClearMoveStatus()
{
#ifndef DISABLE_DEEPIN_WM
    m_kwinUtilsInter->EndTouchPadToMoveWindow();
#endif
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

void DeepinWMFaker::SetDecorationTheme(const QString &type, const QString &name)
{
    m_kwinConfig->group("org.kde.kdecoration2").writeEntry("theme", QVariant());
    m_kwinConfig->group("org.kde.kdecoration2").writeEntry("library", "com.deepin.chameleon");
    m_kwinConfig->group("deepin-chameleon").writeEntry("theme", type + "/" + name);

    syncConfigForKWin();
}

void DeepinWMFaker::SetDecorationDeepinTheme(const QString &name)
{
    SetDecorationTheme(name, "deepin");
}

void DeepinWMFaker::setCompositingEnabled(bool on)
{
    if (!compositingAllowSwitch()) {
        return;
    }

    if (on) {
        // 记录opengl被标记为不安全的次数
        if (m_kwinConfig->group("Compositing").readEntry<bool>("OpenGLIsUnsafe", false)) {
            int count = m_kwinConfig->group("Compositing").readEntry<int>("OpenGLIsUnsafeCount", 0);
            m_kwinConfig->group("Compositing").writeEntry("OpenGLIsUnsafeCount", count + 1);
        }

        // 确保3D特效一定能被开启
        m_kwinConfig->group("Compositing").writeEntry("OpenGLIsUnsafe", false);
    }

    m_kwinConfig->group("Compositing").writeEntry("Enabled", on);
    // 只同步配置文件，不要通知kwin重新加载配置
    m_kwinConfig->sync();

    if (compositingEnabled() == on) {
        return;
    }

    if (on)
        m_kwinUtilsInter->ResumeCompositor(1);
    else
        m_kwinUtilsInter->SuspendCompositor(1);

    // !on 时说明再关闭窗口特效，关闭特效往往都能成功，因此不再需要判断是否成功（KWin中给出值时有些延迟，导致未能及时获取到值）
    if (!on || compositingEnabled() == on)
        emit compositingEnabledChanged(on);
}

// 2D效果下不支持显示工作区、显示应用程序所有窗口、显示所有窗口等功能，此处调用对话框告知用户
bool DeepinWMFaker::maybeShowWarningDialog()
{
    if (!compositingEnabled()) {
        return QProcess::startDetached("/usr/lib/deepin-daemon/dde-warning-dialog");
    }

    return false;
}

void DeepinWMFaker::ShowAllWindow()
{
    if (maybeShowWarningDialog())
        return;

    m_kwinUtilsInter->ShowAllWindowsView();
}

void DeepinWMFaker::ShowWindow()
{
    if (maybeShowWarningDialog())
        return;

    m_kwinUtilsInter->ShowWindowsView();
}

void DeepinWMFaker::ShowWorkspace()
{
    if (maybeShowWarningDialog())
        return;

    m_kwinUtilsInter->ShowWorkspacesView();
}

void DeepinWMFaker::setZoneEnabled(bool zoneEnabled)
{
    QDBusInterface interface_kwin_utils(KWinDBusService, "/dde", KWinUtilsDbusInterface);
    interface_kwin_utils.call("EnableZoneDetected", zoneEnabled);
}

static bool updateCursorConfig()
{
    if (!KSharedConfig::openConfig("kcminputrc", KConfig::NoGlobals)->sync()) {
        return false;
    }

    auto message = QDBusMessage::createSignal(QStringLiteral("/KGlobalSettings"),
                                              QStringLiteral("org.kde.KGlobalSettings"),
                                              QStringLiteral("notifyChange"));

    // 添加 notify 类型参数 (ChangeCursor = 5)
    message << 5;
    // 添加任意 int 参数，未被使用
    message << 0;

    return QDBusConnection::sessionBus().send(message);
}

void DeepinWMFaker::setCursorTheme(QString cursorTheme)
{
    KConfigGroup mousecfg(KSharedConfig::openConfig("kcminputrc", KConfig::NoGlobals), "Mouse");
    mousecfg.writeEntry("cursorTheme", cursorTheme);
    updateCursorConfig();
}

void DeepinWMFaker::setCursorSize(int cursorSize)
{
    KConfigGroup mousecfg(KSharedConfig::openConfig("kcminputrc", KConfig::NoGlobals), "Mouse");
    mousecfg.writeEntry("cursorSize", QString::number(cursorSize));
    updateCursorConfig();
}

#ifndef DISABLE_DEEPIN_WM
void DeepinWMFaker::SwitchToWorkspace(bool backward)
{
    backward ? PreviousWorkspace() : NextWorkspace();
}

void DeepinWMFaker::PresentWindows(const QList<uint> &xids)
{
    if (xids.isEmpty())
        return;

    QList<WId> windows;

    for (uint w : xids)
        windows << w;

    KWindowEffects::presentWindows(windows.first(), windows);
}

// TODO(zccrs): 开启/禁用热区
void DeepinWMFaker::EnableZoneDetected(bool enabled)
{
    setZoneEnabled(enabled);
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
        // qDebug() << "ERROR: obtain action from an empty accel id";
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
    //NOTE: this is from KGlobalAccel
    //
    //a isConfigurationAction shortcut combined with NoAutoloading will
    //make it a foreign shortcut, which triggers a dbus signal sent to
    //kglobalaccel when changed. this gives KWin the chance to listen for
    //the externally shortcut changes and and allow effects to respond.
    action->setProperty("isConfigurationAction", true);

    return action;
}

QString DeepinWMFaker::transFromDaemonAccelStr(const QString &accelStr) const
{
    if (accelStr.isEmpty()) {
        return accelStr;
    }

    QString str(accelStr);

    str.remove("<")
            .replace(">", "+")
            .replace("Control", "Ctrl")
            .replace("Super", "Meta");

    for (auto it = SpecialKeyMap.constBegin(); it != SpecialKeyMap.constEnd(); ++it) {
        QString origin(str);
        str.replace(it.key(), it.value());
        if (str != origin) {
            return str;
        }
    }

    for (auto it = SpecialRequireShiftKeyMap.constBegin(); it != SpecialRequireShiftKeyMap.constEnd(); ++it) {
        QString origin(str);
        str.replace(it.key(), it.value());
        if (str != origin) {
            return str.remove("Shift+");
        }
    }

    return str;
}

QString DeepinWMFaker::transToDaemonAccelStr(const QString &accelStr) const
{
    if (accelStr.isEmpty()) {
        return accelStr;
    }

    QString str(accelStr);

    str.replace("Shift+", "<Shift>")
            .replace("Ctrl+", "<Control>")
            .replace("Alt+", "<Alt>")
            .replace("Meta+", "<Super>")
            .replace("Backtab", "Tab");

    for (auto it = SpecialKeyMap.constBegin(); it != SpecialKeyMap.constEnd(); ++it) {
        if (it.value() == str.at(str.length() - 1)) {
            str.chop(1);
            return str.append(it.key());
        }
    }

    for (auto it = SpecialRequireShiftKeyMap.constBegin(); it != SpecialRequireShiftKeyMap.constEnd(); ++it) {
        if (it.value() == str.at(str.length() - 1)) {
            str.chop(1);
            str = str.append(it.key());
            if (!str.contains("<Shift>")) {
                str = str.prepend("<Shift>");
            }
            return str;
        }
    }

    return str;
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

QString DeepinWMFaker::getWorkspaceBackgroundForMonitor(const int index, const QString &strMonitorName) const
{
    return  m_deepinWMConfig->group("WorkspaceBackground").readEntry( QString("%1%2%3").arg(index).arg("@" ,strMonitorName)) ;
}
void DeepinWMFaker::setWorkspaceBackgroundForMonitor(const int index, const QString &strMonitorName, const QString &uri) const
{
    m_deepinWMWorkspaceBackgroundGroup->writeEntry(QString("%1%2%3").arg(index).arg("@" ,strMonitorName), uri);
    m_deepinWMConfig->sync();

#ifndef DISABLE_DEEPIN_WM
    QStringList allWallpaper = _gsettings_dde_appearance->get(GsettingsBackgroundUri).toStringList();

    if (index > allWallpaper.size()) {
        allWallpaper.reserve(index);

        for (int i = allWallpaper.size(); i < index; ++i) {
            allWallpaper.append(QString());
        }
    }

    allWallpaper[index - 1] = uri;
    _gsettings_dde_appearance->set(GsettingsBackgroundUri, allWallpaper);
#endif // DISABLE_DEEPIN_WM
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

void DeepinWMFaker::updateCursorConfig()
{
    if (!::updateCursorConfig() && calledFromDBus()) {
        auto error = QDBusConnection::sessionBus().lastError();

        if (error.type() == QDBusError::NoError) {
            error = QDBusError(QDBusError::Failed, "Failed on sync kcminputrc");
        }

        setDelayedReply(true);
        connection().send(QDBusMessage::createError(error));
    }
}
#endif // DISABLE_DEEPIN_WM

bool DeepinWMFaker::GetMultiTaskingStatus()
{
    return m_isMultitaskingActived;
}

void DeepinWMFaker::SetMultiTaskingStatus(bool isActive)
{
    m_isMultitaskingActived = isActive;
}

bool DeepinWMFaker::GetIsShowDesktop()
{
    return m_isShowDesktop;
}

void DeepinWMFaker::SetShowDesktop(bool isShowDesktop)
{
    m_isShowDesktop = isShowDesktop;
}
