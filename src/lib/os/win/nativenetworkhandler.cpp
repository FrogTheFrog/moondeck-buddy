// header file include
#include "os/win/nativenetworkhandler.h"

// system/Qt includes
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <iostream>
#include <vector>

// local includes
#include "shared/loggingcategories.h"

QString normalizeIPv4MappedAddress(const QString& ip) {
    if (ip.startsWith("::ffff:")) {
        return ip.mid(7);  // Strip the "::ffff:" prefix
    }
    return ip;
}

namespace os
{
QString NativeNetworkHandler::GetMacAddressFromQHostAddress(const QHostAddress& hostAddress) {
    ULONG bufferSize = 15000;
    std::vector<BYTE> buffer(bufferSize);
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

    ULONG result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &bufferSize);
    if (result == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(bufferSize);
        pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
        result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &bufferSize);
    }

    if (result != NO_ERROR) {
        qCWarning(lc::os) << "GetAdaptersAddresses failed with error: " << lc::getErrorString(result);
        return {};
    }

    QString targetIp = normalizeIPv4MappedAddress(hostAddress.toString());

    while (pAddresses) {
        for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pAddresses->FirstUnicastAddress; pUnicast != nullptr; pUnicast = pUnicast->Next) {
            char strBuffer[INET6_ADDRSTRLEN] = { 0 };
            getnameinfo(pUnicast->Address.lpSockaddr, pUnicast->Address.iSockaddrLength, strBuffer, sizeof(strBuffer), nullptr, 0, NI_NUMERICHOST);

            QString adapterIp = QString::fromUtf8(strBuffer);

            if (targetIp == adapterIp) {
                QString macAddress = QString("%1:%2:%3:%4:%5:%6")
                    .arg(pAddresses->PhysicalAddress[0], 2, 16, QLatin1Char('0'))
                    .arg(pAddresses->PhysicalAddress[1], 2, 16, QLatin1Char('0'))
                    .arg(pAddresses->PhysicalAddress[2], 2, 16, QLatin1Char('0'))
                    .arg(pAddresses->PhysicalAddress[3], 2, 16, QLatin1Char('0'))
                    .arg(pAddresses->PhysicalAddress[4], 2, 16, QLatin1Char('0'))
                    .arg(pAddresses->PhysicalAddress[5], 2, 16, QLatin1Char('0'))
                    .toUpper();

                return macAddress;
            }
        }
        pAddresses = pAddresses->Next;
    }

    return {};
}
}  // namespace os
