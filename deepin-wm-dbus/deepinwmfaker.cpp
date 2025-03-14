// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "deepinwmfaker.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDBusInterface>
#include <QProcess>
#include <QStringList>

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

#define DBUS_DEEPIN_WM_SERVICE   "com.deepin.wm"
#define DBUS_DEEPIN_WM_OBJ       "/com/deepin/wm"
#define DBUS_DEEPIN_WM_INTF      "com.deepin.wm"

#define DBUS_APPEARANCE_SERVICE "com.deepin.daemon.Appearance"
#define DBUS_APPEARANCE_OBJ "/com/deepin/daemon/Appearance"
#define DBUS_APPEARANCE_INTF "com.deepin.daemon.Appearance"

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
#define KWinDBusInterface "org.kde.KWin"
#define KWinDBusCompositorInterface "org.kde.kwin.Compositing"
#define KWinDBusCompositorPath "/Compositor"
const char defaultFirstBackgroundUri[] = "file:///usr/share/wallpapers/deepin/desktop.jpg";
const char defaultSecondBackgroundUri[] = "francesco-ungaro-1fzbUyzsHV8-unsplash";

//default cursor size :24
#define DEFAULTCURSORSIZE 24

const char fallback_background_name[] = "file:///usr/share/backgrounds/default_background.jpg";

const char wallpaper_lock[] = "/var/lib/deepin/permission-manager/wallpaper_locked";

using org::kde::KWin;

// deepin-wm's accel as Key
static QMap<QString, QString> AllDeepinWMKWinAccelsMap {
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
    { "preview-workspacea", "ShowMultitaskingA" },
    { "preview-workspacew", "ShowMultitaskingW" },
    { "view-zoom-in", "view_zoom_in" },
    { "view-zoom-out", "view_zoom_out" },
    { "view-actual-size", "view_actual_size" },
};

static const QMap<QString, QString> WaylandDeepinWMKWinAccelsMap {
    { "launcher" , "Launcher"},
    { "terminal" , "Terminal"},
    { "deepin-screen-recorder", "Screen Recorder"},
    { "lock-screen",            "Lock screen"},
    { "show-dock",              "Show/Hide the dock"},
    { "logout",                 "Shutdown interface"},
    { "terminal-quake",         "Terminal Quake Window"},
    { "screenshot",             "Screenshot"},
    { "screenshot-fullscreen",  "Full screenshot"},
    { "screenshot-window",      "Window screenshot"},
    { "screenshot-delayed",     "Delay screenshot"},
    { "file-manager",           "File manager"},
    { "disable-touchpad",       "Disable Touchpad"},
    { "wm-switcher",            "Switch window effects"},
    { "turn-off-screen",        "Fast Screen Off"},
    { "system-monitor",         "System Monitor"},
    { "color-picker",           "Deepin Picker"},
    { "ai-assistant",           "Desktop AI Assistant"},
    { "text-to-speech",         "Text to Speech"},
    { "speech-to-text",         "Speech to Text"},
    { "clipboard",              "Clipboard"},
    { "translation",            "Translation"},
    { "messenger",              "Messenger"},
    { "save",                   "Save"},
    { "new",                    "New"},
    { "wake-up",                "WakeUp"},
    { "audio-rewind",           "AudioRewind"},
    { "audio-mute",             "VolumeMute"},
    { "mon-brightness-up",      "MonBrightnessUp"},//mon-brightness-up
    { "wlan",                   "WLAN"},
    { "audio-media",            "AudioMedia"},
    { "reply",                  "Reply"},
    { "favorites",              "Favorites"},
    { "audio-play",             "AudioPlay"},
    { "audio-mic-mute",         "AudioMicMute"},
    { "audio-pause",            "AudioPause"},
    { "audio-stop",             "AudioStop"},
    { "power-off",              "PowerOff"},
    { "documents",              "Documents"},
    { "game",                   "Game"},
    { "search",                 "Search"},
    { "audio-record",           "AudioRecord"},
    { "display",                "Display"},
    { "reload",                 "Reload"},
    { "explorer",               "Explorer"},
    { "calculator",             "Calculator"},
    { "calendar",               "Calendar"},
    { "forward",                "Forward"},
    { "cut",                    "Cut"},
    { "mon-brightness-down",    "MonBrightnessDown"},
    { "copy",                   "Copy"},
    { "tools",                  "Tools"},
    { "audio-raise-volume",     "VolumeUp"},
    { "media-close",            "Media Close"},
    { "www",                    "WWW"},
    { "home-page",              "HomePage"},
    { "sleep",                  "Sleep"},
    { "audio-lower-volume",     "VolumeDown"},
    { "audio-prev",             "AudioPrev"},
    { "audio-next",             "AudioNext"},
    { "paste",                  "Paste"},
    { "open",                   "Open"},
    { "send",                   "Send"},
    { "my-computer",            "MyComputer"},
    { "mail",                   "Mail"},
    { "adjust-brightness",      "BrightnessAdjust"},
    { "log-off",                "LogOff"},
    { "pictures",               "Pictures"},
    { "terminal",               "Terminal"},
    { "video",                  "Video"},
    { "music",                  "Music"},
    { "app-left",               "ApplicationLeft"},
    { "app-right",              "ApplicationRight"},
    { "meeting",                "Meeting"},
    { "switch-monitors",        "Switch monitors"},
    { "capslock",               "Capslock"},
    { "numlock",                "Numlock"},
    { "script-wayland",         "ScriptWayland"},
    { "notification-center",    "Notification Center"},
    { "screenshot-ocr",    "ScreenshotOcr"},
    { "screenshot-scroll",    "ScreenshotScroll"},
    { "global-search",    "Global Search"},
};


