#include "DyLib.h"

#include <dlfcn.h>

using namespace HomeCompa::Platform;

void* DyLib::InnerOpen(const std::filesystem::path& moduleName)
{
	return dlopen(("lib" + moduleName.generic_string() + ".so").c_str(), RTLD_NOW);
}

bool DyLib::InnerClose(void* handle)
{
	return 0 == dlclose(handle);
}

void* DyLib::InnerGetProc(void* handle, const std::string& procName)
{
	return dlsym(handle, procName.c_str());
}

std::string DyLib::InnerGetErrorDescription()
{
	return dlerror();
}
