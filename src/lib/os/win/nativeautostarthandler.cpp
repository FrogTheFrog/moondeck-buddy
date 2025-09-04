// header file include
#include "os/win/nativeautostarthandler.h"

// system/Qt includes
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

// local includes
#include "shared/appmetadata.h"
#include "shared/loggingcategories.h"

namespace os
{
NativeAutoStartHandler::NativeAutoStartHandler(const shared::AppMetadata& app_meta)
    : m_app_meta{app_meta}
{
}

void NativeAutoStartHandler::setAutoStart(const bool enable)
{
    const auto dir{m_app_meta.getAutoStartDir(shared::AppMetadata::AutoStartDelegation::Desktop)};
    QFile      file{m_app_meta.getAutoStartPath(shared::AppMetadata::AutoStartDelegation::Desktop)};

    if (file.exists() && !file.remove())
    {
        qFatal("Failed to remove %s",
               qUtf8Printable(m_app_meta.getAutoStartPath(shared::AppMetadata::AutoStartDelegation::Desktop)));
        return;
    }

    if (enable)
    {
        if (!QFile::link(m_app_meta.getAutoStartExec(),
                         m_app_meta.getAutoStartPath(shared::AppMetadata::AutoStartDelegation::Desktop)))
        {
            qFatal("Failed to create link for %s -> %s", qUtf8Printable(m_app_meta.getAutoStartExec()),
                   qUtf8Printable(m_app_meta.getAutoStartPath(shared::AppMetadata::AutoStartDelegation::Desktop)));
            return;
        }
    }
}

bool NativeAutoStartHandler::isAutoStartEnabled() const
{
    if (!QFile::exists(m_app_meta.getAutoStartPath(shared::AppMetadata::AutoStartDelegation::Desktop)))
    {
        return false;
    }

    const QFileInfo info(m_app_meta.getAutoStartPath(shared::AppMetadata::AutoStartDelegation::Desktop));
    if (!info.isShortcut())
    {
        return false;
    }

    return QFileInfo(info.symLinkTarget()).canonicalFilePath()
           == QFileInfo(m_app_meta.getAutoStartExec()).canonicalFilePath();
}
}  // namespace os
