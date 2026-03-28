#include "PlatformUtil.h"

namespace HomeCompa::Util
{

bool IsRegisteredExtension(const QString& /*extension*/)
{
	return true;
}

void SetKeyboardLayout(const QString& /*locale*/)
{
	assert(false);
}

bool IsAppAddedToAutostart(const QString& /*key*/)
{
	assert(false);
}

void AddToAutostart(const QString& /*key*/, const QString& /*path*/)
{
	assert(false);
}

void RemoveFromAutostart(const QString& /*key*/)
{
	assert(false);
}

} // namespace HomeCompa::Util
