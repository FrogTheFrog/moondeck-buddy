#pragma once

// system/Qt includes
#include <QString>

//---------------------------------------------------------------------------------------------------------------------

namespace utils
{
class AppSettings
{
    Q_DISABLE_COPY(AppSettings)

public:
    explicit AppSettings(const QString& filename);
    virtual ~AppSettings() = default;

    quint16        getPort() const;
    const QString& getLoggingRules() const;

private:
    bool parseSettingsFile(const QString& filename);
    void saveDefaultFile(const QString& filename) const;

    quint16 m_port;
    QString m_logging_rules;

public:
    // TODO: remove
    bool m_nvidia_reset_mouse_acceleration_after_stream_end_hack;
};
}  // namespace utils
