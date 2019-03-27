#pragma once

#include <string>

class BTHelper
{
public:
    static bool initialize();
    static void finalize();
    static std::string getErrorMessage(int errorCode);
    static std::string toString(LPTSTR str);
};

