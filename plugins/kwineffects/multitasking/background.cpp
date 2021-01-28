#include "background.h"
#include "constants.h"
#include "kwineffects.h"

#include <QtDBus>
#include <QGSettings>
#include <QScreen>
#include <QX11Info>

#define DBUS_APPEARANCE_SERVICE "com.deepin.daemon.Appearance"
#define DBUS_APPEARANCE_OBJ "/com/deepin/daemon/Appearance"
#define DBUS_APPEARANCE_INTF "com.deepin.daemon.Appearance"

#define DBUS_DEEPIN_WM_SERVICE "com.deepin.wm"
#define DBUS_DEEPIN_WM_OBJ "/com/deepin/wm"
#define DBUS_DEEPIN_WM_INTF "com.deepin.wm"

const char fallback_background_name[] = "file:///usr/share/wallpapers/deepin/desktop.jpg";
const char previous_default_background_name[] = "file:///usr/share/backgrounds/default_background.jpg";

Q_GLOBAL_STATIC_WITH_ARGS(QGSettings, _gs_dde_appearance, ("com.deepin.dde.appearance"))

#define GsettingsBackgroundUri "backgroundUris"

BackgroundManager& BackgroundManager::instance()
{
    static BackgroundManager* _self = nullptr;
    if (!_self) {
        _self = new BackgroundManager();
    }

    return *_self;
}

BackgroundManager::BackgroundManager()
    :QObject()
{
    m_wm_interface.reset(new ComDeepinWmInterface("com.deepin.wm",
                                                  "/com/deepin/wm",
                                                  QDBusConnection::sessionBus(), this));
    m_defaultNewDesktopURI = QLatin1String(fallback_background_name);
    onGsettingsDDEAppearanceChanged(GsettingsBackgroundUri);

    connect(_gs_dde_appearance, &QGSettings::changed, this, &BackgroundManager::onGsettingsDDEAppearanceChanged);
    //Refreshes the thumbnails in the multitasking view
    //Temporary circumvention program exits unexpectedly
    //QDBusConnection::sessionBus().connect(DBUS_DEEPIN_WM_SERVICE, DBUS_DEEPIN_WM_OBJ, DBUS_DEEPIN_WM_INTF, "WorkspaceBackgroundChanged", this, SIGNAL(desktopWallpaperChanged(int)));

    emit defaultBackgroundURIChanged();
}

static QString toRealPath(const QString& path)
{
    QString res = path;

    QFileInfo fi(res);
    if (fi.isSymLink()) {
        res = fi.symLinkTarget();
    }

    return res;
}

QPixmap BackgroundManager::getBackground(int workspace, QString screenName, const QSize& size)
{
    QString uri = QLatin1String(fallback_background_name);
    QString strBackgroundPath = QString("%1%2").arg(workspace).arg(screenName);

    if (workspace <= 0) {
        //fallback to first workspace
        workspace = 1;
    }

    QDBusPendingReply<QString> reply = m_wm_interface->GetWorkspaceBackgroundForMonitor(workspace, screenName);
    if (reply.isError()) {
        uri = "";
    }
    if (!reply.value().isEmpty()) {
        uri = reply.value();
    }

    if (uri.startsWith("file:///")) {
        uri.remove("file://");
    }

    uri = toRealPath(uri);

    if (m_cachedPixmaps.contains(uri  + strBackgroundPath)) {
        auto& p = m_cachedPixmaps[uri + strBackgroundPath];
        if (p.first != size) {
            p.first = size;
            p.second = p.second.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        }
        return p.second;
    }

    QPixmap pm;
    if (!pm.load(uri)) {
        uri = toRealPath(QString::fromUtf8(fallback_background_name).remove("file://"));
        pm.load(uri);
    }

    pm = pm.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    m_cachedPixmaps[uri + strBackgroundPath] = qMakePair(size, pm);
    return pm;
}

void BackgroundManager::onGsettingsDDEAppearanceChanged(const QString &key)
{
    //FIXME: no signal received sometimes during append desktop, why?
    qCDebug(BLUR_CAT) << "---------- " << __func__ << key;
    if (key == GsettingsBackgroundUri) {
        m_cachedUris = _gs_dde_appearance->get(GsettingsBackgroundUri).toStringList();
        emit wallpapersChanged();
    }
}

