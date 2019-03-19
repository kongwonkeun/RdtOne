#pragma once

#ifndef _MSC_VER
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif

#include <exception>
#include <string>

class BTException : std::exception
{
	std::string m_message;
public:
	BTException(std::string message) NOEXCEPT {
		this->m_message = message;
	}
	virtual const char* what() const NOEXCEPT {
		return m_message.c_str();
	}
};

