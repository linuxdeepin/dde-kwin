#include "utils.h"

#include <QSize>
#include <QtX11Extras/QX11Info>
#include <X11/extensions/shape.h>

// 设置鼠标穿透
void Utils::passInputEvent(int wid)
{
    //const auto display = QX11Info::display();
    XRectangle *reponseArea = new XRectangle;
    reponseArea->x = 0;
    reponseArea->y = 0;
    reponseArea->width = 0;
    reponseArea->height = 0;

    //XShapeCombineRectangles(QX11Info::display(), wid, ShapeInput, 0, 0, reponseArea ,1 ,ShapeSet, YXBanded);
    XShapeCombineRectangles(QX11Info::display(), wid, ShapeInput, 0, 0, NULL, 0, ShapeSet, YXBanded);

    delete reponseArea;
}

// 取消鼠标穿透
void Utils::unPassInputEvent(int wid, QSize size)
{
    XRectangle *reponseArea = new XRectangle;
    reponseArea->x = 0;
    reponseArea->y = 0;
    reponseArea->width = size.width();
    reponseArea->height = size.height();

    XShapeCombineRectangles(QX11Info::display(), wid, ShapeInput, 0, 0, reponseArea ,1 ,ShapeSet, YXBanded);

    delete reponseArea;
}
