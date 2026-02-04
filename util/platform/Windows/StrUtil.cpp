#pragma warning(push, 0)
#include <Windows.h>
#pragma warning(pop)

#include "StrUtil.h"

namespace HomeCompa::Util
{

int MultiByteToWideCharImpl(const char* lpMultiByteStr, const int cbMultiByte, wchar_t* lpWideCharStr, const int cchWideChar)
{
	return MultiByteToWideChar(CP_UTF8, 0, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);
}

int WideCharToMultiByteImpl(const wchar_t* lpWideCharStr, const int cchWideChar, char* lpMultiByteStr, const int cbMultiByte)
{
	return WideCharToMultiByte(CP_UTF8, 0, lpWideCharStr, cchWideChar, lpMultiByteStr, cbMultiByte, nullptr, nullptr);
}

int CompareStringImpl(const wchar_t* lpString1, const int cchCount1, const wchar_t* lpString2, const int cchCount2)
{
	return CompareStringW(GetThreadLocale(), NORM_IGNORECASE, lpString1, cchCount1, lpString2, cchCount2) - CSTR_EQUAL;
}

}
