#ifndef BACKGROUNDIMAGEPROVIDER_H
#define BACKGROUNDIMAGEPROVIDER_H

#include <QQuickImageProvider>

class BackgroundImageProvider : public QQuickImageProvider
{

public:
    BackgroundImageProvider(QQmlImageProviderBase::ImageType type, QQmlImageProviderBase::Flags flags = Flags());

    virtual QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);

private:

};

#endif // BACKGROUNDIMAGEPROVIDER_H
