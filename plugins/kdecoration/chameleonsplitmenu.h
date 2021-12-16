#ifndef CHAMELEONSPLITMENU_H
#define CHAMELEONSPLITMENU_H

#include "kwineffects.h"

#include <QWidget>
#include <QMouseEvent>
#include <QLabel>
#include <QGraphicsDropShadowEffect>

enum class QuickTileFlag {
    None        = 0,
    Left        = 1 << 0,
    Right       = 1 << 1,
    Top         = 1 << 2,
    Bottom      = 1 << 3,
    Horizontal  = Left | Right,
    Vertical    = Top | Bottom,
    Maximize    = Left | Right | Top | Bottom,
};

class ChameleonSplitMenu : public QWidget
{
    Q_OBJECT
public:
    explicit ChameleonSplitMenu(QWidget *parent = nullptr);
    ~ChameleonSplitMenu();

    void Show(QPoint pos);
    void Hide();
    bool isShow();
    void setShowSt(bool flag);
    bool getMenuSt(){ return m_MenuSt;};
    void setEffect(WId id);

    void startTime();
    void stopTime();

    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;

    bool eventFilter(QObject *obj, QEvent *event);
    void paintEvent(QPaintEvent *e);

private:
    bool m_isShow = false;
    bool m_MenuSt = false;
    QPoint m_pos;
    QLabel *llabel;
    QLabel *clabel;
    QLabel *rlabel;

    QObject *m_client = nullptr;
    QTimer *tip_timer = nullptr;

    QGraphicsDropShadowEffect *shadow = nullptr;
};

#endif
