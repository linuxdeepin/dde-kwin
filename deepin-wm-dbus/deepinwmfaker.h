#ifndef DEEPINWMFAKER_H
#define DEEPINWMFAKER_H

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

    QString GetAllAccels() const;
    QStringList GetAccel(const QString &id) const;
    bool SetAccel(const QString &data);
    void RemoveAccel(const QString &id);

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
    void onDeepinWMSettingsChanged(const QString &key);
#endif

private:
    KWindowSystem *m_windowSystem;
    KConfig *m_config;
    KConfigGroup *m_generalGroup;
    KConfigGroup *m_workspaceBackgroundGroup;
    KGlobalAccel *m_globalAccel;

    QMap<QString, QAction *> m_accelIdActionMap;

    QString m_transientBackgroundUri;
#ifndef DISABLE_DEEPIN_WM
    QString m_deepinWMBackgroundUri;
    int m_currentDesktop = -1;
#endif
};

#endif // DEEPINWMFAKER_H
