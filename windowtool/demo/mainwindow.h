#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDebug>
#include <QMainWindow>
#include <QMap>

#include <windowtool/WindowInfoLoader.h>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    WindowInfoLoader *m_loader;
    QMap<int, QSharedPointer<WindowInfo>> m_windows;

    void onWindowCaptured(QSharedPointer<WindowInfo> info);
};

#endif  // MAINWINDOW_H
