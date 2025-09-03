#pragma once

// system/Qt includes
#include <QObject>

namespace shared
{
class AppMetadata final : public QObject
{
    Q_OBJECT

public:
    enum class App
    {
        Buddy,
        Stream
    };

    enum class AutoStartDelegation
    {
        V1,
#if defined(Q_OS_LINUX)
        V2
#endif
    };
    Q_ENUM(AutoStartDelegation)

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

    QString getAutoStartDir(AutoStartDelegation version) const;
    QString getAutoStartName(AutoStartDelegation version) const;
    QString getAutoStartPath(AutoStartDelegation version) const;
    QString getAutoStartExec() const;

    QString getDefaultSteamExecutable() const;

    QString getSharedEnvRegexKey() const;
    QString getSharedEnvMapKey() const;

    bool isGuiEnabled() const;

private:
    App m_current_app;
};
}  // namespace shared