static const QStringList NotConfigurationAction = {
    "Launcher",
    "Terminal",
    "Screen Recorder",
    "Lock screen",
    "Show/Hide the dock",
    "Shutdown interface",
    "Terminal Quake Window",
    "Screenshot",
    "Full screenshot",
    "Window screenshot",
    "Delay screenshot",
    "File manager",
    "Disable Touchpad",
    "Switch window effects",
    "Fast Screen Off",
    "System Monitor",
    "Deepin Picker",
    "Desktop AI Assistant",
    "Text to Speech",
    "Speech to Text",
    "Clipboard",
    "Translation",
    "Messenger",
    "Save",
    "New",
    "WakeUp",
    "AudioRewind",
    "VolumeMute",
    "MonBrightnessUp",
    "WLAN",
    "AudioMedia",
    "Reply",
    "Favorites",
    "AudioPlay",
    "AudioMicMute",
    "AudioPause",
    "AudioStop",
    "PowerOff",
    "Documents",
    "Game",
    "Search",
    "AudioRecord",
    "Display",
    "Reload",
    "Explorer",
    "Calculator",
    "Calendar",
    "Forward",
    "Cut",
    "MonBrightnessDown",
    "Copy",
    "Tools",
    "VolumeUp",
    "Media Close",
    "WWW",
    "HomePage",
    "Sleep",
    "VolumeDown",
    "AudioPrev",
    "AudioNext",
    "Paste",
    "Open",
    "Send",
    "MyComputer",
    "Mail",
    "BrightnessAdjust",
    "LogOff",
    "Pictures",
    "Terminal",
    "Video",
    "Music",
    "ApplicationLeft",
    "ApplicationRight",
    "Meeting",
    "Switch monitors",
    "Capslock",
    "Numlock",
    "Notification Center",
    "ScreenshotOcr",
    "ScreenshotScroll",
    "Global Search",
    "ScriptWayland",
};

static const QMap<QString, QString> SpecialKeyMap = {
    {"minus", "-"}, {"equal", "="}, {"bracketleft", "["}, {"bracketright", "]"},
    {"backslash", "\\"}, {"semicolon", ";"}, {"apostrophe", "'"}, {"comma", ","},
    {"period", "."}, {"slash", "/"}, {"grave", "`"},
};

