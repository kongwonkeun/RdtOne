#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <initguid.h>
#include <windows.h>
#include <string>
#include <stdlib.h>
#include <winsock2.h>
#include <ws2bth.h>
#include <memory>
#include <bluetoothapis.h>
#include "BTException.h"
#include "BTDeviceInquiry.h"
#include "BTHelper.h"

#ifdef UNICODE
#define tscanf swscanf
#else
#define tscanf sscanf
#endif

using namespace std;

BTDeviceInquiry* BTDeviceInquiry::create()
{
    auto inquire = new BTDeviceInquiry();
    if (!inquire->m_initialized) {
        delete inquire;
        throw BTException("unable to initialize socket library");
    }
    return inquire;
}

BTDeviceInquiry::BTDeviceInquiry()
{
    m_initialized = BTHelper::initialize();
}

BTDeviceInquiry::~BTDeviceInquiry()
{
    if (m_initialized) {
        BTHelper::finalize();
    }
}

vector<BTDevice> BTDeviceInquiry::inquire(int length)
{
    // (void)(length);
    DWORD flags = LUP_CONTAINERS | LUP_FLUSHCACHE | LUP_RETURN_NAME | LUP_RETURN_ADDR;
    DWORD querySetSize = sizeof(WSAQUERYSET);
    unique_ptr<WSAQUERYSET, BTFreeQuerySet> querySet((WSAQUERYSET*)malloc(querySetSize));
    if (querySet == nullptr) {
        throw BTException("unable to allocate memory resource for inquiry");
    }
    ZeroMemory(querySet.get(), querySetSize);
    querySet->dwSize = querySetSize;
    querySet->dwNameSpace = NS_BTH;
    HANDLE lookupServiceHandle;
    int lookupServiceError = WSALookupServiceBegin(querySet.get(), flags, &lookupServiceHandle);
    vector<BTDevice> devices;
    int errorCode = WSAGetLastError();
    if (lookupServiceError == SOCKET_ERROR) {
        if (errorCode == WSASERVICE_NOT_FOUND) {
            return devices;
        }
        else {
            throw BTException(BTHelper::getErrorMessage(errorCode));
        }
    }
    bool inquiryComplete = false;
    while (!inquiryComplete) {
        lookupServiceError = WSALookupServiceNext(lookupServiceHandle, flags, &querySetSize, querySet.get());
        if (lookupServiceError != SOCKET_ERROR) {
            TCHAR address[40] = { 0 };
            char name[248] = { 0 };
            DWORD addressLength = _countof(address);
            SOCKADDR_BTH *bluetoothSocketAddress = (SOCKADDR_BTH *)querySet->lpcsaBuffer->RemoteAddr.lpSockaddr;
            BTH_ADDR bluetoothAddress = bluetoothSocketAddress->btAddr;
            int addressToStringError = WSAAddressToString(
                querySet->lpcsaBuffer->RemoteAddr.lpSockaddr, 
                sizeof(SOCKADDR_BTH), 
                nullptr, 
                address, 
                &addressLength
            );
            if (addressToStringError != SOCKET_ERROR) {
                TCHAR strippedAddress[19] = { 0 };
                auto  sscanfResult = tscanf(address, TEXT("(%18[^)])"), strippedAddress);
                TCHAR *addr = sscanfResult == 1 ? strippedAddress : address;
                auto  addressString = BTHelper::toString(addr);
                int b, c, d, e, f, g;
                tscanf(addr, TEXT("%2x:%2x:%2x:%2x:%2x:%2x"), &b, &c, &d, &e, &f, &g);
                BLUETOOTH_ADDRESS a = { 0 };
                a.rgBytes[5] = b;
                a.rgBytes[4] = c;
                a.rgBytes[3] = d;
                a.rgBytes[2] = e;
                a.rgBytes[1] = f;
                a.rgBytes[0] = g;
                BLUETOOTH_DEVICE_INFO deviceInfo;
                deviceInfo.dwSize = sizeof(BLUETOOTH_DEVICE_INFO);
                deviceInfo.Address = a;
                deviceInfo.ulClassofDevice = 0;
                deviceInfo.fConnected = false;
                deviceInfo.fRemembered = false;
                deviceInfo.fAuthenticated = false;
                auto result = BluetoothGetDeviceInfo(NULL, &deviceInfo);
                BTDevice dev;
                dev.address = addressString;
                if (querySet->lpszServiceInstanceName == NULL || lstrlen(querySet->lpszServiceInstanceName) == 0) {
                    size_t convertedChars;
                    wcstombs_s(&convertedChars, name, sizeof(name), deviceInfo.szName, _TRUNCATE);
                    dev.name = std::string(name);
                }
                else {
                    dev.name = BTHelper::toString(querySet->lpszServiceInstanceName);
                }
                if (result == ERROR_SUCCESS) {
                    ULONG cod = deviceInfo.ulClassofDevice;
                    dev.connected = deviceInfo.fConnected ? true : false;
                    dev.remembered = deviceInfo.fRemembered ? true : false;
                    dev.authenticated = deviceInfo.fAuthenticated ? true : false;
                    dev.lastSeen = convertTime(&deviceInfo.stLastSeen);
                    dev.lastUsed = convertTime(&deviceInfo.stLastUsed);
                    dev.deviceClass = (BTDeviceClass)(cod & 0x1ffc);
                    dev.majorDeviceClass = (BTDeviceClass)(cod & DC_Uncategorized);
                    dev.serviceClass = (BTServiceClass)(cod >> 13);
                }
                else {
                    dev.connected = false;
                    dev.remembered = false;
                    dev.authenticated = false;
                    dev.lastSeen = 0;
                    dev.lastUsed = 0;
                    dev.deviceClass = DC_Miscellaneous;
                    dev.majorDeviceClass = DC_Miscellaneous;
                    dev.serviceClass = SC_None;
                }
                devices.push_back(dev);
            }
        }
        else {
            int lookupServiceErrorNumber = WSAGetLastError();
            if (lookupServiceErrorNumber == WSAEFAULT) {
                querySet.reset((WSAQUERYSET*)malloc(querySetSize));
                if (querySet == nullptr) {
                    WSALookupServiceEnd(lookupServiceHandle);
                    throw BTException("unable to allocate memory resource for inquiry");
                }
            }
            else if (lookupServiceErrorNumber == WSA_E_NO_MORE) {
                inquiryComplete = true;
            }
            else {
                inquiryComplete = true;
            }
        }
    }
    WSALookupServiceEnd(lookupServiceHandle);
    return devices;
}

