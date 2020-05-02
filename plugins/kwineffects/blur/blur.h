/*
 *   Copyright © 2010 Fredrik Höglund <fredrik@kde.org>
 *   Copyright © 2018 Alex Nemeth <alex.nemeth329@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; see the file COPYING.  if not, write to
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *   Boston, MA 02110-1301, USA.
 */

#ifndef BLUR_H
#define BLUR_H

#include <kwineffects.h>
#include <kwinglplatform.h>
#include <kwinglutils.h>

#include <QVector>
#include <QVector2D>
#include <QStack>

using namespace KWin;

namespace KWayland
{
namespace Server
{
class BlurManagerInterface;
}
}

static const int borderSize = 5;

class BlurShader;

class Q_DECL_HIDDEN BlurEffect : public KWin::Effect
{
    Q_OBJECT

public:
    BlurEffect(QObject *parent = nullptr, const QVariantList &args = QVariantList());
    ~BlurEffect() override;

    static bool supported();
    static bool enabledByDefault();

    void reconfigure(ReconfigureFlags flags) override;
    void prePaintScreen(ScreenPrePaintData &data, int time) override;
    void prePaintWindow(EffectWindow* w, WindowPrePaintData& data, int time) override;
    void paintWindow(EffectWindow *w, int mask, QRegion region, WindowPaintData &data) override;
    void paintEffectFrame(EffectFrame* frame, const QRegion &region, double opacity, double frameOpacity) override;

    bool provides(Feature feature) override;

    int requestedEffectChainPosition() const override {
        return 75;
    }

    bool eventFilter(QObject *watched, QEvent *event) override;

public Q_SLOTS:
    void slotWindowAdded(KWin::EffectWindow *w);
    void slotWindowDeleted(KWin::EffectWindow *w);
    void slotPropertyNotify(KWin::EffectWindow *w, long atom);
    void slotScreenGeometryChanged();

private:
    QRect expand(const QRect &rect) const;
    QRegion expand(const QRegion &region) const;
    bool renderTargetsValid() const;
    void deleteFBOs();
    void initBlurStrengthValues();
    void updateTexture();
    QRegion blurRegion(const EffectWindow *w) const;
    bool shouldBlur(const EffectWindow *w, int mask, const WindowPaintData &data) const;
    void updateBlurRegion(EffectWindow *w) const;
    void doBlur(const QRegion &shape, const QRect &screen, const float opacity, const QMatrix4x4 &screenProjection, bool isDock, QRect windowRect);
    void uploadRegion(QVector2D *&map, const QRegion &region, const int downSampleIterations);
    void uploadGeometry(GLVertexBuffer *vbo, const QRegion &blurRegion, const QRegion &windowRegion);
    void generateNoiseTexture();

    void upscaleRenderToScreen(GLVertexBuffer *vbo, int vboStart, int blurRectCount, QMatrix4x4 screenProjection, QPoint windowPosition);
    void downSampleTexture(GLVertexBuffer *vbo, int blurRectCount);
    void upSampleTexture(GLVertexBuffer *vbo, int blurRectCount);
    void copyScreenSampleTexture(GLVertexBuffer *vbo, int blurRectCount, QRegion blurShape, QMatrix4x4 screenProjection);
    void doSaturation(GLVertexBuffer* vbo, int start, int blurRectCount, float sat, QMatrix4x4 screenProjection);

private:
    BlurShader *m_shader;
    QVector <GLRenderTarget*> m_renderTargets;
    QVector <GLTexture> m_renderTextures;
    QStack <GLRenderTarget*> m_renderTargetStack;

    GLTexture m_noiseTexture;

    bool m_renderTargetsValid;
    long net_wm_blur_region;
    QRegion m_damagedArea; // keeps track of the area which has been damaged (from bottom to top)
    QRegion m_paintedArea; // actually painted area which is greater than m_damagedArea
    QRegion m_currentBlur; // keeps track of the currently blured area of the windows(from bottom to top)

    int m_downSampleIterations; // number of times the texture will be downsized to half size
    int m_offset;
    int m_expandSize;
    int m_noiseStrength;
    int m_scalingFactor;

    struct OffsetStruct {
        float minOffset;
        float maxOffset;
        int expandSize;
    };

    QVector <OffsetStruct> blurOffsets;

    struct BlurValuesStruct {
        int iteration;
        float offset;
    };

    QVector <BlurValuesStruct> blurStrengthValues;

    QMap <EffectWindow*, QMetaObject::Connection> windowBlurChangedConnections;
    KWayland::Server::BlurManagerInterface *m_blurManager = nullptr;
};

inline
bool BlurEffect::provides(Effect::Feature feature)
{
    if (feature == Blur) {
        return true;
    }
    return KWin::Effect::provides(feature);
}

#endif

