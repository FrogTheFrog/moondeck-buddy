#pragma once

// system/Qt includes
#include <QtGlobal>
#include <QString>
#include <QHostAddress>

namespace os
{
class NativeNetworkHandler
{
public:
    static QString GetMacAddressFromQHostAddress(const QHostAddress& hostAddress);
};
}  // namespace os
