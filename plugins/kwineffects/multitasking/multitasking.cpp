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

#include <QtCore>

#include "multitasking.h"
#include <QtGui>
#include <QMetaObject>
#include <QtWidgets>
#include <QQmlContext>
#include <QQuickItem>
#include <KGlobalAccel>
#include <KLocalizedString>
#include <QQmlApplicationEngine>
#include <QQmlImageProviderBase>
#include "multitasking_model.h"
#include "imageprovider.h"

#include "multitasking_model.h"

#define ACTION_NAME  "ShowMultitasking"

Q_LOGGING_CATEGORY(BLUR_CAT, "kwin.blur", QtCriticalMsg);

static const QByteArray s_GtkFrameAtomName = QByteArrayLiteral("_GTK_FRAME_EXTENTS");

DesktopThumbnailManager::DesktopThumbnailManager(EffectsHandler* h)
    :QWidget(0),
    m_effectWindow(nullptr),
    m_handler(h)
{
    setObjectName("DesktopThumbnailManager");
    setWindowTitle("DesktopThumbnailManager");

    setWindowFlags(Qt::BypassWindowManagerHint | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);

    QString qm = QString(":/translations/multitasking_%1.qm").arg(QLocale::system().name());
    auto tran = new QTranslator(this);
    if (tran->load(qm)) {
        qApp->installTranslator(tran);
    } else {
        qCDebug(BLUR_CAT) << "load " << qm << "failed";
    }
/*
    m_view = new QQuickWidget(this);
    m_view->setGeometry(0, 0, width(), height());
    m_view->setClearColor(Qt::transparent);
    QSurfaceFormat fmt = m_view->format();
    fmt.setAlphaBufferSize(8);
    m_view->setFormat(fmt);

    m_view->rootContext()->setContextProperty("manager", this);
    m_view->rootContext()->setContextProperty("backgroundManager", &BackgroundManager::instance());

    qmlRegisterType<DesktopThumbnail>("com.deepin.kwin", 1, 0, "DesktopThumbnail");

    m_view->setSource(QUrl("qrc:/qml/thumbmanager.qml"));

    auto root = m_view->rootObject();
    root->setAcceptHoverEvents(true);
    connect(this, SIGNAL(layoutChanged()), root, SLOT(handleLayoutChanged()), Qt::QueuedConnection);
    connect(this, SIGNAL(desktopRemoved(QVariant)), root, SLOT(handleDesktopRemoved(QVariant)),
            Qt::QueuedConnection);
    connect(this, SIGNAL(mouseLeaved()), root, SIGNAL(mouseLeaved()));

    // relay QML signals
    connect(root, SIGNAL(qmlRequestChangeDesktop(int)), this, SIGNAL(requestChangeCurrentDesktop(int)));
    connect(root, SIGNAL(qmlRequestAppendDesktop()), this, SIGNAL(requestAppendDesktop()));
    connect(root, SIGNAL(qmlRequestDeleteDesktop(int)), this, SIGNAL(requestDeleteDesktop(int)));
    connect(root, SIGNAL(qmlRequestMove2Desktop(QVariant, int)), this, SIGNAL(requestMove2Desktop(QVariant, int)));
    connect(root, SIGNAL(qmlRequestSwitchDesktop(int, int)), this, SIGNAL(requestSwitchDesktop(int, int)));

    connect(m_handler, SIGNAL(desktopChanged(int, int, KWin::EffectWindow*)), this, SIGNAL(currentDesktopChanged()));
*/
}

void DesktopThumbnailManager::debugLog(const QString& msg)
{
    qCDebug(BLUR_CAT) << "[dtm]: " << msg;
}

void DesktopThumbnailManager::onDesktopsChanged()
{
    emit desktopCountChanged();
    emit layoutChanged();
    emit showPlusButtonChanged();
}

bool DesktopThumbnailManager::showPlusButton() const
{
    return m_handler->numberOfDesktops() < 4;
}

QSize DesktopThumbnailManager::containerSize() const
{
    return {width(), height()};
}

QSize DesktopThumbnailManager::thumbSize() const
{
    return calculateThumbDesktopSize();
}

int DesktopThumbnailManager::desktopCount() const
{
    return m_handler->numberOfDesktops();
}

int DesktopThumbnailManager::currentDesktop() const
{
    return m_handler->currentDesktop();
}


void DesktopThumbnailManager::resizeEvent(QResizeEvent* re)
{
    QWidget::resizeEvent(re);
    m_wsThumbSize = QSize(); // invalidate
    m_view->resize(re->size());

    emit containerSizeChanged();
    emit thumbSizeChanged();
    emit layoutChanged();
}

int DesktopThumbnailManager::desktopAtPos(QPoint p)
{
    if (!geometry().contains(p)) return -1;

    auto pos = m_view->mapFromGlobal(p);
    auto root = m_view->rootObject();
    auto thumbs = root->findChildren<DesktopThumbnail*>();
    for (auto th: thumbs) {
        QPointF th_pos(th->x(), th->y());
        QRectF r(th->mapToScene(th_pos), QSizeF{th->width(), th->height()});
        qCDebug(BLUR_CAT) << "thumb " << th->property("desktop") << pos << r;
        if (r.contains(pos)) {
            return th->property("desktop").toInt();
        }
    }

    return -1;
}

void DesktopThumbnailManager::windowInputMouseEvent(QMouseEvent* e)
{
    auto w = childAt(e->pos());
    if (w) {
        auto widget_pos = w->mapFromGlobal(e->globalPos());
        QMouseEvent child_ev(e->type(), widget_pos, e->globalPos(), e->button(), e->buttons(), e->modifiers());
        qApp->sendEvent(w, &child_ev);
    } else {
        qApp->sendEvent(this, e);
        //qApp->sendEvent(m_view, e);
    }
}

QSize DesktopThumbnailManager::calculateThumbDesktopSize() const
{
    //TODO: reset on desktop/screen changes
    if (m_wsThumbSize.isEmpty()) {
        //TODO: should be primary screen
        auto area = m_handler->clientArea(ScreenArea, 0, 0);

        auto h = area.height() * Constants::WORKSPACE_WIDTH_PERCENT;
        auto w = area.width() * Constants::WORKSPACE_WIDTH_PERCENT;
        if (h + 80 >= height()) {
            h = height() - 40;
            w = h * area.width() / area.height();

            qCDebug(BLUR_CAT) << "scale down thumb size";
        }

        m_wsThumbSize = QSize(w, h);
    }
    return m_wsThumbSize;
}

QRect DesktopThumbnailManager::calculateDesktopThumbRect(int index)
{
    QRect r;
    r.setSize(calculateThumbDesktopSize());

    auto area = m_handler->clientArea(ScreenArea, 0, 0);
    float space = area.width() * Constants::SPACING_PERCENT;

    auto offset_y = (height() - r.height())/2;

    auto count = m_handler->numberOfDesktops();
    float offset_x = (area.width() - (r.width() + space) * count + space) / 2.0;
    r.moveTo((r.width() + space) * index + offset_x, offset_y);
    return r;
}

void DesktopThumbnailManager::updateWindowsFor(int desktop, QList<WId> ids)
{
    m_windowsHash[desktop] = ids;
    auto root = m_view->rootObject();
    auto thumbs = root->findChildren<DesktopThumbnail*>();
    for (auto th: thumbs) {
        if (th->property("desktop").toInt() == desktop) {
            th->setWindows(ids);
            break;
        }
    }
}

void DesktopThumbnailManager::updateWindowThumbsGeometry(int desktop, const WindowMotionManager& wmm)
{
    auto root = m_view->rootObject();
    auto thumbs = root->findChildren<DesktopThumbnail*>();
    for (auto th: thumbs) {
        if (th->property("desktop").toInt() == desktop) {
            QHash<WId, QRect> data;

            float scale = Constants::WORKSPACE_WIDTH_PERCENT;

            for (auto wid: m_windowsHash[desktop]) {
                auto ew = m_handler->findWindow(wid);
                if (ew && wmm.isManaging(ew)) {
                    auto transformed = wmm.targetGeometry(ew);
                    QRect geom(transformed.x() * scale, transformed.y() * scale,
                            transformed.width() * scale ,transformed.height() * scale);
                    data.insert(wid, geom);
                    qCDebug(BLUR_CAT) << "     ==== " << wid << transformed << geom;
                } else {
                    qCDebug(BLUR_CAT) << "[W]  ==== lost " << ew << wid;
                }
            }

            th->setupLayout(data);
            break;
        }
    }
}

void DesktopThumbnailManager::enterEvent(QEvent* e)
{
}

void DesktopThumbnailManager::leaveEvent(QEvent* e)
{
    auto pos = QCursor::pos();
    auto widget_pos = m_view->mapFromGlobal(pos);
    QMouseEvent child_ev(QEvent::MouseButtonRelease, widget_pos, QCursor::pos(), Qt::LeftButton,
            Qt::LeftButton, Qt::NoModifier);
    qApp->sendEvent(m_view, &child_ev);

    m_view->update();
    emit mouseLeaved();
}

void DesktopThumbnailManager::mouseMoveEvent(QMouseEvent* e)
{
}




MultitaskingEffect::MultitaskingEffect()
    : Effect(),
    m_activated(false),
    m_showAction(new QAction(this)),
	m_multitaskingModel(new MultitaskingModel)
{
    QAction* a = m_showAction;
    a->setObjectName(QStringLiteral(ACTION_NAME));
    a->setText(i18n("Show Multitasking View"));

    QKeySequence ks(Qt::META + Qt::Key_S);
    KGlobalAccel::self()->setDefaultShortcut(a, QList<QKeySequence>() << ks);
    KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>() << ks);
    shortcut = KGlobalAccel::self()->shortcut(a);
    effects->registerGlobalShortcut(ks, a);

    m_gtkFrameExtentsAtom = effects->announceSupportProperty(s_GtkFrameAtomName, this);
    m_targetDesktop = effects->currentDesktop();

    connect(a, SIGNAL(triggered(bool)), this, SLOT(toggleActive()));

    connect(KGlobalAccel::self(), &KGlobalAccel::globalShortcutChanged, this, &MultitaskingEffect::globalShortcutChanged);
    connect(effects, &EffectsHandler::windowAdded, this, &MultitaskingEffect::onWindowAdded);
    connect(effects, &EffectsHandler::windowDeleted, this, &MultitaskingEffect::onWindowDeleted);
    connect(effects, &EffectsHandler::windowClosed, this, &MultitaskingEffect::onWindowClosed);

    connect(effects, &EffectsHandler::numberDesktopsChanged, this, &MultitaskingEffect::onNumberDesktopsChanged);
    connect(effects, SIGNAL(desktopChanged(int, int, KWin::EffectWindow*)), this, SLOT(onCurrentDesktopChanged()));
    //connect(effects, SIGNAL(windowGeometryShapeChanged(KWin::EffectWindow*,QRect)),
    connect(effects, &EffectsHandler::numberScreensChanged, this, &MultitaskingEffect::onNumberScreensChanged);
    connect(effects, &EffectsHandler::virtualScreenGeometryChanged, this, &MultitaskingEffect::onScreenSizeChanged);
    connect(effects, &EffectsHandler::propertyNotify, this, &MultitaskingEffect::onPropertyNotify);
    connect( m_multitaskingModel,SIGNAL(countChanged(int)),this,SLOT( onNumberDesktopsChanged(int) ) );
    connect( m_multitaskingModel,SIGNAL( windowSelectedSignal(QVariant) ),this,SLOT( windowSelectSlot(QVariant) ) );

    BackgroundManager::instance().updateDesktopCount(effects->numberOfDesktops());

    // Load all other configuration details
    reconfigure(ReconfigureAll);
}

