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
        Desktop,
#if defined(Q_OS_LINUX)
        ServiceMain,
        ServiceHelper,
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

    QString getAutoStartDir(AutoStartDelegation type) const;
    QString getAutoStartName(AutoStartDelegation type) const;
    QString getAutoStartPath(AutoStartDelegation type) const;
    QString getAutoStartExec() const;

    QString getDefaultSteamExecutable() const;

    QString getSharedEnvRegexKey() const;
    QString getSharedEnvMapKey() const;

    bool isGuiEnabled() const;

private:
    App m_current_app;
};
}  // namespace shared