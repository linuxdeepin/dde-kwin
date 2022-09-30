// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "backgroundimageprovider.h"
#include "background.h"
#include <QDebug>

BackgroundImageProvider::BackgroundImageProvider(QQmlImageProviderBase::ImageType type, QQmlImageProviderBase::Flags flags)
    : QQuickImageProvider(type, flags)
{

}

QPixmap BackgroundImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    QStringList tmpLst = id.split("/");
    if (tmpLst.count() == 2) {
        return BackgroundManager::instance().getBackgroundPixmap(tmpLst.at(0).toInt(),tmpLst.at(1));
    } else {
        QPixmap pixmap;
        return pixmap;
    }
}
