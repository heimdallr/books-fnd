#include "StrUtil.h"

#include <QStringList>

#include "fnd/IsOneOf.h"

#include "QtTypes.h"

namespace HomeCompa::Util {

std::vector<QString> UniqTitle(QString value)
{
	Normalize(value);
	RemoveIf(value, [](const QChar ch) {
		return ch != ' ' && !IsOneOf(ch.category(), QChar::Number_DecimalDigit, QChar::Letter_Lowercase);
	});
	return value.split(' ', Qt::SkipEmptyParts) | std::views::as_rvalue | std::ranges::to<std::vector>();
}

QString& Normalize(QString& value)
{
	static constexpr std::pair<char16_t, char16_t> replacementChar[] {
		{ 0x0451, 0x0435 },
		{ 0x0439, 0x0438 },
		{ 0x044A, 0x044C },
	};
	const std::pair<QString, QString> replacementString[] {
		{ QString("%1%2").arg(QChar { 0x044B }, QChar { 0x043E }), QString("%1%2").arg(QChar { 0x044C }, QChar { 0x044E }) },
	};

	value = value.toLower();

	std::ranges::transform(value, std::begin(value), [&](const QChar& ch) {
		const auto it = std::ranges::find(replacementChar, ch.unicode(), &std::pair<char16_t, char16_t>::first);
		if (it != std::cend(replacementChar))
			return QChar { it->second };

		const auto category = ch.category();
		if (IsOneOf(category, QChar::Separator_Space, QChar::Separator_Line, QChar::Separator_Paragraph, QChar::Other_Control)
		    || (category >= QChar::Punctuation_Connector && category <= QChar::Punctuation_Other))
			return QChar { 0x20 };

		return ch;
	});

	for (const auto& [from, to] : replacementString)
		value.replace(from, to);

	return value;
}

void AppendTitle(QString& title, const QString& str, const QString& delimiter)
{
	if (title.isEmpty())
	{
		title = str;
		return;
	}

	if (!str.isEmpty())
		title.append(delimiter).append(str);
}

} // namespace HomeCompa::Util
