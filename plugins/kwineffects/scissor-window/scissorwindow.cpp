/*
 * Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@live.com>
 *
 * Maintainer: zccrs <zhangjide@deepin.com>
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
#include "scissorwindow.h"
#ifndef DISBLE_DDE_KWIN_XCB
#include "kwinutils.h"
#endif

#include <kwinglutils.h>
#include <kwinglplatform.h>
#include <kwingltexture.h>

#include <QPainter>
#include <QPainterPath>
#include <QExplicitlySharedDataPointer>
#include <QSignalBlocker>
#include <QPainterPath>

Q_DECLARE_METATYPE(QPainterPath)

class Q_DECL_HIDDEN MaskCache {
public:
    class Texture : public QSharedData, public KWin::GLTexture {
    public:
        Texture(const QImage &mask, bool custom)
            : KWin::GLTexture(mask)
            , cacheKey(mask.cacheKey())
            , customMask(custom)
            , size(mask.size()) {
            MaskCache::instance()->m_cache[cacheKey] = this;
        }

        ~Texture() {
            MaskCache::instance()->m_cache.remove(cacheKey);
        }

        qint64 cacheKey;
        bool customMask;
        QSize size;
    };

    typedef QExplicitlySharedDataPointer<Texture> TextureData;

    static MaskCache *instance()
    {
        static MaskCache *self = new MaskCache();

        return self;
    }

    TextureData getTextureByWindow(KWin::EffectWindow *w)
    {
        const QVariant &data_texture = w->data(ScissorWindow::WindowMaskTextureRole);

        if (data_texture.isValid()) {
            return qvariant_cast<TextureData>(data_texture);
        }

        // 先查看窗口有没有自定义设置裁剪图片
        const QVariant &data_clip_path = w->data(ScissorWindow::WindowClipPathRole);
        static int path_type = qMetaTypeId<QPainterPath>();

        if (data_clip_path.userType() == path_type) {
            const QPainterPath path = qvariant_cast<QPainterPath>(data_clip_path);

            if (path.isEmpty())
                return TextureData();

            QImage mask(w->size(), QImage::Format_ARGB32);
            mask.fill(Qt::transparent);
            QPainter pa(&mask);
            pa.setRenderHint(QPainter::Antialiasing);
            // 必须填充为白色，在着色器中运算时会使用rgb三个通道相乘
            pa.fillPath(path, Qt::white);
            pa.end();

            // 先从缓存中查找材质
            Texture *texture = m_cache.value(mask.cacheKey());

            if (!texture) {
                texture = new Texture(mask, true);
                texture->setFilter(GL_LINEAR);
                texture->setWrapMode(GL_CLAMP_TO_BORDER);
            }

            // 为窗口保存mask材质
            TextureData data(texture);
            w->setData(ScissorWindow::WindowMaskTextureRole, QVariant::fromValue(data));

            return data;
        }

        const QVariant &data_radius = w->data(ScissorWindow::WindowRadiusRole);

        if (!data_radius.isValid())
            return TextureData();

        QPointF window_radius = data_radius.toPointF();

        const int RR = 17;
        if (KWin::effects->waylandDisplay()) {
            if (w->windowClass().contains("Deepin") || w->windowClass().contains("dde-")) {
                window_radius.setX(RR);
                window_radius.setY(RR);
            } else if(w->isTooltip()) {
                window_radius.setX(RR);
                window_radius.setY(RR);
            } else if(w->isDock()) {
                window_radius.setX(RR);
                window_radius.setY(RR);
            }
            else {
            }
        }

        // 排除无效的数据
        if (qIsNull(window_radius.x()) || qIsNull(window_radius.y()))
            return TextureData();

        // 用于获取材质缓存key的key
        auto to_cache_key_key = (qRound(window_radius.x()) << 16) | qRound(window_radius.y());

        Texture *texture = nullptr;

        if (m_radiusToCacheKey.contains(to_cache_key_key)) {
            texture = m_cache.value(m_radiusToCacheKey.value(to_cache_key_key));
        }

        if (!texture) {
            QImage mask(QSize(window_radius.x(), window_radius.y()), QImage::Format_ARGB32);
            mask.fill(Qt::transparent);
            QPainter pa(&mask);
            pa.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.moveTo(window_radius);
            path.arcTo(0, 0, window_radius.x() * 2,  window_radius.y() * 2, 90, 90);
            path.lineTo(window_radius);
            path.closeSubpath();
            // 必须填充为白色，在着色器中运算时会使用rgb三个通道相乘
            pa.fillPath(path, Qt::white);

            texture = new Texture(mask, false);
            texture->setFilter(GL_NEAREST);
            texture->setWrapMode(GL_CLAMP_TO_BORDER);

            m_radiusToCacheKey[to_cache_key_key] = texture->cacheKey;
        }

        // 为窗口保存mask材质
        TextureData data(texture);
        w->setData(ScissorWindow::WindowMaskTextureRole, QVariant::fromValue(data));

        return data;
    }

private:
    MaskCache() {

    }

    QHash<qint64, Texture*> m_cache;
    QMap<int, qint64> m_radiusToCacheKey;

    friend class Texture;
};

Q_DECLARE_METATYPE(MaskCache::TextureData)

bool ScissorWindow::supported()
{
    bool supported = KWin::effects->isOpenGLCompositing() && KWin::GLRenderTarget::supported() && KWin::GLRenderTarget::blitSupported();

    return supported;
}

ScissorWindow::ScissorWindow(QObject *, const QVariantList &)
    : Effect()
{
    // 构建用于窗口圆角特效的着色器
    m_shader = KWin::ShaderManager::instance()->generateShaderFromResources(KWin::ShaderTrait::MapTexture, QString(), "corner-mask.frag");
    // 构建用于自定义窗口形状的着色器
    m_fullMaskShader = KWin::ShaderManager::instance()->generateShaderFromResources(KWin::ShaderTrait::MapTexture, QString(), "full-mask.frag");

    if (!m_shader->isValid()) {
        // qWarning() << Q_FUNC_INFO << "Invalid fragment shader of corner mask";
    }

    if (!m_fullMaskShader->isValid()) {
        // qWarning() << Q_FUNC_INFO << "Invalid fragment shader of full mask";
    }
}

#if KWIN_VERSION_MIN > 17 || (KWIN_VERSION_MIN == 17 && KWIN_VERSION_PAT > 5)
void ScissorWindow::drawWindow(KWin::EffectWindow *w, int mask, const QRegion &_region, KWin::WindowPaintData &data)
{
    QRegion region = _region;
#else
void ScissorWindow::drawWindow(KWin::EffectWindow *w, int mask, QRegion region, KWin::WindowPaintData &data)
{
#endif
    // 工作区特效会使用PAINT_WINDOW_LANCZOS绘制，此时不支持多次调用Effect::drawWindow，
    // 否则只会显示第一次调用绘制的内容, 因此在这种模式下禁用掉窗口裁剪特效
    if (!w->isPaintingEnabled() || (mask & PAINT_WINDOW_LANCZOS) || w->isDesktop()
    || NET::WindowType::Override == w->windowType() || NET::WindowType::OnScreenDisplay == w->windowType()) {
        return Effect::drawWindow(w, mask, region, data);
    }

    MaskCache::TextureData mask_texture = MaskCache::instance()->getTextureByWindow(w);

    if (!mask_texture) {
        return Effect::drawWindow(w, mask, region, data);
    }

    QRegion corner_region;

    if (!mask_texture->customMask) {
        const QRect window_rect = w->geometry();
        QRect corner_rect(window_rect.topLeft(), mask_texture->size);

        // top left
        corner_region += corner_rect;
        // top right
        corner_rect.moveRight(window_rect.right());
        corner_region += corner_rect;
        // bottom right
        corner_rect.moveBottom(window_rect.bottom());
        corner_region += corner_rect;
        // bottom left
        corner_rect.moveLeft(window_rect.left());
        corner_region += corner_rect;

        // 本次绘制未包含圆角区域时则直接按原有的行为渲染
        if ((region & corner_region).isEmpty()) {
            return Effect::drawWindow(w, mask, region, data);
        }

        // 窗口发生几何转换时不能拆分绘制窗口的区域，否则会导致两个区域不契合，例如开启wobbly windows窗口特效,
        // 移动窗口时会导致窗口圆角出现毛刺
        if (mask & PAINT_WINDOW_TRANSFORMED) {
            corner_region = QRegion();
        }
    }

    KWin::WindowQuadList decoration_quad_list;
    KWin::WindowQuadList content_quad_list;

    for (const KWin::WindowQuad &quad : data.quads) {
        switch (quad.type()) {
        case KWin::WindowQuadShadow:
        case KWin::WindowQuadDecoration:
            decoration_quad_list.append(quad);
            break;
        case KWin::WindowQuadContents:
            content_quad_list.append(quad);
            break;
        default:
            break;
        }
    }

    if (!mask_texture->customMask) {
        // 此时只允许绘制窗口边框和阴影
        // 针对设置了自定义裁剪的窗口，则不绘制标题栏和阴影
        data.quads = decoration_quad_list;

        if (KWin::effects->waylandDisplay()) {
            if (!w->isDock()) {
                Effect::drawWindow(w, mask, region, data);
            }
        } else {
            Effect::drawWindow(w, mask, region, data);
        }
    }

    if (!corner_region.isEmpty()) {
        QRegion new_region = region - corner_region;

        // 先绘制未处于mask区域的窗口材质
        if (!new_region.isEmpty()) {
            data.quads = content_quad_list;
            Effect::drawWindow(w, mask, new_region, data);
        }

        // 重新设置要绘制的区域
        region = region - new_region;
    }

    // 将mask材质绑定到第二个材质
    glActiveTexture(GL_TEXTURE1);
    mask_texture->bind();

    // 对于窗口圆角的材质，由于其需要被访问材质范围外的像素，在此设置范围外材质的颜色
    // 其中alpha通道为1表示此处的窗口像素完全显示
    if (!mask_texture->customMask) {
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    }

    // 重新激活第一个材质
    glActiveTexture(GL_TEXTURE0);

    // 激活着色器
    KWin::GLShader *shader = mask_texture->customMask ? m_fullMaskShader : m_shader;
    KWin::ShaderManager::instance()->pushShader(shader);
    shader->setUniform("mask", 1);

    if (!mask_texture->customMask) {
        shader->setUniform("scale", QVector2D(w->width() / qreal(mask_texture->size.width()), w->height() / qreal(mask_texture->size.height())));
    }

    // 此时只允许绘制窗口内容
    auto old_shader = data.shader;
    data.quads = content_quad_list;
    data.shader = shader;

#ifndef DISBLE_DDE_KWIN_XCB
    class SetWindowDepth {
    public:
        SetWindowDepth(KWin::EffectWindow *w, int depth)
            : m_window(w)
        {
            // 此时正在进行窗口绘制，会有大量的调用，应当避免窗口发射hasAlphaChanged信号
            QSignalBlocker blocker(w->parent());
            Q_UNUSED(blocker)
            KWinUtils::setClientDepth(w->parent(), depth);
        }

        ~SetWindowDepth() {
            bool ok = false;
            int depth = m_window->data(WindowDepthRole).toInt(&ok);
            QObject *client = m_window->parent();

            if (!ok) {
                depth = KWinUtils::getWindowDepth(client);
                // 保存以便下次使用
                m_window->setData(WindowDepthRole, depth);
            }

            // 此时正在进行窗口绘制，会有大量的调用，应当避免窗口发射hasAlphaChanged信号
            QSignalBlocker blocker(client);
            Q_UNUSED(blocker)
            KWinUtils::setClientDepth(client, depth);
        }

    private:
        KWin::EffectWindow *m_window;
    };

    // 要想窗口裁剪生效，必须要保证窗口材质绘制时开启了alpha通道混合
    if (!w->hasAlpha()) {
        SetWindowDepth set_depth(w, 32);
        Q_UNUSED(set_depth)
        Effect::drawWindow(w, mask, region, data);
    } else
#endif
    {
        Effect::drawWindow(w, mask, region, data);
    }

    data.shader = old_shader;

    KWin::ShaderManager::instance()->popShader();
    // 解除材质绑定
    glActiveTexture(GL_TEXTURE1);
    mask_texture->unbind();
    glActiveTexture(GL_TEXTURE0);
}