MultitaskingEffect::~MultitaskingEffect()
{
    m_thumbManager->deleteLater();
}

QVector<int> MultitaskingEffect::desktopList(const EffectWindow *w) const
{
    if (w->isOnAllDesktops()) {
        static QVector<int> allDesktops;
        if (allDesktops.count() != effects->numberOfDesktops()) {
            allDesktops.resize(effects->numberOfDesktops());
            for (int i = 0; i < effects->numberOfDesktops(); ++i)
                allDesktops[i] = i+1;
        }
        return allDesktops;
    }

    QVector<int> desks;
    desks.resize(w->desktops().count());
    int i = 0;
    for (const int desk : w->desktops()) {
        desks[i++] = desk;
    }
    return desks;
}

void MultitaskingEffect::initWindowData(DataHash::iterator wd, EffectWindow* w)
{
    qCDebug(BLUR_CAT) << "--------- init window " << w->windowClass() << w;
    wd->isAbove = w->keepAbove();
    wd->icon = createIconFor(w);
    wd->csd = !w->hasDecoration();
    updateGtkFrameExtents(w);
    auto createIcon = [](const char* icon_path, int size) {
        auto icon = effects->effectFrame(EffectFrameUnstyled, false);
        icon->setAlignment(Qt::AlignCenter);
        icon->setIcon(QIcon(icon_path));
        icon->setIconSize(QSize(size, size));
        return icon;
    };
    wd->close = createIcon(":/icons/data/close_normal.svg", Constants::ACTION_SIZE);
    wd->pin = createIcon(":/icons/data/unsticked_normal.svg", Constants::ACTION_SIZE);
    wd->unpin = createIcon(":/icons/data/sticked_normal.svg", Constants::ACTION_SIZE);
}

EffectFrame* MultitaskingEffect::createIconFor(EffectWindow* w)
{
    auto icon = effects->effectFrame(EffectFrameUnstyled, false);
    icon->setAlignment(Qt::AlignCenter);
    icon->setIcon(w->icon());
    icon->setIconSize(QSize(Constants::ICON_SIZE, Constants::ICON_SIZE));

    return icon;
}

void MultitaskingEffect::onPropertyNotify(KWin::EffectWindow *w, long atom)
{
    if (w && atom == m_gtkFrameExtentsAtom) {
        updateGtkFrameExtents(w);
    }
}


void MultitaskingEffect::updateGtkFrameExtents(EffectWindow *w)
{
    if (!m_activated) return;

    auto value = w->readProperty(m_gtkFrameExtentsAtom, XCB_ATOM_CARDINAL, 32);
    if (value.size() > 0 && !(value.size() % (4 * sizeof(uint32_t)))) {
        const uint32_t *cardinals = reinterpret_cast<const uint32_t*>(value.constData());
        for (unsigned int i = 0; i < value.size() / sizeof(uint32_t);) {
            int left = cardinals[i++];
            int right = cardinals[i++];
            int top = cardinals[i++];
            int bottom = cardinals[i++];

            auto wd = m_windowDatas.find(w);
            if (wd == m_windowDatas.end()) {
                wd = m_windowDatas.insert(w, WindowData());
                initWindowData(wd, w);
            }
            wd->csd = true;
            wd->gtkFrameExtents = QMargins(left, top, right, bottom);
        }
    }
}

void MultitaskingEffect::onWindowAdded(KWin::EffectWindow* w)
{
    if (!m_activated)
        return;

    if (!isRelevantWithPresentWindows(w))
        return; // don't add


    qCDebug(BLUR_CAT) << __func__;
    auto wd = m_windowDatas.insert(w, WindowData());
    initWindowData(wd, w);

    foreach (const int i, desktopList(w)) {
        WindowMotionManager& wmm = m_motionManagers[i-1];
        wmm.manage(w);
        calculateWindowTransformations(wmm.managedWindows(), wmm);
        updateDesktopWindows(i);
    }

    effects->addRepaintFull();
}

void MultitaskingEffect::onWindowClosed(KWin::EffectWindow* w)
{
    if (!m_multitaskingViewVisible)
        return;

    refreshWindows();
    m_multitaskingModel->setCurrentSelectIndex(-1);
    if(m_multitaskingModel->isCurrentScreensEmpty()) 
    {
        m_multitaskingModel->setCurrentSelectIndex(-1);
    }

    emit modeChanged();

    // if (!m_activated && m_toggleTimeline.currentValue() == 0)
    //     return;

    // qCDebug(BLUR_CAT) << __func__;

    // if (w == m_movingWindow) {
    //     m_movingWindow = nullptr;
    //     m_isWindowMoving = false;
    //     effects->defineCursor(Qt::PointingHandCursor);
    // }

    // if (w == m_highlightWindow) {
    //     updateHighlightWindow(nullptr);
    // }

    // if (w == m_closingdWindow) {
    //     m_closingdWindow = nullptr;
    // }

    // foreach (const int i, desktopList(w)) {
    //     WindowMotionManager& wmm = m_motionManagers[i-1];
    //     wmm.unmanage(w);
    //     calculateWindowTransformations(wmm.managedWindows(), wmm);
    //     updateDesktopWindows(i);
    // }
    // effects->addRepaintFull();
}

void MultitaskingEffect::onWindowDeleted(KWin::EffectWindow* w)
{
    qCDebug(BLUR_CAT) << __func__;
    if (m_thumbManager && w == m_thumbManager->effectWindow()) {
        m_thumbManager->setEffectWindow(nullptr);
    }

    auto wd = m_windowDatas.find(w);
    if (wd != m_windowDatas.end()) {
        delete wd->icon;
        delete wd->close;
        delete wd->pin;
        delete wd->unpin;
        m_windowDatas.erase(wd);
    }

    auto p = m_motionManagers.begin();
    while (p != m_motionManagers.end()) {
        p->unmanage(w);
        ++p;
    }

    if (m_activated) {
        foreach (const int i, desktopList(w)) {
            updateDesktopWindows(i);
        }
    }

    if (w == m_movingWindow) {
        m_movingWindow = nullptr;
        m_isWindowMoving = false;
        effects->defineCursor(Qt::PointingHandCursor);
    }

    if (w == m_highlightWindow) {
        m_selectedWindow = nullptr;
        updateHighlightWindow(nullptr);
        selectNextWindow();
    }

    if (w == m_closingdWindow) {
        m_closingdWindow = nullptr;
    }
}

void MultitaskingEffect::onNumberScreensChanged()
{
    qCDebug(BLUR_CAT) << "------- screens changed " << effects->numScreens();
}

void MultitaskingEffect::updateDesktopWindows(int desktop)
{
    if (desktop <= 0 || desktop > effects->numberOfDesktops())
        return;

    //m_thumbManager->updateWindowsFor(desktop, windowsFor(desktop));
    m_thumbManager->updateWindowThumbsGeometry(desktop, m_motionManagers[desktop-1]);
}

void MultitaskingEffect::updateDesktopWindows()
{
    for (int i = 1; i <= effects->numberOfDesktops(); i++) {
        updateDesktopWindows(i);
    }
}

QVariantList MultitaskingEffect::windowsFor(int screen, int desktop)
{
    QVariantList vl;
    QDesktopWidget dw;
    for (const auto& ew: effects->stackingOrder()) {
        if (isRelevantWithPresentWindows(ew) && ew->desktop() == desktop) {
            if (effects->screenNumber(ew->pos()) == screen) {
                auto wid = findWId(ew);
                assert (effects->findWindow(wid) == ew);
                vl.append(wid);
            }
        }
    }
    return vl;
}

void MultitaskingEffect::onScreenSizeChanged()
{
    if( m_multitaskingViewVisible )
    {
        toggleActive();
    }
}

void MultitaskingEffect::onNumberDesktopsChanged(int old)
{
    qCDebug(BLUR_CAT) << "-------- " << __func__;
    BackgroundManager::instance().updateDesktopCount(effects->numberOfDesktops());

    if (old < effects->numberOfDesktops()) {
        // add new
        for (int i = old+1; i <= effects->numberOfDesktops(); i++) {
            WindowMotionManager wmm;
            for (const auto& w: effects->stackingOrder()) {
                if (w->isOnDesktop(i) && isRelevantWithPresentWindows(w)) {
                    wmm.manage(w);
                }
            }

            calculateWindowTransformations(wmm.managedWindows(), wmm);
            m_motionManagers.append(wmm);
        }

    } else {
        while (m_motionManagers.size() > effects->numberOfDesktops()) {
            m_motionManagers.last().unmanageAll();
            m_motionManagers.removeLast();
        }
    }

    if (m_thumbManager) m_thumbManager->onDesktopsChanged();
    effects->addRepaintFull();
}

void MultitaskingEffect::onCurrentDesktopChanged()
{
    qCDebug(BLUR_CAT) << "------------- " << __func__;
    if (m_targetDesktop != effects->currentDesktop()) {
        m_targetDesktop = effects->currentDesktop();
        effects->addRepaintFull();
    }
}

void MultitaskingEffect::reconfigure(ReconfigureFlags)
{
    qCDebug(BLUR_CAT) << "-------------- " << __func__;
    m_toggleTimeline.setDuration(Constants::WORKSPACE_FADE_DURATION);
    m_toggleTimeline.setEasingCurve(Constants::TOGGLE_MODE);
}

