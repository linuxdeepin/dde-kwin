#include "imageprovider.h"
#include "multitasking_model.h"

ImageProvider::ImageProvider(QQmlImageProviderBase::ImageType type, QQmlImageProviderBase::Flags flags)
    : QQuickImageProvider(type, flags)
{

}

QPixmap ImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(size);
    Q_UNUSED(requestedSize);
    return  m_pMultitaskingModel->getWindowIcon( id );
}
