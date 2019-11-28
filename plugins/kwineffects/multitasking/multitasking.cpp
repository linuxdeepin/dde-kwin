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

#include "constants.h"
#include "multitasking.h"
#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QQmlContext>
#include <QQuickItem>
#include <KGlobalAccel>
#include <KLocalizedString>

#define ACTION_NAME  "ShowMultitasking"


namespace KWin {

class KWIN_EXPORT VirtualDesktop : public QObject
{
};

class VirtualDesktopManager: public QObject {
    public:
        static VirtualDesktopManager* self() { return s_manager; }
        void removeVirtualDesktop(const QByteArray& id);
        VirtualDesktop *desktopForX11Id(uint id) const;
        VirtualDesktop *desktopForId(const QByteArray &id) const;
    public:
        QVector<VirtualDesktop*> m_desktops;

    private:
        static VirtualDesktopManager* s_manager;
};
}

DesktopThumbnailManager::DesktopThumbnailManager(EffectsHandler* h)
    :QWidget(0),
    m_effectWindow(nullptr),
    m_handler(h)
{
    setObjectName("DesktopThumbnailManager");

    setWindowFlags(Qt::BypassWindowManagerHint | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);

    connect(h, &EffectsHandler::numberDesktopsChanged, this, &DesktopThumbnailManager::onDesktopsChanged,
            Qt::QueuedConnection);


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
    connect(this, SIGNAL(layoutChanged()), root, SLOT(handleLayoutChanged()), Qt::QueuedConnection);
    connect(this, SIGNAL(desktopRemoved(QVariant)), root, SLOT(handleDesktopRemoved(QVariant)),
            Qt::QueuedConnection);
    connect(this, SIGNAL(desktopWindowsChanged(QVariant)), root, SLOT(handleDesktopWindowsChanged(QVariant)),
            Qt::QueuedConnection);

    // relay QML signals
    connect(root, SIGNAL(qmlRequestChangeDesktop(int)), this, SIGNAL(requestChangeCurrentDesktop(int)));
    connect(root, SIGNAL(qmlRequestAppendDesktop()), this, SIGNAL(requestAppendDesktop()));
    connect(root, SIGNAL(qmlRequestDeleteDesktop(int)), this, SIGNAL(requestDeleteDesktop(int)));
    connect(root, SIGNAL(qmlRequestMove2Desktop(QVariant, int)), this, SIGNAL(requestMove2Desktop(QVariant, int)));

    connect(m_handler, SIGNAL(desktopChanged(int, int, KWin::EffectWindow*)), this, SIGNAL(currentDesktopChanged()));

    auto vds = VirtualDesktopManager::self();
    QList<VirtualDesktop*> ds;
    for (auto o: vds->children()) {
        if (QLatin1String("KWin::VirtualDesktop") == o->metaObject()->className()) {
            ds.append(dynamic_cast<VirtualDesktop*>(o));
        }
    }

    for (auto vd: ds) {
        qDebug() << "~~~~~~~~~~~~~~~~~~~~ " 
            << vd->property("id")
            << vd->property("x11DesktopNumber") 
            << vd->property("name");
    }
}

//FIXME: performance!
QVariantList DesktopThumbnailManager::windowsFor(int desktop)
{
    QVariantList vl;
    for (auto wid: KWindowSystem::self()->windows()) {
        KWindowInfo info(wid, NET::WMDesktop);
        if (info.valid() && info.desktop() == desktop) {
            vl.append(wid);
        }
    }
    return vl;
}

void DesktopThumbnailManager::onDesktopsChanged()
{
    //NOTE: VirtualDesktop is lazy destroyed, so this may contain stale VD
    auto vds = VirtualDesktopManager::self();
    QList<VirtualDesktop*> ds;
    for (auto o: vds->children()) {
        if (QLatin1String("KWin::VirtualDesktop") == o->metaObject()->className()) {
            ds.append(dynamic_cast<VirtualDesktop*>(o));
        }
    }

    for (auto vd: ds) {
        qDebug() << "~~~~~~~~~~~~~~~~~~~~ " << __func__
            << vd->property("id")
            << vd->property("x11DesktopNumber") 
            << vd->property("name");
    }
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
    m_view->resize(re->size());
    emit containerSizeChanged();
}