// Screen painting
void MultitaskingEffect::prePaintScreen(ScreenPrePaintData &data, int time)
{
    if (m_thumbManager) {
        if (!m_thumbManager->effectWindow()) {
            auto* ew = effects->findWindow(m_thumbManager->winId());
            if (ew) {
                // without this, it crashes when draw window
                ew->setData(WindowForceBlurRole, QVariant(true));
                m_thumbManager->setEffectWindow(ew);

                m_thumbMotion.manage(ew);
                QRect target = ew->geometry();
                target.moveTopLeft({0, 0});
                m_thumbMotion.moveWindow(ew, target);
            }
        }
    }

    if (isActive()) {
        int new_time = m_toggleTimeline.currentTime() + (m_activated ? time: -time);
        m_toggleTimeline.setCurrentTime(new_time);

        //qCDebug(BLUR_CAT) << "-------------- " << __func__ << time << m_toggleTimeline.currentValue();

        //The window that displays all screens during the multitasking preview
        //data.mask |= PAINT_SCREEN_TRANSFORMED;
        data.mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS;

        for (auto& mm: m_motionManagers) {
            mm.calculate(time/2.0);
        }

        if (m_thumbManager->effectWindow()) {
            m_thumbMotion.calculate(time);
        }

        if (m_activated && m_toggleTimeline.currentValue()) {
            if (m_thumbManager->y() < 0) {
                //NOTE: we have to move real window here to receive events
                m_thumbManager->move(0, 0);
            }
        } else if (!m_activated && m_toggleTimeline.currentValue()) {
            if (m_thumbManager->y() == 0) {
                //NOTE: we have to move real window here to receive events
                m_thumbManager->move(0, -m_thumbManager->height());
            }
        }

        if (!m_activated && m_toggleTimeline.currentValue() == 0) {
            cleanup();
        }
    }

    // this makes blurred windows work, should we need it ?
    for (auto const& w: effects->stackingOrder()) {
        w->setData(WindowForceBlurRole, QVariant(true));
    }

    effects->prePaintScreen(data, time);
}

void MultitaskingEffect::paintScreen(int mask, QRegion region, ScreenPaintData &data)
{
    effects->paintScreen(mask, region, data);

    if (isActive() && m_movingWindow && m_isWindowMoving) {
        // the moving window has to be painted on top of all desktops
        QPoint diff = cursorPos() - m_movingWindowStartPoint;
        QRect geo = m_movingWindowGeometry.translated(diff);
        WindowPaintData d(m_movingWindow, data.projectionMatrix());
        d *= QVector2D((qreal)geo.width() / (qreal)m_movingWindow->width(), (qreal)geo.height() / (qreal)m_movingWindow->height());
        d += QPoint(geo.left() - m_movingWindow->x(), geo.top() - m_movingWindow->y());
        //paint with PAINT_WINDOW_LANCZOS seems cause problem when holding the moving window
        //for a few seconds
        effects->drawWindow(m_movingWindow, PAINT_WINDOW_TRANSFORMED, infiniteRegion(), d);
    }

}

void MultitaskingEffect::postPaintScreen()
{
    if ((m_activated && m_toggleTimeline.currentValue() != 1)
            || (!m_activated && m_toggleTimeline.currentValue() != 0))
        effects->addRepaintFull();

    for (auto const& w: effects->stackingOrder()) {
        w->setData(WindowForceBlurRole, QVariant());
    }
    effects->postPaintScreen();
}


// Window painting
void MultitaskingEffect::prePaintWindow(EffectWindow *w, WindowPrePaintData &data, int time)
{
    data.mask |= PAINT_WINDOW_TRANSFORMED;

    if (m_activated) {
        w->enablePainting(EffectWindow::PAINT_DISABLED_BY_MINIMIZE);   // Display always
    }
    w->enablePainting(EffectWindow::PAINT_DISABLED);
    if (!(w->isDock() || w->isDesktop() || isRelevantWithPresentWindows(w))
            && (w != m_thumbManager->effectWindow())) {
        w->disablePainting(EffectWindow::PAINT_DISABLED);
        w->disablePainting(EffectWindow::PAINT_DISABLED_BY_MINIMIZE);
    }

    effects->prePaintWindow(w, data, time);
}

void MultitaskingEffect::paintWindow(EffectWindow *w, int mask, QRegion region, WindowPaintData &data)
{
    if (m_isWindowMoving && m_movingWindow == w)
        return;

    if (!isActive()) {
        effects->paintWindow(w, mask, region, data);
        return;
    }

    if (w == m_thumbManager->effectWindow()) {
        mask &= ~PAINT_WINDOW_LANCZOS;
        auto ew = m_thumbManager->effectWindow();
        WindowPaintData d = data;

        float scale = m_toggleTimeline.currentValue();
        auto geo = m_thumbMotion.transformedGeometry(ew);
        auto new_pos = QPointF(
                interpolate( 0, 0, scale),
                interpolate( -m_thumbManager->height(), 0, scale));
        d += QPoint(qRound(new_pos.x() - ew->x()), qRound(new_pos.y() - ew->y()));
        effects->paintWindow(ew, mask, infiniteRegion(), d);
        return;
    }

    int desktop = effects->currentDesktop();
    WindowMotionManager& wmm = m_motionManagers[desktop-1];
    if (wmm.isManaging(w) || w->isDesktop()) {
        auto area = effects->clientArea(ScreenArea, 0, 0);

        WindowPaintData d = data;
        if (w->isDesktop()) {
            d.setBrightness(0.4);
            effects->paintWindow(w, mask, area, d);

        } else if (!w->isDesktop()) {
            //NOTE: add lanczos will make partial visible window be rendered completely,
            //but slow down the animation
            //mask |= PAINT_WINDOW_LANCZOS;
            auto geo = m_motionManagers[desktop-1].transformedGeometry(w);

            if (m_selectedWindow == w) {
                geo = highlightedGeometry(geo);
            }

            d += QPoint(qRound(geo.x() - w->x()), qRound(geo.y() - w->y()));
            d.setScale(QVector2D((float)geo.width() / w->width(), (float)geo.height() / w->height()));


            auto wd = m_windowDatas.constFind(w);
            if (m_selectedWindow == w) {
                if (!m_highlightFrame) {
                    m_highlightFrame = effects->effectFrame(EffectFrameStyled, false);
                }
                QRect geo_frame = geo.toRect();
                if (wd != m_windowDatas.constEnd()) {
                    auto ext = wd->gtkFrameExtents;
                    QMargins scaled_ext(ext.left() * d.xScale(), ext.top() * d.yScale(),
                            ext.right() * d.xScale(), ext.bottom() * d.yScale());
                    geo_frame = geo_frame.marginsRemoved(scaled_ext);
                }
                geo_frame.adjust(-1, -1, 1, 1);
                m_highlightFrame->setGeometry(geo_frame);
                m_highlightFrame->render(infiniteRegion(), 1.0, 0.8);
            }

            //qCDebug(BLUR_CAT) << "--------- window " << w->geometry() << geo;
            effects->paintWindow(w, mask, area, d);
            if (wd != m_windowDatas.constEnd()) {
                auto ext = wd->gtkFrameExtents;
                QPoint wp(geo.center().rx(), geo.bottom());
                if (wd->csd && !ext.isNull()) {
                    wp.ry() -= ext.bottom() * d.yScale();
                }
                if (wd->icon) {
                    wd->icon->setPosition(wp);
                    wd->icon->render(region, 1.0, 0.0);
                }
                    if (!wd->icon) {
                        qCDebug(BLUR_CAT) << "---------- no icon ! " << w << w->windowClass();
                    }

                if (m_highlightWindow == w) {
                    wp = geo.topRight().toPoint();
                    if (wd->csd && !ext.isNull()) {
                        wp.rx() -= ext.right() * d.xScale();
                        wp.ry() += ext.top() * d.yScale();
                    }

                    if (wd->close) {
                        wd->close->setPosition(wp);
                        wd->close->render(region, 1.0, 0.0);
                    }

                    wp = geo.topLeft().toPoint();
                    if (wd->csd && !ext.isNull()) {
                        wp.rx() += ext.left() * d.xScale();
                        wp.ry() += ext.top() * d.yScale();
                    }
                    if (wd->isAbove && wd->unpin) {
                        wd->unpin->setPosition(wp);
                        wd->unpin->render(region, 1.0, 0.0);
                    } else if (wd->pin) {
                        wd->pin->setPosition(wp);
                        wd->pin->render(region, 1.0, 0.0);
                    }
                }
            }
        }
    } else {
        effects->paintWindow(w, mask, region, data);
    }
}

bool MultitaskingEffect::isRelevantWithPresentWindows(EffectWindow *w) const
{
    if (w->isSpecialWindow() || w->isUtility()) {
        return false;
    }

    if (w->isDock()) {
        return false;
    }

    if (w->isSkipSwitcher()) {
        return false;
    }

    if (w->isDeleted()) {
        return false;
    }

    if (!w->acceptsFocus()) {
        return false;
    }

    if (!w->isCurrentTab()) {
        return false;
    }

    if (!w->isOnCurrentActivity()) {
        return false;
    }

    return true;
}

// User interaction
void MultitaskingEffect::windowInputMouseEvent(QEvent *e)
{
    static bool s_insideThumbManager = false;

    // switch (e->type()) {
    //     case QEvent::MouseMove:
    //     case QEvent::MouseButtonPress:
    //     case QEvent::MouseButtonRelease:
    //         if (m_toggleTimeline.currentValue() != 1) {
    //             return;
    //         }
    //         break;

    //     default:
    //         return;
    // }

     auto me = static_cast<QMouseEvent*>(e);

    // if (m_thumbManager && !m_isWindowMoving) {
    //     if (m_thumbManager->geometry().contains(me->pos())) {
    //         if (!s_insideThumbManager) {
    //             s_insideThumbManager = true;
    //             QEnterEvent ee(me->localPos(), me->windowPos(), me->screenPos());
    //             qApp->sendEvent(m_thumbManager, &ee);
    //         }

    //         auto pos = m_thumbManager->mapFromGlobal(me->pos());
    //         QMouseEvent new_event(me->type(), pos, me->pos(), me->button(), me->buttons(), me->modifiers());
    //         m_thumbManager->windowInputMouseEvent(&new_event);
    //         return;
    //     } else if (s_insideThumbManager) {
    //         s_insideThumbManager = false;

    //         QEvent le(QEvent::Leave);
    //         qApp->sendEvent(m_thumbManager, &le);
    //     }
    // }

    // if (m_thumbManager) {
    //     m_thumbManager->update();
    // }
    qApp->sendEvent(m_multitaskingView, e);
    updateWindowStates(me);
}

QRectF MultitaskingEffect::highlightedGeometry(QRectF geometry)
{
    auto center = geometry.center();
    geometry.setWidth(geometry.width() * Constants::HIGHLIGHT_SCALE);
    geometry.setHeight(geometry.height() * Constants::HIGHLIGHT_SCALE);
    geometry.moveCenter(center);

    return geometry;
}

