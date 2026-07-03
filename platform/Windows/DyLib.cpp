#include "DyLib.h"

#include <Windows.h>

#include <cassert>
#include <cwctype>

using namespace HomeCompa::Platform;

namespace
{

int WideCharToMultiByteImpl(const wchar_t* lpWideCharStr, const int cchWideChar, char* lpMultiByteStr, const int cbMultiByte)
{
	return WideCharToMultiByte(CP_UTF8, 0, lpWideCharStr, cchWideChar, lpMultiByteStr, cbMultiByte, nullptr, nullptr);
}

std::string ToMultiByte(const std::wstring& src)
{
	const auto  size = static_cast<std::wstring::size_type>(WideCharToMultiByteImpl(src.data(), static_cast<int>(src.size()), nullptr, 0));
	std::string dst(size, 0);
	WideCharToMultiByteImpl(src.data(), static_cast<int>(src.size()), dst.data(), static_cast<int>(dst.size()));
	return dst;
}

} // namespace

void* DyLib::InnerOpen(const std::filesystem::path& moduleName)
{
	const auto handle = LoadLibraryW(moduleName.wstring().data());
	return handle;
}

bool DyLib::InnerClose(void* handle)
{
	return !!FreeLibrary(static_cast<HMODULE>(handle));
}

void* DyLib::InnerGetProc(void* handle, const std::string& procName)
{
	return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), procName.data()));
}

std::string DyLib::InnerGetErrorDescription()
{
	const DWORD errCode = GetLastError();
	assert(errCode != 0);
	if (errCode == 0)
		return "Undefined";

	LPWSTR lpMsg        = nullptr;
	LPVOID lpBuf        = &lpMsg;
	size_t literalCount = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		errCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		static_cast<LPWSTR>(lpBuf),
		0,
		nullptr
	);
	assert(literalCount > 0);

	while (literalCount > 1 && std::iswspace(lpMsg[literalCount - 1]))
		--literalCount;

	auto message = ToMultiByte(std::wstring(lpMsg, lpMsg + literalCount));
	assert(nullptr == ::LocalFree(lpMsg));
	return message;
}
