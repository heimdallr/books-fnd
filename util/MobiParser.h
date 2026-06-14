#pragma once

#include "CommonParser.h"

#include "export/util.h"

class QIODevice;

namespace HomeCompa
{

class Zip;

}

namespace HomeCompa::Util::MobiParser
{

UTIL_EXPORT CommonParser::ParseResult Parse(QIODevice& stream, CommonParser::Mode mode = CommonParser::Mode::None);
UTIL_EXPORT CommonParser::ParseResult Parse(const Zip& zip, const QString& fileName, CommonParser::Mode mode = CommonParser::Mode::None);

} // namespace HomeCompa::Util::MobiParser