void MultitaskingEffect::updateWindowStates(QMouseEvent* me)
{
     static bool is_smooth_scrolling = false;

    // if (!m_activated) return;

    // EffectWindow* target = nullptr;
    // WindowMotionManager& mm = m_motionManagers[m_targetDesktop-1];

    // bool highlight_changed = true;
    // if (m_highlightWindow) {
    //     auto geom = highlightedGeometry(mm.transformedGeometry(m_highlightWindow));
    //     if (geom.contains(me->pos())) {
    //         highlight_changed = false;
    //         target = m_highlightWindow;
    //     }
    // }

    // if (highlight_changed) {
    //     auto windows = mm.managedWindows();
    //     for (auto win: windows) {
    //         // add margins to receive events for `close` and `pin` buttons
    //         auto geom = highlightedGeometry(mm.transformedGeometry(win));
    //         if (m_closingdWindow != win && geom.contains(me->pos())) {
    //             target = win;
    //             break;
    //         }
    //     }
    // }

    // if (m_isWindowMoving)
    //     updateHighlightWindow(nullptr);
    // else
    //     updateHighlightWindow(target);

   if (me->button() == Qt::ForwardButton || me->button() == Qt::BackButton) {
        if (me->type() != QEvent::MouseButtonPress || is_smooth_scrolling) return;

        if (me->buttons() == Qt::ForwardButton) {
            is_smooth_scrolling = true;
            if (m_multitaskingModel->currentIndex()+1 < 4) {
                m_multitaskingModel->setCurrentIndex(m_multitaskingModel->currentIndex()+1);
            } else {
                m_multitaskingModel->setCurrentIndex(0);
            }
        } else if (me->buttons() == Qt::BackButton) {
            is_smooth_scrolling = true;
             if (m_multitaskingModel->currentIndex()-1 >= 0) {
                 m_multitaskingModel->setCurrentIndex(m_multitaskingModel->currentIndex()-1);
             } else {
                 m_multitaskingModel->setCurrentIndex(3);
            }
        }

        QTimer::singleShot(400, [&]() { is_smooth_scrolling = false; });
        return;
    }

    // switch (me->type()) {
    //     case QEvent::Wheel:
    //         break;

    //     case QEvent::MouseMove:
    //         if (m_movingWindow) {
    //             if (!m_isWindowMoving) {
    //                 if (me->buttons() == Qt::LeftButton &&
    //                         (me->pos() - m_dragStartPos).manhattanLength() > QApplication::startDragDistance()) {

    //                     auto ids = desktopList(m_movingWindow);
    //                     //Do not handle sticky windows right now
    //                     //It does not make sense to drag them to other desktops
    //                     if (ids.size() > 1) return;

    //                     qCDebug(BLUR_CAT) << "----- start drag";

    //                     WindowMotionManager& wmm = m_motionManagers[ids[0]-1];
    //                     const QRectF transformedGeo = wmm.transformedGeometry(m_movingWindow);
    //                     const QPointF pos = transformedGeo.topLeft();

    //                     float scale = 150.0 / transformedGeo.width();
    //                     const QSize size(150, scale * transformedGeo.height());
    //                     m_movingWindowGeometry = QRect(pos.toPoint(), size);
    //                     m_movingWindowGeometry.moveCenter(me->pos());
    //                     m_movingWindowStartPoint = me->pos();

    //                     wmm.unmanage(m_movingWindow);
    //                     if (EffectWindow* modal = m_movingWindow->findModal()) {
    //                         if (wmm.isManaging(modal))
    //                             wmm.unmanage(modal);
    //                     }

    //                     calculateWindowTransformations(wmm.managedWindows(), wmm);

    //                     updateHighlightWindow(nullptr);
    //                     m_isWindowMoving = true;
    //                     effects->defineCursor(Qt::ClosedHandCursor);
    //                 }
    //             }

    //             effects->addRepaintFull();
    //         }

    //         break;

    //     case QEvent::MouseButtonPress:

    //         if (target) {
    //             effects->setElevatedWindow(target, true);
    //             m_movingWindow = target;
    //             m_dragStartPos = me->pos();
    //             effects->addRepaintFull();
    //         }
    //         break;

    //     case QEvent::MouseButtonRelease:
    //         if (m_movingWindow && m_isWindowMoving) {
    //             auto switch_to = m_thumbManager->desktopAtPos(me->pos());
    //             //qCDebug(BLUR_CAT) << "---------- switch_to " << switch_to;
    //             effects->defineCursor(Qt::PointingHandCursor);
    //             effects->setElevatedWindow(m_movingWindow, false);

    //             if (switch_to <= 0) {
    //                 auto last_rect = m_thumbManager->calculateDesktopThumbRect(effects->numberOfDesktops()-1);
    //                 auto pos = m_thumbManager->mapToGlobal(last_rect.topRight());
    //                 if (me->pos().x() > pos.x() && effects->numberOfDesktops() < 4) {
    //                     appendDesktop();

    //                     auto target_window = m_movingWindow;
    //                     // wait for thumbmanager handles layout change (add new ws thumb)
    //                     QTimer::singleShot(100, [=]() {
    //                         moveEffectWindow2Desktop(target_window, effects->numberOfDesktops());
    //                     });

    //                 } else {
    //                     // restore
    //                     auto ids = desktopList(m_movingWindow);
    //                     if (ids.size() > 0) {
    //                         qCDebug(BLUR_CAT) << "---------- no new desktop, restore window to " << ids[0];
    //                         auto& wmm = m_motionManagers[ids[0]-1];
    //                         wmm.manage(m_movingWindow);
    //                         if (EffectWindow* modal = m_movingWindow->findModal()) {
    //                             wmm.manage(modal);
    //                         }

    //                         calculateWindowTransformations(wmm.managedWindows(), wmm);
    //                         effects->addRepaintFull();
    //                     }
    //                 }
    //             } else {
    //                 moveEffectWindow2Desktop(m_movingWindow, switch_to);
    //             }

    //             m_movingWindow = nullptr;
    //             m_isWindowMoving = false;

    //         } else if (target) { // this must be m_highlightWindow now
    //             if (m_highlightWindow != target) break;

    //             auto wd = m_windowDatas.find(target);
    //             if (wd != m_windowDatas.end()) {
    //                 if (wd->close->geometry().contains(me->pos())) {
    //                     QMetaObject::invokeMethod(this, "closeWindow", Qt::QueuedConnection);
    //                 } else if (wd->isAbove && wd->unpin->geometry().contains(me->pos())) {
    //                     QMetaObject::invokeMethod(this, "toggleWindowKeepAbove", Qt::QueuedConnection);
    //                 } else if (!wd->isAbove && wd->pin->geometry().contains(me->pos())) {
    //                     QMetaObject::invokeMethod(this, "toggleWindowKeepAbove", Qt::QueuedConnection);
    //                 } else {
    //                     updateHighlightWindow(nullptr);
    //                     effects->activateWindow(target);
    //                     setActive(false);
    //                 }
    //             }
    //         } else {
    //             setActive(false);
    //         }
    //         break;

    //     default: break;
    // }

}

void MultitaskingEffect::toggleWindowKeepAbove()
{
    if (m_highlightWindow) {
        auto& wd = m_windowDatas[m_highlightWindow];

        //HACK: find wid of the highlighted window
        WId highlight_wid = 0;
        for (auto wid: KWindowSystem::self()->windows()) {
            if (effects->findWindow(wid) == m_highlightWindow) {
                highlight_wid = wid;
                break;
            }
        }

        if (highlight_wid == 0) return;

        if (m_highlightWindow->keepAbove()) {
            qCDebug(BLUR_CAT) << "--------- click unpin";
            KWindowSystem::self()->clearState(highlight_wid, NET::KeepAbove);
            wd.isAbove = false;
        } else {
            qCDebug(BLUR_CAT) << "--------- click pin";
            KWindowSystem::self()->setState(highlight_wid, NET::KeepAbove);
            wd.isAbove = true;
        }

        effects->addRepaintFull();
    }
}

void MultitaskingEffect::closeWindow()
{
    if (m_highlightWindow) {
        qCDebug(BLUR_CAT) << "--------- click close";
        m_highlightWindow->closeWindow();
        m_closingdWindow = m_highlightWindow;

        if (m_selectedWindow == m_highlightWindow) {
            m_selectedWindow = nullptr;
        }
        updateHighlightWindow(nullptr);
    }
}

void MultitaskingEffect::updateHighlightWindow(EffectWindow* w)
{
    if (w == m_highlightWindow) return;

    m_highlightWindow = w;

    if (m_highlightWindow) {
        qCDebug(BLUR_CAT) << __func__ << w->geometry() << m_windowDatas[w].csd
            << m_windowDatas[w].gtkFrameExtents;
        selectWindow(m_highlightWindow);
    }

    effects->addRepaintFull();
}

