#pragma once

#include "export/platform.h"

class QString;

namespace HomeCompa::Util
{

PLATFORM_EXPORT bool IsRegisteredExtension(const QString& extension);

}
