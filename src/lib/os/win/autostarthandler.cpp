// header file include
#include "os/win/autostarthandler.h"

// system/Qt includes
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

// local includes
#include "shared/appmetadata.h"
#include "shared/loggingcategories.h"

namespace os
{
AutoStartHandler::AutoStartHandler(const shared::AppMetadata& app_meta)
    : m_app_meta{app_meta}
{
}

void AutoStartHandler::setAutoStart(bool enable)
{
    const auto dir{m_app_meta.getAutoStartDir()};
    QFile      file{m_app_meta.getAutoStartPath()};

    if (file.exists() && !file.remove())
    {
        qFatal("Failed to remove %s", qUtf8Printable(m_app_meta.getAutoStartPath()));
        return;
    }

    if (enable)
    {
        if (!QFile::link(m_app_meta.getAutoStartExec(), m_app_meta.getAutoStartPath()))
        {
            qFatal("Failed to create link for %s -> %s", qUtf8Printable(m_app_meta.getAutoStartExec()),
                   qUtf8Printable(m_app_meta.getAutoStartPath()));
            return;
        }
    }
}

bool AutoStartHandler::isAutoStartEnabled() const
{
    if (!QFile::exists(m_app_meta.getAutoStartPath()))
    {
        return false;
    }

    const QFileInfo info(m_app_meta.getAutoStartPath());
    if (!info.isShortcut())
    {
        return false;
    }

    return QFileInfo(info.symLinkTarget()).canonicalFilePath()
           == QFileInfo(m_app_meta.getAutoStartExec()).canonicalFilePath();
}
}  // namespace os