int BTDeviceInquiry::sdpSearch(string address)
{
    DWORD flags = LUP_FLUSHCACHE | LUP_RETURN_ADDR;
    DWORD querySetSize = sizeof(WSAQUERYSET);
    unique_ptr<WSAQUERYSET, BTFreeQuerySet> querySet((WSAQUERYSET*)malloc(querySetSize));
    if (querySet == nullptr) {
        throw BTException("unable to allocate memory resource for sdp search");
    }
    TCHAR addressBuffer[40];
    if (address.length() >= 40) {
        throw BTException("address length is invalid");
    }
    for (size_t i = 0; i < address.length(); i++) {
        addressBuffer[i] = (TCHAR)address[i];
    }
    addressBuffer[address.length()] = 0;
    ZeroMemory(querySet.get(), querySetSize);
    querySet->dwSize = querySetSize;
    querySet->dwNameSpace = NS_BTH;
    querySet->lpServiceClassId = (LPGUID)&SerialPortServiceClass_UUID; // '{00001101-0000-1000-8000-00805f9b34fb}'
    querySet->dwNumberOfCsAddrs = 0;
    querySet->lpszContext = addressBuffer;
    HANDLE lookupServiceHandle;
    int lookupServiceError = WSALookupServiceBegin(querySet.get(), flags, &lookupServiceHandle);
    int channelID = -1;
    if (lookupServiceError == SOCKET_ERROR) {
        throw BTException(BTHelper::getErrorMessage(WSAGetLastError()));
    }
    bool inquiryComplete = false;
    while (!inquiryComplete) {
        lookupServiceError = WSALookupServiceNext(lookupServiceHandle, flags, &querySetSize, querySet.get());
        if (lookupServiceError != SOCKET_ERROR) {
            char address[19] = { 0 };
            SOCKADDR_BTH* bluetoothSocketAddress = (SOCKADDR_BTH*)querySet->lpcsaBuffer->RemoteAddr.lpSockaddr;
            channelID = bluetoothSocketAddress->port;
            inquiryComplete = true;
        }
        else {
            int lookupServiceErrorNumber = WSAGetLastError();
            if (lookupServiceErrorNumber == WSAEFAULT) {
                querySet.reset((WSAQUERYSET*)malloc(querySetSize));
                if (querySet == nullptr) {
                    WSALookupServiceEnd(lookupServiceHandle);
                    throw BTException("unable to allocate memory resource for sdp search");
                }
            }
            else if (lookupServiceErrorNumber == WSA_E_NO_MORE) {
                inquiryComplete = true;
            }
            else {
                inquiryComplete = true;
            }
        }
    }
    WSALookupServiceEnd(lookupServiceHandle);
    return channelID;
}

time_t BTDeviceInquiry::convertTime(SYSTEMTIME *t)
{
    tm x;
    x.tm_sec   = t->wSecond;
    x.tm_min   = t->wMinute;
    x.tm_hour  = t->wHour;
    x.tm_mday  = t->wDay;
    x.tm_mon   = t->wMonth;
    x.tm_year  = t->wYear - 1900;
    x.tm_isdst = 0;
    return mktime(&x);
}
