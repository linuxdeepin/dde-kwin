#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_loader = WindowInfoLoader::loader();

    connect(m_loader, &WindowInfoLoader::windowCaptured, this,
            [this](QSharedPointer<WindowInfo> info) { onWindowCaptured(info); });
}

MainWindow::~MainWindow() {}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    QMap<int, QSharedPointer<WindowInfo>> windows = m_loader->getWindowInfos();

    QMapIterator<int, QSharedPointer<WindowInfo>> iterator(windows);
    while (iterator.hasNext()) {
        iterator.next();
        m_loader->captureWindow(iterator.value());
    }
}

void MainWindow::onWindowCaptured(QSharedPointer<WindowInfo> info)
{
    QImage img(
            (uchar *)info->data, info->width, info->height, (QImage::Format)info->format);

    QString fileName = "window_screenshot_";
    fileName += QString::number(info->windowId);
    fileName += "_";
    fileName += info->resourceName;
    fileName += ".png";

    img.save(fileName, "PNG", 100);
    qDebug() << __func__ << ":" << __LINE__ << " windowId" << info->windowId << " "
             << fileName;
}
