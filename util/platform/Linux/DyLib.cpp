#include "DyLib.h"

#include <filesystem>

namespace HomeCompa::Util
{

DyLib::DyLib() = default;

DyLib::DyLib(std::filesystem::path /*moduleName*/)
{
}

DyLib::~DyLib() = default;

bool DyLib::IsOpen() const
{
	return false;
}

void DyLib::Detach()
{
}

bool DyLib::Open(std::filesystem::path moduleName)
{
}

void DyLib::Close()
{
}

void* DyLib::GetProc(const std::string& procName)
{
	return nullptr;
}

} // namespace HomeCompa::Util