void MultitaskingEffect::grabbedKeyboardEvent(QKeyEvent *e)
{
    if (e->type() == QEvent::KeyPress) {
        // check for global shortcuts
        // HACK: keyboard grab disables the global shortcuts so we have to check for global shortcut (bug 156155)
        if (shortcut.contains(e->key() + e->modifiers())) {
            toggleActive();
            return;
        }
//        m_multitaskingModel->setCurrentIndex(0);
        m_pEffectWindow = effects->activeWindow();
        //if (!isActive()) return;

        qCDebug(BLUR_CAT) << e;
        if (e->isAutoRepeat()) return;
        switch (e->key()) {
            case Qt::Key_Escape:
//                if (isActive()) toggleActive();
                setActive(false);
                break;

            case Qt::Key_Enter:
            case Qt::Key_Return:
                if (m_selectedWindow) {
                    updateHighlightWindow(nullptr);
                    effects->activateWindow(m_selectedWindow);
                    setActive(false);
                }
                break;

            case Qt::Key_Right:  // include super+->
                if (e->modifiers() == Qt::MetaModifier) {
                    if (m_multitaskingModel->currentIndex()+1 < m_multitaskingModel->rowCount()) {
                        m_multitaskingModel->setCurrentIndex(m_multitaskingModel->currentIndex()+1);
                    } else {
                        m_multitaskingModel->setCurrentIndex(0);
                    }
                } else if (e->modifiers() == Qt::NoModifier) {
                    selectNextWindow();
                }
                break;

            case Qt::Key_Left:
                if (e->modifiers() == Qt::MetaModifier) {
                     if (m_multitaskingModel->currentIndex()-1 >= 0) {
                         m_multitaskingModel->setCurrentIndex(m_multitaskingModel->currentIndex()-1);
                    } else {
                         m_multitaskingModel->setCurrentIndex(3);
                    }
                } else if (e->modifiers() == Qt::NoModifier) {
                    selectPrevWindow();
                }
                break;

            case Qt::Key_Down:
                if (e->modifiers() == Qt::NoModifier) {
                    selectNextWindowVert(1);
                }
                break;
            case Qt::Key_Up:
                if (e->modifiers() == Qt::NoModifier) {
                    selectNextWindowVert(-1);
                }
                break;
            case Qt::Key_Home:
                if (e->modifiers() == Qt::NoModifier) {
                    selectFirstWindow();
                }
                break;
            case Qt::Key_End:
                if (e->modifiers() == Qt::NoModifier) {
                    selectLastWindow();
                }
                break;

            case Qt::Key_1:
            case Qt::Key_2:
            case Qt::Key_3:
            case Qt::Key_4:
                if (e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::MetaModifier) {
                    m_multitaskingModel->setCurrentIndex(e->key() - Qt::Key_1);
                }
                break;

            case Qt::Key_Exclam: // shift+1
            case Qt::Key_At: // shift+2
            case Qt::Key_NumberSign: // shift+3
            case Qt::Key_Dollar: // shift+4
                if (e->modifiers() == (Qt::ShiftModifier | Qt::MetaModifier)) {
                    int target_desktop = 1;
                    switch(e->key()) {
                        case Qt::Key_Exclam:  target_desktop = 1; break;
                        case Qt::Key_At:  target_desktop = 2; break;
                        case Qt::Key_NumberSign:  target_desktop = 3; break;
                        case Qt::Key_Dollar:  target_desktop = 4; break;
                        default: break;
                    }
                    m_multitaskingModel->setCurrentIndex(target_desktop-1);
                    qCDebug(BLUR_CAT) << "----------- super+shift+"<<target_desktop;

                    if( m_multitaskingModel->currentSelectIndex() == 0 )
                    {
                        if( !m_pEffectWindow->isDesktop() )
                        {
                            auto winId = findWId(m_pEffectWindow);
                            m_multitaskingModel->setCurrentSelectIndex( (int)winId );
                        }
                    }
                    QVariant  winId  = m_multitaskingModel->currentSelectIndex();
                    EffectWindow *ew = effects->findWindow(winId.toULongLong());
                    if(ew)
                    {
                        moveWindow2Desktop( ew->screen(),target_desktop ,m_multitaskingModel->currentSelectIndex());
                    }
                }
                break;

            case Qt::Key_Equal:
                if (e->modifiers() == Qt::AltModifier) {
                    m_multitaskingModel->append();
                    m_multitaskingModel->setCurrentIndex(m_multitaskingModel->count() - 1);
                }
                break;

            case Qt::Key_Minus:
                if (e->modifiers() == Qt::AltModifier) {
                    m_multitaskingModel->remove(m_targetDesktop - 1);
                    m_multitaskingModel->setCurrentIndex(m_targetDesktop - 1);
                }
                break;

            case Qt::Key_Delete:
                if (e->modifiers() == Qt::NoModifier && m_selectedWindow) {
                    qCDebug(BLUR_CAT) << "------------[TODO]: close highlighted window";
                }
                break;

            case Qt::Key_Backtab:
                if (e->modifiers() == Qt::ShiftModifier) {
                    selectPrevWindow();
                }
                break;

            case Qt::Key_Tab:
                if (e->modifiers() == Qt::NoModifier) {
                    selectNextWindow();
                }
                break;

            case Qt::Key_QuoteLeft:
                if (e->modifiers() == Qt::AltModifier) {
                    selectNextGroupWindow();
                }
                break;

            case Qt::Key_AsciiTilde:
                if (e->modifiers() == (Qt::AltModifier | Qt::ShiftModifier)) {
                    selectPrevGroupWindow();
                }
                break;

            default: break;
        }
    }
}

void MultitaskingEffect::selectNextGroupWindow()
{
    int current = effects->currentDesktop();
    const auto& wmm = m_motionManagers[current-1];
    if (!m_selectedWindow) {
        selectWindow(wmm.managedWindows().first());
    } else {
        auto clz = m_selectedWindow->windowClass();

        auto wl = wmm.managedWindows();
        auto k = wl.indexOf(m_selectedWindow);
        if (k < 0) return;

        int i = (k+1) % wl.size();
        while (i != k) {
            if (wl[i]->windowClass() == clz) {
                selectWindow(wl[i]);
                break;
            }
            i = (i+1) % wl.size();
        }
    }
}

void MultitaskingEffect::selectPrevGroupWindow()
{
    int current = effects->currentDesktop();
    const auto& wmm = m_motionManagers[current-1];
    if (!m_selectedWindow) {
        selectWindow(wmm.managedWindows().first());
    } else {
        auto clz = m_selectedWindow->windowClass();

        auto wl = wmm.managedWindows();
        auto k = wl.indexOf(m_selectedWindow);
        if (k < 0) return;

        int i = (k-1+wl.size()) % wl.size();
        while (i != k) {
            if (wl[i]->windowClass() == clz) {
                selectWindow(wl[i]);
                break;
            }
            i = (i-1+wl.size()) % wl.size();
        }
    }
}

void MultitaskingEffect::selectPrevWindow()
{
    m_multitaskingModel->selectPrevWindow();
    /*int d = effects->currentDesktop();
    const auto& takenSlots = m_takenSlots[d];

    auto current = m_selectedWindow;
    if (!current || takenSlots.size() == 0) {
        return;
    }

    const auto& wmm = m_motionManagers[d-1];
    if (wmm.managedWindows().size() == 1) return;

    int columns = m_gridSizes[d].columns;
    int rows = m_gridSizes[d].rows;
    if (takenSlots.size() != columns * rows)
        return;

    int slot = 0;
    for (; slot < columns * rows; slot++) {
        if (current == takenSlots[slot]) {
            int col = slot % columns;
            int row = slot / columns;

            int max = columns * rows;
            while (max-- > 0) {
                if (col > 0) {
                    col--;
                } else if (row > 0) {
                    row--;
                    col = columns-1;
                } else {
                    row = rows-1;
                    col = columns-1;
                }

                int nextslot = row * columns + col;
                if (nextslot < 0 || nextslot >= takenSlots.size())
                    return;

                qCDebug(BLUR_CAT) << "---------- next " << row << col << nextslot;
                if (takenSlots[nextslot]) {
                    selectWindow(takenSlots[nextslot]);
                    break;
                }
            }
            break;
        }
    }*/
}

void MultitaskingEffect::selectNextWindow()
{
    m_multitaskingModel->selectNextWindow();
    /*
    int d = effects->currentDesktop();
    const auto& takenSlots = m_takenSlots[d];

    auto current = m_selectedWindow;
    if (!current || takenSlots.size() == 0) {
        return;
    }

    const auto& wmm = m_motionManagers[d-1];
    if (wmm.managedWindows().size() == 1) return;

    int columns = m_gridSizes[d].columns;
    int rows = m_gridSizes[d].rows;
    qCDebug(BLUR_CAT) << "------- " << d << takenSlots.size() << columns*rows;

    if (takenSlots.size() != columns * rows)
        return;


    int slot = 0;
    for (; slot < columns * rows; slot++) {
        if (current == takenSlots[slot]) {
            int col = slot % columns;
            int row = slot / columns;

            int max = columns * rows;
            while (max-- > 0) {
                if (col + 1 < columns) {
                    col++;
                } else if (row + 1 < rows) {
                    row++;
                    col = 0;
                } else {
                    row = 0;
                    col = 0;
                }

                int nextslot = row * columns + col;
                if (nextslot < 0 || nextslot >= takenSlots.size())
                    return;

                qCDebug(BLUR_CAT) << "---------- next " << row << col << nextslot;
                if (takenSlots[nextslot]) {
                    selectWindow(takenSlots[nextslot]);
                    break;
                }
            }
            break;
        }
    }
    */
}

void MultitaskingEffect::selectFirstWindow()
{
    int d = effects->currentDesktop();
    const auto& takenSlots = m_takenSlots[d];

    if (takenSlots.size() == 0) {
        return;
    }

    int columns = m_gridSizes[d].columns;
    int rows = m_gridSizes[d].rows;
    if (takenSlots.size() != columns * rows)
        return;

    int row = 0;
    int col = 0;
    int max = columns * rows;
    while (max-- > 0) {
        int nextslot = row * columns + col;
        if (nextslot < 0 || nextslot >= takenSlots.size())
            return;

        if (takenSlots[nextslot]) {
            selectWindow(takenSlots[nextslot]);
            break;
        }

        if (col + 1 < columns) {
            col++;
        } else if (row + 1 < rows) {
            row++;
            col = 0;
        } else {
            row = 0;
            col = 0;
        }
    }
}

void MultitaskingEffect::selectLastWindow()
{
    int d = effects->currentDesktop();
    const auto& takenSlots = m_takenSlots[d];

    if (takenSlots.size() == 0) {
        return;
    }

    int columns = m_gridSizes[d].columns;
    int rows = m_gridSizes[d].rows;
    if (takenSlots.size() != columns * rows)
        return;

    int row = rows-1;
    int col = columns-1;
    int max = columns * rows;
    while (max-- > 0) {
        int nextslot = row * columns + col;
        if (nextslot < 0 || nextslot >= takenSlots.size())
            return;

        qCDebug(BLUR_CAT) << "---------- next " << row << col << nextslot;
        if (takenSlots[nextslot]) {
            selectWindow(takenSlots[nextslot]);
            break;
        }

        if (col > 0) {
            col--;
        } else if (row > 0) {
            row--;
            col = columns-1;
        } else {
            row = rows-1;
            col = columns-1;
        }
    }
}

void MultitaskingEffect::selectNextWindowVert(int dir)
{
    m_multitaskingModel->selectNextWindowVert(dir);
    /*int d = effects->currentDesktop();
    const auto& takenSlots = m_takenSlots[d];

    auto current = m_selectedWindow;
    if (!current || takenSlots.size() == 0) {
        return;
    }

    int columns = m_gridSizes[d].columns;
    int rows = m_gridSizes[d].rows;
    if (takenSlots.size() != columns * rows)
        return;

    int slot = 0;
    for (; slot < columns * rows; slot++) {
        if (current == takenSlots[slot]) {
            int col = slot % columns;
            int row = slot / columns;

            int nextrow = row + dir;
            int nextslot = nextrow * columns + col;
            if (nextrow < 0 || nextslot < 0 || nextslot >= takenSlots.size())
                return;

            qCDebug(BLUR_CAT) << "---------- next " << nextrow << col << nextslot;
            if (takenSlots[nextslot])
                selectWindow(takenSlots[nextslot]);
            break;
        }
    }
    */
}


void MultitaskingEffect::selectWindow(EffectWindow* w)
{
    if (m_selectedWindow == w) {
        return;
    }
    qCDebug(BLUR_CAT) << "------ select window " << w;

    if (m_selectedWindow) {
        effects->setElevatedWindow(m_selectedWindow, false);
        m_selectedWindow->addRepaintFull();
    }

    m_selectedWindow = w;
    if (w) {
        effects->setElevatedWindow(m_selectedWindow, true);
        m_selectedWindow->addRepaintFull();
    }
}

bool MultitaskingEffect::isActive() const
{
    return (m_activated || m_toggleTimeline.currentValue() != 0) && !effects->isScreenLocked();
}

