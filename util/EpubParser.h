#pragma once

#include "CommonParser.h"

#include "export/util.h"

class QIODevice;

namespace HomeCompa
{

class Zip;

}

namespace HomeCompa::Util::EpubParser
{

using ImageIndex = std::vector<std::pair<QString, int>>;

UTIL_EXPORT CommonParser::ParseResult Parse(QIODevice& stream, CommonParser::Mode mode = CommonParser::Mode::None);
UTIL_EXPORT CommonParser::ParseResult Parse(const Zip& zip, const QString& fileName, CommonParser::Mode mode = CommonParser::Mode::None);
UTIL_EXPORT ImageIndex                GetImageIndex(const QByteArray& bytes);
UTIL_EXPORT bool                      IsEPubTextFile(QStringView fileName);

} // namespace HomeCompa::Util::EpubParser
