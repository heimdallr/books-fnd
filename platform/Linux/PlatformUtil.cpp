#include "PlatformUtil.h"

#include <QString>

namespace HomeCompa::Platform
{

PlatformType GetPlatformType() noexcept
{
	return PlatformType::Linux;
}

bool IsRegisteredExtension(const QString& /*extension*/)
{
	return true;
}

void SetKeyboardLayout(const QString& /*locale*/)
{
	assert(false);
}

void SetKeyboardLayoutId(const QString& /*id*/)
{
	assert(false);
}

QString GetKeyboardLayoutId()
{
	assert(false);
}

bool IsAppAddedToAutostart(const QString& /*key*/)
{
	return false;
}

void AddToAutostart(const QString& /*key*/, const QString& /*path*/)
{
	assert(false);
}

void RemoveFromAutostart(const QString& /*key*/)
{
	assert(false);
}

QString GetDefaultInstallerSuffix()
{
	return "deb";
}

} // namespace HomeCompa::Platform
