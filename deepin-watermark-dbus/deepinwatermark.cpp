/*
 * Copyright (C) 2022 ~ 2022 Deepin Technology Co., Ltd.
 *
 * Author:     xupeidong <xupeidong@uniontech.com>
 *
 * Maintainer: xupeidong <xupeidong@uniontech.com>
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
#include "deepinwatermark.h"
#include "utils.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QPainter>
#include <QDebug>
#include <QPixmap>
#include <QBitmap>
#include <QPaintEvent>
#include <QScreen>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusInterface>
#include <QDBusReply>
#include <QProcess>
#include <QTimer>
#include <QJsonParseError>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDBusConnectionInterface>
#include <QDateTime>
#include <QPainterPath>
#include <QX11Info>
Q_LOGGING_CATEGORY(DEEPINWATERMARK, "deepinwatermark")
// 水印配置文件
const QString WATERMARK_CONFIG = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kwatermark";
const int TIME_INTERVAL = 1000;
const int DEFAULT_ERROR = -4;
const float DENSITY = 0.15;
const int LEAN_ANGLE = -30;
const int SCREENSAVER_INTERVAL = 50;

DeepinWatermark::DeepinWatermark(QWidget *parent) :
    QWidget(parent)
{
    // 判断当前平台是x11还是wayland
    m_isX11Server = QX11Info::isPlatformX11();
    setGeometry(0, 0, qApp->desktop()->width(), qApp->desktop()->height());//设置水印覆盖整个整个屏幕
    QDBusInterface watermarkInterface("org.kde.KWin", "/Compositor", "org.kde.kwin.Compositing");
    m_compositorActive = watermarkInterface.property("active").toBool();
    /*
     * Qt::FramelessWindowHint : 产生一个无窗口边框的窗口，此时用户无法移动该窗口和改变它的大小。
     * Qt::X11BypassWindowManagerHint : 完全忽视窗口管理器，它的作用是产生一个根本不被管理器的无窗口边框的窗口，此时，用户无法使用键盘进行输入，除非手动调用QWidget::ActivateWindow()函数。
     * Qt::WindowStaysOnTopHint : 置于所有窗口最上层
     */
    setAttribute(Qt::WA_ShowWithoutActivating);
    setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowState(Qt::WindowNoState | Qt::WindowFullScreen);
    setFocusPolicy(Qt::NoFocus);
    // x11设置鼠标穿透
    if (m_isX11Server) {
        Utils::passInputEvent(winId());
    }
    m_painter = new QPainter(this);
    // 监听kwin切换2D和3D的信号
    QDBusConnection::sessionBus().connect("org.kde.KWin", "/Compositor", "org.kde.kwin.Compositing", "compositingToggled", this, SLOT(compositingToggled(bool)));

    // 监听kwin compositor setup信号
    QDBusConnection::sessionBus().connect("org.kde.KWin", "/Compositor", "org.kde.kwin.Compositing", "compositingSetup", this, SLOT(compositingSetup()));

    m_currentTime = new QTimer(this);
    m_currentTime->setInterval(TIME_INTERVAL);
    connect(m_currentTime, &QTimer::timeout, this, [this](){
       QString strCurrentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd");
       if (m_currentTimeString != strCurrentTime) {
           m_currentTimeString = strCurrentTime;
           update();
       }
    });

    if (m_isX11Server) {
        m_getDesktopStatusTimer = new QTimer(this);
        m_getDesktopStatusTimer->setInterval(SCREENSAVER_INTERVAL);
        connect(m_getDesktopStatusTimer, &QTimer::timeout, this, [this](){
            // 监听屏保状态
            QDBusInterface screensaverInterface("com.deepin.ScreenSaver", "/com/deepin/ScreenSaver", "com.deepin.ScreenSaver");
            bool isRunning = screensaverInterface.property("isRunning").toBool();
            if (isRunning != m_isOpenScreenSaver) {
                m_isOpenScreenSaver = isRunning;
                clearMask();
                update();
            }
        });
        m_getDesktopStatusTimer->start();
    }

    initConfig();
}

DeepinWatermark::~DeepinWatermark()
{
}