void MultitaskingEffect::cleanup()
{
    if (m_activated)
        return;

    qCDebug(BLUR_CAT) << "-------- " << __func__;
    m_thumbMotion.unmanage(m_thumbManager->effectWindow());
    m_thumbMotion.reset();
    m_thumbManager->hide();

    auto wd = m_windowDatas.begin();
    while (wd != m_windowDatas.end()) {
        delete wd.value().icon;
        delete wd.value().close;
        delete wd.value().pin;
        delete wd.value().unpin;
        ++wd;
    }
    m_windowDatas.clear();


    if (m_hasKeyboardGrab) effects->ungrabKeyboard();
    m_hasKeyboardGrab = false;
    effects->stopMouseInterception(this);
    effects->setActiveFullScreenEffect(0);

    while (m_motionManagers.size() > 0) {
        m_motionManagers.first().unmanageAll();
        m_motionManagers.removeFirst();
    }
}

void MultitaskingEffect::appendDesktop()
{
    effects->setNumberOfDesktops(effects->numberOfDesktops() + 1);
    QTimer::singleShot(400, [=]() {
        changeCurrentDesktop(effects->numberOfDesktops());
    });
}


void MultitaskingEffect::removeDesktop(int d)
{
    if (d <= 0 || d > effects->numberOfDesktops() || effects->numberOfDesktops() == 1) {
        return;
    }

    for (const auto& ew: effects->stackingOrder()) {
        if (ew->isOnAllDesktops()) {
            continue;
        }
        auto dl = ew->desktops();
        if (dl.size() == 0 || dl[0] < d) {
            continue;
        }
        int newd = dl[0] == 1 ? 1 : dl[0] - 1;
        QVector<uint> desks {(uint)newd};
        qCDebug(BLUR_CAT) << "     ---- move" << ew << "from" << dl[0] << "to" << newd;
        effects->windowToDesktops(ew, desks);
    }

    emit m_thumbManager->desktopRemoved(QVariant(d));
    // shift wallpapers before acutally removing it
    BackgroundManager::instance().desktopAboutToRemoved(d);
    effects->setNumberOfDesktops(effects->numberOfDesktops()-1);
    //effects->addRepaintFull();
    // elay this process, make sure layoutChanged has been handled
    //QTimer::singleShot(10, [=]() { desktopRemoved(d); });
}

void MultitaskingEffect::desktopRemoved(int d)
{
    remanageAll();
    updateDesktopWindows();
//    effects->addRepaintFull();
}

WId MultitaskingEffect::findWId(EffectWindow* ew)
{
    auto cli = ew->parent(); // this is a Toplevel
    return cli->property("windowId").toULongLong();
}

void MultitaskingEffect::switchTwoDesktop(int to, int from)
{
    qCDebug(BLUR_CAT) << "---- swtich" << to << "with" << from;
    EffectWindowList to_wins;
    EffectWindowList from_wins;

    int dir = from < to ? 1 : -1;
    for (const auto& ew: effects->stackingOrder()) {
        if (ew->isOnAllDesktops())
            continue;

        auto dl = ew->desktops();
        if (dl.size() == 0 || (dir > 0 && (dl[0] > to || dl[0] < from)) ||
                (dir < 0 && (dl[0] < to || dl[0] > from)))
            continue;

        int newd = dl[0] == from ? to: dl[0]-dir;
        QVector<uint> desks {(uint)newd};
        qCDebug(BLUR_CAT) << "     ---- move" << ew << "from" << dl[0] << "to" << newd;
        effects->windowToDesktops(ew, desks);
    }

    BackgroundManager::instance().desktopSwitchedPosition(to, from);

    remanageAll();

    effects->addRepaintFull();

}

void MultitaskingEffect::moveWindow2Desktop(int screen, int desktop, QVariant winId) 
{
    auto* ew = effects->findWindow(winId.toULongLong());
    if (!ew) {
        return;
    }
    qDebug() << "--------1719" << ew << "-------" << desktop;
    effects->windowToScreen(ew, screen);
    moveEffectWindow2Desktop(ew, desktop);
}

void MultitaskingEffect::moveEffectWindow2Desktop(EffectWindow* ew, int desktop)
{
    auto prev_desktop = ew->desktops().first();
    if (prev_desktop == desktop) {
        qCDebug(BLUR_CAT) << "------------ the same desktop";
        return;
    }

    if( m_motionManagers.count() < desktop )
    {
        return;
    }

    for (auto id: desktopList(ew)) {
        WindowMotionManager& pre_wmm = m_motionManagers[id-1];
        pre_wmm.unmanage(ew);
        if (EffectWindow* modal = ew->findModal()) {
            pre_wmm.unmanage(modal);
        }
        calculateWindowTransformations(pre_wmm.managedWindows(), pre_wmm);
        qCDebug(BLUR_CAT) << "   ---- unmanage from " << id;
    }
    // update new desktop
    WindowMotionManager& new_wmm = m_motionManagers[desktop-1];
    new_wmm.manage(ew);
    qCDebug(BLUR_CAT) << "   ---- manage to " << desktop;
    if (EffectWindow* modal = ew->findModal()) {
        new_wmm.manage(modal);
    }
    calculateWindowTransformations(new_wmm.managedWindows(), new_wmm);;
    QVector<uint> ids {(uint)desktop};
    effects->windowToDesktops(ew, ids);

    QRect area = effects->clientArea( ScreenArea,ew->screen(),desktop );
    effects->moveWindow(ew,QPoint(area.topLeft().x(),area.topLeft().y()));

    //zhd add 
    refreshWindows();
    emit modeChanged();
    //zhd  add end 

    //wxb add
    m_multitaskingModel->updateWindowDestop(desktop);
    //wxb add end

/*
    updateDesktopWindows(prev_desktop);
    updateDesktopWindows(desktop);
    effects->addRepaintFull();
    */
}

void MultitaskingEffect::changeCurrentDesktop(int d)
{
    if (d <= 0 || d > effects->numberOfDesktops()) {
        return;
    }

    if (m_targetDesktop == d) {
        return;
    }

    updateHighlightWindow(nullptr);
    selectWindow(nullptr);

    m_targetDesktop = d;
    if (effects->currentDesktop() != m_targetDesktop) {
        effects->setCurrentDesktop(d);
        if (m_activated) {
            effects->addRepaintFull();
        }
    }

}

void MultitaskingEffect::remanageAll()
{
    while (m_motionManagers.size() > 0) {
        m_motionManagers.first().unmanageAll();
        m_motionManagers.removeFirst();
    }

    for (int i = 1; i <= effects->numberOfDesktops(); i++) {
        WindowMotionManager wmm;
        for (const auto& w: effects->stackingOrder()) {
            if (w->isOnDesktop(i) && isRelevantWithPresentWindows(w)) {
                wmm.manage(w);
            }
        }

        calculateWindowTransformations(wmm.managedWindows(), wmm);
        m_motionManagers.append(wmm);
    }
}

void MultitaskingEffect::clearGrids()
{
    m_gridSizes.clear();
}