void DesktopThumbnailManager::updateDesktopWindows()
{
    auto root = m_view->rootObject();
    auto thumbs = root->findChildren<DesktopThumbnail*>();
    for (auto th: thumbs) {
        QMetaObject::invokeMethod(th, "refreshWindows", Qt::QueuedConnection);
    }
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
        qDebug() << "thumb " << th->property("desktop") << pos << r;
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

        m_wsThumbSize = QSize(area.width() * Constants::WORKSPACE_WIDTH_PERCENT, 
                area.height() * Constants::WORKSPACE_WIDTH_PERCENT); 
    }
    return m_wsThumbSize;
}

QRect DesktopThumbnailManager::calculateDesktopThumbRect(int index)
{
    QRect r;
    r.setSize(calculateThumbDesktopSize());

    auto area = m_handler->clientArea(ScreenArea, 0, 0);
    float space = area.width() * Constants::SPACING_PERCENT;

    auto count = m_handler->numberOfDesktops() + 1;
    float offset_x = (area.width() - (r.width() + space) * count + space) / 2.0;
    r.moveTo((r.width() + space) * index + offset_x, 40);
    return r;
}


void DesktopThumbnailManager::mouseMoveEvent(QMouseEvent* e)
{
    qDebug() << "-------------- " << __func__;
}

void DesktopThumbnailManager::setEnabled(bool v)
{
}




MultitaskingEffect::MultitaskingEffect()
    : Effect(),
    m_activated(false),
    m_showAction(new QAction(this))
{
    QAction* a = m_showAction;
    a->setObjectName(QStringLiteral(ACTION_NAME));
    a->setText(i18n("Show Multitasking View"));

    QKeySequence ks(Qt::META + Qt::Key_S);
    KGlobalAccel::self()->setDefaultShortcut(a, QList<QKeySequence>() << ks);
    KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>() << ks);
    shortcut = KGlobalAccel::self()->shortcut(a);
    effects->registerGlobalShortcut(ks, a);

    connect(a, SIGNAL(triggered(bool)), this, SLOT(toggleActive()));

    connect(KGlobalAccel::self(), &KGlobalAccel::globalShortcutChanged, this, &MultitaskingEffect::globalShortcutChanged);
    connect(effects, &EffectsHandler::windowAdded, this, &MultitaskingEffect::onWindowAdded);
    connect(effects, &EffectsHandler::windowDeleted, this, &MultitaskingEffect::onWindowDeleted);
    connect(effects, &EffectsHandler::windowClosed, this, &MultitaskingEffect::onWindowClosed);

    connect(effects, &EffectsHandler::numberDesktopsChanged, this, &MultitaskingEffect::onNumberDesktopsChanged);
    connect(effects, SIGNAL(desktopChanged(int, int, KWin::EffectWindow*)), this, SLOT(onCurrentDesktopChanged()));
    //connect(effects, SIGNAL(windowGeometryShapeChanged(KWin::EffectWindow*,QRect)),
    //    this, SLOT(slotWindowGeometryShapeChanged(KWin::EffectWindow*,QRect)));
    //connect(effects, &EffectsHandler::numberScreensChanged, this, &DesktopGridEffect::setup);

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


void MultitaskingEffect::onWindowAdded(KWin::EffectWindow* w)
{
    if (!m_activated)
        return;

    qDebug() << __func__;
    if (!isRelevantWithPresentWindows(w))
        return; // don't add
    foreach (const int i, desktopList(w)) {
        WindowMotionManager& wmm = m_motionManagers[i-1];
        wmm.manage(w);
        calculateWindowTransformations(wmm.managedWindows(), wmm);
    }
    effects->addRepaintFull();
}

void MultitaskingEffect::onWindowClosed(KWin::EffectWindow* w)
{
    if (!m_activated && m_toggleTimeline.currentValue() == 0)
        return;
    qDebug() << __func__;

    if (w == m_movingWindow) {
        m_movingWindow = nullptr;
        m_isWindowMoving = false;
        effects->defineCursor(Qt::PointingHandCursor);
    }

    foreach (const int i, desktopList(w)) {
        WindowMotionManager& wmm = m_motionManagers[i-1];
        wmm.unmanage(w);
        calculateWindowTransformations(wmm.managedWindows(), wmm);
    }
    effects->addRepaintFull();
}

void MultitaskingEffect::onWindowDeleted(KWin::EffectWindow* w)
{
    qDebug() << __func__;
    if (m_thumbManager && w == m_thumbManager->effectWindow()) {
        m_thumbManager->setEffectWindow(nullptr);
    }

    auto p = m_motionManagers.begin();
    while (p != m_motionManagers.end()) {
        p->unmanage(w);
        ++p;
    }

    if (w == m_movingWindow) {
        m_movingWindow = nullptr;
        m_isWindowMoving = false;
        effects->defineCursor(Qt::PointingHandCursor);
    }
}

