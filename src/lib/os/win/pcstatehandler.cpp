// header file include
#include "pcstatehandler.h"

// system/Qt includes
#include <QProcess>

// local includes
#include "shared/constants.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
const int EXTRA_DELAY_SECS{10};
const int MS_TO_SEC{1000};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
PcStateHandler::PcStateHandler()
{
    m_state_change_back_timer.setSingleShot(true);
    connect(&m_state_change_back_timer, &QTimer::timeout, this,
            [this]() { emit signalPcStateChanged(shared::PcState::Normal); });
}

//---------------------------------------------------------------------------------------------------------------------

void PcStateHandler::shutdownPC(uint grace_period_in_sec)
{
    if (m_state_change_back_timer.isActive())
    {
        qDebug("PC is already being shut down. Aborting request.");
        return;
    }

    const auto result =
        QProcess::startDetached("shutdown", {"-s", "-t", QString::number(grace_period_in_sec), "-f", "-c",
                                             shared::APP_NAME_BUDDY + " is putting you to sleep :)", "-y"});

    if (!result)
    {
        qWarning("Failed to start shutdown sequence!");
        return;
    }

    m_state_change_back_timer.start(static_cast<int>(grace_period_in_sec + EXTRA_DELAY_SECS) * MS_TO_SEC);
    emit signalPcStateChanged(shared::PcState::ShuttingDown);
}

//---------------------------------------------------------------------------------------------------------------------

void PcStateHandler::restartPC(uint grace_period_in_sec)
{
    if (m_state_change_back_timer.isActive())
    {
        qDebug("PC is already being restarted. Aborting request.");
        return;
    }

    const auto result =
        QProcess::startDetached("shutdown", {"-r", "-t", QString::number(grace_period_in_sec), "-f", "-c",
                                             shared::APP_NAME_BUDDY + " is giving you new live :?", "-y"});

    if (!result)
    {
        qWarning("Failed to start restart sequence!");
        return;
    }

    m_state_change_back_timer.start(static_cast<int>(grace_period_in_sec + EXTRA_DELAY_SECS) * MS_TO_SEC);
    emit signalPcStateChanged(shared::PcState::Restarting);
}
}  // namespace os
