// Copyright (C) 2020 Uniontech Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <KF5/KWindowSystem/KWindowSystem>
#include <QAbstractListModel>
#include "multitasking.h"

class DesktopThumbnailItem {
public:
	explicit DesktopThumbnailItem();
};

class MultitaskingModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int displayMode READ displayMode)
    Q_PROPERTY(bool isExtensionMode READ isExtensionMode)
public:
    enum DataRoles{
        ThumbnailRole = Qt::UserRole + 1,
    };

    explicit MultitaskingModel(QObject *parent = nullptr);
    ~MultitaskingModel();

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_PROPERTY(int currentWindowThumbnail READ currentSelectIndex WRITE setCurrentSelectIndex NOTIFY currentWindowThumbnailChanged)

    Q_PROPERTY(int currentDeskIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

    // Remove data:
	Q_INVOKABLE void load(int desktopCount);
    Q_INVOKABLE void append();
    Q_INVOKABLE void remove(int index);
	void clear();
    int count() const;

    QList<DesktopThumbnailItem> desktopThumbnailItemList() const {
        return m_desktopThumbnailItemList;
    }
	
    Q_INVOKABLE int getWindowHeight(QVariant winId);
    Q_INVOKABLE int getWindowWidth(QVariant winId);
    Q_INVOKABLE void addWindow(int screen, int desktop, const QVariant &winId){};
    Q_INVOKABLE void removeWindow(int screen, int desktop, const QVariant &winId){};
    Q_INVOKABLE void setWindows(int screen, int desktop, const QVariantList &windows);
    Q_INVOKABLE QVariantList windows(int screen, int desktop) const; 
    Q_INVOKABLE bool isCurrentScreenWindows(int screen, int desktop, QVariant wid);
    Q_INVOKABLE void moveToScreen(int screen, int desktop, QVariant wid);
    Q_INVOKABLE int getCalculateRowCount(int screen, int desktop);
    Q_INVOKABLE int getCalculateColumnsCount(int screen, int desktop);
    Q_INVOKABLE int getDesktopClientCount(int screen, int desktop);
    
    Q_INVOKABLE int numScreens() const;
    Q_INVOKABLE QRect screenGeometry(int screen) const;
    Q_INVOKABLE void setCurrentIndex(int index);
    Q_INVOKABLE int currentIndex() const;
    Q_INVOKABLE void move(int from, int to);
    Q_INVOKABLE void setCurrentSelectIndex(int index);
    Q_INVOKABLE int currentSelectIndex() const;
    Q_INVOKABLE void updateWindowDestop( int nDesktop );
    void selectNextWindow();
    void selectPrevWindow();
    void selectNextSametypeWindow();
    void selectPrevSametypeWindow();
    void selectFirstWindow();
    void selectLastWindow();
    QPair<int,int> getScreenDesktopByWinID(int winid);
    bool isAllScreensEmpty();
    Q_INVOKABLE void windowSelected( QVariant winId );
    Q_INVOKABLE QPixmap getWindowIcon( QVariant winId );
    bool isCurrentScreensEmpty();
    void selectNextWindowVert(int dir);
    int getNextWindowID();
    int getPrevWindowID();
    int getNextSametypeWindowID();
    int getPrevSametypeWindowID();
    Q_INVOKABLE bool getWindowKeepAbove(QVariant winId);
    Q_INVOKABLE void setWindowKeepAbove(QVariant winId);
    Q_INVOKABLE void forceResetModel();
    Q_INVOKABLE QString screenName(int x,int y);

    //0 custom Mode; 1 copy mode; 2 extension mode; 3 single screen mode
    int displayMode() const;
    bool isExtensionMode() const;

    int currentDesktop();
    int lastNoEmptyScreen();
    int firstNoEmptyScreen();

signals:
    void countChanged(int count);
    void appendDesktop();
    void removeDesktop(int desktop);
    void currentIndexChanged(int currentIndex);
    void currentDesktopChanged(int desktop);
    void move2Desktop(QVariant, int);
    void switchDesktop(int from, int to);
    void refreshWindows();
    void currentWindowThumbnailChanged();
    void windowSelectedSignal( QVariant winId );
    void currentDesktopIndexChanged();
    void updateQmlBackground();

protected: // interface QAbstractListModel
    virtual QHash<int, QByteArray> roleNames() const;

private:
    QList<DesktopThumbnailItem> m_desktopThumbnailItemList;
    //QMap<screen, QMap<desktop, window>;
    // screen begin from 0. desktop begin from 1;
    QMap<int, QMap<int, QVariantList> > m_windows;
    int m_currentIndex; // is Current Desktop Id - 1
    int m_nCurrentSelectIndex; // is Current Window Id
};

#endif // DATAMODEL_H