static const QMap<QString, QString> KeysCombineWithShift = {
    {"`", "~"}, {"1", "!"}, {"2", "@"}, {"3", "#"}, {"4", "$"}, {"5", "%"}, {"6", "^"},
    {"7", "&"}, {"8", "*"}, {"9", "("}, {"0", ")"}, {"-", "_"}, {"=", "+"}, {"[", "{"},
    {"]", "}"}, {"\\", "|"}, {";", ":"}, {"'", "\""}, {",", "<"}, {".", ">"}, {"/", "?"}
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
    m_isPlatformX11 = isX11Platform();
#ifndef DISABLE_DEEPIN_WM
    m_currentDesktop = m_kwinConfig->group("Workspace").readEntry<int>("CurrentDesktop", 1);

    connect(m_windowSystem, &KWindowSystem::currentDesktopChanged, this, [this] (int to) {
        Q_EMIT WorkspaceSwitched(m_currentDesktop, to);
        m_currentDesktop = to;
    });
    connect(m_windowSystem, &KWindowSystem::numberOfDesktopsChanged, this, &DeepinWMFaker::workspaceCountChanged);
    connect(_gsettings_dde_appearance, &QGSettings::changed, this, &DeepinWMFaker::onGsettingsDDEAppearanceChanged);
    connect(_gsettings_dde_zone, &QGSettings::changed, this, &DeepinWMFaker::onGsettingsDDEZoneChanged);
    QDBusConnection::sessionBus().connect(KWinDBusService, KWinDBusPath, KWinDBusInterface, "MultitaskStateChanged", this, SLOT(slotUpdateMultiTaskingStatus(bool)));

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
    QDBusConnection::sessionBus().connect(KWinDBusService, KWinDBusCompositorPath, KWinDBusCompositorInterface,
                                          "effectsEnabledChanged", "b", this, SLOT(compositingEnabledChanged(bool)));

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

    auto e = QProcessEnvironment::systemEnvironment();
    QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));

    if (XDG_SESSION_TYPE != QLatin1String("x11")) {
        for (auto iter = WaylandDeepinWMKWinAccelsMap.begin(); iter != WaylandDeepinWMKWinAccelsMap.end(); iter++) {
            AllDeepinWMKWinAccelsMap.insert(iter.key(), iter.value());
        }
    }

    m_whiteProcess.insert("kwin_x11");
    m_whiteProcess.insert("kwin_wayland");
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
    return QDBusInterface(KWinDBusService, KWinDBusCompositorPath, KWinDBusCompositorInterface).property("active").toBool()
            && compositingType().startsWith('g');
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

QString DeepinWMFaker::compositingType() const
{
    return QDBusInterface(KWinDBusService, KWinDBusCompositorPath, KWinDBusCompositorInterface).property("compositingType").toString();
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
                QDBusInterface remoteApp(DBUS_APPEARANCE_SERVICE, DBUS_APPEARANCE_OBJ, DBUS_APPEARANCE_INTF);
                QDBusReply<QString> reply = remoteApp.call( "List", "background");

                QJsonDocument json = QJsonDocument::fromJson(reply.value().toUtf8());
                QJsonArray arr = json.array();
                if (!arr.isEmpty()) {
                    auto p = arr.constBegin();
                    while (p != arr.constEnd()) {
                        auto o = p->toObject();
                        if (!o.value("Id").isUndefined() && !o.value("Deletable").toBool()) {
                            if(o.value("Id").toString().contains(defaultSecondBackgroundUri)) {
                                uri = o.value("Id").toString();
                                break;
                            }
                        }
                        ++p;
                    }
                }
            }
        }
        const QString &workSpaceBackgroundUri = uri.toString();
        setWorkspaceBackgroundForMonitor(index, strMonitorName, workSpaceBackgroundUri);
    }
    return uri.toString();
}

bool DeepinWMFaker::isValidInvoker(const uint &pid)
{
    QFileInfo fileInfo(QString("/proc/%1/exe").arg(pid));
    if (!fileInfo.exists()) {
        return false;
    }

    QString invokerPath = fileInfo.canonicalFilePath().split("/").last();
    return m_whiteProcess.contains(invokerPath);
}