void DeepinWatermark::writeConfig()
{
    QSettings settings(WATERMARK_CONFIG, QSettings::IniFormat);
    settings.setIniCodec("UTF8");
    settings.beginGroup("basic");
    settings.setValue("status", m_isOpen);
    settings.setValue("custom_content", m_content);
    settings.setValue("current_time_display", m_showTime);
    settings.setValue("font_size", m_fontSize);
    settings.setValue("transparency", m_transparency);
    settings.setValue("density_grade", m_density);
    settings.setValue("format", m_fontFormate);
    settings.endGroup();

    settings.beginGroup("username_display");
    settings.setValue("status", m_showUsrName);
    settings.setValue("content", m_usrName);
    settings.endGroup();

    settings.beginGroup("hostname_display");
    settings.setValue("status", m_showHostName);
    settings.setValue("content", m_hostName);
    settings.endGroup();

    settings.beginGroup("terminal_alias_display");
    settings.setValue("status", m_showTerminalAliasName);
    settings.setValue("content", m_terminalAliasName);
    settings.endGroup();

    settings.beginGroup("ip_address_display");
    settings.setValue("status", m_showIpAddress);
    settings.setValue("content", m_ipAddress);
    settings.endGroup();

    settings.beginGroup("mac_address_display");
    settings.setValue("status", m_showMacAddress);
    settings.setValue("content", m_macAddress);
    settings.endGroup();

    settings.beginGroup("whitelist");
    if (!settings.contains("names")) {
        settings.setValue("names", QVariant("kwin_x11 kwin_wayland dcmc-session dcmc-guard"));
    }
    settings.endGroup();
}

void DeepinWatermark::readConfig()
{
    QSettings settings(WATERMARK_CONFIG, QSettings::IniFormat);
    settings.setIniCodec("UTF8");
    m_isOpen = settings.value("basic/status").toBool();
    m_content = settings.value("basic/custom_content").toString();
    m_showTime = settings.value("basic/current_time_display").toBool();
    m_fontSize = settings.value("basic/font_size").toInt();
    m_transparency = settings.value("basic/transparency").toInt();
    m_density = settings.value("basic/density_grade").toInt();
    m_fontFormate = settings.value("basic/format").toInt();

    m_showUsrName = settings.value("username_display/status").toBool();
    m_usrName = settings.value("username_display/content").toString();

    m_showHostName = settings.value("hostname_display/status").toBool();
    m_hostName = settings.value("hostname_display/content").toString();

    m_showTerminalAliasName = settings.value("terminal_alias_display/status").toBool();
    m_terminalAliasName = settings.value("terminal_alias_display/content").toString();

    m_showIpAddress = settings.value("ip_address_display/status").toBool();
    m_ipAddress = settings.value("ip_address_display/content").toString();

    m_showMacAddress = settings.value("mac_address_display/status").toBool();
    m_macAddress = settings.value("mac_address_display/content").toString();

    QStringList whitelist = settings.value("whitelist/names").toString().split(" ");
    for (auto str : whitelist) {
        m_whiteProcess.insert(str);
    }
}

bool DeepinWatermark::isValidInvoker(const uint &pid)
{
    QFileInfo fileInfo(QString("/proc/%1/exe").arg(pid));
    if (!fileInfo.exists()) {
        return false;
    }
    QString invokerPath = fileInfo.canonicalFilePath().split("/").last();
    return m_whiteProcess.contains(invokerPath);
}

void DeepinWatermark::refreshWindow()
{
    if (!m_isX11Server) {
        clearMask();
    }
    if (m_isOpen) {
        if (m_showTime) {
            m_currentTimeString = QDateTime::currentDateTime().toString("yyyy-MM-dd");
            m_currentTime->start();
        } else {
            m_currentTime->stop();
        }
        show();
    } else {
        hide();
    }
    update();
}

QString DeepinWatermark::getCustomContent() const
{
    // 水印顺序:自定义内容+用户名+主机名+终端别名+当前时间+IP地址+MAC地址
    QString strContent;
    if (!m_content.isEmpty()) {
        strContent.append(m_content + " ");
    }
    if (m_showUsrName) {
        strContent.append(m_usrName + " ");
    }
    if (m_showHostName) {
        strContent.append(m_hostName + " ");
    }
    if (m_showTerminalAliasName) {
        strContent.append(m_terminalAliasName + " ");
    }
    if (m_showTime) {
        strContent.append(m_currentTimeString + " ");
    }
    if (m_showIpAddress) {
        strContent.append(m_ipAddress + " ");
    }
    if (m_showMacAddress) {
        strContent.append(m_macAddress + " ");
    }

    return strContent.trimmed();
}

