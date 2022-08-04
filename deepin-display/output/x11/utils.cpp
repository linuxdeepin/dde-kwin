/*
 *  SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "utils.h"

#include <QVector>

AbstractOutput::Type Utils::guessOutputType(const QString &type, const QString &name)
{
    static const auto embedded = {QLatin1String("LVDS"), QLatin1String("IDP"), QLatin1String("EDP"), QLatin1String("LCD"), QLatin1String("DSI")};

    for (const QLatin1String &pre : embedded) {
        if (name.startsWith(pre, Qt::CaseInsensitive)) {
            return AbstractOutput::Panel;
        }
    }

    if (type.contains(QLatin1String("VGA"))) {
        return AbstractOutput::VGA;
    } else if (type.contains(QLatin1String("DVI"))) {
        return AbstractOutput::DVI;
    } else if (type.contains(QLatin1String("DVI-I"))) {
        return AbstractOutput::DVII;
    } else if (type.contains(QLatin1String("DVI-A"))) {
        return AbstractOutput::DVIA;
    } else if (type.contains(QLatin1String("DVI-D"))) {
        return AbstractOutput::DVID;
    } else if (type.contains(QLatin1String("HDMI"))) {
        return AbstractOutput::HDMI;
    } else if (type.contains(QLatin1String("Panel"))) {
        return AbstractOutput::Panel;
    } else if (type.contains(QLatin1String("TV-Composite"))) {
        return AbstractOutput::TVComposite;
    } else if (type.contains(QLatin1String("TV-SVideo"))) {
        return AbstractOutput::TVSVideo;
    } else if (type.contains(QLatin1String("TV-Component"))) {
        return AbstractOutput::TVComponent;
    } else if (type.contains(QLatin1String("TV-SCART"))) {
        return AbstractOutput::TVSCART;
    } else if (type.contains(QLatin1String("TV-C4"))) {
        return AbstractOutput::TVC4;
    } else if (type.contains(QLatin1String("TV"))) {
        return AbstractOutput::TV;
    } else if (type.contains(QLatin1String("DisplayPort")) || type.startsWith(QLatin1String("DP"))) {
        return AbstractOutput::DisplayPort;
    } else if (type.contains(QLatin1String("unknown"))) {
        return AbstractOutput::Unknown;
    } else {
        return AbstractOutput::Unknown;
    }
}
