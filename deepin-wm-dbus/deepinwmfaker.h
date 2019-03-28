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

private:
    QAction *accelAction(const QString accelId) const;
    QString transFromDaemonAccelStr(const QString &accelStr) const;
    QString transToDaemonAccelStr(const QString &accelStr) const;

private:
    KWindowSystem *m_windowSystem;
    KConfig *m_config;
    KConfigGroup *m_generalGroup;
    KConfigGroup *m_workspaceBackgroundGroup;
    KGlobalAccel *m_globalAccel;

    QMap<QString, QAction *> m_accelIdActionMap;
};

#endif // DEEPINWMFAKER_H
