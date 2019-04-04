#ifndef DEEPINWMFAKER_H
#define DEEPINWMFAKER_H

#include "kwinutils_interface.h"

#include <QAction>
#include <QObject>

class KWindowSystem;
class KConfig;
class KConfigGroup;
class KGlobalAccel;

class DeepinWMFaker : public QObject
{
    Q_OBJECT
public:
    explicit DeepinWMFaker(QObject *parent = nullptr);

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

public Q_SLOTS:
    QString GetWorkspaceBackground(const int index) const;
    void SetWorkspaceBackground(const int index, const QString &uri);
    QString GetCurrentWorkspaceBackground() const;
    void SetCurrentWorkspaceBackground(const QString &uri);
    // 壁纸预览
    void SetTransientBackground(const QString &uri);
#ifndef DISABLE_DEEPIN_WM
    void ChangeCurrentWorkspaceBackground(const QString &uri);
#endif

    int GetCurrentWorkspace() const;
    void SetCurrentWorkspace(const int index);
    void NextWorkspace();
    void PreviousWorkspace();
#ifndef DISABLE_DEEPIN_WM
    void SwitchToWorkspace(bool backward);
#endif

    QString GetAllAccels() const;
    QStringList GetAccel(const QString &id) const;
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

Q_SIGNALS:
    void WorkspaceBackgroundChanged(int index, const QString &newUri);
#ifndef DISABLE_DEEPIN_WM
    // 兼容deepin-wm提供的接口
    void WorkspaceSwitched(int from, int to);
#endif

private:
    QAction *accelAction(const QString accelKid) const;
    QString transFromDaemonAccelStr(const QString &accelStr) const;
    QString transToDaemonAccelStr(const QString &accelStr) const;

    QString getWorkspaceBackground(const int index) const;
    void setWorkspaceBackground(const int index, const QString &uri);
    void quitTransientBackground();

#ifndef DISABLE_DEEPIN_WM
    void onGsettingsDDEAppearanceChanged(const QString &key);
    void onGsettingsDDEZoneChanged(const QString &key);
#endif

private:
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
};

#endif // DEEPINWMFAKER_H