void MultitaskingEffect::setActive(bool active)
{
    if (!m_thumbManager) {
        m_thumbManager = new DesktopThumbnailManager(effects);
        connect(m_thumbManager, &DesktopThumbnailManager::requestAppendDesktop,
                this, &MultitaskingEffect::appendDesktop);
        connect(m_thumbManager, &DesktopThumbnailManager::requestDeleteDesktop,
                this, &MultitaskingEffect::removeDesktop);
        connect(m_thumbManager, &DesktopThumbnailManager::requestChangeCurrentDesktop,
                this, &MultitaskingEffect::changeCurrentDesktop);
        connect(m_thumbManager, &DesktopThumbnailManager::requestMove2Desktop,
                this, &MultitaskingEffect::moveWindow2Desktop);
        connect(m_thumbManager, &DesktopThumbnailManager::requestSwitchDesktop,
                this, &MultitaskingEffect::switchTwoDesktop);
    }

    m_multitaskingViewVisible = active;
    if (m_multitaskingViewVisible) {
        if (m_targetDesktop != effects->currentDesktop()) {
            m_targetDesktop = effects->currentDesktop();
        }
        const int desktopCount = m_thumbManager->desktopCount();
        for (int d = 1; d <= desktopCount; ++d) {
            for (int screen = 0; screen < effects->numScreens(); ++screen) {
                auto windows = windowsFor(screen, d);
                m_multitaskingModel->setWindows(screen, d, windows);
            }
        }

        if (!m_multitaskingView) {
            m_multitaskingView = new QQuickWidget;
            m_multitaskingView->engine()->addImageProvider( QLatin1String("imageProvider"), new ImageProvider(QQmlImageProviderBase::Pixmap));
            m_multitaskingView->setAttribute(Qt::WA_TranslucentBackground, true);
            m_multitaskingView->setClearColor(Qt::transparent);
            QSurfaceFormat fmt = m_multitaskingView->format();
            fmt.setAlphaBufferSize(8);
            m_multitaskingView->setFormat(fmt);
            qmlRegisterType<DesktopThumbnail>("com.deepin.kwin", 1, 0, "DesktopThumbnail");
            m_multitaskingView->rootContext()->setContextProperty("manager", m_thumbManager);
            m_multitaskingView->rootContext()->setContextProperty("backgroundManager", &BackgroundManager::instance());
            m_multitaskingView->rootContext()->setContextProperty("$Model", m_multitaskingModel);
            m_multitaskingView->rootContext()->setContextProperty("numScreens", effects->numScreens());
            m_multitaskingView->setWindowFlags(Qt::BypassWindowManagerHint);
            auto root = m_multitaskingView->rootObject();
            connect(m_multitaskingModel, SIGNAL(appendDesktop()), m_thumbManager, SIGNAL(requestAppendDesktop()));
            connect(m_multitaskingModel, SIGNAL(removeDesktop(int)), m_thumbManager, SIGNAL(requestDeleteDesktop(int)));
            connect(m_multitaskingModel, SIGNAL(currentDesktopChanged(int)), m_thumbManager, SIGNAL(requestChangeCurrentDesktop(int)));
            connect(m_multitaskingModel, SIGNAL(switchDesktop(int, int)), m_thumbManager, SIGNAL(requestSwitchDesktop(int, int)));
            connect(m_multitaskingModel, SIGNAL(refreshWindows()), this, SLOT(refreshWindows()));
        }

        m_multitaskingModel->setCurrentIndex(effects->currentDesktop() - 1);
        m_multitaskingView->setSource(QUrl("qrc:/qml/thumbmanager.qml"));
        m_multitaskingView->setGeometry(effects->virtualScreenGeometry());
        m_multitaskingModel->load(desktopCount);
        m_hasKeyboardGrab = effects->grabKeyboard(this);
        effects->startMouseInterception(this, Qt::PointingHandCursor);

        auto root = m_multitaskingView->rootObject();
        root->setAcceptHoverEvents(true);
        connect(root, SIGNAL(qmlRequestMove2Desktop(int, int, QVariant)), 
                m_thumbManager, SIGNAL(requestMove2Desktop(int, int, QVariant)));
        connect(root, SIGNAL(qmlCloseMultitask()), this, SLOT(toggleActive()));
//zhd add 
        connect(this, SIGNAL(modeChanged()),root, SIGNAL(resetModel()));
//zhd add end 

        connect(root, SIGNAL(qmlRemoveWindowThumbnail(int, int, int, QVariant)), this, SLOT(removeEffectWindow(int, int, int, QVariant)));  
    } else {
        effects->ungrabKeyboard();
        effects->stopMouseInterception(this);
    }
    
    
    m_multitaskingView->setVisible(m_multitaskingViewVisible);

    EffectWindowList windows = effects->stackingOrder();
    EffectWindow* active_window = nullptr;
    for (const auto& w: windows) {
        if (!isRelevantWithPresentWindows(w)) {
            continue;
        }
        auto wd = m_windowDatas.find(w);
        if (wd != m_windowDatas.end()) {
            qCDebug(BLUR_CAT) << "------- [init] wd exists " << w << w->windowClass();
            continue;
        }
        wd = m_windowDatas.insert(w, WindowData());
        initWindowData(wd, w);
    }

    for (int i = 1; i <= effects->numberOfDesktops(); i++) {
        WindowMotionManager wmm;
        for (const auto& w: windows) {
            if (w->isOnDesktop(i) && isRelevantWithPresentWindows(w)) {
                // the last window is on top of the stack
                if (i == m_targetDesktop) {
                    active_window = w;
                }
                wmm.manage(w);
            }
        }

        calculateWindowTransformations(wmm.managedWindows(), wmm);
        m_motionManagers.append(wmm);

        //updateDesktopWindows(i);
    }

    if(active_window) 
    {
        m_multitaskingModel->setCurrentSelectIndex(findWId(active_window));
    }
        

    /*
    if (effects->activeFullScreenEffect() && effects->activeFullScreenEffect() != this)
    return;

       if (m_activated == active)
       return;

    if (!m_activated && m_toggleTimeline.currentValue() != 0) {
        // closing animation is still in effect
        return;
    }

    m_activated = active;

    if (active) {
        effects->setShowingDesktop(false);
        effects->startMouseInterception(this, Qt::PointingHandCursor);
        m_hasKeyboardGrab = effects->grabKeyboard(this);
        effects->setActiveFullScreenEffect(this);

        clearGrids();

        changeCurrentDesktop(effects->currentDesktop());

        auto height = qRound(effects->virtualScreenSize().height() * Constants::FLOW_WORKSPACE_TOP_OFFSET_PERCENT);
        if (!m_thumbManager) {
            m_thumbManager = new DesktopThumbnailManager(effects);
            m_thumbManager->setGeometry(0, 0, effects->virtualScreenSize().width(), height);
            connect(m_thumbManager, &DesktopThumbnailManager::requestChangeCurrentDesktop,
                    this, &MultitaskingEffect::changeCurrentDesktop);
            connect(m_thumbManager, &DesktopThumbnailManager::requestAppendDesktop,
                    this, &MultitaskingEffect::appendDesktop);
            connect(m_thumbManager, &DesktopThumbnailManager::requestDeleteDesktop,
                    this, &MultitaskingEffect::removeDesktop);
            connect(m_thumbManager, &DesktopThumbnailManager::requestMove2Desktop,
                    this, &MultitaskingEffect::moveWindow2Desktop);
            connect(m_thumbManager, &DesktopThumbnailManager::requestSwitchDesktop,
                    this, &MultitaskingEffect::switchTwoDesktop);
        }
        m_thumbManager->move(0, -height);
        m_thumbManager->show();


        EffectWindowList windows = effects->stackingOrder();
        EffectWindow* active_window = nullptr;
        for (const auto& w: windows) {
            if (!isRelevantWithPresentWindows(w)) {
                continue;
            }
            auto wd = m_windowDatas.find(w);
            if (wd != m_windowDatas.end()) {
                qCDebug(BLUR_CAT) << "------- [init] wd exists " << w << w->windowClass();
                continue;
            }
            wd = m_windowDatas.insert(w, WindowData());
            initWindowData(wd, w);
        }

        for (int i = 1; i <= effects->numberOfDesktops(); i++) {
            WindowMotionManager wmm;
            for (const auto& w: windows) {
                if (w->isOnDesktop(i) && isRelevantWithPresentWindows(w)) {
                    // the last window is on top of the stack
                    if (i == m_targetDesktop) {
                        active_window = w;
                    }
                    wmm.manage(w);
                }
            }

            calculateWindowTransformations(wmm.managedWindows(), wmm);
            m_motionManagers.append(wmm);

            updateDesktopWindows(i);
        }

        selectWindow(active_window);

    } else {
        auto p = m_motionManagers.begin();
        while (p != m_motionManagers.end()) {
            foreach (EffectWindow* w, p->managedWindows()) {
                p->moveWindow(w, w->geometry());
            }
            ++p;
        }

        updateHighlightWindow(nullptr);
        selectWindow(nullptr);
    }

    effects->addRepaintFull();
    */
}

void MultitaskingEffect::OnWindowLocateChanged(int screen,int desktop,int winid){
}

void MultitaskingEffect::globalShortcutChanged(QAction *action, const QKeySequence &seq)
{
    if (action->objectName() != QStringLiteral(ACTION_NAME)) {
        return;
    }
    shortcut.clear();
    shortcut.append(seq);
}

void MultitaskingEffect::calculateWindowTransformations(EffectWindowList windows, WindowMotionManager& wmm)
{
    if (windows.size() == 0)
        return;

    calculateWindowTransformationsClosest(windows, 0, wmm);
    //calculateWindowTransformationsNatural(windows, 0, wmm);
}

QMargins MultitaskingEffect::desktopMargins()
{
    if (m_desktopMargins.isNull()) {
        auto area = effects->clientArea(ScreenArea, 0, 0);

        int top_offset = (int)(area.height() * Constants::FLOW_WORKSPACE_TOP_OFFSET_PERCENT);
        int bottom_offset = (int)(area.height() * Constants::HORIZONTAL_OFFSET_PERCENT);
        float scale = Constants::HORIZONTAL_OFFSET_PERCENT + Constants::FLOW_WORKSPACE_TOP_OFFSET_PERCENT;
        int hpad = area.width() * scale / 2.0;

        // keep it outside of thumbnails area
        m_desktopMargins = QMargins(hpad, top_offset, hpad, bottom_offset);
    }

    return m_desktopMargins;
}


