#include "StrUtil.h"

namespace HomeCompa::Util
{

int MultiByteToWideCharImpl(const char* lpMultiByteStr, const int cbMultiByte, wchar_t* lpWideCharStr, const int cchWideChar)
{
}

int WideCharToMultiByteImpl(const wchar_t* lpWideCharStr, const int cchWideChar, char* lpMultiByteStr, const int cbMultiByte)
{
}

int CompareStringImpl(const wchar_t* lpString1, const int cchCount1, const wchar_t* lpString2, const int cchCount2)
{
}

static_assert(false);

}
