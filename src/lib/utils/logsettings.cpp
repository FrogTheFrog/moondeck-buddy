// header file include
#include "utils/logsettings.h"

// system/Qt includes
#include <QDateTime>
#include <QDir>
#include <csignal>

// local includes
#include "shared/loggingcategories.h"

namespace
{
void writeToStd(FILE* handle, const QStringView view)
{
    QTextStream stream(handle, QIODeviceBase::WriteOnly);
    stream << view << Qt::endl;
}

bool trySwapFilesIfNeeded(utils::LogSettings& instance, const QString& filepath)
{
    constexpr int max_size{2 * 1024 * 1024};
    if (const QFileInfo file_info(filepath); file_info.size() > max_size)
    {
        const auto filename{file_info.fileName()};
        const auto old_filename{filename + ".old"};

        auto file_dir{file_info.absoluteDir()};
        if (file_dir.exists(old_filename) && !file_dir.remove(old_filename))
        {
            const QString error{"File could not be removed: " + old_filename};
            instance.writeToStdErr(error);
            return false;
        }

        if (!file_dir.rename(filename, old_filename))
        {
            const QString error{"File could not be renamed: " + filename + " -> " + old_filename};
            instance.writeToStdErr(error);
            return false;
        }
    }

    return true;
}

void messageHandler(const QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    auto&         instance{utils::LogSettings::getInstance()};
    const QString formatted_msg{qFormatLogMessage(type, context, msg)};

    type == QtInfoMsg || type == QtDebugMsg ? instance.writeToStdOut(formatted_msg)
                                            : instance.writeToStdErr(formatted_msg);
    instance.writeToFile(formatted_msg);
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

    m_filepath = filepath;

    qInstallMessageHandler(messageHandler);
    std::atexit([]() { qInstallMessageHandler(nullptr); });

    qCInfo(lc::utils) << "Log location:" << m_filepath;
}

void LogSettings::writeToStdOut(const QStringView view)
{
    writeToStd(stdout, view);
}

void LogSettings::writeToStdErr(const QStringView view)
{
    writeToStd(stderr, view);
}

void LogSettings::writeToFile(const QStringView view)
{
    if (!m_filepath.isEmpty() && trySwapFilesIfNeeded(*this, m_filepath))
    {
        QFile file(m_filepath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            const QString error{"File could not be opened for writing: " + m_filepath};
            writeToStdErr(error);
            return;
        }

        if (!m_file_appended)
        {
            if (const QFileInfo file_info(m_filepath); file_info.size() > 0)
            {
                file.write("\n\n");
            }
            m_file_appended = true;
        }

        QTextStream stream(&file);
        stream << view << Qt::endl;
    }
}

void LogSettings::logSignalBeforeExit(const int code)
{
    qInstallMessageHandler(nullptr);

    const QString error{"interrupted by signal " + QString::number(code) + ([code]()
        {
            switch (code)
            {
                case SIGINT:
                    return " (SIGINT)";
                case SIGTERM:
                    return " (SIGTERM)";
#if defined(Q_OS_LINUX)
                case SIGHUP:
                    return " (SIGHUP)";
                case SIGQUIT:
                    return " (SIGQUIT)";
#endif
                default:
                    return "";
            }
        }())};
    writeToStdErr(error);
    writeToFile(error);
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
