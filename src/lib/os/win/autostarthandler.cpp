// header file include
#include "autostarthandler.h"

// system/Qt includes
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
std::optional<QString> getLinkLocation()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    if (base.isEmpty())
    {
        return std::nullopt;
    }

    const QFileInfo fileInfo(QCoreApplication::applicationFilePath());
    return base + QDir::separator() + "Startup" + QDir::separator() + fileInfo.completeBaseName() + ".lnk";
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
// NOLINTNEXTLINE(*-static)
void AutoStartHandler::setAutoStart(bool enable)
{
    const auto location{getLinkLocation()};
    if (!location)
    {
        qWarning("Could not determine autostart location!");
        return;
    }

    if (QFile::exists(*location))
    {
        if (!QFile::remove(*location))
        {
            qWarning("Failed to remove %s!", qUtf8Printable(*location));
            return;
        }
    }

    if (enable)
    {
        if (!QFile::link(QCoreApplication::applicationFilePath(), *location))
        {
            qWarning("Failed to create link for %s!", qUtf8Printable(*location));
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

// NOLINTNEXTLINE(*-static)
bool AutoStartHandler::isAutoStartEnabled() const
{
    const auto location{getLinkLocation()};
    if (!location || !QFile::exists(*location))
    {
        return false;
    }

    const QFileInfo info(*location);
    if (!info.isShortcut())
    {
        return false;
    }

    return QFileInfo(info.symLinkTarget()).canonicalFilePath()
           == QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath();
}
}  // namespace os
