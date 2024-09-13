#pragma once

// system/Qt includes
#include <QObject>

// forward declaration
namespace server
{
class ClientIds;
}

namespace server
{
class PairingManager : public QObject
{
    Q_OBJECT

public:
    explicit PairingManager(ClientIds& client_ids);
    ~PairingManager() override = default;

    bool isPaired(const QString& client_id) const;
    bool isPairing(const QString& client_id) const;
    bool isPairing() const;

    bool startPairing(const QString& id, const QString& hashed_id);
    bool abortPairing(const QString& id);

signals:
    void signalRequestUserInputForPairing();
    void signalAbortPairing();

public slots:
    void slotFinishPairing(uint pin);
    void slotPairingRejected();

private:
    struct PairingData
    {
        QString m_id;
        QString m_hashed_id;
    };

    ClientIds&                 m_client_ids;
    std::optional<PairingData> m_pairing_data;
};
}  // namespace server
