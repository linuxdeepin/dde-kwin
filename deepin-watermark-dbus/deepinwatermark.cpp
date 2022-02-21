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
Q_LOGGING_CATEGORY(WATERMARK_LOG, "watermark_log")
// 水印配置文件
const QString WATERMARK_CONFIG = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/kwatermark";

DeepinWatermark::DeepinWatermark(QWidget *parent) :
    QWidget(parent)
{
    // 判断当前平台是x11还是wayland
    executeLinuxCmd("loginctl show-session $(loginctl | grep $(whoami) | awk '{print $1}') -p Type");
    setGeometry(0, 0, qApp->desktop()->width(), qApp->desktop()->height());//设置水印覆盖整个整个屏幕
    QDBusInterface watermarkInterface("org.kde.KWin", "/Compositor", "org.kde.kwin.Compositing");
    m_bCompositorActive = watermarkInterface.property("active").toBool();
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
    if (m_bIsX11Server) {
        Utils::passInputEvent(winId());
    }
    m_pPainter = new QPainter(this);
    // 监听kwin切换2D和3D的信号
    QDBusConnection::sessionBus().connect("org.kde.KWin", "/Compositor", "org.kde.kwin.Compositing", "compositingToggled",
                                          this, SLOT(compositingToggled(bool)));

    // 监听kwin compositor setup信号
    QDBusConnection::sessionBus().connect("org.kde.KWin", "/Compositor", "org.kde.kwin.Compositing", "compositingSetup",
                                          this, SLOT(compositingSetup()));

    m_pCurrentTime = new QTimer(this);
    m_pCurrentTime->setInterval(1000);
    connect(m_pCurrentTime, &QTimer::timeout, this, [this](){
       QString strCurrentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd");
       if (m_strCurrentTime != strCurrentTime) {
           m_strCurrentTime = strCurrentTime;
           update();
       }
    });

    initConfig();
}

DeepinWatermark::~DeepinWatermark()
{
}

void DeepinWatermark::executeLinuxCmd(const QString &strCmd)
{
    QProcess p;
    p.start("bash", QStringList() <<"-c" << strCmd);
    p.waitForFinished();
    QString result = p.readAllStandardOutput();
    qCDebug(WATERMARK_LOG) << "the current display server is : " << result;
    if (result.replace("\n", "").contains("Type=x11")) {
        m_bIsX11Server = true;
    } else {
        m_bIsX11Server = false;
    }
}

void DeepinWatermark::writeConfig()
{
    QSettings settings(WATERMARK_CONFIG, QSettings::IniFormat);
    settings.setIniCodec("UTF8");
    settings.beginGroup("basic");
    settings.setValue("status", m_bIsOpen);
    settings.setValue("custom_content", m_strContent);
    settings.setValue("current_time_display", m_bShowTime);
    settings.setValue("font_size", m_nFontSize);
    settings.setValue("transparency", m_nTransparency);
    settings.setValue("density_grade", m_nDensity);
    settings.setValue("format", m_nFontFormate);
    settings.endGroup();

    settings.beginGroup("username_display");
    settings.setValue("status", m_bShowUsrName);
    settings.setValue("content", m_strUsrName);
    settings.endGroup();

    settings.beginGroup("hostname_display");
    settings.setValue("status", m_bShowHostName);
    settings.setValue("content", m_strHostName);
    settings.endGroup();

    settings.beginGroup("terminal_alias_display");
    settings.setValue("status", m_bshowTerminalAliasName);
    settings.setValue("content", m_strTerminalAliasName);
    settings.endGroup();

    settings.beginGroup("ip_address_display");
    settings.setValue("status", m_bShowIpAddress);
    settings.setValue("content", m_strIpAddress);
    settings.endGroup();

    settings.beginGroup("mac_address_display");
    settings.setValue("status", m_bShowMacAddress);
    settings.setValue("content", m_strMacAddress);
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
    m_bIsOpen = settings.value("basic/status").toBool();
    m_strContent = settings.value("basic/custom_content").toString();
    m_bShowTime = settings.value("basic/current_time_display").toBool();
    m_nFontSize = settings.value("basic/font_size").toInt();
    m_nTransparency = settings.value("basic/transparency").toInt();
    m_nDensity = settings.value("basic/density_grade").toInt();
    m_nFontFormate = settings.value("basic/format").toInt();

    m_bShowUsrName = settings.value("username_display/status").toBool();
    m_strUsrName = settings.value("username_display/content").toString();

    m_bShowHostName = settings.value("hostname_display/status").toBool();
    m_strHostName = settings.value("hostname_display/content").toString();

    m_bshowTerminalAliasName = settings.value("terminal_alias_display/status").toBool();
    m_strTerminalAliasName = settings.value("terminal_alias_display/content").toString();

    m_bShowIpAddress = settings.value("ip_address_display/status").toBool();
    m_strIpAddress = settings.value("ip_address_display/content").toString();

    m_bShowMacAddress = settings.value("mac_address_display/status").toBool();
    m_strMacAddress = settings.value("mac_address_display/content").toString();

    QStringList whitelist = settings.value("whitelist/names").toString().split(" ");
    for (auto str : whitelist) {
        m_setWhiteProcess.insert(str);
    }
}

