// header file include
#include "registryfilewatcher.h"

// local includes
#include "shared/loggingcategories.h"

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
RegistryFileWatcher::RegistryFileWatcher(QString path)
    : m_path{std::move(path)}
{
    connect(&m_file_watcher, &QFileSystemWatcher::fileChanged, this, &RegistryFileWatcher::slotFileChanged);
    connect(&m_retry_timer, &QTimer::timeout, this, &RegistryFileWatcher::slotRetry);

    const int retry_inverval{1000};
    m_retry_timer.setInterval(retry_inverval);
    m_retry_timer.setSingleShot(true);

    // Delay until next event loop
    QTimer::singleShot(0, this, &RegistryFileWatcher::slotRetry);
}

//---------------------------------------------------------------------------------------------------------------------

const RegistryFileWatcher::NodeList& RegistryFileWatcher::getData() const
{
    return m_parser.getRoot();
}

//---------------------------------------------------------------------------------------------------------------------

void RegistryFileWatcher::slotFileChanged()
{
    if (!m_file_watcher.files().contains(m_path))
    {
        slotRetry();
        return;
    }

    parseFile();
}

//---------------------------------------------------------------------------------------------------------------------

void RegistryFileWatcher::slotRetry()
{
    if (!m_file_watcher.addPath(m_path))
    {
        m_retry_timer.start();
        return;
    }

    parseFile();
}

//---------------------------------------------------------------------------------------------------------------------

void RegistryFileWatcher::parseFile()
{
    if (!m_parser.parse(m_path))
    {
        qCWarning(lc::os) << "failed at parsing registry file" << m_path;
    }

    emit signalRegistryChanged();
}
}  // namespace os
