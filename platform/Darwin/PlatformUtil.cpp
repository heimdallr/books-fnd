#include "PlatformUtil.h"

#include <QString>

namespace HomeCompa::Platform
{

PlatformType GetPlatformType() noexcept
{
	return PlatformType::MacOS;
}

QString GetPlatformName()
{
	return "Darwin";
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
	return {};
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
	return "dmg";
}

} // namespace HomeCompa::Platform
