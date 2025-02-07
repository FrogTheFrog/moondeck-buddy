#pragma once

// system/Qt includes
#include <QLoggingCategory>
#include <QMetaEnum>
#include <system_error>

//! LC - logging categories
namespace lc
{
template<class T>
QString qEnumToString(const T value)
{
    return QMetaEnum::fromType<T>().valueToKey(static_cast<int>(value));
}

QString getErrorString(auto&& error)
{
    return QString::fromStdString(std::system_category().message(static_cast<int>(error)));
}

Q_DECLARE_LOGGING_CATEGORY(buddyMain);
Q_DECLARE_LOGGING_CATEGORY(streamMain);
Q_DECLARE_LOGGING_CATEGORY(server);
Q_DECLARE_LOGGING_CATEGORY(shared);
Q_DECLARE_LOGGING_CATEGORY(utils);
Q_DECLARE_LOGGING_CATEGORY(os);
}  // namespace lc