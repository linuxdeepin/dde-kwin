/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright 2018 Roman Gilg <subdiff@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "abstract_output.h"
#include "wayland_server.h"
#include "screens.h"

// KWayland
#include <KWayland/Server/display.h>
#include <KWayland/Server/outputchangeset.h>
#include <KWayland/Server/xdgoutput_interface.h>
// KF5
#include <KLocalizedString>

#include <cmath>

namespace KWin
{

AbstractOutput::AbstractOutput(QObject *parent)
    : QObject(parent)
{
    m_waylandOutput = waylandServer()->display()->createOutput(this);
    m_waylandOutputDevice = waylandServer()->display()->createOutputDevice(this);
    m_xdgOutput = waylandServer()->xdgOutputManager()->createXdgOutput(m_waylandOutput, this);
    connect(m_waylandOutput, &KWayland::Server::Global::aboutToDestroyGlobal, this,
            [this]() {
            qDebug() << "------ about to destroy output global" << m_waylandOutputDevice->uuid();
        });

    connect(m_waylandOutput, &KWayland::Server::OutputInterface::dpmsModeRequested, this,
        [this] (KWayland::Server::OutputInterface::DpmsMode mode) {
        if (mode == KWayland::Server::OutputInterface::DpmsMode::On) {
            qDebug() << "-------" << "dpmsModeRequested on" << m_waylandOutput;
            QTimer::singleShot(50, this, [=] { updateDpms(mode); });
        } else {
            qDebug() << "-------" << "dpmsModeRequested off" << m_waylandOutput;
            updateDpms(mode);
        }
        }, Qt::DirectConnection
    );
}

AbstractOutput::~AbstractOutput()
{
}

QString AbstractOutput::name() const
{
    if (!m_waylandOutput) {
        return i18n("unknown");
    }
    return QStringLiteral("%1 %2").arg(m_waylandOutput->manufacturer()).arg(m_waylandOutput->model());
}

QRect AbstractOutput::geometry() const
{
    return QRect(globalPos(), pixelSize() / scale());
}

QSize AbstractOutput::physicalSize() const
{
    return orientateSize(m_physicalSize);
}

int AbstractOutput::refreshRate() const
{
    if (!m_waylandOutput) {
        return 60000;
    }
    return m_waylandOutput->refreshRate();
}

void AbstractOutput::setGlobalPos(const QPoint &pos)
{
    if (!isEnabled()) return;
    m_waylandOutputDevice->setGlobalPosition(pos);

    m_waylandOutput->setGlobalPosition(pos);
    m_xdgOutput->setLogicalPosition(pos);
    m_xdgOutput->done();
}

void AbstractOutput::setScale(qreal scale)
{
    if (!isEnabled()) return;
    m_waylandOutputDevice->setScaleF(scale);

    // this is the scale that clients will ideally use for their buffers
    // this has to be an int which is fine

    // I don't know whether we want to round or ceil
    // or maybe even set this to 3 when we're scaling to 1.5
    // don't treat this like it's chosen deliberately
    m_waylandOutput->setScale(std::ceil(scale));
    m_xdgOutput->setLogicalSize(pixelSize() / scale);
    m_xdgOutput->done();
}

void AbstractOutput::setChanges(KWayland::Server::OutputChangeSet *changes)
{
    qCDebug(KWIN_CORE) << "Set changes in AbstractOutput." << m_waylandOutputDevice->uuid();

    bool updated = false;
    bool overallSizeCheckNeeded = false;

    if (!changes) {
        qCDebug(KWIN_CORE) << "No changes.";
        // No changes to an output is an entirely valid thing
        return;
    }
    //enabledChanged is handled by plugin code
    if (changes->modeChanged()) {
        qCDebug(KWIN_CORE) << "Setting new mode:" << changes->mode();
        m_waylandOutputDevice->setCurrentMode(changes->mode());
        updateMode(changes->mode());
        updated = true;
    }
    if (changes->transformChanged()) {
        qCDebug(KWIN_CORE) << "Server setting transform: " << (int)(changes->transform());
        transform(changes->transform());
        updated = true;
    }
    if (changes->positionChanged()) {
        qCDebug(KWIN_CORE) << "Server setting position: " << changes->position();
        setGlobalPos(changes->position());
        // may just work already!
        overallSizeCheckNeeded = true;
    }
    if (changes->scaleChanged()) {
        qCDebug(KWIN_CORE) << "Setting scale:" << changes->scale();
        setScale(changes->scaleF());
        updated = true;
    }

    overallSizeCheckNeeded |= updated;
    if (overallSizeCheckNeeded) {
        emit screens()->changed();
    }

    if (updated) {
        emit modeChanged();
    }
}

void AbstractOutput::setEnabled(bool enable)
{
    if (enable == isEnabled()) {
        return;
    }

    qDebug() << "-------- " << __func__ << enable << this;
    if (enable) {
        m_waylandOutputDevice->setEnabled(KWayland::Server::OutputDeviceInterface::Enablement::Enabled);
        m_waylandOutput->create();
        updateEnablement(true);
    } else {
        m_waylandOutputDevice->setEnabled(KWayland::Server::OutputDeviceInterface::Enablement::Disabled);
        m_waylandOutput->destroy();
        updateEnablement(false);
    }
}

void AbstractOutput::setWaylandMode(const QSize &size, int refreshRate)
{
    if (!isEnabled()) return;
    m_waylandOutput->setCurrentMode(size, refreshRate);
    m_xdgOutput->setLogicalSize(pixelSize() / scale());
    m_xdgOutput->done();
}

void AbstractOutput::initWaylandOutputDevice(const QString &model,
                                             const QString &manufacturer,
                                             const QByteArray &uuid,
                                             const QVector<KWayland::Server::OutputDeviceInterface::Mode> &modes)
{
    qDebug() << "-------" << __func__ << model << manufacturer << uuid;
    m_waylandOutputDevice->setUuid(uuid);

    if (!manufacturer.isEmpty()) {
        m_waylandOutputDevice->setManufacturer(manufacturer);
    } else {
        m_waylandOutputDevice->setManufacturer(i18n("unknown"));
    }

    m_waylandOutputDevice->setModel(model);
    m_waylandOutputDevice->setPhysicalSize(m_physicalSize);
    /*
     *  add base wayland output data
     */
    m_waylandOutput->setManufacturer(m_waylandOutputDevice->manufacturer());
    m_waylandOutput->setModel(m_waylandOutputDevice->model());
    m_waylandOutput->setPhysicalSize(rawPhysicalSize());

    int i = 0;
    for (auto mode : modes) {
        QString flags_str;
        KWayland::Server::OutputInterface::ModeFlags flags;

        if (mode.flags & KWayland::Server::OutputDeviceInterface::ModeFlag::Preferred) {
            flags_str += " preferred";
            flags |= KWayland::Server::OutputInterface::ModeFlag::Preferred;
        }
        if (mode.flags & KWayland::Server::OutputDeviceInterface::ModeFlag::Current) {
            flags_str += " current";
            flags |= KWayland::Server::OutputInterface::ModeFlag::Current;
        }
        qCDebug(KWIN_CORE).nospace() << "Adding mode " << ++i << ": " << mode.size
            << " [" << mode.refreshRate << "]" << flags_str;

        m_waylandOutputDevice->addMode(mode);
        m_waylandOutput->addMode(mode.size, flags, mode.refreshRate);
    }
    m_waylandOutputDevice->create();

    m_waylandOutput->create();
    m_xdgOutput->setLogicalSize(pixelSize() / scale());
    m_xdgOutput->done();
}

QSize AbstractOutput::orientateSize(const QSize &size) const
{
    if (m_orientation == Qt::PortraitOrientation || m_orientation == Qt::InvertedPortraitOrientation) {
        return size.transposed();
    }
    return size;
}

}
