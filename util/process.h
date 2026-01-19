#pragma once

#include <QStringList>

#include "export/util.h"

namespace HomeCompa::Util
{

UTIL_EXPORT QStringList SplitStringWithQuotes(const QString& str);
UTIL_EXPORT bool        RunProcess(const QString& file, const QString& parameters, const QString& cwd);

}
