#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Windows.h>
#include <WinSock2.h>
#include <ws2bth.h>
#include <string>
#include <stdlib.h>
#include "BTException.h"
#include "BTSerialPort.h"
#include "BTHelper.h"

using namespace std;

struct BTSerialPortSocket
{
    SOCKET s;
    bool initialized;
};

BTSerialPort* BTSerialPort::create(string address, int channel)
{
    if (channel <= 0) {
        throw BTException("channel id should be a positive int value");
    }
    auto obj = new BTSerialPort(address, channel);
    if (!obj->m_socket->initialized) {
        delete obj;
        throw BTException("unable to initialize socket library");
    }
    return obj;
}

BTSerialPort::BTSerialPort(string address, int channel) : m_address(address), m_channel(channel), m_socket(new BTSerialPortSocket)
{
    m_socket->s = INVALID_SOCKET;
    m_socket->initialized = BTHelper::initialize();
}

BTSerialPort::~BTSerialPort()
{
    if (m_socket->initialized) {
        BTHelper::finalize();
    }
}

void BTSerialPort::connectSocket()
{
    closeSocket();
    int status = SOCKET_ERROR;
    m_socket->s = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    if (m_socket->s != SOCKET_ERROR) {
        SOCKADDR_BTH addr = { 0 };
        int size = sizeof(SOCKADDR_BTH);
        TCHAR addrBuf[40];
        if (m_address.length() >= 40) {
            throw BTException("address lenght is invalid");
        }
        for (size_t i = 0; i < m_address.length(); i++) {
            addrBuf[i] = (TCHAR)m_address[i];
        }
        addrBuf[m_address.length()] = 0;
        status = WSAStringToAddress(addrBuf, AF_BTH, nullptr, (LPSOCKADDR)&addr, &size);
        if (status != SOCKET_ERROR) {
            addr.port = m_channel;
            status = connect(m_socket->s, (LPSOCKADDR)&addr, size);
            if (status != SOCKET_ERROR) {
                unsigned long enableNonBlocking = 1;
                ioctlsocket(m_socket->s, FIONBIO, &enableNonBlocking);
            }
        }
    }
    if (status != 0) {
        string msg = BTHelper::getErrorMessage(WSAGetLastError());
        if (m_socket->s != INVALID_SOCKET) {
            closesocket(m_socket->s);
        }
        throw BTException("cannot connect: " + msg);
    }
}

void BTSerialPort::closeSocket()
{
    if (m_socket->s != INVALID_SOCKET) {
        closesocket(m_socket->s);
        m_socket->s = INVALID_SOCKET;
    }
}

int  BTSerialPort::readData(char* buffer, int length)
{
    if (m_socket->s == INVALID_SOCKET) {
        throw BTException("connection has been closed");
    }
    if (buffer == nullptr) {
        throw BTException("buffer cannot be null");
    }
    if (length == 0) {
        return 0;
    }
    fd_set set;
    FD_ZERO(&set);
    FD_SET(m_socket->s, &set);
    int size = -1;
    if (select(static_cast<int>(m_socket->s) + 1, &set, nullptr, nullptr, nullptr) >= 0) {
        if (FD_ISSET(m_socket->s, &set)) {
            size = recv(m_socket->s, buffer, length, 0);
        }
        else {
            size = 0;
        }
    }
    if (size < 0) {
        throw BTException("error reading from connection");
    }
    return size;
}

void BTSerialPort::writeData(const char* buffer, int length)
{
    if (buffer == nullptr) {
        throw BTException("buffer cannot be null");
    }
    if (length == 0) {
        return;
    }
    if (m_socket->s == INVALID_SOCKET) {
        throw BTException("attempting to write to a closed connection");
    }
    if (send(m_socket->s, buffer, length, 0) != length) {
        throw BTException("writing attempt was unsuccessful");
    }
}

bool BTSerialPort::isDataAvailable()
{
    if (m_socket->s == INVALID_SOCKET) {
        throw BTException("connection has been closed");
    }
    u_long count;
    ioctlsocket(m_socket->s, FIONREAD, &count);
    return count > 0;
}


