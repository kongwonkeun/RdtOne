#pragma once

#include <string>
#include <memory>

struct BTSerialPortSocket;

class  BTSerialPort
{
	std::string m_address;
	int m_channel;
	std::unique_ptr<BTSerialPortSocket> m_socket;
	BTSerialPort(std::string address, int channel);
public:
	~BTSerialPort();
	static BTSerialPort* createSocket(std::string address, int channel);
	void connectSocket();
	void closeSocket();
	int  readData(char* buffer, int length);
	void writeData(const char* buffer, int length);
	bool isDataAvailable();
};

