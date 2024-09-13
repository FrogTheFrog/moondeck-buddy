// header file include
#include "utils/pairinginput.h"

namespace
{
const int DEFAULT_TIMEOUT{1 * 60 * 1000 /*1 minute*/};
const int MIN_PIN_VALUE{1000};
const int MAX_PIN_VALUE{9999};
}  // namespace

namespace utils
{
PairingInput::PairingInput()
{
    connect(&m_dialog, &QInputDialog::rejected, this,
            [this]()
            {
                m_timeout.stop();
                emit signalPairingRejected();
            });
    connect(&m_dialog, &QInputDialog::accepted, this,
            [this]()
            {
                m_timeout.stop();
                emit signalFinishPairing(m_dialog.intValue());
            });
    m_dialog.setInputMode(QInputDialog::IntInput);
    m_dialog.setIntMinimum(MIN_PIN_VALUE);
    m_dialog.setIntMaximum(MAX_PIN_VALUE);
    m_dialog.setWindowTitle("Pairing");
    m_dialog.setLabelText("Please enter the pairing number as shown on SteamDeck.");

    connect(&m_timeout, &QTimer::timeout, &m_dialog, &QInputDialog::reject);
    m_timeout.setSingleShot(true);
}

void PairingInput::slotRequestUserInputForPairing()
{
    m_dialog.close();
    m_timeout.start(DEFAULT_TIMEOUT);
    m_dialog.open();
}

void PairingInput::slotAbortPairing()
{
    m_timeout.stop();
    m_dialog.close();
}
}  // namespace utils
