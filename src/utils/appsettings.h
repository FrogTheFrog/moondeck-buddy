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
    const static quint16 DEFAULT_PORT{59999};

    bool parseSettingsFile(const QString& filename);
    void saveDefaultFile(const QString& filename) const;

    quint16 m_port{DEFAULT_PORT};
    bool    m_verbose{false};
};
}  // namespace utils
