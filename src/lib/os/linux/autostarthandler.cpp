// header file include
#include "autostarthandler.h"

// system/Qt includes
#include <QDir>

// local includes
#include "shared/appmetadata.h"
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace
{
QString getAutoStartContents(const shared::AppMetadata& app_meta)
{
    QString     contents;
    QTextStream stream(&contents);

    stream << "[Desktop Entry]" << Qt::endl;
    stream << "Type=Application" << Qt::endl;
    stream << "Name=" << app_meta.getAppName() << Qt::endl;
    stream << "Exec=" << app_meta.getAutoStartExec() << Qt::endl;
    stream << "Icon=" << app_meta.getAppName() << Qt::endl;

    return contents;
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
AutoStartHandler::AutoStartHandler(const shared::AppMetadata& app_meta)
    : m_app_meta{app_meta}
{
}

//---------------------------------------------------------------------------------------------------------------------

void AutoStartHandler::setAutoStart(bool enable)
{
    const auto dir{m_app_meta.getAutoStartDir()};
    QFile      file{m_app_meta.getAutoStartPath()};

    if (file.exists() && !file.remove())
    {
        qCWarning(lc::os) << "Failed to remove" << file.fileName();
        return;
    }

    if (enable)
    {
        QDir autostart_dir;
        if (!autostart_dir.mkpath(dir))
        {
            qFatal("Failed at mkpath %s", qUtf8Printable(dir));
            return;
        }

        if (!file.open(QIODevice::WriteOnly))
        {
            qFatal("Failed to open %s", qUtf8Printable(m_app_meta.getAutoStartPath()));
            return;
        }

        if (!file.write(getAutoStartContents(m_app_meta).toUtf8()))
        {
            qFatal("Failed to write to %s", qUtf8Printable(m_app_meta.getAutoStartPath()));
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

bool AutoStartHandler::isAutoStartEnabled() const
{
    QFile file{m_app_meta.getAutoStartPath()};

    if (!file.exists())
    {
        return false;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        qFatal("Failed to open %s", qUtf8Printable(m_app_meta.getAutoStartPath()));
        return false;
    }

    return file.readAll() == getAutoStartContents(m_app_meta);
}
}  // namespace os
