/*
 * Copyright (C) 2019 Deepin Technology Co., Ltd.
 *
 * Author:     Sian Cao <yinshuiboy@gmail.com>
 *
 * Maintainer: Sian Cao <yinshuiboy@gmail.com>
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


#ifndef _DEEPIN_MULTITASKING_H
#define _DEEPIN_MULTITASKING_H

#include <QObject>
#include <QPoint>
#include <QHash>
#include <QtWidgets>
#include <QQuickView>
#include <QQuickPaintedItem>
#include <QQuickWidget>
#include <QWindow>
#include <QTimeLine>
#include <kwineffects.h>
#include <KF5/KWindowSystem/KWindowSystem>

#include "kwinutils.h"
#include "background.h"
#include "constants.h"

using namespace KWin;

class MultitaskingModel;

class DesktopThumbnail: public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(int desktop READ desktop WRITE setDesktop NOTIFY desktopChanged)
    Q_PROPERTY(float radius READ radius WRITE setRadius NOTIFY radiusChanged)
    Q_PROPERTY(QVariant windows READ windows NOTIFY windowsChanged)
    Q_PROPERTY(QString monitor READ monitor WRITE setMonitor NOTIFY monitorChanged)
public:
    DesktopThumbnail(QQuickItem* parent = 0): QQuickPaintedItem(parent) {
        setRenderTarget(QQuickPaintedItem::FramebufferObject);

        connect(&BackgroundManager::instance(), &BackgroundManager::desktopWallpaperChanged,
            [=](int d) {
                if (d == m_desktop) {
                    m_bg = BackgroundManager::instance().getBackground(m_desktop, m_monitor, size().toSize());
                    update();
                }
            });
    }

    int desktop() const { return m_desktop; }
    void setDesktop(int d) {
        if (m_desktop != d) {
            qCDebug(BLUR_CAT) << "[dm]: desktop changed from " << m_desktop << "->" << d;
            m_desktop = d;

            if (!size().isEmpty()) {
                m_bg = BackgroundManager::instance().getBackground(m_desktop, m_monitor, size().toSize());
            }

            emit desktopChanged();

            update();
        }
    }

    float radius() const { return m_radius; }
    void setRadius(float d) {
        if (m_radius != d) {
            m_radius = d;
            emit radiusChanged();
        }
    }

    QString monitor() const { return m_monitor; }
    void setMonitor(QString d) {
        if (m_monitor != d) {
            m_monitor = d;
            emit monitorChanged();
        }
    }

    Q_INVOKABLE void refreshWindows() {
        QList<WId> vl;
        for (auto wid: KWindowSystem::self()->windows()) {
            KWindowInfo info(wid, NET::WMDesktop);
            if (info.valid() && info.desktop() == m_desktop) {
                vl.append(wid);
            }
        }
        setWindows(vl);
    }

    QVariant windows() const { return QVariant::fromValue(m_windows); }
    void setWindows(QList<WId> ids) {
        m_windows.clear();
        for (auto id: ids) {
            m_windows.append(id);
        }
        emit windowsChanged();
        update();
    }

    Q_INVOKABLE QRect geometryForWindow(QVariant wid) {
        QRect r(0, 0, 150, 150);

        WId id = wid.toULongLong();
        if (geoData.contains(id)) {
            r = geoData[id];
        }

        return r;
    }

    QHash<WId, QRect> geoData;

    Q_INVOKABLE void setupLayout(QHash<WId, QRect> data) {
        geoData = data;
        emit windowsLayoutChanged();
        update();
    }

    void paint(QPainter* p) override {
        QRect rect(0, 0, width(), height());

        QPainterPath clip;
        clip.addRoundedRect(rect, m_radius, m_radius);
        p->setClipPath(clip);

        p->drawPixmap(0, 0, m_bg);
    }

    Q_INVOKABLE void getDesktopThumbnailBackground(int nDesktop, QString screenName, int nWidth, int nHeight) {
        m_bg = BackgroundManager::instance().getBackground(nDesktop, screenName, QSize(nWidth,nHeight));
        update();
    }

signals:
    void desktopChanged();
    void radiusChanged();
    void windowsChanged();
    void windowsLayoutChanged();
    void monitorChanged();

private:
    int m_desktop {0};
    float m_radius {0};
    QString m_monitor;
    QVariantList m_windows;
    QPixmap m_bg;
};

//manage DesktopThumbnails
class DesktopThumbnailManager: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QSize thumbSize READ thumbSize NOTIFY thumbSizeChanged)
    Q_PROPERTY(int currentDesktop READ currentDesktop NOTIFY currentDesktopChanged)
    Q_PROPERTY(int desktopCount READ desktopCount NOTIFY desktopCountChanged)
    Q_PROPERTY(bool showPlusButton READ showPlusButton NOTIFY showPlusButtonChanged)
    Q_PROPERTY(QSize containerSize READ containerSize NOTIFY containerSizeChanged)

public:
    DesktopThumbnailManager(EffectsHandler* h);
    void windowInputMouseEvent(QMouseEvent* e);
    void setEffectWindow(EffectWindow* w) {
        m_effectWindow = w;
    }

    int desktopAtPos(QPoint);

    EffectWindow* effectWindow() {
        return m_effectWindow;
    }

    QSize thumbSize() const;
    int desktopCount() const;
    int currentDesktop() const;
    bool showPlusButton() const;
    QSize containerSize() const;

    void updateWindowThumbsGeometry(int desktop, const WindowMotionManager& wmm);

    Q_INVOKABLE QRect calculateDesktopThumbRect(int index);
    void updateWindowsFor(int desktop, QList<WId> ids);

    void onDesktopsChanged();

    Q_INVOKABLE void debugLog(const QString& msg);

protected:
    void mouseMoveEvent(QMouseEvent* e) override;
    void resizeEvent(QResizeEvent* re) override;
    void enterEvent(QEvent* e) override;
    void leaveEvent(QEvent* e) override;

signals:
    void currentDesktopChanged();
    void desktopCountChanged();
    void showPlusButtonChanged();
    void containerSizeChanged();
    void thumbSizeChanged();
    void layoutChanged();
    void desktopWindowsChanged(QVariant id);
    void desktopRemoved(QVariant id);

    // internal
    void switchDesktop(int left, int right);
    void requestChangeCurrentDesktop(int d);
    void requestAppendDesktop();
    void requestDeleteDesktop(int);
    void requestMove2Desktop(int, int, QVariant);
    void requestSwitchDesktop(int, int);

    void mouseLeaved();

private:
    EffectWindow* m_effectWindow {nullptr};
    EffectsHandler* m_handler {nullptr};

    // <desktop id -> wid list>
    QHash<int, QList<WId>> m_windowsHash;

    QQuickWidget* m_view {nullptr};

    mutable QSize m_wsThumbSize;

    QSize calculateThumbDesktopSize() const;
};

/**
 *  Deepin Multitasking View Effect
 **/
