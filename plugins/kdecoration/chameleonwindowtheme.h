// Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CHAMELEONWINDOWTHEME_H
#define CHAMELEONWINDOWTHEME_H

#include <QObject>
#include <QMarginsF>
#include <QScreen>

class ChameleonWindowTheme : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qint64 validProperties READ validProperties WRITE setValidProperties NOTIFY validPropertiesChanged)
    Q_PROPERTY(QString theme READ theme NOTIFY themeChanged)
    Q_PROPERTY(QPointF windowRadius READ windowRadius WRITE setWindowRadius NOTIFY windowRadiusChanged)
    Q_PROPERTY(qreal borderWidth READ borderWidth NOTIFY borderWidthChanged)
    Q_PROPERTY(QColor borderColor READ borderColor NOTIFY borderColorChanged)
    Q_PROPERTY(qreal shadowRadius READ shadowRadius NOTIFY shadowRadiusChanged)
    Q_PROPERTY(QPointF shadowOffset READ shadowOffset  NOTIFY shadowOffectChanged)
    Q_PROPERTY(QColor shadowColor READ shadowColor NOTIFY shadowColorChanged)
    Q_PROPERTY(QMarginsF mouseInputAreaMargins READ mouseInputAreaMargins NOTIFY mouseInputAreaMarginsChanged)
    Q_PROPERTY(qreal windowPixelRatio READ windowPixelRatio NOTIFY windowPixelRatioChanged)

public:
    enum PropertyFlag {
        ThemeProperty = 0x02,
        WindowRadiusProperty = 0x04,
        BorderWidthProperty = 0x08,
        BorderColorProperty = 0x10,
        ShadowRadiusProperty = 0x20,
        ShadowOffsetProperty = 0x40,
        ShadowColorProperty = 0x80,
        MouseInputAreaMargins = 0x100,
        WindowPixelRatioProperty = 0x200
    };
    Q_DECLARE_FLAGS(PropertyFlags, PropertyFlag)
    Q_FLAG(PropertyFlags)

    explicit ChameleonWindowTheme(QObject *window, QObject *parent = nullptr);

    PropertyFlags validProperties() const;
    bool propertyIsValid(PropertyFlag p) const;

    QString theme() const;
    QPointF windowRadius() const;
    void setWindowRadius(const QPointF value);
    qreal borderWidth() const;
    QColor borderColor() const;
    qreal shadowRadius() const;
    QPointF shadowOffset() const;
    QColor shadowColor() const;
    QMarginsF mouseInputAreaMargins() const;
    qreal windowPixelRatio() const;

public slots:
    void setValidProperties(qint64 validProperties);

signals:
    void validPropertiesChanged(qint64 validProperties);
    void themeChanged();
    void windowRadiusChanged();
    void borderWidthChanged();
    void borderColorChanged();
    void shadowRadiusChanged();
    void shadowOffectChanged();
    void shadowColorChanged();
    void mouseInputAreaMarginsChanged();
    void windowPixelRatioChanged();

private:
    void updateScreen();
    void updateScreenScale();

    QObject *m_window = nullptr;
    QScreen *m_screen = nullptr;
    PropertyFlags m_validProperties;
    qreal m_windowPixelRatio = 1.0;
    QPointF m_windowRadius = QPointF(0.0,0.0);
};

Q_DECLARE_METATYPE(QMarginsF)
#endif // CHAMELEONWINDOWTHEME_H
