#include "imageprovider.h"
#include "multitasking_model.h"

#include <QDebug>

ImageProvider::ImageProvider(QQmlImageProviderBase::ImageType type, QQmlImageProviderBase::Flags flags)
    : QQuickImageProvider(type, flags)
{

}

QPixmap ImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
//    EffectWindow *ew = effects->findWindow(winId.toULongLong());
//    QString strIconPath;
//    if(ew)
//    {
//        qDebug() << "---------242" <<ew->icon().pixmap(QSize(48,48));
//    }
    return  m_pMultitaskingModel->getWindowIcon( id );

//    QString strFileName = "/home/uos/Desktop/Face.png";
//    qDebug() << strFileName;
//    QPixmap pixmap(strFileName);
//    return pixmap;
}
