// header file include
#include "pccontrolimpl.h"

// system/Qt includes
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
namespace
{
std::optional<QString> getLinkLocation()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    if (base.isEmpty())
    {
        return std::nullopt;
    }

    const QFileInfo fileInfo(QCoreApplication::applicationFilePath());
    return base + QDir::separator() + "Startup" + QDir::separator() + fileInfo.completeBaseName() + ".lnk";
}

//---------------------------------------------------------------------------------------------------------------------

const int EXTRA_DELAY_SECS{10};
const int MS_TO_SEC{1000};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

PcControlImpl::PcControlImpl(QString app_name)
    : m_app_name{std::move(app_name)}
{
    // Delay timer
    m_pc_delay_timer.setSingleShot(true);
    connect(&m_pc_delay_timer, &QTimer::timeout, this,
            [this]() { emit signalPcStateChanged(shared::PcState::Normal); });

    connect(&m_steam_handler, &SteamHandler::signalSteamStarted, this, &PcControlImpl::slotHandleSteamStart);
    connect(&m_steam_handler, &SteamHandler::signalSteamClosed, this, &PcControlImpl::slotHandleSteamExit);

    connect(&m_stream_state_handler, &StreamStateHandler::signalStreamStateChanged, this,
            &PcControlImpl::slotHandleStreamStateChange);
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::launchSteamApp(uint app_id)
{
    QStringList steam_args;
    if (m_stream_state_handler.getCurrentState() == shared::StreamState::Streaming)
    {
        steam_args += "-bigpicture";
    }
    m_steam_handler.launchApp(app_id, steam_args);
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::exitSteam(std::optional<uint> grace_period_in_sec)
{
    m_steam_handler.close(grace_period_in_sec);
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::shutdownPC(uint grace_period_in_sec)
{
    if (m_pc_delay_timer.isActive())
    {
        qDebug("PC is already being shut down. Aborting request.");
        return;
    }

    QProcess::execute("shutdown", {"-s", "-t", QString::number(grace_period_in_sec), "-f", "-c",
                                   m_app_name + " is putting you to sleep :)", "-y"});

    m_pc_delay_timer.start(static_cast<int>(grace_period_in_sec + EXTRA_DELAY_SECS) * MS_TO_SEC);
    emit signalPcStateChanged(shared::PcState::ShuttingDown);
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::restartPC(uint grace_period_in_sec)
{
    if (m_pc_delay_timer.isActive())
    {
        qDebug("PC is already being restarted. Aborting request.");
        return;
    }

    QProcess::execute("shutdown", {"-r", "-t", QString::number(grace_period_in_sec), "-f", "-c",
                                   m_app_name + " is giving you new live :?", "-y"});

    m_pc_delay_timer.start(static_cast<int>(grace_period_in_sec + EXTRA_DELAY_SECS) * MS_TO_SEC);
    emit signalPcStateChanged(shared::PcState::Restarting);
}

//---------------------------------------------------------------------------------------------------------------------

uint PcControlImpl::getRunningApp() const
{
    return m_steam_handler.getRunningApp();
}

//---------------------------------------------------------------------------------------------------------------------

std::optional<uint> PcControlImpl::isLastLaunchedAppUpdating() const
{
    return m_steam_handler.isLastLaunchedAppUpdating();
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControlImpl::isSteamRunning() const
{
    return m_steam_handler.isRunning();
}

//---------------------------------------------------------------------------------------------------------------------

shared::StreamState PcControlImpl::getStreamState() const
{
    return m_stream_state_handler.getCurrentState();
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::setAutoStart(bool enable)
{
    const auto location{getLinkLocation()};
    if (!location)
    {
        qWarning("Could not determine autostart location!");
        return;
    }

    if (QFile::exists(*location))
    {
        if (!QFile::remove(*location))
        {
            qWarning("Failed to remove %s!", qUtf8Printable(*location));
            return;
        }
    }

    if (enable)
    {
        if (!QFile::link(QCoreApplication::applicationFilePath(), *location))
        {
            qWarning("Failed to create link for %s!", qUtf8Printable(*location));
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

bool PcControlImpl::isAutoStartEnabled() const
{
    const auto location{getLinkLocation()};
    return location && QFile::exists(*location);
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::changeResolution(uint width, uint height, bool immediate)
{
    if (immediate)
    {
        m_resolution_handler.changeResolution(width, height);
    }
    else if (!m_steam_handler.isRunningNow() || getRunningApp() == 0)
    {
        m_resolution_handler.setPendingResolution(width, height);
    }
    else
    {
        qDebug("Non-immediate change is discarded.");
    }
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::abortPendingResolutionChange()
{
    m_resolution_handler.clearPendingResolution();
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::restoreChangedResolution()
{
    m_resolution_handler.restoreResolution();
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::slotHandleSteamStart()
{
    qDebug("Handling Steam start.");
    m_resolution_handler.clearPendingResolution();
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::slotHandleSteamExit()
{
    qDebug("Handling Steam exit.");
    m_stream_state_handler.endStream();
    m_resolution_handler.restoreResolution();
}

//---------------------------------------------------------------------------------------------------------------------

void PcControlImpl::slotHandleStreamStateChange()
{
    switch (m_stream_state_handler.getCurrentState())
    {
        case shared::StreamState::NotStreaming:
        {
            qDebug("Stream has ended.");
            break;
        }
        case shared::StreamState::Streaming:
        {
            qDebug("Stream started.");
            m_resolution_handler.applyPendingChange();
            break;
        }
        case shared::StreamState::StreamEnding:
        {
            qDebug("Stream is ending.");
            m_steam_handler.close(std::nullopt);
            m_resolution_handler.restoreResolution();
            break;
        }
    }
}
}  // namespace os
