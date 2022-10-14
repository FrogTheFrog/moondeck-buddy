#pragma once

// system/Qt includes
#include <QObject>
#include <QTimer>
#include <QVariant>
#include <QWinEventNotifier>
#include <windows.h>

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
class RegKey : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(RegKey)

public:
    explicit RegKey() = default;
    ~RegKey() override;

    void open(const QString& path, const QStringList& notification_names = {}, bool auto_retry = true);
    void close();

    bool     isOpen() const;
    QVariant getValue(const QString& names) const;

    bool isNotificationEnabled() const;
    void addNotifyOnValueChange(const QStringList& names);
    void removeNotifyOnValueChange(const QStringList& names);

signals:
    void signalValuesChanged(const QMap<QString, QVariant>& values);

private:
    void checkForChangedValues(const QStringList& names);

    QString                            m_path;
    std::unique_ptr<QWinEventNotifier> m_notifier;
    HKEY                               m_key_handle{nullptr};
    QMap<QString, QVariant>            m_watched_names;
    QTimer                             m_retry_timer;
};
}  // namespace os
