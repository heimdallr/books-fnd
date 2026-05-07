#pragma once

#include <QStringList>

#include "export/util.h"

class QIODevice;
class QDateTime;

namespace HomeCompa
{

class Zip;

}

namespace HomeCompa::Util::EpubParser
{

struct Image
{
	QString    id;
	QByteArray body;
};

struct ParseResult
{
	QString                  title;
	QString                  language;
	QStringList              genres;
	std::vector<QStringList> authors;
	QString                  annotation;
	std::vector<Image>       images;
};

UTIL_EXPORT ParseResult Parse(QIODevice& stream, bool parseImages = false);
UTIL_EXPORT ParseResult Parse(const Zip& zip, const QString& fileName, bool parseImages = false);

} // namespace HomeCompa::Util::EpubParser