QString DeepinWatermark::jsonAutoTest() const
{
    QJsonObject jsonObj;
    jsonObj.insert("status", true);
    jsonObj.insert("custom_content", "xxx");
    jsonObj.insert("current_time_display", true);
    jsonObj.insert("font_size", 11);
    jsonObj.insert("transparency", 50);
    jsonObj.insert("density_grade", 15);
    jsonObj.insert("format", 1);
    QJsonObject objUsrname;
    objUsrname.insert("status", true);
    objUsrname.insert("content", "xxx");
    jsonObj.insert("username_display", objUsrname);
    QJsonObject objHostname;
    objHostname.insert("status", true);
    objHostname.insert("content", "xxx");
    jsonObj.insert("hostname_display", objHostname);
    QJsonObject objTerminal;
    objTerminal.insert("status", true);
    objTerminal.insert("content", "xxx");
    jsonObj.insert("terminal_alias_display", objTerminal);
    QJsonObject objIpaddress;
    objIpaddress.insert("status", true);
    objIpaddress.insert("content", "xxx");
    jsonObj.insert("ip_address_display", objIpaddress);
    QJsonObject objMacaddress;
    objMacaddress.insert("status", true);
    objMacaddress.insert("content", "xxx");
    jsonObj.insert("mac_address_display", objMacaddress);
    QJsonDocument doc;
    doc.setObject(jsonObj);
    return QString(doc.toJson(QJsonDocument::Compact));
}

int DeepinWatermark::calculateCoordinate(const int &index, const int &textWidth, const int &hSpace)
{
    int startX = 0;
    int tmpWidth = index * hSpace;
    if (tmpWidth == hSpace) {
        startX = hSpace;
    }else {
        // 计算本行首个水印需要展示的位置
        QStringList listSpace = QString::number(float(tmpWidth) / textWidth, 'f', 1).split(".");
        if (listSpace.size() > 1) {
            if (listSpace.at(1).toInt() > 0) {
                startX = -(textWidth - (listSpace.at(1).toFloat() / 10) * textWidth);
            }
        }
    }
    return startX;
}

int DeepinWatermark::setProhibitScreenShot(bool prohibit)
{
    QDBusInterface screenShotInterface("com.deepin.prohibitscreenshot", "/com/deepin/prohibitscreenshot",
                                       "deepin.prohibitscreenshot.interface", QDBusConnection::systemBus());
    QDBusPendingReply<int> reply = screenShotInterface.call("setProhibited", prohibit);
    reply.waitForFinished();
    if (reply.isValid()) {
        // 0:成功,-1:鉴权失败,-2:进程白名单鉴权失败,-3:存在其他白名单进程打开防截屏,关闭防截屏失败
        qCDebug(DEEPINWATERMARK) << "com.deepin.prohibitscreenshot return : " << reply.value() << __FUNCTION__ << __LINE__;
        return reply.value();
    } else {
        qCDebug(DEEPINWATERMARK) << "com.deepin.prohibitscreenshot error : " << reply.error();
        return DEFAULT_ERROR;
    }
}

bool DeepinWatermark::watermarkStatus() const
{
    return m_watermarkStatus;
}