void MultitaskingEffect::calculateWindowTransformationsNatural(EffectWindowList windowlist, int screen,
        WindowMotionManager& motionManager)
{
    float m_accuracy = 20.0;

    // If windows do not overlap they scale into nothingness, fix by resetting. To reproduce
    // just have a single window on a Xinerama screen or have two windows that do not touch.
    // TODO: Work out why this happens, is most likely a bug in the manager.
    foreach (EffectWindow * w, windowlist)
        if (motionManager.transformedGeometry(w) == w->geometry())
            motionManager.reset(w);

    // As we are using pseudo-random movement (See "slot") we need to make sure the list
    // is always sorted the same way no matter which window is currently active.
    qSort(windowlist);

    bool showPanel = true;
    QRect area = effects->clientArea(ScreenArea, screen, effects->currentDesktop());
    if (showPanel)   // reserve space for the panel
        area = effects->clientArea(MaximizeArea, screen, effects->currentDesktop());

    area -= desktopMargins();
    QRect bounds = area;
    qCDebug(BLUR_CAT) << "---------- area" << area << bounds << desktopMargins();

    int direction = 0;
    QHash<EffectWindow*, QRect> targets;
    QHash<EffectWindow*, int> directions;
    foreach (EffectWindow * w, windowlist) {
        bounds = bounds.united(w->geometry());
        targets[w] = w->geometry();
        // Reuse the unused "slot" as a preferred direction attribute. This is used when the window
        // is on the edge of the screen to try to use as much screen real estate as possible.
        directions[w] = direction;
        direction++;
        if (direction == 4)
            direction = 0;
    }

    // Iterate over all windows, if two overlap push them apart _slightly_ as we try to
    // brute-force the most optimal positions over many iterations.
    bool overlap;
    do {
        overlap = false;
        foreach (EffectWindow * w, windowlist) {
            QRect *target_w = &targets[w];
            foreach (EffectWindow * e, windowlist) {
                if (w == e)
                    continue;
                QRect *target_e = &targets[e];
                if (target_w->adjusted(-5, -5, 5, 5).intersects(target_e->adjusted(-5, -5, 5, 5))) {
                    overlap = true;

                    // Determine pushing direction
                    QPoint diff(target_e->center() - target_w->center());
                    // Prevent dividing by zero and non-movement
                    if (diff.x() == 0 && diff.y() == 0)
                        diff.setX(1);
                    // Try to keep screen aspect ratio
                    //if (bounds.height() / bounds.width() > area.height() / area.width())
                    //    diff.setY(diff.y() / 2);
                    //else
                    //    diff.setX(diff.x() / 2);
                    // Approximate a vector of between 10px and 20px in magnitude in the same direction
                    diff *= m_accuracy / double(diff.manhattanLength());
                    // Move both windows apart
                    target_w->translate(-diff);
                    target_e->translate(diff);

                    // Try to keep the bounding rect the same aspect as the screen so that more
                    // screen real estate is utilised. We do this by splitting the screen into nine
                    // equal sections, if the window center is in any of the corner sections pull the
                    // window towards the outer corner. If it is in any of the other edge sections
                    // alternate between each corner on that edge. We don't want to determine it
                    // randomly as it will not produce consistant locations when using the filter.
                    // Only move one window so we don't cause large amounts of unnecessary zooming
                    // in some situations. We need to do this even when expanding later just in case
                    // all windows are the same size.
                    // (We are using an old bounding rect for this, hopefully it doesn't matter)
                    int xSection = (target_w->x() - bounds.x()) / (bounds.width() / 3);
                    int ySection = (target_w->y() - bounds.y()) / (bounds.height() / 3);
                    diff = QPoint(0, 0);
                    if (xSection != 1 || ySection != 1) { // Remove this if you want the center to pull as well
                        if (xSection == 1)
                            xSection = (directions[w] / 2 ? 2 : 0);
                        if (ySection == 1)
                            ySection = (directions[w] % 2 ? 2 : 0);
                    }
                    if (xSection == 0 && ySection == 0)
                        diff = QPoint(bounds.topLeft() - target_w->center());
                    if (xSection == 2 && ySection == 0)
                        diff = QPoint(bounds.topRight() - target_w->center());
                    if (xSection == 2 && ySection == 2)
                        diff = QPoint(bounds.bottomRight() - target_w->center());
                    if (xSection == 0 && ySection == 2)
                        diff = QPoint(bounds.bottomLeft() - target_w->center());
                    if (diff.x() != 0 || diff.y() != 0) {
                        diff *= m_accuracy / double(diff.manhattanLength());
                        target_w->translate(diff);
                    }

                    // Update bounding rect
                    bounds = bounds.united(*target_w);
                    bounds = bounds.united(*target_e);
                }
            }
        }
    } while (overlap);

    // Work out scaling by getting the most top-left and most bottom-right window coords.
    // The 20's and 10's are so that the windows don't touch the edge of the screen.
    double scale;
    if (bounds == area)
        scale = 1.0; // Don't add borders to the screen
    else if (area.width() / double(bounds.width()) < area.height() / double(bounds.height()))
        scale = (area.width() - 20) / double(bounds.width());
    else
        scale = (area.height() - 20) / double(bounds.height());
    // Make bounding rect fill the screen size for later steps
    bounds = QRect(
            bounds.x() - (area.width() - 20 - bounds.width() * scale) / 2 - 10 / scale,
            bounds.y() - (area.height() - 20 - bounds.height() * scale) / 2 - 10 / scale,
            area.width() / scale,
            area.height() / scale
            );

    // Move all windows back onto the screen and set their scale
    QHash<EffectWindow*, QRect>::iterator target = targets.begin();
    while (target != targets.end()) {
        target->setRect((target->x() - bounds.x()) * scale + area.x(),
                (target->y() - bounds.y()) * scale + area.y(),
                target->width() * scale,
                target->height() * scale
                );
        ++target;
    }

    bool m_fillGaps = true;
    // Try to fill the gaps by enlarging windows if they have the space
    if (m_fillGaps) {
        // Don't expand onto or over the border
        QRegion borderRegion(area.adjusted(-200, -200, 200, 200));
        borderRegion ^= area.adjusted(10 / scale, 10 / scale, -10 / scale, -10 / scale);

        bool moved;
        do {
            moved = false;
            foreach (EffectWindow * w, windowlist) {
                QRect oldRect;
                QRect *target = &targets[w];
                // This may cause some slight distortion if the windows are enlarged a large amount
                int widthDiff = m_accuracy;
                int heightDiff = heightForWidth(w, target->width() + widthDiff) - target->height();
                int xDiff = widthDiff / 2;  // Also move a bit in the direction of the enlarge, allows the
                int yDiff = heightDiff / 2; // center windows to be enlarged if there is gaps on the side.

                // heightDiff (and yDiff) will be re-computed after each successfull enlargement attempt
                // so that the error introduced in the window's aspect ratio is minimized

                // Attempt enlarging to the top-right
                oldRect = *target;
                target->setRect(target->x() + xDiff,
                        target->y() - yDiff - heightDiff,
                        target->width() + widthDiff,
                        target->height() + heightDiff
                        );
                if (isOverlappingAny(w, targets, borderRegion))
                    *target = oldRect;
                else {
                    moved = true;
                    heightDiff = heightForWidth(w, target->width() + widthDiff) - target->height();
                    yDiff = heightDiff / 2;
                }

                // Attempt enlarging to the bottom-right
                oldRect = *target;
                target->setRect(
                        target->x() + xDiff,
                        target->y() + yDiff,
                        target->width() + widthDiff,
                        target->height() + heightDiff
                        );
                if (isOverlappingAny(w, targets, borderRegion))
                    *target = oldRect;
                else {
                    moved = true;
                    heightDiff = heightForWidth(w, target->width() + widthDiff) - target->height();
                    yDiff = heightDiff / 2;
                }

                // Attempt enlarging to the bottom-left
                oldRect = *target;
                target->setRect(
                        target->x() - xDiff - widthDiff,
                        target->y() + yDiff,
                        target->width() + widthDiff,
                        target->height() + heightDiff
                        );
                if (isOverlappingAny(w, targets, borderRegion))
                    *target = oldRect;
                else {
                    moved = true;
                    heightDiff = heightForWidth(w, target->width() + widthDiff) - target->height();
                    yDiff = heightDiff / 2;
                }

                // Attempt enlarging to the top-left
                oldRect = *target;
                target->setRect(
                        target->x() - xDiff - widthDiff,
                        target->y() - yDiff - heightDiff,
                        target->width() + widthDiff,
                        target->height() + heightDiff
                        );
                if (isOverlappingAny(w, targets, borderRegion))
                    *target = oldRect;
                else
                    moved = true;
            }
        } while (moved);

        // The expanding code above can actually enlarge windows over 1.0/2.0 scale, we don't like this
        // We can't add this to the loop above as it would cause a never-ending loop so we have to make
        // do with the less-than-optimal space usage with using this method.
        foreach (EffectWindow * w, windowlist) {
            QRect *target = &targets[w];
            double scale = target->width() / double(w->width());
            if (scale > 2.0 || (scale > 1.0 && (w->width() > 300 || w->height() > 300))) {
                scale = (w->width() > 300 || w->height() > 300) ? 1.0 : 2.0;
                target->setRect(
                        target->center().x() - int(w->width() * scale) / 2,
                        target->center().y() - int(w->height() * scale) / 2,
                        w->width() * scale,
                        w->height() * scale);
            }
        }
    }

    // Notify the motion manager of the targets
    foreach (EffectWindow * w, windowlist)
        motionManager.moveWindow(w, targets.value(w));
}

static inline int distance(QPoint &pos1, QPoint &pos2)
{
    const int xdiff = pos1.x() - pos2.x();
    const int ydiff = pos1.y() - pos2.y();
    return int(sqrt(float(xdiff*xdiff + ydiff*ydiff)));
}

void MultitaskingEffect::calculateWindowTransformationsClosest(EffectWindowList windowlist, int screen,
        WindowMotionManager& motionManager)
{
    // This layout mode requires at least one window visible
    if (windowlist.count() == 0)
        return;

    bool showPanel = true;
    QRect area = effects->clientArea(ScreenArea, screen, effects->currentDesktop());
    if (showPanel)   // reserve space for the panel
        area = effects->clientArea(MaximizeArea, screen, effects->currentDesktop());
    area -= desktopMargins();

    int columns = int(ceil(sqrt(double(windowlist.count()))));
    int rows = int(ceil(windowlist.count() / double(columns)));

    int desktop = windowlist[0]->desktop();
    qCDebug(BLUR_CAT) << "------- calc " << desktop << columns << rows;
    m_gridSizes[desktop].columns = columns;
    m_gridSizes[desktop].rows = rows;

    // Assign slots
    int slotWidth = area.width() / columns;
    int slotHeight = area.height() / rows;
    QVector<EffectWindow*> takenSlots;
    takenSlots.resize(rows*columns);
    takenSlots.fill(0);

    // precalculate all slot centers
    QVector<QPoint> slotCenters;
    slotCenters.resize(rows*columns);
    for (int x = 0; x < columns; ++x)
        for (int y = 0; y < rows; ++y) {
            slotCenters[x + y*columns] = QPoint(area.x() + slotWidth * x + slotWidth / 2,
                                                area.y() + slotHeight * y + slotHeight / 2);
        }

    // Assign each window to the closest available slot
    EffectWindowList tmpList = windowlist; // use a QLinkedList copy instead?
    QPoint otherPos;
    while (!tmpList.isEmpty()) {
        EffectWindow *w = tmpList.first();
        int slotCandidate = -1, slotCandidateDistance = INT_MAX;
        QPoint pos = w->geometry().center();
        for (int i = 0; i < columns*rows; ++i) { // all slots
            const int dist = distance(pos, slotCenters[i]);
            if (dist < slotCandidateDistance) { // window is interested in this slot
                EffectWindow *occupier = takenSlots[i];
                assert(occupier != w);
                if (!occupier || dist < distance((otherPos = occupier->geometry().center()), slotCenters[i])) {
                    // either nobody lives here, or we're better - takeover the slot if it's our best
                    slotCandidate = i;
                    slotCandidateDistance = dist;
                }
            }
        }
        assert(slotCandidate != -1);
        if (takenSlots[slotCandidate])
            tmpList << takenSlots[slotCandidate]; // occupier needs a new home now :p
        tmpList.removeAll(w);
        takenSlots[slotCandidate] = w; // ...and we rumble in =)
    }

    m_takenSlots[desktop] = takenSlots;
    for (int slot = 0; slot < columns*rows; ++slot) {
        EffectWindow *w = takenSlots[slot];
        if (!w) // some slots might be empty
            continue;

        // Work out where the slot is
        QRect target(
            area.x() + (slot % columns) * slotWidth,
            area.y() + (slot / columns) * slotHeight,
            slotWidth, slotHeight);
        target.adjust(35, 35, -35, -35);   // Borders
        double scale;
        if (target.width() / double(w->width()) < target.height() / double(w->height())) {
            // Center vertically
            scale = target.width() / double(w->width());
            target.moveTop(target.top() + (target.height() - int(w->height() * scale)) / 2);
            target.setHeight(int(w->height() * scale));
        } else {
            // Center horizontally
            scale = target.height() / double(w->height());
            target.moveLeft(target.left() + (target.width() - int(w->width() * scale)) / 2);
            target.setWidth(int(w->width() * scale));
        }
        // Don't scale the windows too much
        if (scale > 2.0 || (scale > 1.0 && (w->width() > 300 || w->height() > 300))) {
            scale = (w->width() > 300 || w->height() > 300) ? 1.0 : 2.0;
            target = QRect(
                         target.center().x() - int(w->width() * scale) / 2,
                         target.center().y() - int(w->height() * scale) / 2,
                         scale * w->width(), scale * w->height());
        }
        motionManager.moveWindow(w, target);
    }
}


bool MultitaskingEffect::isOverlappingAny(EffectWindow *w, const QHash<EffectWindow*, QRect> &targets, const QRegion &border)
{
    QHash<EffectWindow*, QRect>::const_iterator winTarget = targets.find(w);
    if (winTarget == targets.constEnd())
        return false;
    if (border.intersects(*winTarget))
        return true;
    // Is there a better way to do this?
    QHash<EffectWindow*, QRect>::const_iterator target;
    for (target = targets.constBegin(); target != targets.constEnd(); ++target) {
        if (target == winTarget)
            continue;
        if (winTarget->adjusted(-5, -5, 5, 5).intersects(target->adjusted(-5, -5, 5, 5)))
            return true;
    }
    return false;
}

void MultitaskingEffect::refreshWindows()
{
    const int desktopCount = m_thumbManager->desktopCount();
    for (int d = 1; d <= desktopCount; ++d) {
        for (int screen = 0; screen < effects->numScreens(); ++screen) {
            auto windows = windowsFor(screen, d);
            m_multitaskingModel->setWindows(screen, d, windows);
        }
    }
}

void MultitaskingEffect::windowSelectSlot( QVariant winid )
{
    toggleActive();
    EffectWindow *ew = effects->findWindow(winid.toULongLong());
    if(ew)
    {
        effects->activateWindow( ew );
    }

}

void MultitaskingEffect::removeEffectWindow(int screen, int desktop, int index, QVariant winid)
{
    if(!m_multitaskingModel) 
        return;
    auto* ew = effects->findWindow(winid.toULongLong());
    ew->closeWindow();
}
