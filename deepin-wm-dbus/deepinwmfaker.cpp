#include "deepinwmfaker.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <KF5/KConfigCore/KConfig>
#include <KF5/KConfigCore/KConfigGroup>
#include <KF5/KWindowSystem/KWindowSystem>
#include <KF5/KGlobalAccel/KGlobalAccel>

#define ConfigName "deepinwmrc"
#define GeneralGroupName "General"
#define WorkspaceBackgroundGroupName "WorkspaceBackground"

#define GlobalAccelComponentName "kwin"
#define GlobalAccelComponentDisplayName "KWin"

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
    , m_config(new KConfig(ConfigName, KConfig::SimpleConfig))
    , m_generalGroup(new KConfigGroup(m_config, GeneralGroupName))
    , m_workspaceBackgroundGroup(new KConfigGroup(m_config, WorkspaceBackgroundGroupName))
    , m_globalAccel(KGlobalAccel::self())
{
}

QString DeepinWMFaker::GetWorkspaceBackground(const int index) const
{
    return m_workspaceBackgroundGroup->readEntry(QString::number(index), QString());
}

void DeepinWMFaker::SetWorkspaceBackground(const int index, const QString &uri)
{
    m_workspaceBackgroundGroup->writeEntry(QString::number(index), uri);

    Q_EMIT WorkspaceBackgroundChanged(index, uri);
}

QString DeepinWMFaker::GetCurrentWorkspaceBackground() const
{
    return GetWorkspaceBackground(m_windowSystem->currentDesktop());
}

void DeepinWMFaker::SetCurrentWorkspaceBackground(const QString &uri)
{
    SetWorkspaceBackground(m_windowSystem->currentDesktop(), uri);
}

int DeepinWMFaker::GetCurrentWorkspace() const
{
    return m_windowSystem->currentDesktop();
}

void DeepinWMFaker::SetCurrentWorkspace(const int index)
{
    m_windowSystem->setCurrentDesktop(index);
}

void DeepinWMFaker::NextWorkspace()
{
    // loopback support
//    int current = m_windowSystem->currentDesktop();
//    ++current < m_windowSystem->numberOfDesktops() ? current : loopback ? 0 : --current;
//    m_windowSystem->setCurrentDesktop(current);

    m_windowSystem->setCurrentDesktop(m_windowSystem->currentDesktop() + 1);
}

void DeepinWMFaker::PreviousWorkspace()
{
    // loopback support
//    int current = m_windowSystem->currentDesktop();
//    --current >= 0 ? current : loopback ? --(m_windowSystem->numberOfDesktops()) : 0;
//    m_windowSystem->setCurrentDesktop(current);

    m_windowSystem->setCurrentDesktop(m_windowSystem->currentDesktop() - 1);
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
