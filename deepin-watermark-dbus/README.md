# watermark
鼠标点击穿透：所有鼠标键盘操作全部会穿透窗口到下方窗口。
  
在Linux下，Qt的Qt::WA_TransparentForMouseEvents属性可以对子部件实现鼠标穿透，但是对整
个窗口不行，要想在Linux下对Qt的整个窗口设置鼠标穿透，这时候需要用到X11中x11shape的XShapeCombineRectangles()库函数，很多窗口挂件都是这样做的，如osd桌面歌词程序等等。

void XShapeCombineRectangles (
Display *dpy,
XID dest,
int destKind,
int xOff,
int yOff,
XRectangle *rects,
int n_rects,
int op,
int ordering);

在设置鼠标穿透的时候给函数传的第六个参数为NULL（参数为XRectangle*类型），那整个窗口都
将被穿透，第七个参数就是控制设置穿透的，为0时表示设置鼠标穿透，为1时表示取消鼠标穿透>，当取消设置鼠标穿透的时候，必须设置区域，即第六个参数不再为NULL。
