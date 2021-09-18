#include "chameleonsplitmenu.h"
#include <QEventLoop>
#include <QVBoxLayout>
#include <QDebug>
#include <QPainter>
#include <QBitmap>
#include "kwinutils.h"


ChameleonSplitMenu::ChameleonSplitMenu(QWidget *parent) : QWidget (parent)
{
    setWindowFlags(Qt::X11BypassWindowManagerHint);
    setAttribute(Qt::WA_TranslucentBackground);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setSpacing(5);

    llabel = new QLabel(this);
    llabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/left_split_normal.svg); background-repeat:no-repeat;background-position:center;");

    clabel = new QLabel(this);
    clabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/right_split_normal.svg); background-repeat:no-repeat;background-position:center;");

    rlabel = new QLabel(this);
    rlabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/max_split_normal.svg); background-repeat:no-repeat;background-position:center;");

    layout->addWidget(llabel);
    layout->addWidget(clabel);
    layout->addWidget(rlabel);
    llabel->installEventFilter(this);
    clabel->installEventFilter(this);
    rlabel->installEventFilter(this);
    layout->setContentsMargins(2, 10, 2, 2);
    setLayout(layout);
}

ChameleonSplitMenu::~ChameleonSplitMenu()
{

}

void ChameleonSplitMenu::enterEvent(QEvent *e)
{
    QWidget::enterEvent(e);
    m_MenuSt = true;
    if (!m_isShow) {
        Show(m_pos);
    }
}

void ChameleonSplitMenu::leaveEvent(QEvent *e)
{
    QWidget::leaveEvent(e);
    m_MenuSt = false;
    if (m_isShow) {
        Hide();
    }
}

void ChameleonSplitMenu::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    QColor col_menu(255, 255, 255, 200);//(114, 164, 250, 200);
    painter.setBrush(QBrush(col_menu));

    QPainterPath painterPath;
    painterPath.addRoundedRect(QRect(0, 10, width(), height() - 10), 17, 17);
    painter.drawPath(painterPath);

    int spot = (width() / 2) + 17;
    QPointF points[3] = {
        QPointF(spot, 0),
        QPointF(spot + 8, 10),
        QPointF(spot - 8, 10),
    };

    painter.setBrush(QBrush(col_menu));
    QPen pen;
    pen.setColor(col_menu);
    painter.setPen(pen);
    painter.drawPolygon(points, 3);
}

bool ChameleonSplitMenu::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == llabel) {
        if (event->type() == QEvent::MouseButtonRelease) {
            llabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/left_split_hover.svg); background-repeat:no-repeat;background-position:center;");

            if (m_client)
                KWinUtils::Window::setQuikTileMode(m_client, (int)QuickTileFlag::Left, true);
            llabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/left_split_normal.svg); background-repeat:no-repeat;background-position:center;");
            Hide();
        }
        else if (event->type() == QEvent::Enter) {
            llabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/left_split_hover.svg); background-repeat:no-repeat;background-position:center;");
        }
        else if (event->type() == QEvent::Leave) {
            llabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/left_split_normal.svg); background-repeat:no-repeat;background-position:center;");
        }
        return false;
    } else if (obj == clabel) {
        if (event->type() == QEvent::MouseButtonRelease) {
            clabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/right_split_hover.svg); background-repeat:no-repeat;background-position:center;");

            if (m_client)
                KWinUtils::Window::setQuikTileMode(m_client, (int)QuickTileFlag::Right, true);
            clabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/right_split_normal.svg); background-repeat:no-repeat;background-position:center;");
            Hide();
        }
        else if (event->type() == QEvent::Enter) {
            clabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/right_split_hover.svg); background-repeat:no-repeat;background-position:center;");
        }
        else if (event->type() == QEvent::Leave) {
            clabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/right_split_normal.svg); background-repeat:no-repeat;background-position:center;");
        }
        return false;
    } else if (obj == rlabel) {
        if (event->type() == QEvent::MouseButtonRelease) {
            rlabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/max_split_hover.svg); background-repeat:no-repeat;background-position:center;");

            if (m_client)
                KWinUtils::Window::setQuikTileMode(m_client, (int)QuickTileFlag::Maximize);
            rlabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/max_split_normal.svg); background-repeat:no-repeat;background-position:center;");
            Hide();
        }
        else if (event->type() == QEvent::Enter) {
            rlabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/max_split_hover.svg); background-repeat:no-repeat;background-position:center;");
        }
        else if (event->type() == QEvent::Leave) {
            rlabel->setStyleSheet("background-image:url(:/deepin/themes/deepin/light/icons/max_split_normal.svg); background-repeat:no-repeat;background-position:center;");
        }
        return false;
    }

    return QWidget::eventFilter(obj, event);
}

void ChameleonSplitMenu::Show(QPoint pos)
{
    m_isShow = true;
    m_pos = pos;
    pos.setX(m_pos.x() -70);
    QRect rect(pos, QSize(148, 70));
    setGeometry(rect);

    show();
}

void ChameleonSplitMenu::Hide()
{
    m_isShow = false;
    hide();
}

bool ChameleonSplitMenu::isShow()
{
    return m_isShow;
}

void ChameleonSplitMenu::setShowSt(bool flag)
{
    m_MenuSt = flag;
}

void ChameleonSplitMenu::setEffect(WId id)
{
    m_client = KWinUtils::findClient(KWinUtils::Predicate::WindowMatch, id);
}
