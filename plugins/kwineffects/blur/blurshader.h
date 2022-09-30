// Copyright © 2010 Fredrik Höglund <fredrik@kde.org>
// Copyright © 2018 Alex Nemeth <alex.nemeth329@gmail.com>
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BLURSHADER_H
#define BLURSHADER_H

#include <kwinglutils.h>

#include <QMatrix4x4>
#include <QObject>
#include <QScopedPointer>
#include <QVector2D>
#include <QVector4D>

using namespace KWin;

class Q_DECL_HIDDEN BlurShader : public QObject
{
    Q_OBJECT

public:
    BlurShader(QObject *parent = nullptr);
    ~BlurShader() override;

    bool isValid() const;

    enum SampleType {
        DownSampleType,
        UpSampleType,
        CopySampleType,
        NoiseSampleType,
        SaturationSampleType
    };

    void bind(SampleType sampleType);
    void unbind();

    void setModelViewProjectionMatrix(const QMatrix4x4 &matrix);
    void setOffset(float offset);
    void setTargetTextureSize(const QSize &renderTextureSize);
    void setNoiseTextureSize(const QSize &noiseTextureSize);
    void setTexturePosition(const QPoint &texPos);
    void setBlurRect(const QRect &blurRect, const QSize &screenSize);
    void setSaturation(float sat);

private:
    QScopedPointer<GLShader> m_shaderDownsample;
    QScopedPointer<GLShader> m_shaderUpsample;
    QScopedPointer<GLShader> m_shaderCopysample;
    QScopedPointer<GLShader> m_shaderNoisesample;
    QScopedPointer<GLShader> m_shaderSaturationsample;

    int m_mvpMatrixLocationDownsample;
    int m_offsetLocationDownsample;
    int m_renderTextureSizeLocationDownsample;
    int m_halfpixelLocationDownsample;

    int m_mvpMatrixLocationUpsample;
    int m_offsetLocationUpsample;
    int m_renderTextureSizeLocationUpsample;
    int m_halfpixelLocationUpsample;

    int m_mvpMatrixLocationCopysample;
    int m_renderTextureSizeLocationCopysample;
    int m_blurRectLocationCopysample;

    int m_mvpMatrixLocationNoisesample;
    int m_offsetLocationNoisesample;
    int m_renderTextureSizeLocationNoisesample;
    int m_noiseTextureSizeLocationNoisesample;
    int m_texStartPosLocationNoisesample;
    int m_halfpixelLocationNoisesample;

    int m_mvpMatrixLocationSaturationsample;
    int m_renderTextureSizeLocationSaturationsample;
    int m_satLocationSaturationsample;

    //Caching uniform values to aviod unnecessary setUniform calls
    int m_activeSampleType = -1;

    float m_offsetDownsample = 0.0;
    QMatrix4x4 m_matrixDownsample;

    float m_offsetUpsample = 0.0;
    QMatrix4x4 m_matrixUpsample;

    QMatrix4x4 m_matrixCopysample;

    float m_offsetNoisesample = 0.0;
    QVector2D m_noiseTextureSizeNoisesample;
    QMatrix4x4 m_matrixNoisesample;

    QMatrix4x4 m_matrixSaturationsample;

    bool m_valid = false;

    Q_DISABLE_COPY(BlurShader);
};

inline bool BlurShader::isValid() const
{
    return m_valid;
}

#endif
