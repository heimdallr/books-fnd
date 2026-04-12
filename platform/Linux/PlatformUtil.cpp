#include "PlatformUtil.h"

#include <QString>

#include "log.h"

#include "XKeyboard.h"

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
}

QString GetKeyboardLayoutId()
{
    try{
        XKeyboard xKeyboard;
        return QString::fromStdString(xKeyboard.currentGroupName());
    }
    catch(const std::exception& ex)
    {
        PLOGE << ex.what();
    }

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
	return "deb";
}

} // namespace HomeCompa::Platform
