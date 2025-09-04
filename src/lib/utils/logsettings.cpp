// header file include
#include "utils/logsettings.h"

// system/Qt includes
#include <QDateTime>
#include <QDir>

// local includes
#include "shared/loggingcategories.h"

namespace
{
void swapFilesIfNeeded(const QString& filepath)
{
    constexpr int max_size{2 * 1024 * 1024};
    if (const QFileInfo file_info(filepath); file_info.size() > max_size)
    {
        const auto filename{file_info.fileName()};
        const auto old_filename{filename + ".old"};

        auto file_dir{file_info.absoluteDir()};
        if (file_dir.exists(old_filename) && !file_dir.remove(old_filename))
        {
            qFatal("File could not be removed: \"%s\".", qUtf8Printable(old_filename));
        }

        if (!file_dir.rename(filename, old_filename))
        {
            qFatal("File could not be renamed: \"%s\" -> \"%s\".", qUtf8Printable(filename),
                   qUtf8Printable(old_filename));
        }
    }
}

void appendEmptyLine(const QString& filepath)
{
    if (const QFileInfo file_info(filepath); file_info.size() > 0)
    {
        QFile file(filepath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            qFatal("File could not be opened for writing: \"%s\".", qUtf8Printable(filepath));
        }

        file.write("\n\n");
    }
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    const QString formatted_msg{qFormatLogMessage(type, context, msg)};
    {
        QTextStream stream(stdout);
        stream << formatted_msg << Qt::endl;
    }

    const auto& filepath{utils::LogSettings::getInstance().getFilepath()};
    swapFilesIfNeeded(filepath);

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        qFatal("File could not be opened for writing: \"%s\".", qUtf8Printable(filepath));
    }

    QTextStream stream(&file);
    stream << formatted_msg << Qt::endl;
}
}  // namespace

namespace utils
{
LogSettings& LogSettings::getInstance()
{
    static LogSettings instance;
    return instance;
}

void LogSettings::init(const QString& filepath)
{
    qSetMessagePattern("[%{time hh:mm:ss.zzz}] "
                       "%{if-debug}DEBUG    %{endif}"
                       "%{if-info}INFO     %{endif}"
                       "%{if-warning}WARNING  %{endif}"
                       "%{if-critical}CRITICAL %{endif}"
                       "%{if-fatal}FATAL    %{endif}"
                       "%{if-category}%{category}: %{endif}%{message}");

    if (filepath.isEmpty())
    {
        return;
    }

    m_filepath = filepath;
    swapFilesIfNeeded(filepath);
    appendEmptyLine(filepath);
    qInstallMessageHandler(messageHandler);

    const auto uninstall_handler{[]() { qInstallMessageHandler(nullptr); }};
    std::at_quick_exit(uninstall_handler);
    std::atexit(uninstall_handler);

    qCInfo(lc::utils) << "Log location:" << m_filepath;
}

const QString& LogSettings::getFilepath() const
{
    return m_filepath;
}

// NOLINTNEXTLINE(*-to-static)
void LogSettings::setLoggingRules(const QString& rules)
{
    if (!rules.isEmpty())
    {
        QLoggingCategory::setFilterRules(rules);
    }
}
}  // namespace utils
