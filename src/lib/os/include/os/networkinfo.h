#pragma once

// system/Qt includes
#include <QHostAddress>

namespace os
{
class NetworkInfo
{
public:
    static QString getMacAddress(const QHostAddress& host_address);
};
}  // namespace os
