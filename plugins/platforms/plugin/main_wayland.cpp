/*
 * Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@live.com>
 *
 * Maintainer: zccrs <zhangjide@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "vtablehook.h"
#include "kwinutils.h"
#include "kwinutilsadaptor.h"

#include <dshellsurface.h>

#include <KWayland/Server/display.h>

#include <qpa/qplatformintegrationplugin.h>
#include <qpa/qplatformintegrationfactory_p.h>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>

#include <QDebug>
#include <QProcess>
#include <QPluginLoader>
#include <QDir>
#include <QRect>
#include <QQuickItem>
#include <QQuickWindow>
#include <QDBusConnection>

#include "libkwinpreload.h"

using namespace DWaylandServer;
QT_BEGIN_NAMESPACE

#define KWinUtilsDbusService "org.kde.KWin"
#define KWinUtilsDbusPath "/dde"
#define KWinUtilsDbusInterface "org.kde.KWin"

class Mischievous;
class Mischievous : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *workspace READ workspace)
    Q_PROPERTY(QObject *scripting READ scripting)
    Q_PROPERTY(KWinUtils *kwinUtils READ kwinUtils)
public:
    explicit Mischievous() {
        self = this;
    }

    QObject *workspace() const
    {
        return KWinUtils::workspace();
    }

    QObject *scripting() const
    {
        return KWinUtils::scripting();
    }

    KWinUtils *kwinUtils() const
    {
        return KWinUtils::instance();
    }

    Q_INVOKABLE QObject *require(const QString &module)
    {
        if (QObject *obj = moduleMap.value(module)) {
            return obj;
        }

        QString file = module;
        bool isFile = QFile::exists(file);

        if (!isFile) {
            static QStringList pluginPaths {
                QDir::home().absoluteFilePath(QStringLiteral(".local/lib/" PROJECT_NAME "/plugins")),
                QStringLiteral("/usr/lib/" PROJECT_NAME "/plugins")
            };

            for (const QString &path : pluginPaths) {
                QDir dir(path);

                file.prepend("lib");
                file.append(".so");
                file = dir.absoluteFilePath(file);
                isFile = QFile::exists(file);

                if (isFile) {
                    break;
                }
            }
        }

        if (!isFile) {
            return nullptr;
        }

        QPluginLoader loader(file);

        if (!loader.load()) {
            // qWarning() << Q_FUNC_INFO << loader.errorString();
            return nullptr;
        }

        QObject *obj = loader.instance();
        moduleMap[module] = obj;

        if (obj) {
            obj->metaObject()->invokeMethod(obj, "init", Q_ARG(QObject*, this));
        }

        return obj;
    }

    Q_INVOKABLE int execute(const QString &program, const QStringList &arguments, const QString &workingDirectory = QString())
    {
        QProcess p;

        p.setProgram(program);
        p.setArguments(arguments);
        p.setWorkingDirectory(workingDirectory);

        p.start(QIODevice::ReadOnly);
        p.waitForFinished();

        return p.exitCode();
    }
    Q_INVOKABLE int execute(const QString &command, const QString &workingDirectory = QString())
    {
        QProcess p;

        p.setWorkingDirectory(workingDirectory);
        p.start(command, QIODevice::ReadOnly);
        p.waitForFinished();

        return p.exitCode();
    }
    Q_INVOKABLE bool startDetached(const QString &program, const QStringList &arguments, const QString &workingDirectory = QString())
    {
        return QProcess::startDetached(program, arguments, workingDirectory);
    }
    Q_INVOKABLE bool startDetached(const QString &command)
    {
        return QProcess::startDetached(command);
    }

    Q_INVOKABLE bool setObjectProperty(QObject *obj, const QString &name, const QVariant &value)
    {
        return obj->setProperty(name.toLatin1().constData(), value);
    }

public slots:
    void init() {

        if (!KWinUtils::scripting())
            return;

        const QObjectList scripting_children = KWinUtils::scripting()->children();
        QObject *jsWorkspaceWrapper = KWinUtils::findObjectByClassName(QByteArrayLiteral("KWin::QtScriptWorkspaceWrapper"), scripting_children);
        QObject *qmlWorkspaceWrapper = KWinUtils::findObjectByClassName(QByteArrayLiteral("KWin::DeclarativeScriptWorkspaceWrapper"), scripting_children);

        // 给js脚本引擎添加对象
        if (jsWorkspaceWrapper) {
            jsWorkspaceWrapper->setProperty("__dde__", QVariant::fromValue(this));
        }

        // 给qml脚本引擎添加对象
        if (qmlWorkspaceWrapper) {
            qmlWorkspaceWrapper->setProperty("__dde__", QVariant::fromValue(this));
        }

        KWinUtils::scriptingRegisterObject(QStringLiteral("dde"), this);

        // 注册 dbus 对象 提供更多的 kwin 相关接口
        new KWinAdaptor(kwinUtils());
        QDBusConnection::sessionBus().registerObject(KWinUtilsDbusPath, KWinUtilsDbusInterface, kwinUtils());

        if (QObject *cursor = kwinUtils()->cursor()) {
            connect(cursor, SIGNAL(themeChanged()), this, SLOT(onCursorThemeChanged()), Qt::QueuedConnection);
        }

        // 初始化翻译资源
        QTranslator *ts = new QTranslator(this);
        const QString &lang_name = QLocale::system().name();
        QString ts_file = TRANSLATE_NAME "_" + lang_name;
        QString ts_fallback_file;

        {
            int index = lang_name.indexOf("_");

            if (index > 0) {
                ts_fallback_file = lang_name.left(index);
            }
        }

        auto ts_dir_list = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);

        while (!ts_file.isEmpty()) {
            bool ok = false;

            for (QString dir : ts_dir_list) {
                dir += "/" TRANSLATE_NAME "/translations";

                if (!QDir(dir).exists()) {
                    continue;
                }

                if (ts->load(ts_file, dir) && qApp->installTranslator(ts)) {
                    ok = true;
                    break;
                } else {
                    // qWarning() << Q_FUNC_INFO << "Failed on load translators, file:" << dir + "/" + ts_file;
                }
            }

            ts_file.clear();

            if (!ok && !ts_fallback_file.isEmpty()) {
                ts_file = ts_fallback_file;
                ts_fallback_file.clear();
            }
        }

        // 初始化wayland相关的内容
        if (kwinUtils()->initForWayland()) {
            auto display = qobject_cast<KWayland::Server::Display*>(kwinUtils()->waylandDisplay());
            auto dde_shell_manager = new DShellSurfaceManager(display->operator wl_display *());
            display->setProperty("_d_dwayland_dde_shell_manager", QVariant::fromValue(dde_shell_manager));
            // 管理dde shell surface窗口
            connect(dde_shell_manager, &DShellSurfaceManager::surfaceCreated, this, &Mischievous::onDDEShellSurfaceCreated);
            connect(kwinUtils(), &KWinUtils::shellClientAdded, this, &Mischievous::onShellClientAdded);
        }

        // 通知程序初始化完成
        kwinUtils()->setInitialized();
    }

    void onExec() {
        if (KWinUtils::scripting()) {
            init();
        } else {
            connect(qApp, SIGNAL(workspaceCreated()), this, SLOT(init()));
        }
    }

    void updateCursorSize() {
        bool ok = false;
        int cursorSize = QDBusInterface("com.deepin.wm", "/com/deepin/wm").property("cursorSize").toInt(&ok);

        // 应该跟随dpi缩放设置光标大小
        if (!ok || cursorSize <= 0) {
            if (QScreen *s = QGuiApplication::primaryScreen()) {
                cursorSize = qRound(s->logicalDotsPerInchY() * 16 / 72);
                qputenv("XCURSOR_SIZE", QByteArray::number(cursorSize));
            }
        }
    }

    void onCursorThemeChanged() {
        updateCursorSize();

        // 光标主题改变后应该立即更新所有客户端的当前光标
        for (QObject *client : kwinUtils()->clientList()) {
            QMetaObject::invokeMethod(client, "moveResizeCursorChanged", Q_ARG(Qt::CursorShape, Qt::ArrowCursor));
            const QVariant &wrapper = kwinUtils()->getParentWindow(client);

            // KWin会为client创建一个父窗口wrapper，wrapper初始化时设置了光标为ArrowCursor类型,
            // gtk应用默认不会给主窗口设置任何光标，因此gtk窗口会跟随其父窗口wrapper的光标样式，
            // 当光标主题改变时，应该主动更新wrapper的光标，否则会导致gtk应用的窗口默认光标不跟随主题
            if (wrapper.isValid())
                KWinUtils::defineWindowCursor(wrapper.toUInt(), Qt::ArrowCursor);
        }
    }

    void onShellClientAdded(QObject *client)
    {
        Q_UNUSED(client)
        if (feralShellSurface.size() > 3) {
            // 不应该存在这么多未匹配的对象，除非findShellClient函数失效了
            qWarning() << "Warning: Plase check KWinUtils::findShellClient";
            return;
        }

        auto itor = feralShellSurface.begin();
        while (itor != feralShellSurface.end()) {
            if (onDDEShellSurfaceCreated(*itor)) {
                itor = feralShellSurface.erase(itor);
            } else {
                ++itor;
            }
        }
    }

    static void updateSurfaceInfos(DShellSurface *surface, const QObject *client)
    {
        // 为其更关联窗口的geometry属性，此属性包含窗口边框信息
        const QRect &geometry = client->property("geometry").toRect();

        if (!geometry.isValid())
            return;

        // 使用rect是因为其容易和QVariant转换
        const QPoint client_pos = client->property("clientPos").toPoint();
        const QSize client_size = client->property("clientSize").toSize();
        QRect frameMargins(client_pos, QPoint(0, 0));

        if (client_size.isValid()) {
            frameMargins.setRight(geometry.width() - client_size.width() - client_pos.x());
            frameMargins.setBottom(geometry.height() - client_size.height() - client_pos.y());
        }

        surface->setProperty("frameMargins", frameMargins);
        surface->setGeometry(geometry);
    }

    bool onDDEShellSurfaceCreated(DShellSurface *surface)
    {
        if (!surface) {
            qCritical()<<__FILE__<< __FUNCTION__<<__LINE__<<"surface is null";
            return false;
        }
        connect(surface, &DShellSurface::propertyChanged, [surface,this](const QString &name, const QVariant &value) {
            if (surface && !name.isNull() && !value.isNull()) {
                wl_resource * wr = surface->surfaceResource();
                if (wr) {
                    kwinUtils()->setWindowProperty(wr, name, value);
                } else {
                    qCritical()<<__FILE__<< __FUNCTION__<<__LINE__<<"wl_resource is null";
                }
            }
        });
        wl_resource * wr = surface->surfaceResource();
        if (!wr) {
            qCritical()<<__FILE__<< __FUNCTION__<<__LINE__<<"wl_resource is null";
            return false;
        }
        auto shell_client = kwinUtils()->findShellClient(wr);
        if (!shell_client) {
            if (!feralShellSurface.contains(surface))
                feralShellSurface << surface;
            return false;
        }

        shell_client->setProperty("_d_dwayland_dde_shell_surface", QVariant::fromValue(surface));
        connect(shell_client, SIGNAL(geometryChanged()), this, SLOT(onShellClientGeometryChanged()));
        connect(surface, &DShellSurface::activationRequested, [shell_client, this]() {
            kwinUtils()->activateClient(shell_client);
        });
        updateSurfaceInfos(surface, shell_client);
        return true;
    }

    static inline DShellSurface *getShellSurface(const QObject *shellClient)
    {
        return qvariant_cast<DShellSurface*>(shellClient->property("_d_dwayland_dde_shell_surface"));
    }

    Q_INVOKABLE void onShellClientGeometryChanged()
    {
        QObject *shell_client = sender();
        DShellSurface *surface = getShellSurface(shell_client);
        updateSurfaceInfos(surface, shell_client);
    }

public:
    static Mischievous *self;
    QMap<QString, QObject*> moduleMap;
    // 未能匹配到ShellClient的ShellSurface，需要等ShellClient创建后重新调用onDDEShellSurfaceCreated
    QList<DShellSurface*> feralShellSurface;
};

Mischievous *Mischievous::self = nullptr;
Q_GLOBAL_STATIC(Mischievous, _m)

static void overrideInitialize(QPlatformIntegration *i)
{
    *QGuiApplicationPrivate::platform_name = "wayland";
    VtableHook::callOriginalFun(i, &QPlatformIntegration::initialize);
}

class DKWinWaylandPlatformIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "dde-kwin-wayland.json")

public:
    QPlatformIntegration *create(const QString&, const QStringList&, int &, char **) Q_DECL_OVERRIDE;
};

QPlatformIntegration* DKWinWaylandPlatformIntegrationPlugin::create(const QString& system, const QStringList& parameters, int &argc, char **argv)
{
    if (system == TARGET_NAME) {
        // 清理对libdde-kwin-wayland.so的ld preload，防止使用QProcess调用其他进程时被传递
        qunsetenv("LD_PRELOAD");

        QPlatformIntegration *integration;

        integration = QPlatformIntegrationFactory::create("wayland-org.kde.kwin.qpa", parameters, argc, argv, PLATFORMS_PLUGIN_PATH);

        VtableHook::overrideVfptrFun(integration, &QPlatformIntegration::initialize, overrideInitialize);
        QMetaObject::invokeMethod(_m.operator ->(), "onExec", Qt::QueuedConnection);

        return integration;
    }

    return 0;
}

QT_END_NAMESPACE

#include "main_wayland.moc"
