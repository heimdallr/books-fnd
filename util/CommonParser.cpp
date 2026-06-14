#include "CommonParser.h"

#include "QtTypes.h"

namespace HomeCompa::Util::CommonParser
{

QStringList ParseAuthor(const QString& str)
{
	auto author = str.split(" ", Qt::SkipEmptyParts);
	if (author.size() == 2)
		std::swap(author[0], author[1]);
	else if (author.size() >= 3)
	{
		auto firstName = std::move(author.front());
		author.pop_front();
		auto middleName = std::move(author.front());
		author.pop_front();
		auto lastName = author.join(' ');
		author        = QStringList { std::move(lastName), std::move(firstName), std::move(middleName) };
	}
	Resize(author, 3);

	return author;
}

}
