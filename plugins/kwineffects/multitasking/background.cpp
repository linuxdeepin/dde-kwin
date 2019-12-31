#include "background.h"
#include "constants.h"

#include <QtDBus>
#include <QGSettings>

#define DBUS_APPEARANCE_SERVICE "com.deepin.daemon.Appearance"
#define DBUS_APPEARANCE_OBJ "/com/deepin/daemon/Appearance"
#define DBUS_APPEARANCE_INTF "com.deepin.daemon.Appearance"

#define DBUS_DEEPIN_WM_SERVICE "com.deepin.wm"
#define DBUS_DEEPIN_WM_OBJ "/com/deepin/wm"
#define DBUS_DEEPIN_WM_INTF "com.deepin.wm"

const char fallback_background_name[] = "file:///usr/share/backgrounds/default_background.jpg";

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
    m_defaultNewDesktopURI = QLatin1String(fallback_background_name);
    onGsettingsDDEAppearanceChanged(GsettingsBackgroundUri);

    connect(_gs_dde_appearance, &QGSettings::changed, this, &BackgroundManager::onGsettingsDDEAppearanceChanged);

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

QPixmap BackgroundManager::getBackground(int workspace, int monitor, const QSize& size)
{
    QString uri = QLatin1String(fallback_background_name);

    if (workspace <= 0) {
        //fallback to first workspace
        workspace = 1;
    }

    QDBusInterface wm(DBUS_DEEPIN_WM_SERVICE, DBUS_DEEPIN_WM_OBJ, DBUS_DEEPIN_WM_INTF);
    QDBusReply<QString> reply = wm.call( "GetWorkspaceBackground", workspace);
    if (!reply.value().isEmpty()) {
        uri = reply.value();
    }

    if (uri.startsWith("file:///")) {
        uri.remove("file://");
    }

    uri = toRealPath(uri);

    if (m_cachedPixmaps.contains(uri)) {
        auto& p = m_cachedPixmaps[uri];
        if (p.first != size) {
            p.first = size;
            p.second = p.second.scaled(size, Qt::KeepAspectRatioByExpanding);
        }
        return p.second;
    }

    QPixmap pm;
    if (!pm.load(uri)) {
        uri = toRealPath(QString::fromUtf8(fallback_background_name).remove("file://"));
        pm.load(uri);
    }

    pm = pm.scaled(size, Qt::KeepAspectRatioByExpanding);
    m_cachedPixmaps[uri] = qMakePair(size, pm);
    //qCDebug(BLUR_CAT) << "--------- " << __func__ << workspace << uri << pm.isNull();
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
    m_cachedUris = _gs_dde_appearance->get(GsettingsBackgroundUri).toStringList();
    const auto& uris = m_cachedUris;

    QDBusInterface wm(DBUS_DEEPIN_WM_SERVICE, DBUS_DEEPIN_WM_OBJ, DBUS_DEEPIN_WM_INTF);

    auto n = qMin(uris.size(), m_desktopCount);
    for (int i = d; i < n; i++) {
        qCDebug(BLUR_CAT) << "-------- dbus SetWorkspaceBackground" << i << uris[i];
        QDBusReply<QString> reply = wm.call( "SetWorkspaceBackground", i, uris[i]);
    }
}

void BackgroundManager::desktopSwitchedPosition(int to, int from)
{
    m_cachedUris = _gs_dde_appearance->get(GsettingsBackgroundUri).toStringList();
    auto uris = m_cachedUris;

    QDBusInterface wm(DBUS_DEEPIN_WM_SERVICE, DBUS_DEEPIN_WM_OBJ, DBUS_DEEPIN_WM_INTF);

    int dir = from < to ? 1 : -1;
    auto n = qMin(uris.size(), m_desktopCount);
    for (int i = 0; i < n; i++) {
        int d = i+1; // desktop id
        if ((dir > 0 && (d > to || d < from)) ||
                (dir < 0 && (d < to || d > from)))
            continue;

        int newd = d == from ? to: d-dir;
        qCDebug(BLUR_CAT) << "-------- dbus SetWorkspaceBackground" << d << newd << uris[d-1];
        QDBusReply<QString> reply = wm.call( "SetWorkspaceBackground", newd, uris[d-1]);

        emit desktopWallpaperChanged(newd);
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

