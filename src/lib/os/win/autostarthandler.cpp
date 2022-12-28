// header file include
#include "autostarthandler.h"

// system/Qt includes
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

// local includes
#include "shared/loggingcategories.h"

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
        qCWarning(lc::os) << "Could not determine autostart location!";
        return;
    }

    if (QFile::exists(*location))
    {
        if (!QFile::remove(*location))
        {
            qCWarning(lc::os) << "Failed to remove" << *location;
            return;
        }
    }

    if (enable)
    {
        if (!QFile::link(QCoreApplication::applicationFilePath(), *location))
        {
            qCWarning(lc::os) << "Failed to create link for" << *location;
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
