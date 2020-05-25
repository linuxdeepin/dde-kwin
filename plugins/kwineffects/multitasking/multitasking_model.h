/*
 * Copyright (C) 2020 Uniontech Technology Co., Ltd.
 *
 * Author:     Lei Su <axylp@qq.com>
 *
 * Maintainer: Lei Su <axylp@qq.com>
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


#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <QAbstractListModel>
#include "multitasking.h"

class DesktopThumbnailItem {
public:
	explicit DesktopThumbnailItem();
};

class MultitaskingModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum DataRoles{
        ThumbnailRole = Qt::UserRole + 1,
    };

    explicit MultitaskingModel(QObject *parent = nullptr);
    ~MultitaskingModel();

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Add data:
    Q_INVOKABLE void insert(int index, const DesktopThumbnailItem &desktopThumbnailItem);

    // Remove data:
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void append();
    int count() const;

    QList<DesktopThumbnailItem> desktopThumbnailItemList() const {
        return m_desktopThumbnailItemList;
    }
	
	Q_INVOKABLE void setWindows(int screen, int desktop, QList<WId> &windows);
	Q_INVOKABLE QVariantList windows(int screen, int desktop) const; 

	Q_INVOKABLE int numScreens() const;
	Q_INVOKABLE QRect geometry(int screen) const;
	Q_INVOKABLE void setCurrentIndex(int index);
	Q_INVOKABLE int currentIndex() const;
signals:
    void countChanged(int count);
	void appendDesktop();
	void removeDesktop(int desktop);
	void currentIndexChanged(int currentIndex);

protected: // interface QAbstractListModel
    virtual QHash<int, QByteArray> roleNames() const;

private:
    QList<DesktopThumbnailItem> m_desktopThumbnailItemList;
	//QMap<screen, QMap<desktop, window>;
	QMap<int, QMap<int, QVariantList> > m_windows;
	int m_currentIndex;
};

#endif // DATAMODEL_H
