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
#include "chameleontheme.h"

#include <QSettings>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

#define BASE_THEME "deepin"
#define BASE_THEME_DIR ":/deepin/themes"

QPair<qreal, qreal> ChameleonTheme::takePair(const QVariant &value, const QPair<qreal, qreal> defaultValue)
{
    if (!value.isValid()) {
        return defaultValue;
    }

    QStringList l = value.toStringList();

    if (l.isEmpty()) {
        l = value.toString().split(",");
    }

    if (l.count() < 2) {
        return defaultValue;
    }

    QPair<qreal, qreal> ret;

    ret.first = l.first().toDouble();
    ret.second = l.at(1).toDouble();

    return ret;
}

QMarginsF ChameleonTheme::takeMargins(const QVariant &value, const QMarginsF &defaultValue)
{
    if (!value.isValid()) {
        return defaultValue;
    }

    QStringList l = value.toStringList();

    if (l.isEmpty()) {
        l = value.toString().split(",");
    }

    if (l.count() < 4) {
        return defaultValue;
    }

    return QMarginsF(l.at(0).toDouble(), l.at(1).toDouble(),
                     l.at(2).toDouble(), l.at(3).toDouble());
}

static QColor takeColor(const QVariant &value, const QColor &defaultValue)
{
    const QString &color_name = value.toString();

    QColor c(color_name);

    if (!c.isValid())
        return defaultValue;

    return c;
}

static inline QPointF toPos(const QPair<qreal, qreal> &pair)
{
    return QPointF(pair.first, pair.second);
}

QPointF ChameleonTheme::takePos(const QVariant &value, const QPointF defaultValue)
{
    return toPos(takePair(value, qMakePair(defaultValue.x(), defaultValue.y())));
}

static QIcon takeIcon(const QSettings &setting, QIcon base, const QString &key, QString defaultValue)
{
    if (!base.isNull()) {
        defaultValue.clear();
    }

    const QString normal = setting.value(key + ".normal", defaultValue + "_normal.svg").toString();
    const QString hover = setting.value(key + ".hover", defaultValue + "_hover.svg").toString();
    const QString press = setting.value(key + ".press", defaultValue + "_press.svg").toString();
    const QString disabled = setting.value(key + ".disabled", defaultValue + "_disabled.svg").toString();

    if (base.isNull()) {
        base.addFile(normal);
        base.addFile(hover, QSize(), QIcon::Active);
        base.addFile(press, QSize(), QIcon::Selected);
        base.addFile(disabled, QSize(), QIcon::Disabled);
    } else { // 开启fallback到base icon的行为
        if (!normal.startsWith("_"))
            base.addFile(normal);

        if (!hover.startsWith("_"))
            base.addFile(hover, QSize(), QIcon::Active);

        if (!press.startsWith("_"))
            base.addFile(press, QSize(), QIcon::Selected);

        if (!disabled.startsWith("_"))
            base.addFile(disabled, QSize(), QIcon::Disabled);
    }

    return base;
}

static void writeDecorationConfig(const QSettings &setting, ChameleonTheme::DecorationConfig &config, const ChameleonTheme::DecorationConfig *base = nullptr)
{
    config.borderWidth = setting.value("borderWidth", base ? base->borderWidth : 1.0).toDouble();
    config.shadowRadius = setting.value("shadowRadius", base ? base->shadowRadius : 60.0).toDouble();
    config.shadowOffset = ChameleonTheme::takePos(setting.value("shadowOffset"), base ? base->shadowOffset : QPointF(0.0, 16.0));
    config.windowRadius = ChameleonTheme::takePos(setting.value("windowRadius"), base ? base->windowRadius : QPointF(4.0, 4.0));
    config.mouseInputAreaMargins = ChameleonTheme::takeMargins(setting.value("mouseInputAreaMargins"), base ? base->mouseInputAreaMargins : QMarginsF(5, 5, 5, 5));

    // colors
    config.borderColor = takeColor(setting.value("borderColor"), base ? base->borderColor : QColor(0, 0, 0, 255 * 0.15));
    config.shadowColor = takeColor(setting.value("shadowColor"), base ? base->shadowColor : QColor(0, 0, 0, 255 * 0.6));
}

