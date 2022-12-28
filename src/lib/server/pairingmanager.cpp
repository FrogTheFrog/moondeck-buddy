// header file include
#include "pairingmanager.h"

// local includes
#include "clientids.h"
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace server
{
PairingManager::PairingManager(ClientIds& client_ids)
    : m_client_ids{client_ids}
{
}

//---------------------------------------------------------------------------------------------------------------------

bool PairingManager::isPaired(const QString& client_id) const
{
    return m_client_ids.containsId(client_id);
}

//---------------------------------------------------------------------------------------------------------------------

bool PairingManager::isPairing(const QString& client_id) const
{
    return m_pairing_data && m_pairing_data->m_id == client_id;
}

//---------------------------------------------------------------------------------------------------------------------

bool PairingManager::isPairing() const
{
    return static_cast<bool>(m_pairing_data);
}

//---------------------------------------------------------------------------------------------------------------------

bool PairingManager::startPairing(const QString& id, const QString& hashed_id)
{
    if (m_pairing_data)
    {
        qCWarning(lc::server) << "Cannot start pairing as" << m_pairing_data->m_id << "is currently being paired!";
        return false;
    }

    if (id.isEmpty() || hashed_id.isEmpty())
    {
        qCWarning(lc::server) << "Invalid id or hashed_id provided for pairing!";
        return false;
    }

    if (m_client_ids.containsId(id))
    {
        qCWarning(lc::server) << "Id" << id << "is already paired!";
        return false;
    }

    m_pairing_data = {id, hashed_id};
    emit signalRequestUserInputForPairing();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

bool PairingManager::abortPairing(const QString& id)
{
    if (!isPairing())
    {
        return true;
    }

    if (!isPairing(id))
    {
        qCWarning(lc::server) << "Cannot abort pairing for other id than" << id;
        return false;
    }

    qCDebug(lc::server) << "Aborting pairing for" << id;
    emit signalAbortPairing();
    m_pairing_data = std::nullopt;
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

void PairingManager::slotFinishPairing(uint pin)
{
    if (!m_pairing_data)
    {
        qCWarning(lc::server) << "Pairing is not in progress!";
        return;
    }

    const QString string_for_hashing{m_pairing_data->m_id + QString::number(pin)};
    if (string_for_hashing.toUtf8().toBase64() != m_pairing_data->m_hashed_id)
    {
        qCWarning(lc::server) << "Pairing code does not match.";
        return;
    }

    m_client_ids.addId(m_pairing_data->m_id);
    m_client_ids.save();

    m_pairing_data = std::nullopt;
}

//---------------------------------------------------------------------------------------------------------------------

void PairingManager::slotPairingRejected()
{
    qCDebug(lc::server) << "Pairing was rejected.";
    m_pairing_data = std::nullopt;
}

}  // namespace server
