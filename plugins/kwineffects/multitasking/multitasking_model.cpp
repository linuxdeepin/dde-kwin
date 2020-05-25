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


#include "multitasking_model.h"

DesktopThumbnailItem::DesktopThumbnailItem()
{

}

MultitaskingModel::MultitaskingModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

MultitaskingModel::~MultitaskingModel()
{
}

int MultitaskingModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_desktopThumbnailItemList.count();
}

QVariant MultitaskingModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row < 0 || row >= m_desktopThumbnailItemList.count()) {
        return QVariant();
    }

    const DesktopThumbnailItem &data = m_desktopThumbnailItemList[row];
    switch (role) {
    case ThumbnailRole:
//        return data.thumbnail();
    default:
        break;
    }

    return QVariant();
}



QHash<int, QByteArray> MultitaskingModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ThumbnailRole] = "dmThumbnail";
    return roles;
}

void MultitaskingModel::setWindows(int screen, int desktop, QList<WId> &windows)
{
	QVariantList windowList;
	for (auto wid : windows) {
		windowList.append(wid);
	}
	m_windows[screen][desktop] = windowList;
}

QVariantList MultitaskingModel::windows(int screen, int desktop) const 
{
	return m_windows[screen][desktop];
}


int MultitaskingModel::numScreens() const
{
	return effects->numScreens();
}

QRect MultitaskingModel::geometry(int screen) const
{
	QDesktopWidget desktop;
	return desktop.screenGeometry(screen);
}

void MultitaskingModel::insert(int index, const DesktopThumbnailItem &data)
{
    if(index < 0 || index >= 4) {
        return;
    }

    emit beginInsertRows(QModelIndex(), index, index);
    m_desktopThumbnailItemList.append(data);
    emit endInsertRows();
	emit appendDesktop();
    emit countChanged(m_desktopThumbnailItemList.count());
}

void MultitaskingModel::remove(int index)
{
    if(index < 0 || index >= m_desktopThumbnailItemList.count()) {
        return;
    }

    if (m_desktopThumbnailItemList.count() == 1)
    {
        return;
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_desktopThumbnailItemList.removeAt(index);
    endRemoveRows();
	emit removeDesktop(index + 1);
    emit countChanged(m_desktopThumbnailItemList.count());
}


void MultitaskingModel::append()
{
    const QString &filePath = "/home/mac/Workspace/untitled6/abc-123.jpg";
    insert(count(), DesktopThumbnailItem());
}


int MultitaskingModel::count() const
{
    return rowCount(QModelIndex());
}