void DeepinWatermark::setScreenWatermark(const QString &strPolicy)
{
    uint invokerPid = connection().interface()->servicePid(message().service());
    if (!isValidInvoker(invokerPid)) {
        qCDebug(DEEPINWATERMARK) << QString("this pid=%1 not in the whitelist").arg(invokerPid) << __FUNCTION__ << __LINE__;
        m_watermarkStatus = false;
        return;
    }

    qCDebug(DEEPINWATERMARK) << strPolicy << __FUNCTION__ << __LINE__;

    QJsonObject jsonObj;
    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(strPolicy.toUtf8(), &jsonError);
    if (jsonError.error == QJsonParseError::NoError) {
        if (jsonDoc.isObject()) {
            jsonObj = jsonDoc.object();
            if (jsonObj.contains("status")) {
                m_isOpen = jsonObj.take("status").toBool();
            }
            if (jsonObj.contains("custom_content")) {
                m_content = jsonObj.take("custom_content").toString();
            }
            if (jsonObj.contains("current_time_display")) {
                m_showTime = jsonObj.take("current_time_display").toBool();
            }
            if (jsonObj.contains("font_size")) {
                m_fontSize = jsonObj.take("font_size").toInt();
            }
            if (jsonObj.contains("transparency")) {
                m_transparency = jsonObj.take("transparency").toInt();
            }
            if (jsonObj.contains("density_grade")) {
                m_density = jsonObj.take("density_grade").toInt();
            }
            if (jsonObj.contains("format")) {
                m_fontFormate = jsonObj.take("format").toInt();
            }
            if (jsonObj.contains("username_display")) {
                QJsonObject objUsername = jsonObj.take("username_display").toObject();
                if (objUsername.contains("status")) {
                    m_showUsrName = objUsername.take("status").toBool();
                }
                if (objUsername.contains("content")) {
                    m_usrName = objUsername.take("content").toString();
                }
            }
            if (jsonObj.contains("hostname_display")) {
                QJsonObject objHostname = jsonObj.take("hostname_display").toObject();
                if (objHostname.contains("status")) {
                    m_showHostName = objHostname.take("status").toBool();
                }
                if (objHostname.contains("content")) {
                    m_hostName = objHostname.take("content").toString();
                }
            }
            if (jsonObj.contains("terminal_alias_display")) {
                QJsonObject objTerminalName = jsonObj.take("terminal_alias_display").toObject();
                if (objTerminalName.contains("status")) {
                    m_showTerminalAliasName = objTerminalName.take("status").toBool();
                }
                if (objTerminalName.contains("content")) {
                    m_terminalAliasName = objTerminalName.take("content").toString();
                }
            }
            if (jsonObj.contains("ip_address_display")) {
                QJsonObject objIpaddress = jsonObj.take("ip_address_display").toObject();
                if (objIpaddress.contains("status")) {
                    m_showIpAddress = objIpaddress.take("status").toBool();
                }
                if (objIpaddress.contains("content")) {
                    m_ipAddress = objIpaddress.take("content").toString();
                }
            }
            if (jsonObj.contains("mac_address_display")) {
                QJsonObject objMacaddress = jsonObj.take("mac_address_display").toObject();
                if (objMacaddress.contains("status")) {
                    m_showMacAddress = objMacaddress.take("status").toBool();
                }
                if (objMacaddress.contains("content")) {
                    m_macAddress = objMacaddress.take("content").toString();
                }
            }
        }
    } else {
        m_watermarkStatus = false;
        qCDebug(DEEPINWATERMARK) << "this string is not a json object" << __FUNCTION__ << __LINE__;
    }
    writeConfig();
    refreshWindow();
    m_watermarkStatus = true;
}

void DeepinWatermark::initConfig()
{
    QFile file(WATERMARK_CONFIG);
    if (file.exists()) {
        readConfig();
    } else {
        if (file.open(QIODevice::ReadOnly | QIODevice::WriteOnly)) {
            writeConfig();
            file.close();
        }
    }
    refreshWindow();
}