class MultitaskingEffect : public Effect
{
    Q_OBJECT
public:
    MultitaskingEffect();
    virtual ~MultitaskingEffect();

    virtual void reconfigure(ReconfigureFlags) override;

    // Screen painting
    virtual void prePaintScreen(ScreenPrePaintData &data, TimeArgType time) override;
#if KWIN_VERSION_MIN > 17 || (KWIN_VERSION_MIN == 17 && KWIN_VERSION_PAT > 5)
    virtual void paintScreen(int mask, const QRegion &region, ScreenPaintData &data) override;
#else
    virtual void paintScreen(int mask, QRegion region, ScreenPaintData &data) override;
#endif
    virtual void postPaintScreen() override;

    // Window painting
    virtual void prePaintWindow(EffectWindow *w, WindowPrePaintData &data, TimeArgType time) override;
    virtual void paintWindow(EffectWindow *w, int mask, QRegion region, WindowPaintData &data) override;

    // User interaction
    virtual void windowInputMouseEvent(QEvent *e) override;
    virtual void grabbedKeyboardEvent(QKeyEvent *e) override;
    virtual bool isActive() const override;

    int requestedEffectChainPosition() const override {
        return 10;
    }

    void updateHighlightWindow(EffectWindow* w);
    QVector<int> desktopList(const EffectWindow *w) const;

Q_SIGNALS:
    void modeChanged();
    void forceResetDesktopModel();
    void updateDesktopThumBackground();
    void forceResetWindowThumbnailModel();

public Q_SLOTS:
    void setActive(bool active);
    void toggleActive()  {
        setActive(!m_multitaskingViewVisible);
    }
    void globalShortcutChanged(QAction *action, const QKeySequence &seq);
    void onWindowAdded(KWin::EffectWindow*);
    void onWindowClosed(KWin::EffectWindow*);
    void onWindowDeleted(KWin::EffectWindow*);
    void onPropertyNotify(KWin::EffectWindow *w, long atom);

    void appendDesktop();
    void removeDesktop(int d);


    void selectNextWindow();
    void selectPrevWindow();

    void selectNextWindowVert(int dir);
    void selectFirstWindow();
    void selectLastWindow();

    void selectNextGroupWindow();
    void selectPrevGroupWindow();


    void selectWindow(EffectWindow* w);

    void changeCurrentDesktop(int d);

