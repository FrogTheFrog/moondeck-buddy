#pragma once

// system/Qt includes
#include <QObject>

namespace shared
{
class AppMetadata final : public QObject
{
public:
    enum class App
    {
        Buddy,
        Stream
    };

    explicit AppMetadata(App app);
    ~AppMetadata() override = default;

    QString getAppName() const;
    QString getAppName(App app) const;

    QString getLogDir() const;
    QString getLogName() const;
    QString getLogPath() const;

    QString getSettingsDir() const;
    QString getSettingsName() const;
    QString getSettingsPath() const;

    QString getAutoStartDir() const;
    QString getAutoStartName() const;
    QString getAutoStartPath() const;
    QString getAutoStartExec() const;

    QString getDefaultSteamExecutable() const;

    QString getSharedEnvRegexKey() const;
    QString getSharedEnvMapKey() const;

private:
    App m_current_app;
};
}  // namespace shared