void DeepinWMFaker::SetWorkspaceBackgroundForMonitor(const int index, const QString &strMonitorName, const QString &uri)
{
    if (QFileInfo(wallpaper_lock).isFile()) {
        uint invokerPid = connection().interface()->servicePid(message().service());
        if (!isValidInvoker(invokerPid)) {
            QDBusConnection::sessionBus().send(message().createErrorReply(QLatin1String("com.deepin.wm.Error.locked"), QLatin1String("wallpaper locked")));
            return;
        }
    }
    m_transientBackgroundUri.clear();
    setWorkspaceBackgroundForMonitor( index,strMonitorName,uri );
}

QString DeepinWMFaker::GetCurrentWorkspaceBackgroundForMonitor(const QString &strMonitorName)
{
    return GetWorkspaceBackgroundForMonitor(m_currentDesktop, strMonitorName);
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
    int current = m_windowSystem->currentDesktop();
    if (current < m_windowSystem->numberOfDesktops()) {
        SetCurrentWorkspace(current + 1);
    }
}

void DeepinWMFaker::PreviousWorkspace()
{
    int current = m_windowSystem->currentDesktop();
    if (current > 1) {
        SetCurrentWorkspace(current - 1);
    }
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
    QString kId = AllDeepinWMKWinAccelsMap.value(jsonObj.value("Id").toString());
    if (kId.isEmpty()) {
        kId = jsonObj.value("Id").toString();
        qDebug() << "Info: add a new shortcut for kId:"<<kId;
    }
    QAction *action = accelAction(kId);
    if (!action) {
        return false;
    }

    QList<QKeySequence> accelList;
    const QJsonArray &accelArray = jsonObj.value("Accels").toArray();
    for (const QJsonValue &jsonValue : accelArray) {
        const QString &accelStr = jsonValue.toString();

        qDebug() << "transFromDaemonAccelStr:" << transFromDaemonAccelStr(accelStr);

    QKeySequence seq;
    if(transFromDaemonAccelStr(accelStr).contains("Qt::Key_")){
            bool isOk = false;
            QMetaEnum metaEnum = QMetaEnum::fromType<Qt::Key>();
            int iRet = metaEnum.keyToValue(accelStr.toStdString().c_str(),&isOk);
            if (iRet <0 || !isOk){
                qDebug() << "metaEnum err:" << accelStr;
                return false;
            }
            Qt::Key keycode = Qt::Key(iRet);
            seq = QKeySequence(keycode);
        }else{
            seq = QKeySequence(transFromDaemonAccelStr(accelStr));
        }

        if (seq.isEmpty() || seq.toString() == "") {
             qDebug() << "WARNING: got an empty key sequence for accel string:" << accelStr;
             continue;
        }

        if(!qgetenv("WAYLAND_DISPLAY").isEmpty()) {
            m_globalAccel->stealShortcutSystemwide(seq);
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
    QDBusMessage message = QDBusMessage::createSignal(DBUS_DEEPIN_WM_OBJ, DBUS_DEEPIN_WM_INTF, "QuickTileWindow");
    message << uint(side);
    QDBusConnection::sessionBus().send(message);
}

void DeepinWMFaker::ToggleActiveWindowMaximize()
{
    QDBusMessage message = QDBusMessage::createSignal(DBUS_DEEPIN_WM_OBJ, DBUS_DEEPIN_WM_INTF, "WindowMaximize");
    QDBusConnection::sessionBus().send(message);
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

    if (compositingEnabled() == on) {
        return;
    }

    QDBusInterface interfaceRequire("org.desktopspec.ConfigManager", "/", "org.desktopspec.ConfigManager", QDBusConnection::systemBus());
    QDBusReply<QDBusObjectPath> reply = interfaceRequire.call("acquireManager", "org.kde.kwin", "org.kde.kwin.compositing", "");
    if (!reply.isValid()) {
        qWarning() << "Error in DConfig reply:" << reply.error();
        return;
    }
    int type = on ? 1 : 4;
    QDBusInterface interfaceValue("org.desktopspec.ConfigManager", reply.value().path(), "org.desktopspec.ConfigManager.Manager", QDBusConnection::systemBus());
    interfaceValue.call("setValue", "user_type", QVariant::fromValue(QDBusVariant(type)));

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

    emit ShowWorkspaceChanged();
}

void DeepinWMFaker::setZoneEnabled(bool zoneEnabled)
{
    m_kwinCloseWindowGroup->writeEntry("Enabled", zoneEnabled);
    m_kwinRunCommandGroup->writeEntry("Enabled", zoneEnabled);
    syncConfigForKWin();
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
    if (m_isPlatformX11) {
        QList<WId> windows;

        for (uint w : xids)
            windows << w;

        KWindowEffects::presentWindows(windows.first(), windows);
    } else {
        QDBusInterface Interface("org.kde.KWin",
                                 "/org/kde/KWin/PresentWindows",
                                "org.kde.KWin.PresentWindows",
                                QDBusConnection::sessionBus());
        QStringList strList;
        for (uint w : xids)
            strList << QString::number(w);
        Interface.call("PresentWindows",strList);
    }
}

// TODO(zccrs): 开启/禁用热区
void DeepinWMFaker::EnableZoneDetected(bool enabled)
{
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
    action->setProperty("isConfigurationAction", !NotConfigurationAction.contains(accelKid));

    return action;
}

QString DeepinWMFaker::transFromDaemonAccelStr(const QString &accelStr) const
{
    if (accelStr.isEmpty()) {
        return accelStr;
    }

    QStringList keys;
    {
        QString str(accelStr);
        str.remove("<")
                .replace("Control", "Ctrl")
                .replace("Super", "Meta");
        keys = str.split('>');
    }

    int shift_index = keys.indexOf("Shift");

    for (QString &key : keys) {
        if (SpecialKeyMap.contains(key)) {
            key = SpecialKeyMap[key];
            if (shift_index == -1) {
                return keys.join('+');
            }
        }
    }

    if (shift_index != -1) {
        for (QString &key : keys) {
            if (KeysCombineWithShift.contains(key)) {
                key = KeysCombineWithShift[key];
                keys.removeAt(shift_index);
                return keys.join('+');
            }
        }
    }

    for (QString &key : keys) {
        if (SpecialRequireShiftKeyMap.contains(key)) {
            key = SpecialRequireShiftKeyMap[key];
            if (shift_index != -1) {
                keys.removeAt(shift_index);
            }
            return keys.join('+');
        }
    }

    return keys.join('+');
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

bool DeepinWMFaker::isX11Platform()
{
    QString strCmd = "loginctl show-session $(loginctl | grep $(whoami) | awk '{print $1}') -p Type";
    QProcess p;
    p.start("bash", QStringList() <<"-c" << strCmd);
    p.waitForFinished();
    QString result = p.readAllStandardOutput();
    if (result.replace("\n", "").contains("Type=x11")) {
        return  true;
    } else {
        return  false;
    }
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
    qWarning() << "sync config";
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

void DeepinWMFaker::slotUpdateMultiTaskingStatus(bool isActive)
{
    m_isMultitaskingActived = isActive;
}

bool DeepinWMFaker::GetIsShowDesktop()
{
    QDBusInterface interface_kwin(KWinDBusService, KWinDBusPath);

    QDBusReply<bool> reply = interface_kwin.call("showingDesktop");
    if (reply.isValid()) {
        return reply.value();
    } else {
        QDBusInterface interface(KWinDBusService, KWinDBusPath, "org.freedesktop.DBus.Properties", QDBusConnection::sessionBus());
        QDBusReply<QVariant> reply = interface.call("Get", KWinDBusInterface, "showingDesktop");
        if (reply.isValid()) {
            return reply.value().toBool();
        }
    }
    return false;
}
