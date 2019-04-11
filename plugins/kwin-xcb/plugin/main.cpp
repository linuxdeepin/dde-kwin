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
    }

    void onExec() {
        connect(qApp, SIGNAL(workspaceCreated()), this, SLOT(init()));
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
}

class DPlatformIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "dde-kwin-xcb.json")

public:
    QPlatformIntegration *create(const QString&, const QStringList&, int &, char **) Q_DECL_OVERRIDE;
};

QPlatformIntegration* DPlatformIntegrationPlugin::create(const QString& system, const QStringList& parameters, int &argc, char **argv)
{
    if (system == "dde-kwin-xcb") {
        QPlatformIntegration *integration = QPlatformIntegrationFactory::create("xcb", parameters, argc, argv, PLATFORMS_PLUGIN_PATH);
        VtableHook::overrideVfptrFun(integration, &QPlatformIntegration::initialize, overrideInitialize);
        QMetaObject::invokeMethod(_m.operator ->(), "onExec", Qt::QueuedConnection);

        return integration;
    }

    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