void DeepinWatermark::paintEvent(QPaintEvent *event)
{
    if (nullptr == m_painter) {
        return;
    }
    int pixmapWidth = width();
    int pixmapHeight = height();
    // 计算旋转矩阵
    QTransform transform;
    int transformx = 0;
    int transformy = 0;
    if (m_compositorActive && m_fontFormate == FORMAT_LEAN) {
        transform.translate(width() / 2, height() / 2);
        transform.rotate(LEAN_ANGLE);
        transform.translate(-width() / 2, -height() / 2);
        transformx = transform.m11() * 0 + transform.m21() * 0 + transform.dx();
        transformy = transform.m22() * 0 + transform.m12() * width() + transform.dy();
        pixmapWidth = width() + qAbs(transformx) * 2;
        pixmapHeight = height() + qAbs(transformy) * 2;
    }

    QPixmap pixmap(pixmapWidth, pixmapHeight);
    pixmap.fill(Qt::transparent);
    m_painter->begin(&pixmap);
    m_painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    // 开启屏保的时候只画一个背景，不显示水印
    if (m_isOpenScreenSaver) {
        m_painter->drawRect(0, 0, width(), height());
        m_painter->end();
        m_painter->begin(this);
        m_painter->drawPixmap(QPoint(0, 0), pixmap);
        m_painter->end();
        setMask(pixmap.mask());
        return;
    }

    // 获取水印展示内容
    QString text = getCustomContent();
    QFont font = m_painter->font();
    font.setPointSize(m_fontSize);
    m_painter->setFont(font);
    //根据分辨率设置密度
    int hSpace = 0;
    int vSpace = 0;
    if (m_compositorActive) {
        hSpace = QGuiApplication::primaryScreen()->availableGeometry().width() * (m_density / 100.0f);
        vSpace = QGuiApplication::primaryScreen()->availableGeometry().height() * (m_density / 100.0f);
    }else {
        hSpace = QGuiApplication::primaryScreen()->availableGeometry().width() * (DENSITY);
        vSpace = QGuiApplication::primaryScreen()->availableGeometry().height() * (DENSITY);
    }
    // 获取字体的长和宽
    QFontMetrics fm(font);
    QRect rec = fm.boundingRect(text);
    int textWidth = hSpace + rec.width();
    int textHeight = vSpace + rec.height();
    int index = 0;
    bool isBreak = false;
    // 设置绘制区域
    int xStart = 0;
    int yStart = rec.height();
    int xEnd = width();
    int yEnd = height();
    if (m_compositorActive && m_fontFormate == FORMAT_LEAN) {
        xStart = transformx;
        yStart = transformy;
        xEnd = pixmapWidth;
        yEnd = pixmapHeight;
    }
    for (int yCoord = yStart; yCoord < yEnd; yCoord += textHeight, index++) {
        isBreak = false;
        for (int xCoord = xStart; xCoord < xEnd; xCoord += textWidth) {
            if (index > 0) {
                if (!isBreak) {
                    isBreak = true;
                    xCoord = calculateCoordinate(index, textWidth, hSpace);
                }
            }

            m_painter->save();
            // 2D下的旋转
            if (m_fontFormate == FORMAT_HORIZONTAL && !m_compositorActive) {
                m_painter->rotate(0);
            }else if (m_fontFormate == FORMAT_LEAN && !m_compositorActive) {
#if 1
                m_painter->translate(xCoord + rec.width() / 2, yCoord + rec.height() / 2);
                m_painter->rotate(LEAN_ANGLE);
                m_painter->translate(-(xCoord + rec.width() / 2), -(yCoord + rec.height() / 2));
#else
                m_painter->translate(xx, yy);
                m_painter->rotate(-30);
                m_painter->translate(-xx, -yy);
#endif
            }

            QPainterPath path;
            path.addText(xCoord, yCoord, font, text);
            QPen pen;
            pen.setWidth(1);
            pen.setColor(QColor("#000000"));
            if (m_compositorActive) {
                m_painter->setOpacity(0.1);
            }
            m_painter->strokePath(path, pen);
            m_painter->drawPath(path);
            if (m_compositorActive) {
                m_painter->setOpacity(m_transparency / 100.0f);
            }
            m_painter->fillPath(path, QBrush(QColor("#DFDFDF")));
            m_painter->restore();
        }
    }
    m_painter->end();
    if (!m_isX11Server) {
        setMask(QRegion(0,0,1,1));
    } else {
        if (!m_compositorActive) {
            setMask(pixmap.mask());
        }
    }
    m_painter->begin(this);
    int pixmapX = 0;
    int pixmapY = 0;
    // 3D下的旋转
    if (m_fontFormate == FORMAT_HORIZONTAL && m_compositorActive) {
        m_painter->rotate(0);
    }else if (m_fontFormate == FORMAT_LEAN && m_compositorActive) {
        m_painter->setTransform(transform);
        pixmapX = transformx;
        pixmapY = transformy;
    }
    m_painter->drawPixmap(QPoint(pixmapX, pixmapY), pixmap);
    m_painter->end();
    event->accept();
}

void DeepinWatermark::compositingToggled(bool active)
{
    qCDebug(DEEPINWATERMARK) << "kwin compositor is " << active << __FUNCTION__ << __LINE__;
    clearMask();
    if (m_isOpen) {
        m_compositorActive = active;
        // 部分机器防止2D切3D的时候屏幕闪黑，延迟显示
        QTimer::singleShot(50, this, [this](){
            show();
            update();
        });
    }
}

void DeepinWatermark::compositingSetup()
{
    qCDebug(DEEPINWATERMARK) << "kwin compositor setup" << __FUNCTION__ << __LINE__;
    // 部分机器防止2D切3D的时候屏幕闪黑，先隐藏水印
    hide();
}