void MultitaskingEffect::onNumberDesktopsChanged(int old)
{
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

    effects->addRepaintFull();
}

void MultitaskingEffect::onCurrentDesktopChanged()
{
    qDebug() << "------------- " << __func__;
    if (m_targetDesktop != effects->currentDesktop()) {
        m_targetDesktop = effects->currentDesktop();
        effects->addRepaintFull();
    }
}

void MultitaskingEffect::reconfigure(ReconfigureFlags)
{
    qDebug() << "-------------- " << __func__;
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

        //qDebug() << "-------------- " << __func__ << time << m_toggleTimeline.currentValue();

        data.mask |= PAINT_SCREEN_TRANSFORMED;

        for (auto& mm: m_motionManagers) {
            mm.calculate(time);
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

    if (m_isWindowMoving) {
        // the moving window has to be painted on top of all desktops
        QPoint diff = cursorPos() - m_movingWindowStartPoint;
        QRect geo = m_movingWindowGeometry.translated(diff);
        WindowPaintData d(m_movingWindow, data.projectionMatrix());
        d *= QVector2D((qreal)geo.width() / (qreal)m_movingWindow->width(), (qreal)geo.height() / (qreal)m_movingWindow->height());
        d += QPoint(geo.left() - m_movingWindow->x(), geo.top() - m_movingWindow->y());
        effects->drawWindow(m_movingWindow, PAINT_WINDOW_TRANSFORMED | PAINT_WINDOW_LANCZOS, infiniteRegion(), d);
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

    w->enablePainting(EffectWindow::PAINT_DISABLED);
    if (!(w->isDesktop() || isRelevantWithPresentWindows(w)) && (w != m_thumbManager->effectWindow())) {
        w->disablePainting(EffectWindow::PAINT_DISABLED);
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
        if (w->isDesktop() && m_thumbManager) {
            d.setSaturation(0.1);
            effects->paintWindow(w, mask, area, d);

#if 0
            for (int i = 0; i < effects->numberOfDesktops(); i++) {
                auto r = m_thumbManager->calculateDesktopThumbRect(i);
                WindowPaintData d2 = data;
                d2.setScale({Constants::WORKSPACE_WIDTH_PERCENT, Constants::WORKSPACE_WIDTH_PERCENT});
                d2.setXTranslation(r.x());
                d2.setYTranslation(r.y());
                effects->drawWindow(w, mask, area, d2);
            }
#endif

        } else if (!w->isDesktop()) {
            auto geo = m_motionManagers[desktop-1].transformedGeometry(w);

            if (m_highlightWindow == w) {
                auto center = geo.center();
                geo.setWidth(geo.width() * 1.05f);
                geo.setHeight(geo.height() * 1.05f);
                geo.moveCenter(center);
            }

            d += QPoint(qRound(geo.x() - w->x()), qRound(geo.y() - w->y()));
            d.setScale(QVector2D((float)geo.width() / w->width(), (float)geo.height() / w->height()));
            
            //qDebug() << "--------- window " << w->geometry() << geo;
            effects->paintWindow(w, mask, area, d);
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
    switch (e->type()) {
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
            if (m_toggleTimeline.currentValue() != 1) {
                return;
            }
            break;

        default:
            return;
    }

    auto me = static_cast<QMouseEvent*>(e);
    
    if (m_thumbManager && !m_isWindowMoving) {
        if (m_thumbManager->geometry().contains(me->pos())) {
            auto pos = m_thumbManager->mapFromGlobal(me->pos());
            QMouseEvent new_event(me->type(), pos, me->pos(), me->button(), me->buttons(), me->modifiers());
            m_thumbManager->windowInputMouseEvent(&new_event);
            return;
        }
    }

    updateWindowStates(me);
}

void MultitaskingEffect::updateWindowStates(QMouseEvent* me)
{
    if (!m_activated) return;

    EffectWindow* target = nullptr;
    WindowMotionManager& mm = m_motionManagers[m_targetDesktop-1];
    auto windows = mm.managedWindows();
    for (auto win: windows) {
        auto geom = mm.transformedGeometry(win);
        if (geom.contains(me->pos())) {
            target = win;
            break;
        }
    }

    updateHighlightWindow(target);

    switch (me->type()) {
        case QEvent::MouseMove:
            if (m_movingWindow) {
                if (!m_isWindowMoving) {
                    if (me->buttons() == Qt::LeftButton &&
                            (me->pos() - m_dragStartPos).manhattanLength() > QApplication::startDragDistance()) {

                        auto ids = desktopList(m_movingWindow);
                        //Do not handle sticky windows right now
                        //It does not make sense to drag them to other desktops
                        if (ids.size() > 1) return;

                        qDebug() << "----- start drag";

                        WindowMotionManager& wmm = m_motionManagers[ids[0]-1];
                        const QRectF transformedGeo = wmm.transformedGeometry(m_movingWindow);
                        const QPointF pos = transformedGeo.topLeft();

                        float scale = 150.0 / transformedGeo.width();
                        const QSize size(150, scale * transformedGeo.height());
                        m_movingWindowGeometry = QRect(pos.toPoint(), size);
                        m_movingWindowGeometry.moveCenter(me->pos());
                        m_movingWindowStartPoint = me->pos();

                        wmm.unmanage(m_movingWindow);
                        if (EffectWindow* modal = m_movingWindow->findModal()) {
                            if (wmm.isManaging(modal))
                                wmm.unmanage(modal);
                        }

                        calculateWindowTransformations(wmm.managedWindows(), wmm);

                        m_isWindowMoving = true;
                        effects->defineCursor(Qt::ClosedHandCursor);
                    }
                }

                effects->addRepaintFull();
            }

            break;

        case QEvent::MouseButtonPress:
            if (target) {
                effects->setElevatedWindow(target, true);
                m_movingWindow = target;
                m_dragStartPos = me->pos();
                effects->addRepaintFull();
            }
            break;

        case QEvent::MouseButtonRelease:
            if (m_movingWindow && m_isWindowMoving) {
                //TODO: move to PLUS to create new 
                auto switch_to = m_thumbManager->desktopAtPos(me->pos());
                qDebug() << "---------- switch_to " << switch_to;
                effects->defineCursor(Qt::PointingHandCursor);
                effects->setElevatedWindow(m_movingWindow, false);

                if (switch_to <= 0) {
                    auto ids = desktopList(m_movingWindow);
                    switch_to = ids[0];

                    WindowMotionManager& wmm = m_motionManagers[switch_to-1];
                    wmm.manage(m_movingWindow);
                    if (EffectWindow* modal = m_movingWindow->findModal()) {
                        wmm.manage(modal);
                    }

                    calculateWindowTransformations(wmm.managedWindows(), wmm);
                    effects->addRepaintFull();
                } else {
                    moveEffectWindow2Desktop(m_movingWindow, switch_to);
                }

                m_movingWindow = nullptr;
                m_isWindowMoving = false;

            } else if (target) {
                effects->activateWindow(target);
                setActive(false);
            } else {
                setActive(false);
            }
            break;

        default: break;
    }
}

void MultitaskingEffect::updateHighlightWindow(EffectWindow* w)
{
    if (w == m_highlightWindow) return;

    qDebug() << __func__;
    if (m_highlightWindow) {
        effects->setElevatedWindow(m_highlightWindow, false);
        m_highlightWindow->addRepaintFull();
    }

    m_highlightWindow = w;
    if (w) {
        effects->setElevatedWindow(m_highlightWindow, true);
        m_highlightWindow->addRepaintFull();
    }
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

        switch (e->key()) {
            case Qt::Key_Escape:
                if (isActive()) toggleActive();
                break;

            case Qt::Key_Right: 
                changeCurrentDesktop(qMin(m_targetDesktop + 1, 4));
                break;

            case Qt::Key_Left: 
                changeCurrentDesktop(qMax(m_targetDesktop - 1, 1));
                break;

            default: break;
        }
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

    qDebug() << "-------- " << __func__;
    m_thumbMotion.unmanage(m_thumbManager->effectWindow());
    m_thumbMotion.reset();
    m_thumbManager->hide();

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
}


void MultitaskingEffect::removeDesktop(int d)
{
    auto vds = VirtualDesktopManager::self();
    for (auto o: vds->children()) {
        if (QLatin1String("KWin::VirtualDesktop") == o->metaObject()->className()) {
            auto vd = dynamic_cast<VirtualDesktop*>(o);
            if (vd->property("x11DesktopNumber").toInt() == d) {
                qDebug() << "~~~~~~~~~~~~~~~~~~~~ remove desktop " 
                    << vd->property("id")
                    << vd->property("x11DesktopNumber") 
                    << vd->property("name");
                vds->removeVirtualDesktop(vd->property("id").toByteArray());

                emit m_thumbManager->desktopRemoved(QVariant(d));
                break;
            }

        }
    }

}

void MultitaskingEffect::moveWindow2Desktop(QVariant wid, int desktop)
{
    auto* ew = effects->findWindow(wid.toULongLong());
    if (!ew) {
        return;
    }

    moveEffectWindow2Desktop(ew, desktop);
}

void MultitaskingEffect::moveEffectWindow2Desktop(EffectWindow* ew, int desktop)
{
    auto prev_desktop = ew->desktops().first();
    if (prev_desktop == desktop) {
        qDebug() << "------------ the same desktop";
        return;
    }

    qDebug() << "---------- move " << ew << "to" << desktop;
    QVector<uint> ids {(uint)desktop};
    effects->windowToDesktops(ew, ids);

    for (auto id: desktopList(ew)) {
        WindowMotionManager& pre_wmm = m_motionManagers[id-1];
        pre_wmm.unmanage(ew);
        if (EffectWindow* modal = ew->findModal()) {
            pre_wmm.unmanage(modal);
        }
        calculateWindowTransformations(pre_wmm.managedWindows(), pre_wmm);
    }

    // update new desktop
    WindowMotionManager& new_wmm = m_motionManagers[desktop-1];
    new_wmm.manage(ew);
    if (EffectWindow* modal = ew->findModal()) {
        new_wmm.manage(modal);
    }
    calculateWindowTransformations(new_wmm.managedWindows(), new_wmm);

    emit m_thumbManager->desktopWindowsChanged(prev_desktop);
    emit m_thumbManager->desktopWindowsChanged(desktop);

    effects->addRepaintFull();
}

void MultitaskingEffect::changeCurrentDesktop(int d)
{
    if (d <= 0 || d > effects->numberOfDesktops()) {
        return;
    }

    if (m_targetDesktop == d) {
        return;
    }

    m_targetDesktop = d;
    effects->setCurrentDesktop(d);
    //TODO: update 

    if (m_activated) {
        effects->addRepaintFull();
    }
}

void MultitaskingEffect::setActive(bool active)
{
    if (effects->activeFullScreenEffect() && effects->activeFullScreenEffect() != this)
        return;

    if (m_activated == active)
        return;

    m_activated = active;

    if (active) {
        effects->setShowingDesktop(false);
        effects->startMouseInterception(this, Qt::PointingHandCursor);
        m_hasKeyboardGrab = effects->grabKeyboard(this);
        effects->setActiveFullScreenEffect(this);

        changeCurrentDesktop(effects->currentDesktop());

        auto height = qRound(effects->workspaceHeight() * Constants::FLOW_WORKSPACE_TOP_OFFSET_PERCENT);
        if (!m_thumbManager) {
            m_thumbManager = new DesktopThumbnailManager(effects);
            m_thumbManager->setGeometry(0, 0, effects->workspaceWidth(), height);
            connect(m_thumbManager, &DesktopThumbnailManager::requestChangeCurrentDesktop,
                    this, &MultitaskingEffect::changeCurrentDesktop);
            connect(m_thumbManager, &DesktopThumbnailManager::requestAppendDesktop,
                    this, &MultitaskingEffect::appendDesktop);
            connect(m_thumbManager, &DesktopThumbnailManager::requestDeleteDesktop,
                    this, &MultitaskingEffect::removeDesktop);
            connect(m_thumbManager, &DesktopThumbnailManager::requestMove2Desktop,
                    this, &MultitaskingEffect::moveWindow2Desktop);
        }
        m_thumbManager->updateDesktopWindows();
        m_thumbManager->move(0, -height);
        m_thumbManager->show();


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

        bool enableAdd = effects->numberOfDesktops() < 7;
        bool enableRemove = effects->numberOfDesktops() > 1;


    } else {
        auto p = m_motionManagers.begin();
        while (p != m_motionManagers.end()) {
            foreach (EffectWindow* w, p->managedWindows()) {
                p->moveWindow(w, w->geometry());
            }
            ++p;
        }

        updateHighlightWindow(nullptr);
    }

    effects->addRepaintFull();
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

    calculateWindowTransformationsNatural(windows, 0, wmm);
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

    QRect area = effects->clientArea(ScreenArea, screen, effects->currentDesktop());
    area -= desktopMargins();

    //if (m_showPanel)   // reserve space for the panel
        //area = effects->clientArea(MaximizeArea, screen, effects->currentDesktop());
    QRect bounds = area;
    qDebug() << "---------- area" << area << bounds << desktopMargins();

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
