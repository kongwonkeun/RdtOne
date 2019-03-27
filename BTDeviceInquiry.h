#pragma once

#include "BTDeviceService.h"
#include <Windows.h>
#include <vector>
#include <string>
#include <ctime>

struct BTFreeQuerySet
{
    void operator()(void* x) { free(x); }
};

struct BTDevice
{
    std::string address;
    std::string name;
    std::time_t lastSeen;
    std::time_t lastUsed;
    bool connected;
    bool remembered;
    bool authenticated;
    BTDeviceClass  deviceClass;
    BTDeviceClass  majorDeviceClass;
    BTServiceClass serviceClass;
};

class BTDeviceInquiry
{
#ifdef _WINDOWS_
    bool m_initialized;
#endif
    BTDeviceInquiry();
public:
    ~BTDeviceInquiry();
    static BTDeviceInquiry* create();
    std::vector<BTDevice> inquire(int length = 8);
    int sdpSearch(std::string address);
    time_t convertTime(SYSTEMTIME *t);
};

