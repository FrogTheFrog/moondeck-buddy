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

    quint16 getPort() const;
    bool    isVerbose() const;

private:
    bool parseSettingsFile(const QString& filename);
    void saveDefaultFile(const QString& filename) const;

    quint16 m_port{59999};
    bool    m_verbose{false};
};
}  // namespace utils