void BackgroundManager::desktopAboutToRemoved(int d)
{
    QDBusInterface wm(DBUS_DEEPIN_WM_SERVICE, DBUS_DEEPIN_WM_OBJ, DBUS_DEEPIN_WM_INTF);

    for(int i = 0; i < m_screenNamelst.count(); i++) {
        QString monitorName = m_screenNamelst.at(i);

        // X11 platform
        if (QX11Info::isPlatformX11()) {
            for(int i = d; i < m_desktopCount; i++) {
                QString backgrounduri;
                QDBusReply<QString> getReply = wm.call( "GetWorkspaceBackgroundForMonitor", i + 1, monitorName);
                if(!getReply.value().isEmpty()) {
                    backgrounduri = getReply.value();
                } else {
                    backgrounduri = QLatin1String(fallback_background_name);
                }
                wm.call( "SetWorkspaceBackgroundForMonitor", i, monitorName, backgrounduri);
            }
        } else {
            for(int i = d; i <  m_desktopCount; i++) {
                QDBusPendingReply<QString> getReply = m_wm_interface->GetWorkspaceBackgroundForMonitor(i + 1, monitorName);
                QDBusPendingReply<QString> setReply = m_wm_interface->SetWorkspaceBackgroundForMonitor(i, monitorName, getReply.value());
            }
        }
    }
}

void BackgroundManager::desktopSwitchedPosition(int to, int from)
{
    QDBusInterface wm(DBUS_DEEPIN_WM_SERVICE, DBUS_DEEPIN_WM_OBJ, DBUS_DEEPIN_WM_INTF);

    for(int i = 0; i < m_screenNamelst.count(); i++) {
        QString monitorName = m_screenNamelst.at(i);
        QString strFromUri;
        // X11 platform
        if (QX11Info::isPlatformX11()) {
            QDBusReply<QString> getReply = wm.call( "GetWorkspaceBackgroundForMonitor", from, monitorName);
            if(!getReply.value().isEmpty()) {
                strFromUri = getReply.value();
            } else {
                strFromUri = QLatin1String(fallback_background_name);
            }
        } else {
            QDBusPendingReply<QString> getReply = m_wm_interface->GetWorkspaceBackgroundForMonitor(from, monitorName);
            strFromUri = getReply.value();
        }

        if (from < to) {
            for(int j = from - 1; j < to; j++) {
                int desktopIndex = j + 1; //desktop index
                if ( desktopIndex == to) {
                    // X11 platform
                    if (QX11Info::isPlatformX11()) {
                        wm.call( "SetWorkspaceBackgroundForMonitor", desktopIndex, monitorName, strFromUri);
                    } else {
                        QDBusPendingReply<QString> setReply = m_wm_interface->SetWorkspaceBackgroundForMonitor(desktopIndex, monitorName, strFromUri);
                    }
                } else {
                    // X11 platform
                    if (QX11Info::isPlatformX11()) {
                        QDBusReply<QString> getReply = wm.call( "GetWorkspaceBackgroundForMonitor", desktopIndex + 1, monitorName);
                        QString backgroundUri;
                        if(!getReply.value().isEmpty()) {
                            backgroundUri = getReply.value();
                        } else {
                            backgroundUri = QLatin1String(fallback_background_name);
                        }
                        wm.call( "SetWorkspaceBackgroundForMonitor", desktopIndex, monitorName, backgroundUri);
                    } else {
                        QDBusPendingReply<QString> getReply = m_wm_interface->GetWorkspaceBackgroundForMonitor(desktopIndex + 1, monitorName);
                        QDBusPendingReply<QString> setReply = m_wm_interface->SetWorkspaceBackgroundForMonitor(desktopIndex, monitorName, getReply.value());
                    }
                }
            }
        } else {
            for (int j = from; j > to - 1; j--) {
                if (j == to) {
                    // X11 platform
                    if (QX11Info::isPlatformX11()) {
                        QDBusReply<QString> setReply = wm.call( "SetWorkspaceBackgroundForMonitor", to, monitorName, strFromUri);
                    } else {
                        QDBusPendingReply<QString> setReply = m_wm_interface->SetWorkspaceBackgroundForMonitor(to, monitorName, strFromUri);
                    }
                } else {
                    // X11 platform
                    if (QX11Info::isPlatformX11()) {
                        QDBusReply<QString> getReply = wm.call( "GetWorkspaceBackgroundForMonitor", j - 1, monitorName);
                        QString backgroundUri;
                        if(!getReply.value().isEmpty()) {
                            backgroundUri = getReply.value();
                        } else {
                            backgroundUri = QLatin1String(fallback_background_name);
                        }
                        QDBusReply<QString> setReply = wm.call( "SetWorkspaceBackgroundForMonitor", j, monitorName, backgroundUri);
                    } else {
                        QDBusPendingReply<QString> getReply = m_wm_interface->GetWorkspaceBackgroundForMonitor(j - 1, monitorName);
                        QDBusPendingReply<QString> setReply = m_wm_interface->SetWorkspaceBackgroundForMonitor(j, monitorName, getReply.value());
                    }
                }
            }
        }
    }
}

