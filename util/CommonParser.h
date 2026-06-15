#pragma once

#include <QStringList>

#include "fnd/EnumBitmask.h"

namespace HomeCompa::Util::CommonParser
{

struct ContentItem
{
	QString    id;
	QByteArray body;
};

enum class Mode
{
	None   = 0,
	Images = 1 << 0,
	Texts  = 1 << 1,
	All    = Images | Texts
};

struct ParseResult
{
	QString                  title;
	QString                  language;
	QStringList              genres;
	std::vector<QStringList> authors;
	QString                  annotation;
	QByteArray               imageIndex;
	bool                     coverExists { false };
	std::vector<ContentItem> images;
	std::vector<ContentItem> texts;
};

QStringList ParseAuthor(const QString& str);

} // namespace HomeCompa::Util::EpubParser

ENABLE_BITMASK_OPERATORS(HomeCompa::Util::CommonParser::Mode);