static void writeTitlebarConfig(const QSettings &setting, ChameleonTheme::TitleBarConfig &config, const ChameleonTheme::TitleBarConfig *base = nullptr)
{
    config.height = setting.value("height", base ? base->height : 40.0).toDouble();
    config.area = static_cast<Qt::Edge>(setting.value("area", base ? base->area : Qt::TopEdge).toInt());

    // colors
    config.textColor = takeColor(setting.value("textColor"), base ? base->textColor : QColor());
    config.backgroundColor = takeColor(setting.value("backgroundColor"), base ? base->backgroundColor : QColor());

    // icons
    config.menuIcon = takeIcon(setting, base ? base->menuIcon : QIcon(), "menuIcon", ":/deepin/themes/deepin/light/icons/menu");
    config.minimizeIcon = takeIcon(setting, base ? base->minimizeIcon : QIcon(), "minimizeIcon", ":/deepin/themes/deepin/light/icons/minimize");
    config.maximizeIcon = takeIcon(setting, base ? base->maximizeIcon : QIcon(), "maximizeIcon", ":/deepin/themes/deepin/light/icons/maximize");
    config.unmaximizeIcon = takeIcon(setting, base ? base->unmaximizeIcon : QIcon(), "unmaximizeIcon", ":/deepin/themes/deepin/light/icons/unmaximize");
    config.closeIcon = takeIcon(setting, base ? base->closeIcon : QIcon(), "closeIcon", ":/deepin/themes/deepin/light/icons/close");
}

static void writeConfig(QSettings *setting_decoration, QSettings *setting_titlebar, const QString &group,
                        ChameleonTheme::Config &config, const ChameleonTheme::Config *base = nullptr)
{
    if (setting_decoration) {
        if (base && !QFile::exists(setting_decoration->fileName())) {
            config.decoration = base->decoration;
        } else {
            setting_decoration->beginGroup(group);
            writeDecorationConfig(*setting_decoration, config.decoration, base ? &base->decoration : nullptr);
            setting_decoration->endGroup();
        }
    }

    if (setting_titlebar) {
        if (base && !QFile::exists(setting_titlebar->fileName())) {
            config.titlebar = base->titlebar;
        } else {
            setting_titlebar->beginGroup(group);
            writeTitlebarConfig(*setting_titlebar, config.titlebar, base ? &base->titlebar : nullptr);
            setting_titlebar->endGroup();
        }
    }
}

class _ChameleonTheme : public ChameleonTheme {
public:
    _ChameleonTheme() : ChameleonTheme() {}
};
Q_GLOBAL_STATIC(_ChameleonTheme, _global_ct)

ChameleonTheme *ChameleonTheme::instance()
{
    return _global_ct;
}

static bool loadTheme(ChameleonTheme::ConfigGroup *configs, const ChameleonTheme::ConfigGroup *base,
                      ChameleonTheme::ThemeType themeType, const QString &themeName, const QList<QDir> &themeDirList)
{
    QDir theme_dir("/");

    for (const QDir &dir : themeDirList) {
        for (const QFileInfo &info : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (info.fileName() == themeName) {
                theme_dir.setPath(info.filePath());
                break;
            }
        }
    }

    if (theme_dir.path() == "/")
        return false;

    QSettings setting_decoration(theme_dir.filePath(ChameleonTheme::typeString(themeType) + "/decoration.ini"), QSettings::IniFormat);
    QSettings setting_titlebar(theme_dir.filePath(ChameleonTheme::typeString(themeType) + "/titlebar.ini"), QSettings::IniFormat);

    if (base) {
        writeConfig(&setting_decoration, &setting_titlebar, "Active", configs->normal, &base->normal);
        writeConfig(&setting_decoration, &setting_titlebar, "Inactive", configs->inactive, &base->inactive);
        writeConfig(&setting_decoration, nullptr, "Unmanaged", configs->unmanaged, &base->unmanaged);
        writeConfig(&setting_decoration, &setting_titlebar, "NoAlpha/Active", configs->noAlphaNormal, &base->noAlphaNormal);
        writeConfig(&setting_decoration, &setting_titlebar, "NoAlpha/Inactive", configs->noAlphaInactive, &base->noAlphaInactive);
        writeConfig(&setting_decoration, nullptr, "NoAlpha/Unmanaged", configs->noAlphaUnmanaged, &base->noAlphaUnmanaged);
    } else {
        writeConfig(&setting_decoration, &setting_titlebar, "Active", configs->normal);
        writeConfig(&setting_decoration, &setting_titlebar, "Inactive", configs->inactive, &configs->normal);
        writeConfig(&setting_decoration, nullptr, "Unmanaged", configs->unmanaged, &configs->normal);
        writeConfig(&setting_decoration, &setting_titlebar, "NoAlpha/Active", configs->noAlphaNormal, &configs->normal);
        writeConfig(&setting_decoration, &setting_titlebar, "NoAlpha/Inactive", configs->noAlphaInactive, &configs->inactive);
        writeConfig(&setting_decoration, nullptr, "NoAlpha/Unmanaged", configs->noAlphaUnmanaged, &configs->unmanaged);
    }

    return true;
}

