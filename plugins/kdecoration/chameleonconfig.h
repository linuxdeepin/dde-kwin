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
#ifndef CHAMELEONCONFIG_H
#define CHAMELEONCONFIG_H

#include <QObject>

#define _DEEPIN_CHAMELEON "_DEEPIN_CHAMELEON_THEME"
#define _DEEPIN_NO_TITLEBAR "_DEEPIN_NO_TITLEBAR"
#define _DEEPIN_SCISSOR_WINDOW "_DEEPIN_SCISSOR_WINDOW"
#define _KDE_NET_WM_SHADOW "_KDE_NET_WM_SHADOW"

namespace KWin {
class Client;
class EffectWindow;
}

class X11Shadow;
class ChameleonConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool activated  READ isActivated NOTIFY activatedChanged)
    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)

public:
    static ChameleonConfig *instance();

    quint32 atomDeepinChameleon() const;
    quint32 atomDeepinNoTitlebar() const;
    quint32 atomDeepinScissorWindow() const;

    bool isActivated() const;

    QString theme() const;

public slots:
    bool setTheme(QString theme);

signals:
    void activatedChanged(bool activated);
    void themeChanged(QString theme);
    void windowNoTitlebarPropertyChanged(quint32 windowId);

protected:
    explicit ChameleonConfig(QObject *parent = nullptr);

private slots:
    void onConfigChanged();
    void onClientAdded(KWin::Client *client);
    void onCompositingToggled(bool active);
    void onWindowPropertyChanged(quint32 windowId, quint32 atom);

    void updateClientX11Shadow();

private:
    void init();

    void setActivated(bool active);
    void buildKWinX11Shadow(QObject *client);
    void buildKWinX11ShadowForNoBorderWindows();
    void clearKWinX11ShadowForWindows();
    void clearX11ShadowCache();

    bool m_activated = false;
    QString m_theme;
#ifndef DISBLE_DDE_KWIN_XCB
    quint32 m_atom_deepin_chameleon;
    quint32 m_atom_deepin_no_titlebar;
    quint32 m_atom_deepin_scissor_window;
#endif
    quint32 m_atom_kde_net_wm_shadow;
    X11Shadow *m_x11ShadowCache[2] = {nullptr};
};

#endif // CHAMELEONCONFIG_H
