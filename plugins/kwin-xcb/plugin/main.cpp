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
#include "kwinutils_adaptor.h"

#include <qpa/qplatformintegrationplugin.h>
#include <qpa/qplatformintegrationfactory_p.h>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>

#include <QDebug>
#include <QProcess>
#include <QPluginLoader>
#include <QDir>
#include <QDBusConnection>

QT_BEGIN_NAMESPACE

#define KWinUtilsDbusService "org.kde.KWin"
#define KWinUtilsDbusPath "/dde"
#define KWinUtilsDbusInterface "org.kde.KWin"

// let startdde know that we've already started.
void RegisterDDESession()
{
    const QString &cookie = qgetenv("DDE_SESSION_PROCESS_COOKIE_ID");
    qunsetenv(cookie.toLocal8Bit().constData());

    if (!cookie.isEmpty()) {
        QDBusInterface("com.deepin.SessionManager", "/com/deepin/SessionManager").call("Register", cookie);
    }
}

class Mischievous;
class  Mischievous : public QObject
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
        static KWinUtils *utils = new KWinUtils(const_cast<Mischievous*>(this));

        return utils;
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
            qWarning() << Q_FUNC_INFO << loader.errorString();
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
        // 通知startdde kwin启动完成
        RegisterDDESession();

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

        // 注册 dbus 对象 提供更多的 kwin 相关接口
        new KWinAdaptor(kwinUtils());
        QDBusConnection::sessionBus().registerObject(KWinUtilsDbusPath, KWinUtilsDbusInterface, kwinUtils());

        if (QObject *cursor = kwinUtils()->cursor()) {
            connect(cursor, SIGNAL(themeChanged()), this, SLOT(onCursorThemeChanged()), Qt::QueuedConnection);
        }

        // 初始化翻译资源
        QTranslator *ts = new QTranslator(this);
        QString ts_file;
        auto ts_dir_list = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);

        for (QString dir : ts_dir_list) {
            dir += TARGET_NAME "/translations";

            if (!QDir(dir).exists()) {
                continue;
            }

            if (ts->load(ts_file, dir) && qApp->installTranslator(ts)) {
                break;
            } else {
                qWarning() << Q_FUNC_INFO << "Failed on load translators, path:" << dir;
            }
        }
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

public:
    static Mischievous *self;
    QMap<QString, QObject*> moduleMap;
};

Mischievous *Mischievous::self = nullptr;
Q_GLOBAL_STATIC(Mischievous, _m)

static void overrideInitialize(QPlatformIntegration *i)
{
    // kwin中只允许使用名称为"xcb"插件
    *QGuiApplicationPrivate::platform_name = "xcb";
    VtableHook::callOriginalFun(i, &QPlatformIntegration::initialize);

    // 初始化设置光标大小
    _m->updateCursorSize();
}

class DKWinPlatformIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "dde-kwin-xcb.json")

public:
    QPlatformIntegration *create(const QString&, const QStringList&, int &, char **) Q_DECL_OVERRIDE;
};

QPlatformIntegration* DKWinPlatformIntegrationPlugin::create(const QString& system, const QStringList& parameters, int &argc, char **argv)
{
    if (system == TARGET_NAME) {
        for (const QString &arg : parameters) {
            const char pre_arg[] = "appFilePath=";

            if (!arg.startsWith(pre_arg)) {
                continue;
            }

            // 覆盖QCoreApplication::applicationFilePath
            // kwin 在意外退出时会重新启动自己，应该在 kwin restart 时调用 kwin_no_scale
            QCoreApplicationPrivate::setApplicationFilePath(arg.mid(strlen(pre_arg)));
        }

        QPlatformIntegration *integration;

#ifndef DISABLE_DXCB
        if (QPlatformIntegrationFactory::keys().contains("dxcb")) {
            integration = QPlatformIntegrationFactory::create("dxcb", parameters, argc, argv, PLATFORMS_PLUGIN_PATH);
        } else
#endif
        {
            integration = QPlatformIntegrationFactory::create("xcb", parameters, argc, argv, PLATFORMS_PLUGIN_PATH);
        }

        VtableHook::overrideVfptrFun(integration, &QPlatformIntegration::initialize, overrideInitialize);
        QMetaObject::invokeMethod(_m.operator ->(), "onExec", Qt::QueuedConnection);

        return integration;
    }

    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
