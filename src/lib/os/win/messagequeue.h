#pragma once

// A SEPARATE WINDOWS INCLUDE BECAUSE OF THE SMELL!
#include <windows.h>

// system/Qt includes
#include <QObject>
#include <QRegularExpression>
#include <QTimer>

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
// Creates an invisible window to receive all the messages for TOP LEVEL window
class MessageQueue : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(MessageQueue)

public:
    explicit MessageQueue(QString class_name);
    ~MessageQueue() override;

signals:
    void signalResolutionChanged();

private:
    QString m_class_name;
    ATOM    m_window_class_handle{0};
    HWND    m_window_handle{nullptr};
};
}  // namespace os
