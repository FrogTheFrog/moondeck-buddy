// header file include
#include "os/networkinfo.h"

// system/Qt includes
#if defined(Q_OS_WIN)
// clang-format off
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    // clang-format on
    #include <vector>
#endif
#include <QtNetwork/QNetworkInterface>

// local includes
#include "shared/loggingcategories.h"

namespace
{
QString getMacAddressGeneric(const QHostAddress& host_address)
{
    static const QSet allowed_types{QNetworkInterface::Ethernet, QNetworkInterface::Wifi};

    for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces())
    {
        if (allowed_types.contains(iface.type()) && iface.flags().testFlag(QNetworkInterface::IsRunning))
        {
            for (const QHostAddress& address_entry : QNetworkInterface::allAddresses())
            {
                if (address_entry.isEqual(host_address))
                {
                    return iface.hardwareAddress();
                }
            }
        }
    }

    return {};
}
}  // namespace

namespace os
{
QString NetworkInfo::getMacAddress(const QHostAddress& host_address)
{
#if defined(Q_OS_WIN)
    ULONG                 buffer_size{0};
    std::vector<BYTE>     buffer;
    PIP_ADAPTER_ADDRESSES addresses{nullptr};

    ULONG result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, addresses, &buffer_size);
    if (result == ERROR_BUFFER_OVERFLOW)
    {
        buffer.resize(buffer_size);
        addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
        result    = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, addresses, &buffer_size);
    }

    if (result != NO_ERROR)
    {
        qCWarning(lc::os) << "GetAdaptersAddresses failed with error (using fallback):" << lc::getErrorString(result);
        return getMacAddressGeneric(host_address);
    }

    while (addresses != nullptr)
    {
        for (auto* unicast_address = addresses->FirstUnicastAddress; unicast_address != nullptr;
             unicast_address       = unicast_address->Next)
        {
            if (getnameinfo(unicast_address->Address.lpSockaddr, unicast_address->Address.iSockaddrLength, nullptr, 0,
                            nullptr, 0, NI_NUMERICHOST)
                != 0)
            {
                qCWarning(lc::os) << "WSAGetLastError failed with error:" << lc::getErrorString(WSAGetLastError());
                return {};
            }

            const QHostAddress address_entry(unicast_address->Address.lpSockaddr);
            if (host_address.isEqual(address_entry))
            {
                if (addresses->PhysicalAddressLength > 0)
                {
                    QString mac_address = QString("%1:%2:%3:%4:%5:%6")
                                              .arg(addresses->PhysicalAddress[0], 2, 16, QLatin1Char('0'))
                                              .arg(addresses->PhysicalAddress[1], 2, 16, QLatin1Char('0'))
                                              .arg(addresses->PhysicalAddress[2], 2, 16, QLatin1Char('0'))
                                              .arg(addresses->PhysicalAddress[3], 2, 16, QLatin1Char('0'))
                                              .arg(addresses->PhysicalAddress[4], 2, 16, QLatin1Char('0'))
                                              .arg(addresses->PhysicalAddress[5], 2, 16, QLatin1Char('0'))
                                              .toUpper();

                    return mac_address;
                }

                qCDebug(lc::os) << "Using fallback MAC detection, because no physical address found for"
                                << address_entry;
                return getMacAddressGeneric(address_entry);
            }
        }
        addresses = addresses->Next;
    }
#endif
    return getMacAddressGeneric(host_address);
}
}  // namespace os
