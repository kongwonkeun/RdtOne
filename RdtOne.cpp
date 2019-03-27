#pragma comment(lib, "crypt32")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "irprops.lib")

#include "pch.h"
#include <iostream>
#include <exception>
#include <vector>
#include <memory>
#include <ctime>
#include "BTDeviceService.h"
#include "BTDeviceInquiry.h"
#include "BTSerialPort.h"
#include "BTException.h"

using namespace std;

// function prototypes
string formatDate(const char* format, time_t time);

int main()
{
    string m_addr;
    int m_ch;
    int i;

    SetConsoleOutputCP(CP_UTF8);
    std::cout << "hello world!\n";
    try {
        unique_ptr<BTDeviceInquiry> inquiry(BTDeviceInquiry::create());
        vector<BTDevice> devices = inquiry->inquire();
        for (const auto& d : devices) {
            cout << "\tname: " << d.name << endl;
            cout << "\taddress: " << d.address << endl;
            cout << "\tclass: " << getBTDeviceClass(d.deviceClass) << endl;
            cout << "\tmajor class: " << getBTDeviceClass(d.majorDeviceClass) << endl;
            cout << "\tservice class: " << getBTServiceClass(d.serviceClass) << endl;
            cout << "\tlast seen: " << formatDate("%c", d.lastSeen) << endl;
            cout << "\tlast used: " << formatDate("%c", d.lastUsed) << endl;
            i = inquiry->sdpSearch(d.address);
            cout << "\tchannel ID: " << i << endl;
            cout << endl;
            if (i > 0) {
                m_addr = d.address;
                m_ch = i;
            }
        }
        cout << endl << "done, found " << devices.size() << " device(s)" << endl;

        unique_ptr<BTSerialPort> m_bt(BTSerialPort::create(m_addr, m_ch));
        m_bt->connectSocket();
        char rxBuf[256] = "";
        int rxBufLength = 255;
        int rxCount = 0;
        while (1) {
            rxCount = m_bt->readData(rxBuf, rxBufLength);
            rxBuf[rxCount] = 0;

            printf("%s", rxBuf); // received data processing
            Sleep(500);

            m_bt->writeData(rxBuf, rxCount); // send control data
        }
    }
    catch (const BTException& e) {
        cout << e.what() << endl;
    }
    return 0;
}

string formatDate(const char* format, time_t time)
{
    if (time <= 0) {
        return "--";
    }
    char buffer[256] = { 0 };
    //tm* timeinfo = localtime(&time);
    tm  lt;
    tm* timeinfo = &lt;
    localtime_s(timeinfo, &time);
    //
    if (timeinfo) {
        strftime(buffer, sizeof(buffer), format, timeinfo);
    }
    return buffer;
}

