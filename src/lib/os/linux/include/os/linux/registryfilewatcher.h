#pragma once

// system/Qt includes
#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

// local includes
#include "os/linux/registryfileparser.h"

namespace os
{
class RegistryFileWatcher : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(RegistryFileWatcher)

public:
    using NodeList = RegistryFileParser::Node::List;

    explicit RegistryFileWatcher(QString path);
    ~RegistryFileWatcher() override = default;

    const NodeList& getData() const;

signals:
    void signalRegistryChanged();

private slots:
    void slotFileChanged();
    void slotRetry();
    void slotParseFile();

private:
    QString            m_path;
    RegistryFileParser m_parser;
    QFileSystemWatcher m_file_watcher;
    QTimer             m_retry_timer;
    QTimer             m_parsing_delay_timer;
};
}  // namespace os