static bool formatThemeName(const QString &fullName, ChameleonTheme::ThemeType &type, QString &name)
{
    int split = fullName.indexOf("/");

    if (split > 0 && split < fullName.size() - 1) {
        type = ChameleonTheme::typeFromString(fullName.left(split));
        name = fullName.mid(split + 1);

        return true;
    }

    return false;
}

ChameleonTheme::ConfigGroupPtr ChameleonTheme::loadTheme(const QString &themeFullName, const QList<QDir> themeDirList)
{
    ThemeType type;
    QString name;

    if (!formatThemeName(themeFullName, type, name)) {
        return ConfigGroupPtr();
    }

    return loadTheme(type, name, themeDirList);
}

ChameleonTheme::ConfigGroupPtr ChameleonTheme::loadTheme(ThemeType themeType, const QString &themeName, const QList<QDir> themeDirList)
{
    auto base = getBaseConfig(themeType, themeDirList);

    if (themeName == BASE_THEME)
        return base;

    ConfigGroup *new_config = new ConfigGroup();
    bool ok = ::loadTheme(new_config, base.data(), themeType, themeName, themeDirList);

    if (ok) {
        return ConfigGroupPtr(new_config);
    } else {
        delete new_config;
    }

    return ConfigGroupPtr(nullptr);
}

ChameleonTheme::ConfigGroupPtr ChameleonTheme::getBaseConfig(ChameleonTheme::ThemeType type, const QList<QDir> &themeDirList)
{
    static ConfigGroupPtr base_configs[ThemeTypeCount];

    if (!base_configs[type]) {
        ConfigGroup *base = new ConfigGroup();
        // 先从默认路径加载最基本的主题
        ::loadTheme(base, nullptr, type, BASE_THEME, {QDir(BASE_THEME_DIR)});
        // 再尝试从其它路径加载主题，以允许基本主题中的值可以被外界覆盖
        ::loadTheme(base, base, type, BASE_THEME, themeDirList);
        // 将对应类型的基础主题缓存
        base_configs[type] = base;
    }

    return base_configs[type];
}

QString ChameleonTheme::typeString(ChameleonTheme::ThemeType type)
{
    return type == Dark ? "dark" : "light";
}

ChameleonTheme::ThemeType ChameleonTheme::typeFromString(const QString &type)
{
    if (type == "dark") {
        return Dark;
    }

    return Light;
}

QString ChameleonTheme::theme() const
{
    return m_theme;
}

bool ChameleonTheme::setTheme(const QString &themeFullName)
{
    ThemeType type;
    QString name;

    if (!formatThemeName(themeFullName, type, name)) {
        return false;
    }

    return setTheme(type, name);
}

bool ChameleonTheme::setTheme(ThemeType type, const QString &theme)
{
    if (m_type == type && m_theme == theme)
        return true;

    ConfigGroupPtr configs = loadTheme(type, theme, m_themeDirList);

    if (configs) {
        m_type = type;
        m_theme = theme;
        m_configGroup = configs;
    }

    return configs;
}

ChameleonTheme::ConfigGroupPtr ChameleonTheme::loadTheme(const QString &themeFullName)
{
    return loadTheme(themeFullName, m_themeDirList);
}

ChameleonTheme::ConfigGroupPtr ChameleonTheme::themeConfig() const
{
    return m_configGroup;
}

ChameleonTheme::ChameleonTheme()
{
    for (const QString &data_path : QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                              "deepin/themes",
                                                              QStandardPaths::LocateDirectory)) {
        m_themeDirList.prepend(QDir(data_path));
    }

    // 默认主题
    setTheme(ThemeType::Light, BASE_THEME);
}

ChameleonTheme::~ChameleonTheme()
{

}
