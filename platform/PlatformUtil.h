#pragma once

#include "export/platform.h"

class QRect;
class QString;
class QWidget;

namespace HomeCompa::Platform
{

enum class PlatformType
{
    Windows,
    Linux,
};

PLATFORM_EXPORT PlatformType GetPlatformType() noexcept;

PLATFORM_EXPORT bool IsRegisteredExtension(const QString& extension);
PLATFORM_EXPORT void SetKeyboardLayout(const QString& locale);

PLATFORM_EXPORT bool IsAppAddedToAutostart(const QString& key);
PLATFORM_EXPORT void AddToAutostart(const QString& key, const QString& path);
PLATFORM_EXPORT void RemoveFromAutostart(const QString& key);

PLATFORM_EXPORT void SetGeometry(QWidget& widget, const QRect& rect);
PLATFORM_EXPORT QString SetDefaultInstallerSuffix();

}
