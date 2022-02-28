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
#ifndef DEEPINWATERMARK_H
#define DEEPINWATERMARK_H

#include <QWidget>
#include <QDBusContext>
#include <QSet>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(DEEPINWATERMARK)

#define FORMAT_HORIZONTAL 1
#define FORMAT_LEAN 2

class DeepinWatermark : public QWidget, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.watermark")

public:
    explicit DeepinWatermark(QWidget *parent = 0);
    ~DeepinWatermark();

protected:
    void paintEvent(QPaintEvent *event);

private:
    void writeConfig();
    void readConfig();
    bool isValidInvoker(const uint &pid);
    void refreshWindow();
    QString getCustomContent() const;
    QString jsonAutoTest() const;
    int calculateCoordinate(const int &index, const int &textWidth, const int &hSpace);

private:
    int m_screenWidth;
    int m_screenHeight;
    QTimer *m_getDesktopStatusTimer{nullptr};
    bool m_isOpenScreenSaver{false};

    bool m_isX11Server{false};
    bool m_watermarkStatus{false};
    QTimer *m_currentTime{nullptr};
    QPainter *m_painter{nullptr};
    bool m_compositorActive{false};
    QSet<QString> m_whiteProcess;

    bool m_isOpen{false};
    QString m_content{""};
    bool m_showTime{false};
    QString m_currentTimeString{""};
    int m_fontSize{11};
    int m_transparency{50};
    int m_density{5};
    int m_fontFormate{FORMAT_LEAN};

    bool m_showUsrName{false};
    QString m_usrName{""};
    bool m_showHostName{false};
    QString m_hostName{""};
    bool m_showTerminalAliasName{false};
    QString m_terminalAliasName{""};
    bool m_showIpAddress{false};
    QString m_ipAddress{""};
    bool m_showMacAddress{false};
    QString m_macAddress{""};
public Q_SLOTS:
    int setProhibitScreenShot(bool prohibit);
    void setScreenWatermark(const QString &strPolicy);
    bool watermarkStatus() const;
    void initConfig();
private Q_SLOTS:
    void compositingToggled(bool active);
    void compositingSetup();
    void desktopResize(QRect rect);
};

#endif // DEEPINWATERMARK_H
