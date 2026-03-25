#pragma once

#include <QString>

#include "export/platform.h"

namespace HomeCompa::Util
{

PLATFORM_EXPORT QString RemoveIllegalPathCharacters(QString str);

}
