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
#include <QtDBus>

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
#include "backgroundimageprovider.h"
#include "windowthumbnail.h"

#include "multitasking.h"
#include "multitasking_model.h"

const QString actionName = "ShowMultitasking";

const QString dbusDeepinWmService = "com.deepin.wm";
const QString dbusDeepinWmObj = "/com/deepin/wm";
const QString dbusDeepinWmInif =  "com.deepin.wm";

Q_LOGGING_CATEGORY(BLUR_CAT, "kwin.blur", QtCriticalMsg);

static const QByteArray s_GtkFrameAtomName = QByteArrayLiteral("_GTK_FRAME_EXTENTS");

DesktopThumbnailManager::DesktopThumbnailManager(EffectsHandler* h)
    :QWidget(nullptr),
    m_effectWindow(nullptr),
    m_handler(h)
{
    setObjectName("DesktopThumbnailManager");
    setWindowTitle("DesktopThumbnailManager");

    QString qm = QString(":/translations/multitasking_%1.qm").arg(QLocale::system().name());
    auto tran = new QTranslator(this);
    if (tran->load(qm)) {
        qApp->installTranslator(tran);
    } else {
        qCDebug(BLUR_CAT) << "load " << qm << "failed";
    }
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
    a->setObjectName(actionName);
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
    connect(effects, SIGNAL(closeEffect(bool)), this, SLOT(slotCloseEffect(bool)));

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
    m_multitaskingView->deleteLater();
    m_multitaskingModel->deleteLater();
    m_thumbManager->deleteLater();
    m_multitaskingModel->deleteLater();
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
        if (isRelevantWithPresentWindows(ew) && ew->isOnAllDesktops() && effects->screenNumber(ew->pos()) == screen) {
            auto wid = findWId(ew);
            assert (effects->findWindow(wid) == ew);
            vl.append(wid);
        }
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
        setActive(false);
        setActive(true);
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
void MultitaskingEffect::prePaintScreen(ScreenPrePaintData &data, TimeArgType time)
{
    //EffectWindow of m_multitaskingView changes every time when
    //multitaskingView show, we need to set EffectWindow before paintwindow
    if (m_multitaskingView) { 
        if (!multitaskingViewEffectWindow()) {
            auto* ew = effects->findWindow(m_multitaskingView->winId());
            if (ew) {
                setMultitaskingViewEffectWindow(ew);
            }
        }
    }
    effects->prePaintScreen(data, time);
}

#if KWIN_VERSION_MIN > 17 || (KWIN_VERSION_MIN == 17 && KWIN_VERSION_PAT > 5)
void MultitaskingEffect::paintScreen(int mask, const QRegion &region, ScreenPaintData &data)
#else
void MultitaskingEffect::paintScreen(int mask, QRegion region, ScreenPaintData &data)
#endif
{
    effects->paintScreen(mask, region, data);
}

void MultitaskingEffect::postPaintScreen()
{
    for (auto const& w: effects->stackingOrder()) { 
        w->setData(WindowForceBlurRole, QVariant()); 
    }
    effects->postPaintScreen();
}


// Window painting
void MultitaskingEffect::prePaintWindow(EffectWindow *w, WindowPrePaintData &data, TimeArgType time)
{
    if (m_multitaskingView && multitaskingViewEffectWindow() 
        && w == multitaskingViewEffectWindow()) {
        effects->prePaintWindow(w, data, time);
        return;
    }

    data.mask |= PAINT_WINDOW_TRANSFORMED;

    if (m_multitaskingViewVisible) {
        w->enablePainting(EffectWindow::PAINT_DISABLED_BY_MINIMIZE);   // Display always
    }
    w->enablePainting(EffectWindow::PAINT_DISABLED);
    if (!(w->isDesktop() || isRelevantWithPresentWindows(w)) || w->isMinimized()) {
        w->disablePainting(EffectWindow::PAINT_DISABLED);
        w->disablePainting(EffectWindow::PAINT_DISABLED_BY_MINIMIZE);
    }

    effects->prePaintWindow(w, data, time);
}

void MultitaskingEffect::paintWindow(EffectWindow *w, int mask, QRegion region, WindowPaintData &data)
{
    effects->paintWindow(w, mask, region, data);
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
    auto me = static_cast<QMouseEvent*>(e);

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

   if (me->button() == Qt::ForwardButton || me->button() == Qt::BackButton) {
        if (me->type() != QEvent::MouseButtonPress || is_smooth_scrolling) return;
        if (me->buttons() == Qt::ForwardButton) {
            is_smooth_scrolling = true;
            if (m_multitaskingModel->currentIndex()+1 < m_multitaskingModel->rowCount()) {
                m_multitaskingModel->setCurrentIndex(m_multitaskingModel->currentIndex()+1);
            } else {
                m_multitaskingModel->setCurrentIndex(0);
            }
        } else if (me->buttons() == Qt::BackButton) {
            is_smooth_scrolling = true;
            if (m_multitaskingModel->currentIndex()-1 >= 0) {
                m_multitaskingModel->setCurrentIndex(m_multitaskingModel->currentIndex()-1);
            } else {
                int count = m_multitaskingModel->rowCount();
                if (count > 0) {
                    m_multitaskingModel->setCurrentIndex(count - 1);
                }
            }
        }
        QTimer::singleShot(400, [&]() { is_smooth_scrolling = false; });
        return;
    }
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
        m_pEffectWindow = effects->activeWindow();

        qCDebug(BLUR_CAT) << e;
        if (e->isAutoRepeat()) return;
        switch (e->key()) {
            case Qt::Key_Escape:
                setActive(false);
                break;

            case Qt::Key_Enter:
            case Qt::Key_Return:
                windowSelectSlot(m_multitaskingModel->currentSelectIndex());
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
                        int count = m_multitaskingModel->rowCount();
                        if (count > 0) {
                            m_multitaskingModel->setCurrentIndex(count - 1);
                        }
                    }
                } else if (e->modifiers() == Qt::NoModifier) {
                    selectPrevWindow();
                }
                if (e->modifiers() == (Qt::ShiftModifier|Qt::MetaModifier|Qt::KeypadModifier)) {
                    moveWindowThumbnail2Desktop(4);
                }
                break;

            case Qt::Key_Down:
                if (e->modifiers() == Qt::NoModifier) {
                    selectNextWindowVert(1);
                }
                if (e->modifiers() == (Qt::ShiftModifier|Qt::MetaModifier|Qt::KeypadModifier)) {
                    moveWindowThumbnail2Desktop(2);
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
                if (e->modifiers() == (Qt::ShiftModifier|Qt::MetaModifier|Qt::KeypadModifier)) {
                    moveWindowThumbnail2Desktop(1);
                }
                break;
            case Qt::Key_PageDown:
                if (e->modifiers() == (Qt::ShiftModifier|Qt::MetaModifier|Qt::KeypadModifier)) {
                    moveWindowThumbnail2Desktop(3);
                }
                break;
            case Qt::Key_1:
            case Qt::Key_2:
            case Qt::Key_3:
            case Qt::Key_4:
                if (e->modifiers() == Qt::NoModifier || 
                        e->modifiers() == Qt::MetaModifier || 
                        e->modifiers() == (Qt::MetaModifier|Qt::KeypadModifier)) {
                    int index = e->key() - Qt::Key_1;
                    if (m_multitaskingModel->rowCount() > index) {
                       m_multitaskingModel->setCurrentIndex(index);
                    }
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
                    moveWindowThumbnail2Desktop(target_desktop);
                }
                break;

            case Qt::Key_Equal:
                if (e->modifiers() == Qt::AltModifier) {
                    int originNum = m_multitaskingModel->count();
                    m_multitaskingModel->append();
                    if( originNum < m_multitaskingModel->count() )
                        m_multitaskingModel->setCurrentIndex(m_multitaskingModel->count() - 1);
                }
                break;

            case Qt::Key_Plus:
                if (e->modifiers() == (Qt::AltModifier|Qt::KeypadModifier)) {
                    int originNum = m_multitaskingModel->count();
                    m_multitaskingModel->append();
                    if( originNum < m_multitaskingModel->count() )
                        m_multitaskingModel->setCurrentIndex(m_multitaskingModel->count() - 1);
                }
                break;

            case Qt::Key_Minus:
                if (e->modifiers() == Qt::AltModifier || e->modifiers() == (Qt::AltModifier|Qt::KeypadModifier)) {
                    m_multitaskingModel->remove(m_targetDesktop - 1);
                    m_multitaskingModel->setCurrentIndex(m_targetDesktop - 1);
                }
                break;

            case Qt::Key_Delete:
                if (e->modifiers() == Qt::NoModifier) {
                    QVariant wId = m_multitaskingModel->currentSelectIndex();
                    EffectWindow* ew = effects->findWindow(wId.toULongLong());
                    if (ew) {
                        qCDebug(BLUR_CAT) << "-------- screen: " << ew->screen() << " desktop: " << ew->desktop()
                                          << ", close selected window: " << wId.toULongLong();
                        removeEffectWindow(ew->screen(), ew->desktop(), wId);
                    }
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
                    m_multitaskingModel->selectNextSametypeWindow();
                }
                break;

            case Qt::Key_AsciiTilde:
                if (e->modifiers() == (Qt::AltModifier | Qt::ShiftModifier)) {
                    m_multitaskingModel->selectPrevSametypeWindow();
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
}

void MultitaskingEffect::selectNextWindow()
{
    m_multitaskingModel->selectNextWindow();
}

void MultitaskingEffect::selectFirstWindow()
{
    m_multitaskingModel->selectFirstWindow();
}

void MultitaskingEffect::selectLastWindow()
{
    m_multitaskingModel->selectLastWindow();
}

void MultitaskingEffect::selectNextWindowVert(int dir)
{
    m_multitaskingModel->selectNextWindowVert(dir);
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
    return m_multitaskingViewVisible && !effects->isScreenLocked();
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
    BackgroundManager::instance().changeWorkSpaceBackground(effects->numberOfDesktops() + 1);
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

    refreshWindows();
    emit modeChanged();

    emit m_thumbManager->desktopRemoved(QVariant(d));
    // shift wallpapers before acutally removing it
    BackgroundManager::instance().desktopAboutToRemoved(d);
    effects->setNumberOfDesktops(effects->numberOfDesktops()-1);

    emit updateDesktopThumBackground();
}

void MultitaskingEffect::desktopRemoved(int d)
{
    remanageAll();
    updateDesktopWindows();
}

void MultitaskingEffect::slotCloseEffect(bool isSleepBefore)
{
    if (isSleepBefore && isActive()) {
        toggleActive();
    }
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
    refreshWindows();
    emit forceResetWindowThumbnailModel();
}

void MultitaskingEffect::moveWindow2Desktop(int screen, int desktop, QVariant winId) 
{
    auto* ew = effects->findWindow(winId.toULongLong());
    if (!ew) {
        return;
    }
    // qDebug() << "--------1719" << ew << "-------" << desktop;
    effects->windowToScreen(ew, screen);
    moveEffectWindow2Desktop(ew, desktop);
}

void MultitaskingEffect::moveEffectWindow2Desktop(EffectWindow* ew, int desktop)
{
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

    refreshWindows();
    emit modeChanged();
    m_multitaskingModel->updateWindowDestop(desktop);
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

    QDBusInterface wm(dbusDeepinWmService, dbusDeepinWmObj, dbusDeepinWmInif);
    wm.call("SetMultiTaskingStatus", active);

    if (m_multitaskingViewVisible) {
        if (m_targetDesktop != effects->currentDesktop()) {
            m_targetDesktop = effects->currentDesktop();
        }
        const int desktopCount = effects->numberOfDesktops();
        for (int d = 1; d <= desktopCount; ++d) {
            for (int screen = 0; screen < effects->numScreens(); ++screen) {
                auto windows = windowsFor(screen, d);
                m_multitaskingModel->setWindows(screen, d, windows);
            }
        }

        if (!m_multitaskingView) {
            m_multitaskingView = new QQuickWidget;
            m_multitaskingView->engine()->addImageProvider( QLatin1String("imageProvider"), new ImageProvider(QQmlImageProviderBase::Pixmap));
            m_multitaskingView->engine()->addImageProvider( QLatin1String("BackgroundImageProvider"), new BackgroundImageProvider(QQmlImageProviderBase::Pixmap));
            m_multitaskingView->setAttribute(Qt::WA_TranslucentBackground, true);
            m_multitaskingView->setClearColor(Qt::transparent);
            QSurfaceFormat fmt = m_multitaskingView->format();
            fmt.setAlphaBufferSize(8);
            m_multitaskingView->setFormat(fmt);
            qmlRegisterType<Plasma::WindowThumbnail>("org.kde.plasma", 2, 0, "WindowThumbnail");
            m_multitaskingView->rootContext()->setContextProperty("manager", m_thumbManager);
            m_multitaskingView->rootContext()->setContextProperty("backgroundManager", &BackgroundManager::instance());
            m_multitaskingView->rootContext()->setContextProperty("multitaskingModel", m_multitaskingModel);
            m_multitaskingView->rootContext()->setContextProperty("numScreens", effects->numScreens());
            m_multitaskingView->setWindowFlags(Qt::BypassWindowManagerHint);
            connect(m_multitaskingModel, SIGNAL(appendDesktop()), m_thumbManager, SIGNAL(requestAppendDesktop()));
            connect(m_multitaskingModel, SIGNAL(removeDesktop(int)), m_thumbManager, SIGNAL(requestDeleteDesktop(int)));
            connect(m_multitaskingModel, SIGNAL(currentDesktopChanged(int)), m_thumbManager, SIGNAL(requestChangeCurrentDesktop(int)));
            connect(m_multitaskingModel, SIGNAL(refreshWindows()), this, SLOT(refreshWindows()));
            connect(m_multitaskingModel, SIGNAL(switchDesktop(int, int)), this, SLOT(switchTwoDesktop(int, int)));
        }

        QList<QScreen *> screenList = QGuiApplication::screens();
        QList<QMap<QString,QVariant>> screenInfoLst;
        for(int i = 0; i < screenList.count(); i++) {
            QMap<QString,QVariant> screeninfo;
            QScreen *pScreen = screenList.at(i);
            QString monitorName = pScreen->name();
            screeninfo[monitorName] = pScreen->size();
            screenInfoLst << screeninfo;
        }
        BackgroundManager::instance().setMonitorInfo(screenInfoLst);

        m_multitaskingModel->setCurrentIndex(effects->currentDesktop() - 1);
        m_thumbManager->setGeometry(effects->virtualScreenGeometry());
        m_multitaskingModel->load(desktopCount);
        m_multitaskingView->setSource(QUrl("qrc:/qml/thumbmanager.qml"));
        m_multitaskingView->setGeometry(effects->virtualScreenGeometry());
        m_hasKeyboardGrab = effects->grabKeyboard(this);
        effects->startMouseInterception(this, Qt::PointingHandCursor);

        auto root = m_multitaskingView->rootObject();
        root->setAcceptHoverEvents(true);
        connect(root, SIGNAL(qmlRequestMove2Desktop(int, int, QVariant)), 
                m_thumbManager, SIGNAL(requestMove2Desktop(int, int, QVariant)));
        connect(root, SIGNAL(qmlCloseMultitask()), this, SLOT(toggleActive()));
        connect(this, SIGNAL(modeChanged()),root, SIGNAL(resetModel()));
        connect(root, SIGNAL(qmlRemoveWindowThumbnail(int, int, QVariant)), this, SLOT(removeEffectWindow(int, int, QVariant)));
        connect(this, SIGNAL(forceResetDesktopModel()), root, SIGNAL(qmlForceResetDesktopModel()));
        connect(m_multitaskingModel, SIGNAL(currentDesktopChanged(int)), root, SIGNAL(qmlUpdateBackground()));
        connect(m_multitaskingModel, SIGNAL(updateQmlBackground()), root, SIGNAL(qmlUpdateBackground()));

        connect(this, SIGNAL(forceResetWindowThumbnailModel()), root, SIGNAL(qmlForceResetWindowThumbnailModel()));

        EffectWindow* active_window = effects->activeWindow();
        if (active_window && !active_window->isSpecialWindow()) {
           m_multitaskingModel->setCurrentSelectIndex(findWId(active_window));
        } else {
            m_multitaskingModel->setCurrentSelectIndex(-1);
        }
    } else {
        if (m_hasKeyboardGrab) {
            effects->ungrabKeyboard();
        }
        m_hasKeyboardGrab = false;
        effects->stopMouseInterception(this);
    }
    
    m_multitaskingView->setVisible(m_multitaskingViewVisible);
    if (m_multitaskingViewVisible) {
        // set nullptr to avoid confusing prepaintscreen check
        setMultitaskingViewEffectWindow(nullptr);
    }
}

void MultitaskingEffect::OnWindowLocateChanged(int screen,int desktop,int winid){
}

void MultitaskingEffect::globalShortcutChanged(QAction *action, const QKeySequence &seq)
{
    if (action->objectName() != actionName) {
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
    if (winTarget == targets.constEnd()) {
        return false;
    }
    if (border.intersects(*winTarget)) {
        return true;
    }
    // Is there a better way to do this?
    QHash<EffectWindow*, QRect>::const_iterator target;
    for (target = targets.constBegin(); target != targets.constEnd(); ++target) {
        if (target == winTarget) {
            continue;
        }
        if (winTarget->adjusted(-5, -5, 5, 5).intersects(target->adjusted(-5, -5, 5, 5))) {
            return true;
        }
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
    if (ew)
    {
        effects->activateWindow( ew );
    }

}

void MultitaskingEffect::removeEffectWindow(int screen, int desktop, QVariant winid)
{
    if (!m_multitaskingModel) {
       return;
    }
    auto* ew = effects->findWindow(winid.toULongLong());
    ew->closeWindow();
}

void MultitaskingEffect::moveWindowThumbnail2Desktop(int desktop)
{
    if (desktop > m_multitaskingModel->count()
    ||  m_multitaskingModel->currentSelectIndex() == -1
    ||  m_multitaskingModel->currentSelectIndex() == 0) {
        return;
    }
    m_multitaskingModel->setCurrentIndex(desktop-1);
    qCDebug(BLUR_CAT) << "----------- super+shift+"<<desktop;
    QVariant  winId  = m_multitaskingModel->currentSelectIndex();
    EffectWindow *ew = effects->findWindow(winId.toULongLong());
    if (ew) {
        moveWindow2Desktop(ew->screen(), desktop, m_multitaskingModel->currentSelectIndex());
    }
}
