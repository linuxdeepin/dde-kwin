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
    // TODO: GetAllAccels

}

QStringList DeepinWMFaker::GetAccel(const QString &id) const
{
    if (id.isEmpty()) {
        return QStringList();
    }

    const QList<QKeySequence> &seqList = m_globalAccel->globalShortcut(GlobalAccelComponentName, id);
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
 * [ { "Id":"...", "Accels":["...", "..."] }, {...} ]
 */
bool DeepinWMFaker::SetAccels(const QString &data)
{
    if (data.isEmpty()) {
        return false;
    }

    const QJsonDocument &jsonDoc = QJsonDocument::fromJson(data.toUtf8());
    if (jsonDoc.isEmpty()) {
        return false;
    }

    bool result = true;
    for (const QJsonValue &jsonValue : jsonDoc.array()) {
        const QJsonObject &jsonObj = jsonValue.toObject();
        const QString &accelId = jsonObj.value("Id").toString();
        QAction *action = accelAction(accelId);
        if (!action) {
            continue;
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

        // 只要有一个失败就返回 false
        // using setGlobalShortcat() only can set a new accel,
        // it will not override the exist global accel just change the default accel
        if (!m_globalAccel->setShortcut(action, accelList, KGlobalAccel::NoAutoloading)) {
            qDebug() << "WARNING: set accel failed for" << accelId << "with accels:" << accelList;
            result = false;
        }

        m_accelIdActionMap.insert(accelId, action);
    }

    return result;
}

void DeepinWMFaker::RemoveAccel(const QString &id)
{
    if (id.isEmpty()) {
        return;
    }

    const bool contains = m_accelIdActionMap.contains(id);

    QAction *action = accelAction(id);
    if (!action) {
        return;
    }

    // remove will failed if the action is not handled by KGlobalAccel
    if (!contains) {
        m_globalAccel->setShortcut(
                    action, m_globalAccel->globalShortcut(GlobalAccelComponentName, id));
    }

    m_globalAccel->removeAllShortcuts(action);

    m_accelIdActionMap.remove(id);
    action->deleteLater();
}

QAction *DeepinWMFaker::accelAction(const QString accelId) const
{
    if (accelId.isEmpty()) {
        qDebug() << "ERROR: obtain action from an empty accel id";
        return nullptr;
    }

    QAction *action = m_accelIdActionMap.value(accelId, nullptr);
    if (action) {
        return action;
    }

    // pass empty string to the constructor means to not change the Accel Friendly Name
    action = new QAction("");
    action->setObjectName(accelId);
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
