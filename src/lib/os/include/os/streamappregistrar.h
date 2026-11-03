#pragma once

// system/Qt includes
#include <QStringList>

// forward declarations
namespace common
{
class AppMetadata;
}

namespace os
{
//! Manages the MoonDeckStream entry in the apps of Sunshine and its forks (Apollo, Vibepollo).
class StreamAppRegistrar final
{
public:
    explicit StreamAppRegistrar(const common::AppMetadata& app_meta);

    static QStringList findAppsFiles();

    bool isRegistered(const QString& apps_file) const;
    bool registerApp(const QString& apps_file) const;

private:
    const common::AppMetadata& m_app_meta;
};
}  // namespace os
