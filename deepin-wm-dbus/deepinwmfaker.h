#ifndef DEEPINWMFAKER_H
#define DEEPINWMFAKER_H

#include "kwinutils_interface.h"

#include <QAction>
#include <QObject>
#include <QDBusContext>

class KWindowSystem;
class KConfig;
class KConfigGroup;
class KGlobalAccel;

class DeepinWMFaker : public QObject, protected QDBusContext
{
    Q_OBJECT

    Q_PROPERTY(bool compositingEnabled READ compositingEnabled WRITE setCompositingEnabled NOTIFY compositingEnabledChanged)
    Q_PROPERTY(bool compositingAllowSwitch READ compositingAllowSwitch FINAL)
    Q_PROPERTY(bool compositingPossible READ compositingPossible)
    Q_PROPERTY(bool zoneEnabled READ zoneEnabled WRITE setZoneEnabled)
    Q_PROPERTY(QString cursorTheme READ cursorTheme WRITE setCursorTheme)
    Q_PROPERTY(int cursorSize READ cursorSize WRITE setCursorSize)

public:
    explicit DeepinWMFaker(QObject *parent = nullptr);
    ~DeepinWMFaker();

    enum Action {
        wmActionShowWorkspace = 1,
        wmActionToggleMaximize = 2,
        wmActionMinimize = 3,
        wmActionShowWindow    = 6,
        wmActionShowAllWindow = 7,
    };

    enum LayoutDirection {
        wmTileDirectionLeft =  1,
        wmTileDirectionRight = 2,
    };

    // position of possible screen edge(zone)
    // DDE currently only support the four corners
    enum ElectricBorder {
        ElectricTop,
        ElectricTopRight,
        ElectricRight,
        ElectricBottomRight,
        ElectricBottom,
        ElectricBottomLeft,
        ElectricLeft,
        ElectricTopLeft,
        ELECTRIC_COUNT,
        ElectricNone
    };

    bool compositingEnabled() const;
    bool compositingPossible() const;
    bool compositingAllowSwitch() const;
    bool zoneEnabled() const;

    QString cursorTheme() const;
    int cursorSize() const;

public Q_SLOTS:
    QString GetWorkspaceBackground(const int index) const;
    void SetWorkspaceBackground(const int index, const QString &uri);

    QString GetCurrentWorkspaceBackground() const;
    void SetCurrentWorkspaceBackground(const QString &uri);

    QString GetWorkspaceBackgroundForMonitor(const int index,const QString &strMonitorName) const;
    void SetWorkspaceBackgroundForMonitor(const int index, const QString &strMonitorName,const QString &uri);

    QString GetCurrentWorkspaceBackgroundForMonitor(const QString &strMonitorName);
    void SetCurrentWorkspaceBackgroundForMonitor(const QString &uri, const QString &strMonitorName);

    // 壁纸预览
    void SetTransientBackground(const QString &uri);

    void SetTransientBackgroundForMonitor(const QString &uri, const QString &strMonitorName);
#ifndef DISABLE_DEEPIN_WM
    void ChangeCurrentWorkspaceBackground(const QString &uri);
#endif

    int GetCurrentWorkspace() const;
    int WorkspaceCount() const;
    void SetCurrentWorkspace(const int index);
    void NextWorkspace();
    void PreviousWorkspace();
#ifndef DISABLE_DEEPIN_WM
    void SwitchToWorkspace(bool backward);
    void PresentWindows(const QList<uint> &xids);
    void EnableZoneDetected(bool enabled);
#endif

    QString GetAllAccels() const;
    QStringList GetAccel(const QString &id) const;
    QStringList GetDefaultAccel(const QString &id) const;
    bool SetAccel(const QString &data);
    void RemoveAccel(const QString &id);

    void PreviewWindow(uint xid);
    void CancelPreviewWindow();

    void PerformAction(int type);
    void BeginToMoveActiveWindow();
    void SwitchApplication(bool backward);
    void TileActiveWindow(uint side);
    void ToggleActiveWindowMaximize();
    void MinimizeActiveWindow();

    void SetDecorationTheme(const QString &type, const QString &name);
    void SetDecorationDeepinTheme(const QString &name);

    void setCompositingEnabled(bool on);

    void ShowAllWindow();
    void ShowWindow();
    void ShowWorkspace();

    void setZoneEnabled(bool zoneEnabled);

    void setCursorTheme(QString cursorTheme);
    void setCursorSize(int cursorSize);

    // Touch Screen
    bool GetMultiTaskingStatus();
    void SetMultiTaskingStatus(bool isActive);
    void SetTouchBorderInterval(double second = 0.5);
    double GetTouchBorderInterval();

    // minin client
    bool GetIsShowDesktop();
    void SetShowDesktop(bool isShowDesktop);

    bool GetCurrentDesktopStatus();
Q_SIGNALS:
    void WorkspaceBackgroundChanged(int index, const QString &newUri);
    void WorkspaceBackgroundChangedForMonitor(int index, const QString &strMonitorName, const QString &newUri);
#ifndef DISABLE_DEEPIN_WM
    // 兼容deepin-wm提供的接口
    void WorkspaceSwitched(int from, int to);
#endif

    // 由窗管通知的混成变化信号
    void wmCompositingEnabledChanged(bool compositingEnabled);
    // 只在由此DBus调用引起的窗管混成变化时发出
    void compositingEnabledChanged(bool compositingEnabled);
    // 工作区个数
    void workspaceCountChanged(int count);

    void desktopStatusChanged();

private:
    QAction *accelAction(const QString accelKid) const;
    QString transFromDaemonAccelStr(const QString &accelStr) const;
    QString transToDaemonAccelStr(const QString &accelStr) const;

    QString getWorkspaceBackground(const int index) const;
    void setWorkspaceBackground(const int index, const QString &uri);
    void quitTransientBackground();

    QString getWorkspaceBackgroundForMonitor(const int index,const QString &strMonitorName) const;                    // index 为工作区索引，strMonitorName为显示器的名称
    void setWorkspaceBackgroundForMonitor(const int index, const QString &strMonitorName, const QString &uri);         // index 为工作区索引，strMonitorName为显示器的名称，uri为图片资源

#ifndef DISABLE_DEEPIN_WM
    void onGsettingsDDEAppearanceChanged(const QString &key);
    void onGsettingsDDEZoneChanged(const QString &key);
#endif

private:
    void syncConfigForKWin();
    void updateCursorConfig();
    bool maybeShowWarningDialog();

    KWindowSystem *m_windowSystem;
    KConfig *m_deepinWMConfig;
    KConfigGroup *m_deepinWMGeneralGroup;
    KConfigGroup *m_deepinWMWorkspaceBackgroundGroup;
    KConfig *m_kwinConfig;
    KConfigGroup *m_kwinCloseWindowGroup;
    KConfigGroup *m_kwinRunCommandGroup;
    KGlobalAccel *m_globalAccel;
    org::kde::KWin *m_kwinUtilsInter;

    QMap<QString, QAction *> m_accelIdActionMap;

    QString m_transientBackgroundUri;
#ifndef DISABLE_DEEPIN_WM
    QString m_deepinWMBackgroundUri;
    int m_currentDesktop = -1;
#endif

    QPair<uint, bool> m_previewWinMiniPair;

    bool m_isMultitaskingActived = false;
    double touchBorderInterval = 0.5;

    bool m_isShowDesktop = false;
};

#endif // DEEPINWMFAKER_H