QString BackgroundManager::getDefaultBackgroundURI()
{
    return m_defaultNewDesktopURI;
}

void BackgroundManager::shuffleDefaultBackgroundURI()
{
    if (m_preinstalledWallpapers.size() == 0) {
        QDBusInterface remoteApp(DBUS_APPEARANCE_SERVICE, DBUS_APPEARANCE_OBJ, DBUS_APPEARANCE_INTF);
        QDBusReply<QString> reply = remoteApp.call( "List", "background");

        QJsonDocument json = QJsonDocument::fromJson(reply.value().toUtf8());
        QJsonArray arr = json.array();
        if (!arr.isEmpty()) {
            auto p = arr.constBegin();
            while (p != arr.constEnd()) {
                auto o = p->toObject();
                if (!o.value("Id").isUndefined() && !o.value("Deletable").toBool()) {
                    m_preinstalledWallpapers << o.value("Id").toString();
                }
                ++p;
            }
        }
    }

    if (m_preinstalledWallpapers.size() > 0) {
        int id = QRandomGenerator::global()->bounded(m_preinstalledWallpapers.size());
        m_defaultNewDesktopURI = m_preinstalledWallpapers[id];
        emit defaultBackgroundURIChanged();
    }
}

void BackgroundManager::changeWorkSpaceBackground(int workspaceIndex)
{
    QDBusInterface remoteApp(DBUS_APPEARANCE_SERVICE, DBUS_APPEARANCE_OBJ, DBUS_APPEARANCE_INTF);
    QDBusReply<QString> reply = remoteApp.call( "List", "background");

    QStringList backgroundAllLst;
    QString lastBackgroundUri;

    QJsonDocument json = QJsonDocument::fromJson(reply.value().toUtf8());
    QJsonArray arr = json.array();
    if (!arr.isEmpty()) {
        auto p = arr.constBegin();
        while (p != arr.constEnd()) {
            auto o = p->toObject();
            if (!o.value("Id").isUndefined() && !o.value("Deletable").toBool()) {
                backgroundAllLst << o.value("Id").toString();
            }
            ++p;
        }
    }

    QDBusInterface wm(DBUS_DEEPIN_WM_SERVICE, DBUS_DEEPIN_WM_OBJ, DBUS_DEEPIN_WM_INTF);

    for (int i = 0; i < m_screenNamelst.count(); i++) {

        QString monitorName = m_screenNamelst.at(i);
        QList<QString> backgroundUriList;

        for (int i = 0; i < m_desktopCount; i++) {
            int desktopIndex = i + 1;
            QString backgroundUri;
            // X11 platform
            if (QX11Info::isPlatformX11()) {
                QDBusReply<QString> getReply = wm.call( "GetWorkspaceBackgroundForMonitor", desktopIndex, monitorName);
                if(!getReply.value().isEmpty()) {
                    backgroundUri = getReply.value();
                } else {
                    backgroundUri = QLatin1String(fallback_background_name);
                }
                backgroundUriList.append(backgroundUri);
                lastBackgroundUri = backgroundUri;
            } else {
                QDBusPendingReply<QString> getReply = m_wm_interface->GetWorkspaceBackgroundForMonitor(desktopIndex, monitorName);
                backgroundUri.append(getReply.value());
                lastBackgroundUri = getReply.value();
            }
        }

        // When user only upgrade dde-kwin, the previous defaultbackground maybe still stay in ~/.config/deepinwmrc.
        // So we remove default background from all background.
        QString oldVerisonDefaultBackground(previous_default_background_name);
        QString defaultBackground(fallback_background_name);
        if (backgroundUriList.contains(oldVerisonDefaultBackground)) {
            backgroundAllLst.removeAll(defaultBackground);
        }

        backgroundUriList = backgroundUriList.toSet().toList();

        for (int i = 0; i < backgroundUriList.count(); i++) {
            QString background = backgroundUriList.at(i);
            backgroundAllLst.removeOne(background);
        }

        if (backgroundAllLst.count() <= 0) {
            backgroundAllLst.append(lastBackgroundUri);
        }

        int backgroundIndex = backgroundAllLst.count();

        if (backgroundIndex - 1 != 0){
            backgroundIndex = qrand()%(backgroundAllLst.count() - 1);
        } else {
            backgroundIndex -= 1;
        }
        // X11 platform
        if (QX11Info::isPlatformX11()) {
            wm.call( "SetWorkspaceBackgroundForMonitor", workspaceIndex, monitorName, backgroundAllLst.at(backgroundIndex));
        } else {
            QDBusPendingReply<QString> setReply = m_wm_interface->SetWorkspaceBackgroundForMonitor(workspaceIndex, monitorName, backgroundAllLst.at(backgroundIndex));
        }
    }
}

