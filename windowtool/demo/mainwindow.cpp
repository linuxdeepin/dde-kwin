#include <QKeyEvent>

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    m_loader = WindowInfoLoader::get(this);

    connect(m_loader, &WindowInfoLoader::bufferCallback, this,
            [this](QSharedPointer<WindowInfo> info) { onWindowCaptured(info); });
}

MainWindow::~MainWindow() {}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_R) {
        m_count = 0;
        m_loader->startRecording();
    }

    if (event->key() == Qt::Key_O) {
        m_count = 0;
        m_loader->screenshot(1);
    }

    if (event->key() == Qt::Key_W) {
        m_count = 0;

        QMap<int, QSharedPointer<WindowInfo>> windows = m_loader->getWindowInfos();

        QMapIterator<int, QSharedPointer<WindowInfo>> iterator(windows);
        while (iterator.hasNext()) {
            iterator.next();
            m_loader->captureWindow(iterator.value());
        }
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
    fileName += "_";
    fileName += QString::number(m_count++);
    fileName += ".png";

    img.save(fileName, "PNG", 100);
    qDebug() << __func__ << ":" << __LINE__ << " windowId" << info->windowId << " "
             << fileName;
}
