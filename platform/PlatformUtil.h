#pragma once

#include "export/platform.h"

class QRect;
class QString;

namespace HomeCompa::Platform
{

#define PLATFORM_TYPE_ITEMS_X_MACRO \
	PLATFORM_TYPE_ITEM(Windows)     \
	PLATFORM_TYPE_ITEM(Linux)

enum class PlatformType
{
#define PLATFORM_TYPE_ITEM(NAME) NAME,
	PLATFORM_TYPE_ITEMS_X_MACRO
#undef PLATFORM_TYPE_ITEM
};

PLATFORM_EXPORT PlatformType GetPlatformType() noexcept;
PLATFORM_EXPORT QString      GetPlatformName() noexcept;

PLATFORM_EXPORT bool IsRegisteredExtension(const QString& extension);
PLATFORM_EXPORT void SetKeyboardLayout(const QString& locale);

PLATFORM_EXPORT bool IsAppAddedToAutostart(const QString& key);
PLATFORM_EXPORT void AddToAutostart(const QString& key, const QString& path);
PLATFORM_EXPORT void RemoveFromAutostart(const QString& key);

PLATFORM_EXPORT QString GetDefaultInstallerSuffix();

}
