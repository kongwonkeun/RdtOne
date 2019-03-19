#include <WinSock2.h>
#include <ws2bth.h>
#include <string>
#include <codecvt>
#include "BTHelper.h"

using namespace std;

bool BTHelper::initialize()
{
	WSADATA data;
	int  startupError = WSAStartup(MAKEWORD(2, 2), &data);
	bool initSuccess = startupError == 0;
	if (initSuccess) {
		initSuccess = LOBYTE(data.wVersion) == 2 && HIBYTE(data.wVersion) == 2;
		if (!initSuccess) {
			BTHelper::finalize();
		}
	}
	return initSuccess;
}

void BTHelper::finalize()
{
	WSACleanup();
}

string BTHelper::getErrorMessage(int errorCode)
{
	LPTSTR buffer;
	DWORD length;
	length = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&buffer,
		0,
		NULL
	);
	if (length == 0) {
		return "unknow";
	}
	char buf[20];
	auto result = toString(buffer);
	LocalFree(buffer);
	_itoa_s(errorCode, buf, sizeof(buf), 10);
	return string("(") + buf + string(")") + result;
}

string BTHelper::toString(LPTSTR str)
{
#ifdef UNICODE
	wstring_convert<codecvt_utf8<wchar_t>, wchar_t> converter;
	return converter.to_bytes(wstring(str));
#else
	return string(str);
#endif
}

