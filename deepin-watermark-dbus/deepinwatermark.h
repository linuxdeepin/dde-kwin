#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDBusContext>
#include <QSet>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(WATERMARK_LOG)

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
    void executeLinuxCmd(const QString &strCmd);
    void writeConfig();
    void readConfig();
    bool isValidInvoker(const uint &pid);
    void refreshWindow();
    QString getCustomContent() const;
    QString jsonAutoTest() const;
    int calculateCoordinate(const int &index, const int &h_space, const int &hSpace);

private:
    bool m_bIsX11Server{false};
    bool m_bWatermarkStatus{false};
    QTimer *m_pCurrentTime{nullptr};
    QPainter *m_pPainter{nullptr};
    bool m_bCompositorActive{false};
    QSet<QString> m_setWhiteProcess;

    bool m_bIsOpen{false};
    QString m_strContent{"默认水印"};
    bool m_bShowTime{false};
    QString m_strCurrentTime{""};
    int m_nFontSize{11};
    int m_nTransparency{50};
    int m_nDensity{5};
    int m_nFontFormate{FORMAT_LEAN};

    bool m_bShowUsrName{false};
    QString m_strUsrName{"xxx"};
    bool m_bShowHostName{false};
    QString m_strHostName{"xxx"};
    bool m_bshowTerminalAliasName{false};
    QString m_strTerminalAliasName{"xxx"};
    bool m_bShowIpAddress{false};
    QString m_strIpAddress{"xxx"};
    bool m_bShowMacAddress{false};
    QString m_strMacAddress{"xxx"};
public Q_SLOTS:
    int setProhibitScreenShot(bool prohibit);
    void setScreenWatermark(const QString &strPolicy);
    bool watermarkStatus() const;
    void initConfig();
private Q_SLOTS:
    void compositingToggled(bool active);
    void compositingSetup();
};

#endif // WIDGET_H