QPixmap BackgroundManager::getBackgroundPixmap(int workSpace, QString screenName)
{
    QString strBackgroundPath = QString("%1%2").arg(workSpace).arg(screenName);

    QDBusInterface wm(DBUS_DEEPIN_WM_SERVICE, DBUS_DEEPIN_WM_OBJ, DBUS_DEEPIN_WM_INTF);
    QDBusReply<QString> getReply = wm.call( "GetWorkspaceBackgroundForMonitor", workSpace, screenName);

    QString backgroundUri;
    if(!getReply.value().isEmpty()) {
        backgroundUri = getReply.value();
    } else {
        backgroundUri = QLatin1String(fallback_background_name);
    }

    if (backgroundUri.startsWith("file:///")) {
        backgroundUri.remove("file://");
    }

    backgroundUri = toRealPath(backgroundUri);

    QSize size;
    for (int i = 0; i < m_monitorInfoLst.count(); i++)
    {
        QMap<QString,QVariant> monitorInfo = m_monitorInfoLst.at(i);
        if (monitorInfo.contains(screenName)) {
            size = monitorInfo[screenName].toSize();
            break;
        }
    }

    if (m_bigCachedPixmaps.contains(backgroundUri  + strBackgroundPath)) {
        auto& p = m_bigCachedPixmaps[backgroundUri + strBackgroundPath];
        if (p.first.width() == size.width() && p.first.height() == size.height())
            return p.second;
        // for clipping, we should guarantee CachedPixmaps is bigger than current monitor
        if (p.first.width() > size.width() && p.first.height() > size.height()) {
            p.first = size;
            p.second = p.second.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            p.second = p.second.copy(QRect(static_cast<int>((p.second.width() - size.width()) / 2.0),
                                           static_cast<int>((p.second.height() - size.height()) / 2.0), size.width(), size.height()));
            return p.second;
        }
    }

    QPixmap pixmap;
    pixmap.load(backgroundUri);

    pixmap = pixmap.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    if(pixmap.width() > size.width() || pixmap.height() > size.height()) {
        pixmap = pixmap.copy(QRect(static_cast<int>((pixmap.width() - size.width()) / 2.0), static_cast<int>((pixmap.height() - size.height()) / 2.0), size.width(), size.height()));
    }
    m_bigCachedPixmaps[backgroundUri + strBackgroundPath] = qMakePair(size, pixmap);
    return pixmap;
}

void BackgroundManager::setMonitorInfo(QList<QMap<QString,QVariant>> monitorInfoLst)
{
    m_monitorInfoLst = monitorInfoLst;

    QList<QString> monitorNameLst;
    for (int i = 0; i < m_monitorInfoLst.count(); i++) {
        QMap<QString,QVariant> monitorInfo = m_monitorInfoLst.at(i);
        monitorNameLst.append(monitorInfo.keys());
    }
    m_screenNamelst = monitorNameLst.toSet().toList();
}
