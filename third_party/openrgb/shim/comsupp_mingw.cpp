/*---------------------------------------------------------*\
| comsupp_mingw.cpp                                         |
|                                                           |
|   Supplies the COM string conversions declared by MinGW   |
|   but missing from its runtime libraries.                 |
|                                                           |
|   SPDX-License-Identifier: GPL-2.0-only                   |
\*---------------------------------------------------------*/

#if defined(_WIN32) && !defined(_MSC_VER)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <comdef.h>

namespace _com_util {

BSTR ConvertStringToBSTR(const char* text)
{
    if (text == nullptr) {
        return nullptr;
    }
    const int length{MultiByteToWideChar(CP_ACP, 0, text, -1, nullptr, 0)};
    if (length <= 0) {
        return nullptr;
    }
    BSTR converted{SysAllocStringLen(nullptr, static_cast<UINT>(length - 1))};
    if (converted == nullptr) {
        return nullptr;
    }
    MultiByteToWideChar(CP_ACP, 0, text, -1, converted, length);
    return converted;
}

char* ConvertBSTRToString(BSTR text)
{
    if (text == nullptr) {
        return nullptr;
    }
    const int length{WideCharToMultiByte(CP_ACP, 0, text, -1, nullptr, 0, nullptr, nullptr)};
    if (length <= 0) {
        return nullptr;
    }
    char* const converted{new char[static_cast<std::size_t>(length)]};
    WideCharToMultiByte(CP_ACP, 0, text, -1, converted, length, nullptr, nullptr);
    return converted;
}

}

#endif