bool DeepinWatermark::isValidInvoker(const uint &pid)
{
    QFileInfo fileInfo(QString("/proc/%1/exe").arg(pid));
    if (!fileInfo.exists()) {
        return false;
    }
    QString invokerPath = fileInfo.canonicalFilePath().split("/").last();
    return m_setWhiteProcess.contains(invokerPath);
}

void DeepinWatermark::refreshWindow()
{
    if (!m_bIsX11Server) {
        clearMask();
    }
    if (m_bIsOpen) {
        if (m_bShowTime) {
            m_strCurrentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd");
            m_pCurrentTime->start();
        }else {
            m_pCurrentTime->stop();
        }
        show();
    }else {
        hide();
    }
    update();
}

QString DeepinWatermark::getCustomContent() const
{
    // 水印顺序:自定义内容+用户名+主机名+终端别名+当前时间+IP地址+MAC地址
    QString strContent;
    if (!m_strContent.isEmpty()) {
        strContent.append(m_strContent + " ");
    }
    if (m_bShowUsrName) {
        strContent.append(m_strUsrName + " ");
    }
    if (m_bShowHostName) {
        strContent.append(m_strHostName + " ");
    }
    if (m_bshowTerminalAliasName) {
        strContent.append(m_strTerminalAliasName + " ");
    }
    if (m_bShowTime) {
        strContent.append(m_strCurrentTime + " ");
    }
    if (m_bShowIpAddress) {
        strContent.append(m_strIpAddress + " ");
    }
    if (m_bShowMacAddress) {
        strContent.append(m_strMacAddress + " ");
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

int DeepinWatermark::calculateCoordinate(const int &index, const int &h_space, const int &hSpace)
{
    int nx = 0;
    int tmpWidth = index * hSpace;
    if (tmpWidth == hSpace) {
        nx = hSpace;
    }else {
        // 计算本行首个水印需要展示的位置
        QStringList listSpace = QString::number(float(tmpWidth)/h_space, 'f', 1).split(".");
        if (listSpace.size() > 1) {
            if (listSpace.at(1).toInt() > 0) {
                nx = -(h_space - (listSpace.at(1).toFloat() / 10)*h_space);
            }
        }
    }
    return nx;
}

int DeepinWatermark::setProhibitScreenShot(bool prohibit)
{
    QDBusInterface screenShotInterface("com.deepin.prohibitscreenshot", "/com/deepin/prohibitscreenshot",
                                       "deepin.prohibitscreenshot.interface", QDBusConnection::systemBus());
    QDBusPendingReply<int> reply = screenShotInterface.call("setProhibited", prohibit);
    reply.waitForFinished();
    if (reply.isValid()) {
        // 0:成功,-1:鉴权失败,-2:进程白名单鉴权失败,-3:存在其他白名单进程打开防截屏,关闭防截屏失败
        qCDebug(WATERMARK_LOG) << "com.deepin.prohibitscreenshot return : " << reply.value() << __FUNCTION__ << __LINE__;
        return reply.value();
    } else {
        qCDebug(WATERMARK_LOG) << "com.deepin.prohibitscreenshot error : " << reply.error();
        return -4;
    }
}

bool DeepinWatermark::watermarkStatus() const
{
    return m_bWatermarkStatus;
}

void DeepinWatermark::setScreenWatermark(const QString &strPolicy)
{
    uint invokerPid = connection().interface()->servicePid(message().service());
    if (!isValidInvoker(invokerPid)) {
        qCDebug(WATERMARK_LOG) << QString("this pid=%1 not in the whitelist").arg(invokerPid) << __FUNCTION__ << __LINE__;
        m_bWatermarkStatus = false;
        return;
    }

    qCDebug(WATERMARK_LOG) << strPolicy << __FUNCTION__ << __LINE__;

    QJsonObject jsonObj;
    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(strPolicy.toUtf8(), &jsonError);
    if (jsonError.error == QJsonParseError::NoError) {
        if (jsonDoc.isObject()) {
            jsonObj = jsonDoc.object();
            if (jsonObj.contains("status")) {
                m_bIsOpen = jsonObj.take("status").toBool();
            }
            if (jsonObj.contains("custom_content")) {
                m_strContent = jsonObj.take("custom_content").toString();
            }
            if (jsonObj.contains("current_time_display")) {
                m_bShowTime = jsonObj.take("current_time_display").toBool();
            }
            if (jsonObj.contains("font_size")) {
                m_nFontSize = jsonObj.take("font_size").toInt();
            }
            if (jsonObj.contains("transparency")) {
                m_nTransparency = jsonObj.take("transparency").toInt();
            }
            if (jsonObj.contains("density_grade")) {
                m_nDensity = jsonObj.take("density_grade").toInt();
            }
            if (jsonObj.contains("format")) {
                m_nFontFormate = jsonObj.take("format").toInt();
            }
            if (jsonObj.contains("username_display")) {
                QJsonObject objUsername = jsonObj.take("username_display").toObject();
                if (objUsername.contains("status")) {
                    m_bShowUsrName = objUsername.take("status").toBool();
                }
                if (objUsername.contains("content")) {
                    m_strUsrName = objUsername.take("content").toString();
                }
            }
            if (jsonObj.contains("hostname_display")) {
                QJsonObject objHostname = jsonObj.take("hostname_display").toObject();
                if (objHostname.contains("status")) {
                    m_bShowHostName = objHostname.take("status").toBool();
                }
                if (objHostname.contains("content")) {
                    m_strHostName = objHostname.take("content").toString();
                }
            }
            if (jsonObj.contains("terminal_alias_display")) {
                QJsonObject objTerminalName = jsonObj.take("terminal_alias_display").toObject();
                if (objTerminalName.contains("status")) {
                    m_bshowTerminalAliasName = objTerminalName.take("status").toBool();
                }
                if (objTerminalName.contains("content")) {
                    m_strTerminalAliasName = objTerminalName.take("content").toString();
                }
            }
            if (jsonObj.contains("ip_address_display")) {
                QJsonObject objIpaddress = jsonObj.take("ip_address_display").toObject();
                if (objIpaddress.contains("status")) {
                    m_bShowIpAddress = objIpaddress.take("status").toBool();
                }
                if (objIpaddress.contains("content")) {
                    m_strIpAddress = objIpaddress.take("content").toString();
                }
            }
            if (jsonObj.contains("mac_address_display")) {
                QJsonObject objMacaddress = jsonObj.take("mac_address_display").toObject();
                if (objMacaddress.contains("status")) {
                    m_bShowMacAddress = objMacaddress.take("status").toBool();
                }
                if (objMacaddress.contains("content")) {
                    m_strMacAddress = objMacaddress.take("content").toString();
                }
            }
        }
    }else {
        m_bWatermarkStatus = false;
        qCDebug(WATERMARK_LOG) << "this string is not a json object" << __FUNCTION__ << __LINE__;
    }
    writeConfig();
    refreshWindow();
    m_bWatermarkStatus = true;
}

void DeepinWatermark::initConfig()
{
    QFile file(WATERMARK_CONFIG);
    if (file.exists()) {
        readConfig();
    }else {
        if (file.open(QIODevice::ReadOnly | QIODevice::WriteOnly)) {
            writeConfig();
            file.close();
        }
    }
    refreshWindow();
}

void DeepinWatermark::paintEvent(QPaintEvent *event)
{
    if (nullptr == m_pPainter) {
        return;
    }
    int pixmapWidth = width();
    int pixmapHeight = height();
    // 计算旋转矩阵
    QTransform transform;
    int transformx = 0;
    int transformy = 0;
    if (m_bCompositorActive && m_nFontFormate == FORMAT_LEAN) {
        transform.translate(width() / 2, height() / 2);
        transform.rotate(-30);
        transform.translate(-width() / 2, -height() / 2);
        transformx = transform.m11() * 0 + transform.m21() * 0 + transform.dx();
        transformy = transform.m22() * 0 + transform.m12() * width() + transform.dy();
        pixmapWidth = width() + qAbs(transformx) * 2;
        pixmapHeight = height() + qAbs(transformy) * 2;
    }

    QPixmap pixmap(pixmapWidth, pixmapHeight);
    pixmap.fill(Qt::transparent);
    m_pPainter->begin(&pixmap);
    m_pPainter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    // 获取水印展示内容
    QString text = getCustomContent();
    QFont font = m_pPainter->font();
    font.setPointSize(m_nFontSize);
    m_pPainter->setFont(font);
    //根据分辨率设置密度
    int hSpace = 0;
    int vSpace = 0;
    if (m_bCompositorActive) {
        hSpace = QGuiApplication::primaryScreen()->availableGeometry().width() * (float(m_nDensity) / 100);
        vSpace = QGuiApplication::primaryScreen()->availableGeometry().height() * (float(m_nDensity) / 100);
    }else {
        hSpace = QGuiApplication::primaryScreen()->availableGeometry().width() * (0.15);
        vSpace = QGuiApplication::primaryScreen()->availableGeometry().height() * (0.15);
    }
    // 获取字体的长和宽
    QFontMetrics fm(font);
    QRect rec = fm.boundingRect(text);
    int h_space = hSpace + rec.width();
    int v_space = vSpace + rec.height();
    int index = 0;
    bool isBreak = false;
    // 设置绘制区域
    int xStart = 0;
    int yStart = rec.height();
    int xEnd = width();
    int yEnd = height();
    if (m_bCompositorActive && m_nFontFormate == FORMAT_LEAN) {
        xStart = transformx;
        yStart = transformy;
        xEnd = pixmapWidth;
        yEnd = pixmapHeight;
    }
    for (int yy = yStart; yy < yEnd; yy += v_space, index++) {
        isBreak = false;
        for (int xx = xStart; xx < xEnd; xx += h_space) {
            if (index > 0) {
                if (!isBreak) {
                    isBreak = true;
                    xx = calculateCoordinate(index, h_space, hSpace);
                }
            }

            m_pPainter->save();
            // 2D下的旋转
            if (m_nFontFormate == FORMAT_HORIZONTAL && !m_bCompositorActive) {
                m_pPainter->rotate(0);
            }else if (m_nFontFormate == FORMAT_LEAN && !m_bCompositorActive) {
#if 1
                m_pPainter->translate(xx + rec.width() / 2, yy + rec.height() / 2);
                m_pPainter->rotate(-30);
                m_pPainter->translate(-(xx + rec.width() / 2), -(yy + rec.height() / 2));
#else
                m_pPainter->translate(xx, yy);
                m_pPainter->rotate(-30);
                m_pPainter->translate(-xx, -yy);
#endif
            }

            QPainterPath path;
            path.addText(xx, yy, font, text);
            QPen pen;
            pen.setWidth(1);
            pen.setColor(QColor("#000000"));
            if (m_bCompositorActive) {
                m_pPainter->setOpacity(0.1);
            }
            m_pPainter->strokePath(path, pen);
            m_pPainter->drawPath(path);
            if (m_bCompositorActive) {
                m_pPainter->setOpacity(float(m_nTransparency) / 100);
            }
            m_pPainter->fillPath(path, QBrush(QColor("#DFDFDF")));
            m_pPainter->restore();
        }
    }
    m_pPainter->end();
    if (!m_bIsX11Server) {
        setMask(QRegion(0,0,1,1));
    } else {
        if (!m_bCompositorActive) {
            setMask(pixmap.mask());
        }
    }
    m_pPainter->begin(this);
    int pixmapX = 0;
    int pixmapY = 0;
    // 3D下的旋转
    if (m_nFontFormate == FORMAT_HORIZONTAL && m_bCompositorActive) {
        m_pPainter->rotate(0);
    }else if (m_nFontFormate == FORMAT_LEAN && m_bCompositorActive) {
        m_pPainter->setTransform(transform);
        pixmapX = transformx;
        pixmapY = transformy;
    }
    m_pPainter->drawPixmap(QPoint(pixmapX, pixmapY), pixmap);
    m_pPainter->end();
    event->accept();
}

void DeepinWatermark::compositingToggled(bool active)
{
    qCDebug(WATERMARK_LOG) << "kwin compositor is " << active << __FUNCTION__ << __LINE__;
    clearMask();
    if (m_bIsOpen) {
        m_bCompositorActive = active;
        // 部分机器防止2D切3D的时候屏幕闪黑，延迟显示
        QTimer::singleShot(50, this, [this](){
            show();
            update();
        });
    }
}

void DeepinWatermark::compositingSetup()
{
    qCDebug(WATERMARK_LOG) << "kwin compositor setup" << __FUNCTION__ << __LINE__;
    // 部分机器防止2D切3D的时候屏幕闪黑，先隐藏水印
    hide();
}
