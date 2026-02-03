#include "ProcessWrapper.h"

#include <Windows.h>

namespace HomeCompa::Util
{

bool ShellExecuteImpl(const std::wstring& file, const std::wstring& parameters, const std::wstring& cwd, const bool show, const bool wait)
{
	SHELLEXECUTEINFO lpExecInfo {};
	lpExecInfo.cbSize       = sizeof(SHELLEXECUTEINFO);
	lpExecInfo.lpFile       = file.data();
	lpExecInfo.fMask        = SEE_MASK_DOENVSUBST | SEE_MASK_NOCLOSEPROCESS;
	lpExecInfo.hwnd         = nullptr;
	lpExecInfo.lpVerb       = L"open";
	lpExecInfo.lpParameters = parameters.data();
	lpExecInfo.lpDirectory  = cwd.data();
	lpExecInfo.nShow        = show ? SW_SHOWNORMAL : SW_HIDE;
	lpExecInfo.hInstApp     = reinterpret_cast<HINSTANCE>(SE_ERR_DDEFAIL);
	ShellExecuteEx(&lpExecInfo);

	if (lpExecInfo.hProcess == nullptr)
		return false;

	if (wait)
		WaitForSingleObject(lpExecInfo.hProcess, INFINITE);

	CloseHandle(lpExecInfo.hProcess);
	return true;
}

}
