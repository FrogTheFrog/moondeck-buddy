#pragma once

// system/Qt includes
#include <QTimer>
#include <QUuid>
#include <QtWidgets/QInputDialog>

namespace utils
{
class PairingInput : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PairingInput)

public:
    explicit PairingInput();
    ~PairingInput() override = default;

signals:
    void signalFinishPairing(uint pin);
    void signalPairingRejected();

public slots:
    void slotRequestUserInputForPairing();
    void slotAbortPairing();

private:
    QInputDialog m_dialog;
    QTimer       m_timeout;
};
}  // namespace utils