    void moveWindow2Desktop(int screen, int desktop, QVariant winId);
    void moveEffectWindow2Desktop(KWin::EffectWindow* ew, int desktop);
    void switchTwoDesktop(int to, int from);
    void moveWindowThumbnail2Desktop(int desktop);

    WId findWId(EffectWindow* ew);

    QVariantList windowsFor(int screen, int desktop);
    void updateDesktopWindows();
    void updateDesktopWindows(int desktop);

    // added when refactor
    void refreshWindows();

private slots:
    void onNumberDesktopsChanged(int old);
    void onNumberScreensChanged();
    void onScreenSizeChanged();


    void onCurrentDesktopChanged();
    void closeWindow();
    void toggleWindowKeepAbove();
    void remanageAll();

    void desktopRemoved(int d);
    //zhd add start
    void OnWindowLocateChanged(int screen, int desktop, int winId);
    //zhd add end

    void windowSelectSlot( QVariant winid );
    void removeEffectWindow(int screen, int desktop, QVariant winid);

    EffectWindow* multitaskingViewEffectWindow() {
        return m_multitaskingViewEffectWindow;
    }
    void setMultitaskingViewEffectWindow(EffectWindow* w) {
        m_multitaskingViewEffectWindow = w;
    }

    void slotCloseEffect(bool isSleepBefore);

private:
    struct WindowData {
        bool isAbove {false};
        bool csd {false};
        QMargins gtkFrameExtents; //valid only when csd == true

        EffectFrame* close {nullptr};
        EffectFrame* pin {nullptr};
        EffectFrame* unpin {nullptr};
        EffectFrame* icon {nullptr};
    };
    typedef QHash<EffectWindow*, WindowData> DataHash;
    struct GridSize {
        int columns {0};
        int rows {0};
    };


private:
    bool isRelevantWithPresentWindows(EffectWindow *w) const;

    void calculateWindowTransformations(EffectWindowList windows, WindowMotionManager& wmm);

    // close animation finished
    void cleanup();

    void updateGtkFrameExtents(EffectWindow *w);

    void updateWindowStates(QMouseEvent* me);

    QRectF highlightedGeometry(QRectF geometry);

    // borrowed from PresentWindows effect
    void calculateWindowTransformationsClosest(EffectWindowList windowlist, int screen,
            WindowMotionManager& motionManager);
    void clearGrids();
    bool isOverlappingAny(EffectWindow *w, const QHash<EffectWindow*, QRect> &targets, const QRegion &border);
    inline double aspectRatio(EffectWindow *w) {
        return w->width() / double(w->height());
    }
    inline int widthForHeight(EffectWindow *w, int height) {
        return int((height / double(w->height())) * w->width());
    }
    inline int heightForWidth(EffectWindow *w, int width) {
        return int((width / double(w->width())) * w->height());
    }

    EffectFrame* createIconFor(EffectWindow*);
    void initWindowData(DataHash::iterator wd, EffectWindow* w);

private:
    DataHash m_windowDatas;

    // Activation
    bool m_activated {false};
    bool m_hasKeyboardGrab {false};

    // Window data
    QVector<WindowMotionManager> m_motionManagers;
    WindowMotionManager m_thumbMotion;
    EffectWindow* m_highlightWindow {nullptr};
    EffectWindow* m_selectedWindow {nullptr};
    EffectWindow* m_closingdWindow {nullptr};

    EffectWindow* m_movingWindow {nullptr};
    bool m_isWindowMoving {false};
    QRect m_movingWindowGeometry;
    QPoint m_movingWindowStartPoint;
    QPoint m_dragStartPos;

    // Shortcut - needed to toggle the effect
    QList<QKeySequence> shortcut;

    // timeline for toggleActive
    QTimeLine m_toggleTimeline;

    // Desktop currently rendering, could be different thant current desktop
    // when Left/Right key pressed e.g
    // index from 1
    int m_targetDesktop {0};

    QMargins m_desktopMargins;

    EffectFrame* m_highlightFrame {nullptr};

    // Grid layout info
    QHash<int, GridSize> m_gridSizes;
    // <desktop id -> windows>
    QHash<int, QVector<EffectWindow*>> m_takenSlots;

    long m_gtkFrameExtentsAtom {0};

    DesktopThumbnailManager* m_thumbManager {nullptr};

    QAction *m_showAction;

    QMargins desktopMargins();

    QQuickWidget *m_multitaskingView { nullptr };
    bool          m_multitaskingViewVisible { false };
	MultitaskingModel *m_multitaskingModel { nullptr };

    EffectWindow *m_pEffectWindow;
    EffectWindow *m_multitaskingViewEffectWindow { nullptr };

    //Touch Screen
    QDateTime m_currentDateTime;
};



#endif /* ifndef _DEEPIN_MULTITASKING_H */
