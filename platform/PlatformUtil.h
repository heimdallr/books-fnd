#pragma once

#include "export/platform.h"

class QString;

namespace HomeCompa::Util
{

PLATFORM_EXPORT bool IsRegisteredExtension(const QString& extension);
PLATFORM_EXPORT void SetKeyboardLayout(const QString& locale);

PLATFORM_EXPORT bool IsAppAddedToAutostart(const QString& key);
PLATFORM_EXPORT void AddToAutostart(const QString& key, const QString& path);
PLATFORM_EXPORT void RemoveFromAutostart(const QString& key);

}
