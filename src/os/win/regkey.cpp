// header file include
#include "regkey.h"

// system/Qt includes
#include <system_error>
#include <unordered_map>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
QString getError(LSTATUS status)
{
    return QString::fromStdString(std::system_category().message(status));
}

//---------------------------------------------------------------------------------------------------------------------

struct PathComponents
{
    inline static const QChar            PATH_SEPARATOR{'\\'};
    static std::optional<HKEY>           stringToHKey(const QString& path);
    static std::optional<PathComponents> parse(const QString& path);

    HKEY    m_key{nullptr};
    QString m_subkey{};
};

//---------------------------------------------------------------------------------------------------------------------

std::optional<HKEY> PathComponents::stringToHKey(const QString& path_segment)
{
    static std::unordered_map<QString, HKEY> path_mapping{
        {QStringLiteral("HKEY_CLASSES_ROOT"), HKEY_CLASSES_ROOT},
        {QStringLiteral("HKEY_CURRENT_CONFIG"), HKEY_CURRENT_CONFIG},
        {QStringLiteral("HKEY_CURRENT_USER"), HKEY_CURRENT_USER},
        {QStringLiteral("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE},
        {QStringLiteral("HKEY_USERS"), HKEY_USERS},
    };

    auto path_it = path_mapping.find(path_segment.toUpper());
    if (path_it != path_mapping.end())
    {
        return path_it->second;
    }
    return std::nullopt;
}

//---------------------------------------------------------------------------------------------------------------------

std::optional<PathComponents> PathComponents::parse(const QString& path)
{
    const auto first_separator = path.indexOf(PathComponents::PATH_SEPARATOR);
    if (first_separator != -1)
    {
        const QString key_part    = path.sliced(0, first_separator);
        const QString subkey_part = path.sliced(first_separator + 1);

        const auto key = stringToHKey(key_part);
        if (key)
        {
            return PathComponents{key.value(), subkey_part};
        }
    }
    else
    {
        const auto key = stringToHKey(path);
        if (key)
        {
            return PathComponents{key.value()};
        }
    }
    return std::nullopt;
}

//---------------------------------------------------------------------------------------------------------------------

struct RegValueData
{
    QByteArray m_buffer{};
    DWORD      m_type{REG_NONE};
};

//---------------------------------------------------------------------------------------------------------------------

RegValueData getRegValueData(const HKEY& key_handle, const QString& name)
{
    RegValueData data;
    DWORD        data_size{0};

    auto result =
        // NOLINTNEXTLINE(*-reinterpret-cast)
        RegQueryValueExW(key_handle, reinterpret_cast<const wchar_t*>(name.utf16()), nullptr, &data.m_type, nullptr,
                         &data_size);

    if (result != ERROR_SUCCESS)
    {
        qWarning("Could not read the size of the key value '%s'! Reason: %s", qUtf8Printable(name),
                 qUtf8Printable(getError(result)));
    }

    if (data_size <= 0)
    {
        return {};
    }

    data.m_buffer.resize(data_size);
    result =
        // NOLINTNEXTLINE(*-reinterpret-cast)
        RegQueryValueExW(key_handle, reinterpret_cast<const wchar_t*>(name.utf16()), nullptr, nullptr,
                         // NOLINTNEXTLINE(*-reinterpret-cast)
                         reinterpret_cast<LPBYTE>(data.m_buffer.data()), &data_size);

    if (result != ERROR_SUCCESS)
    {
        qWarning("Could not read the data of the key value '%s'! Reason: %s", qUtf8Printable(name),
                 qUtf8Printable(getError(result)));
    }

    return data;
}

//---------------------------------------------------------------------------------------------------------------------

void resetNotifier(std::unique_ptr<QWinEventNotifier>& notifier, const HKEY& key_handle, const QString& name)
{
    auto* const handle = notifier->handle();
    if (ResetEvent(handle) == FALSE)
    {
        qFatal("Failed to reset handle! Reason: %s", qUtf8Printable(getError(GetLastError())));
        return;
    }

    notifier->setHandle(handle);  // Reset the watcher
    notifier->setEnabled(true);

    const auto result = RegNotifyChangeKeyValue(key_handle, TRUE, REG_NOTIFY_CHANGE_LAST_SET, handle, TRUE);
    if (result != ERROR_SUCCESS)
    {
        qFatal("Could not start watching data of the key value '%s'! Reason: %s", qUtf8Printable(name),
               qUtf8Printable(getError(result)));
    }
}

//---------------------------------------------------------------------------------------------------------------------

void deleteNotifier(std::unique_ptr<QWinEventNotifier>& notifier)
{
    if (notifier != nullptr)
    {
        notifier->disconnect();
        if (CloseHandle(notifier->handle()) == FALSE)
        {
            qWarning("Failed to close handle! Reason: %s", qUtf8Printable(getError(GetLastError())));
        }
        notifier.reset();
    }
}

//---------------------------------------------------------------------------------------------------------------------

bool openKey(const PathComponents& path_components, const QString& path, HKEY& key_handle)
{
    const auto result =
        // NOLINTNEXTLINE(*-reinterpret-cast)
        RegOpenKeyExW(path_components.m_key, reinterpret_cast<const wchar_t*>(path_components.m_subkey.utf16()), 0,
                      KEY_READ, &key_handle);

    if (result != ERROR_SUCCESS)
    {
        qDebug("Could not open key '%s'! Reason: %s", qUtf8Printable(path), qUtf8Printable(getError(result)));
    }

    return result == ERROR_SUCCESS;
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------

namespace os
{
RegKey::~RegKey()
{
    close();
}

//---------------------------------------------------------------------------------------------------------------------

void RegKey::open(const QString& path, const QStringList& notification_names, bool auto_retry)
{
    const auto path_components = PathComponents::parse(path);
    if (!path_components)
    {
        qDebug("Registry key '%s' is not valid!", qUtf8Printable(path));
        return;
    }

    if (openKey(*path_components, path, m_key_handle))
    {
        m_path = path;
        addNotifyOnValueChange(notification_names);
        return;
    }

    if (auto_retry)
    {
        connect(&m_retry_timer, &QTimer::timeout, this,
                [this, path, path_components, notification_names]()
                {
                    if (openKey(*path_components, path, m_key_handle))
                    {
                        m_path = path;
                        addNotifyOnValueChange(notification_names);
                        return;
                    }

                    m_retry_timer.start();
                });

        m_retry_timer.setSingleShot(true);

        const int single_second{1000};
        m_retry_timer.start(single_second);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void RegKey::close()
{
    if (isOpen())
    {
        const auto result = RegCloseKey(m_key_handle);
        if (result != ERROR_SUCCESS)
        {
            qWarning("Could not close key '%s'! Reason: %s", qUtf8Printable(m_path), qUtf8Printable(getError(result)));
        }
        m_key_handle = nullptr;
    }

    deleteNotifier(m_notifier);
    m_path = {};
    m_retry_timer.stop();
    m_watched_names.clear();
}

//---------------------------------------------------------------------------------------------------------------------

bool RegKey::isOpen() const
{
    return m_key_handle != nullptr;
}

//---------------------------------------------------------------------------------------------------------------------

QVariant RegKey::getValue(const QString& name) const
{
    if (!isOpen())
    {
        return {};
    }

    QVariant return_value;

    const auto data = getRegValueData(m_key_handle, name);
    switch (data.m_type)
    {
        case REG_DWORD:
        {
            QDataStream stream(data.m_buffer);
            qint32      value{0};

            stream.setByteOrder(QDataStream::LittleEndian);
            stream >> value;

            return_value = value;
            break;
        }
        case REG_SZ:
        {
            // NOLINTNEXTLINE(*-reinterpret-cast)
            return_value = QString::fromUtf16(reinterpret_cast<const char16_t*>(data.m_buffer.constData()),
                                              data.m_buffer.size() / 2)
                               .remove(static_cast<QChar>('\0'));
            break;
        }
        default:
            break;
    }

    return return_value;
}

//---------------------------------------------------------------------------------------------------------------------

bool RegKey::isNotificationEnabled() const
{
    return m_notifier != nullptr;
}

//---------------------------------------------------------------------------------------------------------------------

void RegKey::addNotifyOnValueChange(const QStringList& names)
{
    if (!isOpen())
    {
        return;
    }

    QStringList new_names;
    for (const auto& name : names)
    {
        if (!m_watched_names.contains(name))
        {
            m_watched_names[name] = QVariant{};
            new_names.append(name);
        }
    }

    if (new_names.isEmpty() && isNotificationEnabled())
    {
        return;
    }

    if (new_names.isEmpty() && !isNotificationEnabled())
    {
        new_names = m_watched_names.keys();
    }

    auto* const handle = CreateEventW(nullptr, TRUE, TRUE, nullptr);
    if (handle == nullptr)
    {
        qFatal("Failed to create handle! Reason: %s", qUtf8Printable(getError(GetLastError())));
        return;
    }

    m_notifier = std::make_unique<QWinEventNotifier>();
    m_notifier->setHandle(handle);

    connect(m_notifier.get(), &QWinEventNotifier::activated, this,
            [this]()
            {
                checkForChangedValues(m_watched_names.keys());
                resetNotifier(m_notifier, m_key_handle, m_path);
            });

    checkForChangedValues(new_names);
    resetNotifier(m_notifier, m_key_handle, m_path);
}

//---------------------------------------------------------------------------------------------------------------------

void RegKey::removeNotifyOnValueChange(const QStringList& names)
{
    for (const auto& name : names)
    {
        m_watched_names.remove(name);
    }

    if (m_watched_names.isEmpty() && isNotificationEnabled())
    {
        deleteNotifier(m_notifier);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void RegKey::checkForChangedValues(const QStringList& names)
{
    QMap<QString, QVariant> changed_values;

    for (const auto& name : names)
    {
        const auto value = getValue(name);
        if (m_watched_names[name] != value)
        {
            m_watched_names[name] = value;
            changed_values[name]  = value;
        }
    }

    if (!changed_values.isEmpty())
    {
        emit signalValuesChanged(changed_values);
    }
}
}  // namespace os
