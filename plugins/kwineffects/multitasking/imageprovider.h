// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMAGEPROVIDER_H
#define IMAGEPROVIDER_H

#include <QQuickImageProvider>

#include "multitasking_model.h"

class ImageProvider : public QQuickImageProvider
{
//    Q_OBJECT
public:
    ImageProvider(QQmlImageProviderBase::ImageType type, QQmlImageProviderBase::Flags flags = Flags());

    virtual QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    MultitaskingModel *m_pMultitaskingModel;
};

#endif // IMAGEPROVIDER_H
